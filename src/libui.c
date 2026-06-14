/*
 * Copyright (c) 2026, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <libui/backend.h>
#include <libui/libui.h>
#include <libui/font.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_STACK_CAPACITY 16

#define DEFAULT_PADDING 7
#define BACKGROUND_COLOR (bui_color_t){0xFF, 0x21, 0x21, 0x21}
#define SECONDARY_BACKGROUND_COLOR (bui_color_t){0xFF, 0x18, 0x18, 0x18}
#define TEXT_COLOR (bui_color_t){0xFF, 0xB3, 0xB3, 0xB3}
#define BORDER_COLOR (bui_color_t){0xFF, 0x00, 0x00, 0x00}
#define BORDER_THICKNESS 1
#define BORDER_SHADOW (bui_color_t){0x33, 0xFF, 0xFF, 0xFF}
#define SCROLLBAR_COLOR (bui_color_t){0x80, 0xB3, 0xB3, 0xB3}
#define SCROLLBAR_WIDTH 6
#define SCROLL_STEP 20

void bui_autolayout(bui_wctx_t *wctx);
static bui_widget_t *_bui_find_widget_by_id(bui_widget_t *widget, bui_id_t id);

static bui_id_t _bui_mix_id(bui_id_t parent_id, uint32_t child_index)
{
    uint32_t hash = parent_id ^ 0x9E3779B9u;
    hash ^= child_index + 0x85EBCA6Bu + (hash << 6) + (hash >> 2);
    return hash == 0 ? 1 : hash;
}

static inline uint32_t _sub_or_zero(uint32_t lhs, uint32_t rhs)
{
    return lhs > rhs ? lhs - rhs : 0;
}

static bui_wctx_t *_bui_find_window_by_id(bui_ctx_t *ctx, uint32_t window_id)
{
    bui_wctx_t *current = ctx->first_window;
    while (current) {
        if (current->window_id == window_id)
            return current;
        current = current->next;
    }
    return NULL;
}

bool bui_init_context(bui_ctx_t *ctx)
{
    memset(ctx, 0, sizeof(bui_ctx_t));
    if (!bui_font_init_default())
        return false;
    ctx->running = true;
    return true;
}

void bui_deinit_context(bui_ctx_t *ctx)
{
    bui_wctx_t *current = ctx->first_window;
    while (current) {
        bui_wctx_t *next = current->next;
        bui_destroy_window(ctx, current);
        current = next;
    }
}

bui_wctx_t *bui_new_window(bui_ctx_t *ctx, const char *title, int w, int h, bui_window_flags_t flags)
{
    bui_wctx_t *wctx = calloc(1, sizeof(bui_wctx_t));
    if (!wctx)
        return NULL;

    wctx->title = title;
    wctx->flags = flags;
    wctx->width = w;
    wctx->height = h;
    wctx->state = BUI_WINDOW_STATE_ACTIVE;

    if (!bui_gfx_init(wctx)) {
        free(wctx);
        return NULL;
    }

    wctx->next = ctx->first_window;
    if (ctx->first_window)
        ctx->first_window->prev = wctx;
    ctx->first_window = wctx;

    bui_arena_init(&wctx->arena);
    bui_arena_init(&wctx->persistent_arena);

    return wctx;
}

void bui_destroy_window(bui_ctx_t *ctx, bui_wctx_t *wctx)
{
    if (wctx->prev)
        wctx->prev->next = wctx->next;
    if (wctx->next)
        wctx->next->prev = wctx->prev;
    if (ctx->first_window == wctx)
        ctx->first_window = wctx->next;

    bui_gfx_destroy(wctx);
    bui_arena_free(&wctx->arena);
    bui_arena_free(&wctx->persistent_arena);
    free(wctx->key_pairs);
    free(wctx);
}

static bool _bui_area_contains(bui_area_t area, uint32_t x, uint32_t y)
{
    return x >= area.x && x < area.x + area.width && y >= area.y && y < area.y + area.height;
}

static void _bui_clear_widget_event_state(bui_wctx_t *wctx)
{
    wctx->event_widget_state = BUI_WIDGET_EVENT_NONE;
    wctx->event_widget_id = 0;
}

static void _bui_clear_input_event_state(bui_wctx_t *wctx)
{
    memset(&wctx->input_event, 0, sizeof(wctx->input_event));
}

static void _bui_handle_click_event(bui_wctx_t *wctx)
{
    bui_mouse_state_t mouse_state = wctx->mouse_state;
    for (size_t i = 0; i < wctx->key_pairs_count; i++) {
        bui_widget_t *widget = wctx->key_pairs[i].widget;
        if (!(widget->flags & BUI_WIDGET_FLAG_CLICKABLE))
            continue;

        bui_widget_event_t event = BUI_WIDGET_EVENT_HOVERED;
        if (mouse_state.buttons_state & BUI_MOUSE_BUTTON_LEFT)
            event |= BUI_WIDGET_EVENT_LCLICKED;
        if (mouse_state.buttons_state & BUI_MOUSE_BUTTON_RIGHT)
            event |= BUI_WIDGET_EVENT_RCLICKED;
        if (mouse_state.buttons_state & BUI_MOUSE_BUTTON_MIDDLE)
            event |= BUI_WIDGET_EVENT_MCLICKED;

        if (_bui_area_contains(widget->computed_area, mouse_state.pos_x, mouse_state.pos_y)) {
            widget->last_event |= event;
            if (wctx->event_widget_id != widget->id)
                wctx->event_widget_state = BUI_WIDGET_EVENT_NONE;
            wctx->event_widget_state |= event;
            wctx->event_widget_id = widget->id;
            wctx->focused_widget_id = widget->id;
            wctx->active_widget_id = widget->id;
            widget->flags |= BUI_WIDGET_FLAG_FOCUSED;
            return;
        }
    }

    wctx->focused_widget_id = 0;
    wctx->active_widget_id = 0;
    _bui_clear_widget_event_state(wctx);
}

static void _bui_handle_release_event(bui_wctx_t *wctx, bui_mouse_button_t button)
{
    bui_mouse_state_t mouse_state = wctx->mouse_state;
    bui_widget_event_t clicked = BUI_WIDGET_EVENT_NONE, released = BUI_WIDGET_EVENT_NONE;

    if (button & BUI_MOUSE_BUTTON_LEFT) {
        clicked |= BUI_WIDGET_EVENT_LCLICKED;
        released |= BUI_WIDGET_EVENT_LRELEASED;
    }
    if (button & BUI_MOUSE_BUTTON_RIGHT) {
        clicked |= BUI_WIDGET_EVENT_RCLICKED;
        released |= BUI_WIDGET_EVENT_RRELEASED;
    }
    if (button & BUI_MOUSE_BUTTON_MIDDLE) {
        clicked |= BUI_WIDGET_EVENT_MCLICKED;
        released |= BUI_WIDGET_EVENT_MRELEASED;
    }

    for (size_t i = 0; i < wctx->key_pairs_count; i++) {
        bui_widget_t *widget = wctx->key_pairs[i].widget;
        if (!(widget->flags & BUI_WIDGET_FLAG_CLICKABLE))
            continue;

        if (_bui_area_contains(widget->computed_area, mouse_state.pos_x, mouse_state.pos_y)) {
            bui_widget_event_t event = BUI_WIDGET_EVENT_HOVERED | released;

            widget->last_event |= event;
            if (wctx->event_widget_id != widget->id)
                wctx->event_widget_state = BUI_WIDGET_EVENT_NONE;
            wctx->event_widget_state &= ~clicked;
            wctx->event_widget_state |= event;
            wctx->event_widget_id = widget->id;
            if (button & BUI_MOUSE_BUTTON_LEFT)
                wctx->active_widget_id = 0;
            return;
        }
    }

    if (button & BUI_MOUSE_BUTTON_LEFT)
        wctx->active_widget_id = 0;
    _bui_clear_widget_event_state(wctx);
}

static void _bui_handle_mouse_hover(bui_wctx_t *wctx)
{
    bui_mouse_state_t mouse_state = wctx->mouse_state;
    for (size_t i = 0; i < wctx->key_pairs_count; i++) {
        bui_widget_t *widget = wctx->key_pairs[i].widget;
        if (_bui_area_contains(widget->computed_area, mouse_state.pos_x, mouse_state.pos_y)) {
            if (wctx->event_widget_id == widget->id
                && (wctx->event_widget_state & BUI_WIDGET_EVENT_HOVERED))
                return;

            widget->last_event |= BUI_WIDGET_EVENT_HOVERED;
            if (wctx->event_widget_id != widget->id)
                wctx->event_widget_state = BUI_WIDGET_EVENT_NONE;
            wctx->event_widget_state |= BUI_WIDGET_EVENT_HOVERED;
            wctx->event_widget_id = widget->id;
            return;
        }
    }

    _bui_clear_widget_event_state(wctx);
}

bool bui_pump_events(bui_ctx_t *ctx)
{
    for (bui_wctx_t *current = ctx->first_window; current; current = current->next)
        _bui_clear_input_event_state(current);

    bui_event_t event = {0};
    if (!bui_poll_events(&event)) {
        bui_delay(1);
        return true;
    }

    bui_wctx_t *wctx = _bui_find_window_by_id(ctx, event.window_id);
    if (wctx)
        wctx->input_event = event;

    if (event.type == BUI_EVENT_WINDOW_CLOSED) {
        if (wctx) {
            wctx->close_requested = true;
            wctx->state = BUI_WINDOW_STATE_PENDING;
        }
        ctx->running = false;
        return false;
    }

    switch (event.type) {
    case BUI_EVENT_WINDOW_CREATED:
    case BUI_EVENT_WINDOW_RESIZED:
        wctx->width = event.window_resized.width;
        wctx->height = event.window_resized.height;
        break;
    case BUI_EVENT_MOUSE_MOVE:
        wctx->mouse_state.pos_x = event.mouse_move.pos_x;
        wctx->mouse_state.pos_y = event.mouse_move.pos_y;
        _bui_handle_mouse_hover(wctx);
        break;
    case BUI_EVENT_MOUSE_BUTTON_DOWN:
        wctx->mouse_state.pos_x = event.mouse_button.pos_x;
        wctx->mouse_state.pos_y = event.mouse_button.pos_y;
        wctx->mouse_state.buttons_state |= event.mouse_button.button;
        _bui_handle_click_event(wctx);
        break;
    case BUI_EVENT_MOUSE_BUTTON_UP:
        wctx->mouse_state.pos_x = event.mouse_button.pos_x;
        wctx->mouse_state.pos_y = event.mouse_button.pos_y;
        wctx->mouse_state.buttons_state &= (uint8_t) ~event.mouse_button.button;
        if (event.mouse_button.button & BUI_MOUSE_BUTTON_LEFT)
            wctx->scrollbar_drag_widget_id = 0;
        _bui_handle_release_event(wctx, event.mouse_button.button);
        break;
    case BUI_EVENT_MOUSE_WHEEL:
        wctx->mouse_state.pos_x = event.mouse_wheel.pos_x;
        wctx->mouse_state.pos_y = event.mouse_wheel.pos_y;
        break;
    case BUI_EVENT_KEY_DOWN:
    case BUI_EVENT_KEY_HOLD:
        if (event.keyboard < 128)
            wctx->keyboard_keys[event.keyboard] = event.type == BUI_EVENT_KEY_HOLD
                                                      ? BUI_KEY_STATE_HOLD
                                                      : BUI_KEY_STATE_DOWN;
        break;
    case BUI_EVENT_KEY_UP:
        if (event.keyboard < 128)
            wctx->keyboard_keys[event.keyboard] = BUI_KEY_STATE_UP;
        break;
    default:
        break;
    }

    wctx->hot_state = false;
    return true;
}

bui_widget_t *bui_push_new_child(bui_wctx_t *wctx, bui_widget_t *child)
{
    bui_widget_t *widget = bui_arena_alloc(&wctx->arena, sizeof(bui_widget_t));
    if (widget == NULL)
        return NULL;

    *widget = *child;
    bui_widget_t *parent = wctx->current_widget;
    widget->parent = parent;

    uint32_t child_index = 0;
    if (parent->first_child == NULL) {
        parent->first_child = widget;
    } else {
        bui_widget_t *sibling = parent->first_child;
        child_index = 1;
        while (sibling->next_sibling != NULL) {
            sibling = sibling->next_sibling;
            child_index++;
        }
        sibling->next_sibling = widget;
        widget->prev_sibling = sibling;
    }
    widget->id = _bui_mix_id(parent ? parent->id : 0, child_index);

    bui_widget_t *prev = _bui_find_widget_by_id(wctx->prev_widget_tree, widget->id);
    if (prev) {
        widget->scroll_x = prev->scroll_x;
        widget->scroll_y = prev->scroll_y;
    }

    return widget;
}

bui_widget_t *bui_push_new_parent(bui_wctx_t *wctx, bui_widget_t *parent)
{
    bui_widget_t *widget = bui_push_new_child(wctx, parent);
    if (widget == NULL)
        return NULL;
    widget->flags |= BUI_WIDGET_FLAG_WITH_CHILDREN;
    wctx->current_widget = widget;
    return widget;
}

void bui_pop_widget(bui_wctx_t *wctx)
{
    if (wctx->current_widget == NULL || wctx->current_widget->parent == NULL)
        return;
    if (wctx->current_widget->first_child == NULL)
        wctx->current_widget->flags &= ~BUI_WIDGET_FLAG_WITH_CHILDREN;
    wctx->current_widget = wctx->current_widget->parent;
}

bool bui_begin_window(bui_wctx_t *wctx)
{
    if (wctx->state != BUI_WINDOW_STATE_ACTIVE || wctx->close_requested)
        return false;

    bui_begin_frame(wctx);

    if (!wctx->hot_state) {
        wctx->prev_widget_tree = wctx->widget_tree;
        bui_arena_t prev_arena = wctx->persistent_arena;
        wctx->persistent_arena = wctx->arena;
        wctx->arena = prev_arena;

        if (wctx->arena.head == NULL)
            bui_arena_init(&wctx->arena);
        else
            bui_arena_reset(&wctx->arena);
        if (wctx->arena.head == NULL)
            return false;

        wctx->key_pairs_count = 0;
        wctx->themes_head = NULL;
        wctx->themes_tail = NULL;
        bui_theme_t default_theme = (bui_theme_t){
            .font = bui_get_default_font(),
            .inner_padding = {DEFAULT_PADDING, DEFAULT_PADDING, DEFAULT_PADDING, DEFAULT_PADDING},
            .foreground_color = TEXT_COLOR,
            .background_color = BACKGROUND_COLOR,
            .border_color = BORDER_COLOR,
            .shadow_color = BORDER_SHADOW,
            .border_thickness = BORDER_THICKNESS,
            .shadow_thickness = BORDER_THICKNESS,
            .spacing = DEFAULT_PADDING,
        };
        bui_push_theme(wctx, &default_theme);

        uint32_t content_width = _sub_or_zero(
            wctx->width, default_theme.inner_padding.l + default_theme.inner_padding.r);
        uint32_t content_height = _sub_or_zero(
            wctx->height, default_theme.inner_padding.t + default_theme.inner_padding.b);

        wctx->widget_tree = bui_arena_alloc(&wctx->arena, sizeof(*wctx->widget_tree));
        if (wctx->widget_tree == NULL)
            return false;

        *wctx->widget_tree = (bui_widget_t){
            .id = bui_hash_string("bui.root"),
            .semantic_size = {
                [BUI_AXIS_X] = {
                    .type = BUI_WIDGET_SIZE_TYPE_FIXED,
                    .value = content_width,
                    .strictness = 1.0f,
                },
                [BUI_AXIS_Y] = {
                    .type = BUI_WIDGET_SIZE_TYPE_FIXED,
                    .value = content_height,
                    .strictness = 1.0f,
                },
            },
            .computed_size = {
                [BUI_AXIS_X] = content_width,
                [BUI_AXIS_Y] = content_height,
            },
            .computed_area = {
                .x = default_theme.inner_padding.l,
                .y = default_theme.inner_padding.t,
                .width = content_width,
                .height = content_height,
                },
            .flags = BUI_WIDGET_FLAG_WITH_CHILDREN,
            .layout_axis = BUI_AXIS_Y,
        };
        wctx->current_widget = wctx->widget_tree;
    }

    return true;
}

bool bui_end_window(bui_wctx_t *wctx)
{
    if (!wctx->hot_state)
        bui_autolayout(wctx);

    bui_draw_filled_rect(
        wctx,
        (bui_rect_t){.x = 0, .y = 0, .width = wctx->width, .height = wctx->height},
        wctx->themes_head->background_color);
    for (bui_widget_t *child = wctx->widget_tree->first_child; child != NULL;
         child = child->next_sibling) {
        if (child->draw)
            child->draw(wctx, child);
    }
    bui_end_frame(wctx);
    return true;
}

bool bui_is_mouse_in_area(bui_wctx_t *wctx, bui_area_t area)
{
    return wctx->mouse_state.pos_x >= area.x && wctx->mouse_state.pos_x < area.x + area.width
           && wctx->mouse_state.pos_y >= area.y && wctx->mouse_state.pos_y < area.y + area.height;
}

bui_pos_t bui_get_mouse_pos(bui_wctx_t *wctx)
{
    return (bui_pos_t){wctx->mouse_state.pos_x, wctx->mouse_state.pos_y};
}

bool bui_is_mouse_button_down(bui_wctx_t *wctx, bui_mouse_button_t button)
{
    return (wctx->mouse_state.buttons_state & button) != 0;
}

bool bui_is_key_down(bui_wctx_t *wctx, bui_keyboard_scancode_t keycode)
{
    return wctx && keycode < 128
           && (wctx->keyboard_keys[keycode] == BUI_KEY_STATE_DOWN
               || wctx->keyboard_keys[keycode] == BUI_KEY_STATE_HOLD);
}

bool bui_is_key_up(bui_wctx_t *wctx, bui_keyboard_scancode_t keycode)
{
    return !wctx || keycode >= 128 || wctx->keyboard_keys[keycode] == BUI_KEY_STATE_UP;
}

char bui_char_from_key_scancode(bui_wctx_t *wctx, bui_keyboard_scancode_t key)
{
    if (bui_is_key_down(wctx, BUI_KEYBOARD_SCANCODE_LCTRL)
        || bui_is_key_down(wctx, BUI_KEYBOARD_SCANCODE_LALT))
        return '\0';

    bool shifted = bui_is_key_down(wctx, BUI_KEYBOARD_SCANCODE_LSHIFT)
                   || bui_is_key_down(wctx, BUI_KEYBOARD_SCANCODE_RSHIFT);

    char ch = '\0';
    switch (key) {
    case BUI_KEYBOARD_SCANCODE_A:
        ch = 'a';
        break;
    case BUI_KEYBOARD_SCANCODE_B:
        ch = 'b';
        break;
    case BUI_KEYBOARD_SCANCODE_C:
        ch = 'c';
        break;
    case BUI_KEYBOARD_SCANCODE_D:
        ch = 'd';
        break;
    case BUI_KEYBOARD_SCANCODE_E:
        ch = 'e';
        break;
    case BUI_KEYBOARD_SCANCODE_F:
        ch = 'f';
        break;
    case BUI_KEYBOARD_SCANCODE_G:
        ch = 'g';
        break;
    case BUI_KEYBOARD_SCANCODE_H:
        ch = 'h';
        break;
    case BUI_KEYBOARD_SCANCODE_I:
        ch = 'i';
        break;
    case BUI_KEYBOARD_SCANCODE_J:
        ch = 'j';
        break;
    case BUI_KEYBOARD_SCANCODE_K:
        ch = 'k';
        break;
    case BUI_KEYBOARD_SCANCODE_L:
        ch = 'l';
        break;
    case BUI_KEYBOARD_SCANCODE_M:
        ch = 'm';
        break;
    case BUI_KEYBOARD_SCANCODE_N:
        ch = 'n';
        break;
    case BUI_KEYBOARD_SCANCODE_O:
        ch = 'o';
        break;
    case BUI_KEYBOARD_SCANCODE_P:
        ch = 'p';
        break;
    case BUI_KEYBOARD_SCANCODE_Q:
        ch = 'q';
        break;
    case BUI_KEYBOARD_SCANCODE_R:
        ch = 'r';
        break;
    case BUI_KEYBOARD_SCANCODE_S:
        ch = 's';
        break;
    case BUI_KEYBOARD_SCANCODE_T:
        ch = 't';
        break;
    case BUI_KEYBOARD_SCANCODE_U:
        ch = 'u';
        break;
    case BUI_KEYBOARD_SCANCODE_V:
        ch = 'v';
        break;
    case BUI_KEYBOARD_SCANCODE_W:
        ch = 'w';
        break;
    case BUI_KEYBOARD_SCANCODE_X:
        ch = 'x';
        break;
    case BUI_KEYBOARD_SCANCODE_Y:
        ch = 'y';
        break;
    case BUI_KEYBOARD_SCANCODE_Z:
        ch = 'z';
        break;
    default:
        break;
    }
    if (ch != '\0')
        return shifted ? (char) (ch - 'a' + 'A') : ch;

    if (key >= BUI_KEYBOARD_SCANCODE_1 && key <= BUI_KEYBOARD_SCANCODE_9) {
        static const char normal[] = "123456789";
        static const char shifted_digits[] = "!@#$%^&*(";
        size_t index = (size_t) (key - BUI_KEYBOARD_SCANCODE_1);
        return shifted ? shifted_digits[index] : normal[index];
    }

    switch (key) {
    case BUI_KEYBOARD_SCANCODE_0:
        return shifted ? ')' : '0';
    case BUI_KEYBOARD_SCANCODE_SPACE:
        return ' ';
    case BUI_KEYBOARD_SCANCODE_MINUS:
        return shifted ? '_' : '-';
    case BUI_KEYBOARD_SCANCODE_EQUALS:
        return shifted ? '+' : '=';
    case BUI_KEYBOARD_SCANCODE_LBRACKET:
        return shifted ? '{' : '[';
    case BUI_KEYBOARD_SCANCODE_RBRACKET:
        return shifted ? '}' : ']';
    case BUI_KEYBOARD_SCANCODE_BACKSLASH:
        return shifted ? '|' : '\\';
    case BUI_KEYBOARD_SCANCODE_SEMICOLON:
        return shifted ? ':' : ';';
    case BUI_KEYBOARD_SCANCODE_APOSTROPHE:
        return shifted ? '"' : '\'';
    case BUI_KEYBOARD_SCANCODE_BACKTICK:
        return shifted ? '~' : '`';
    case BUI_KEYBOARD_SCANCODE_COMMA:
        return shifted ? '<' : ',';
    case BUI_KEYBOARD_SCANCODE_PERIOD:
        return shifted ? '>' : '.';
    case BUI_KEYBOARD_SCANCODE_SLASH:
        return shifted ? '?' : '/';
    default:
        return '\0';
    }
}

void bui_push_theme(bui_wctx_t *ctx, bui_theme_t *theme)
{
    bui_theme_t *new = bui_arena_alloc(&ctx->arena, sizeof(bui_theme_t));
    memcpy(new, theme, sizeof(bui_theme_t));
    new->next = NULL;
    new->prev = ctx->themes_tail;
    if (ctx->themes_tail != NULL)
        ctx->themes_tail->next = new;
    ctx->themes_tail = new;
    if (ctx->themes_head == NULL)
        ctx->themes_head = new;
}

bui_theme_t *bui_pop_theme(bui_wctx_t *ctx)
{
    bui_theme_t *theme = ctx->themes_tail;
    if (theme != NULL) {
        ctx->themes_tail = theme->prev;
        if (theme == ctx->themes_head)
            ctx->themes_head = NULL;
        return theme;
    }
    return NULL;
}

bui_theme_t *bui_get_current_theme(bui_wctx_t *ctx)
{
    return ctx->themes_tail;
}

/*
 * Source: https://github.com/ocornut/imgui/blob/master/imgui.cpp#L2413
 */
static const uint32_t _crc32_table[256] = {
    0x00000000, 0xF26B8303, 0xE13B70F7, 0x1350F3F4, 0xC79A971F, 0x35F1141C, 0x26A1E7E8, 0xD4CA64EB,
    0x8AD958CF, 0x78B2DBCC, 0x6BE22838, 0x9989AB3B, 0x4D43CFD0, 0xBF284CD3, 0xAC78BF27, 0x5E133C24,
    0x105EC76F, 0xE235446C, 0xF165B798, 0x030E349B, 0xD7C45070, 0x25AFD373, 0x36FF2087, 0xC494A384,
    0x9A879FA0, 0x68EC1CA3, 0x7BBCEF57, 0x89D76C54, 0x5D1D08BF, 0xAF768BBC, 0xBC267848, 0x4E4DFB4B,
    0x20BD8EDE, 0xD2D60DDD, 0xC186FE29, 0x33ED7D2A, 0xE72719C1, 0x154C9AC2, 0x061C6936, 0xF477EA35,
    0xAA64D611, 0x580F5512, 0x4B5FA6E6, 0xB93425E5, 0x6DFE410E, 0x9F95C20D, 0x8CC531F9, 0x7EAEB2FA,
    0x30E349B1, 0xC288CAB2, 0xD1D83946, 0x23B3BA45, 0xF779DEAE, 0x05125DAD, 0x1642AE59, 0xE4292D5A,
    0xBA3A117E, 0x4851927D, 0x5B016189, 0xA96AE28A, 0x7DA08661, 0x8FCB0562, 0x9C9BF696, 0x6EF07595,
    0x417B1DBC, 0xB3109EBF, 0xA0406D4B, 0x522BEE48, 0x86E18AA3, 0x748A09A0, 0x67DAFA54, 0x95B17957,
    0xCBA24573, 0x39C9C670, 0x2A993584, 0xD8F2B687, 0x0C38D26C, 0xFE53516F, 0xED03A29B, 0x1F682198,
    0x5125DAD3, 0xA34E59D0, 0xB01EAA24, 0x42752927, 0x96BF4DCC, 0x64D4CECF, 0x77843D3B, 0x85EFBE38,
    0xDBFC821C, 0x2997011F, 0x3AC7F2EB, 0xC8AC71E8, 0x1C661503, 0xEE0D9600, 0xFD5D65F4, 0x0F36E6F7,
    0x61C69362, 0x93AD1061, 0x80FDE395, 0x72966096, 0xA65C047D, 0x5437877E, 0x4767748A, 0xB50CF789,
    0xEB1FCBAD, 0x197448AE, 0x0A24BB5A, 0xF84F3859, 0x2C855CB2, 0xDEEEDFB1, 0xCDBE2C45, 0x3FD5AF46,
    0x7198540D, 0x83F3D70E, 0x90A324FA, 0x62C8A7F9, 0xB602C312, 0x44694011, 0x5739B3E5, 0xA55230E6,
    0xFB410CC2, 0x092A8FC1, 0x1A7A7C35, 0xE811FF36, 0x3CDB9BDD, 0xCEB018DE, 0xDDE0EB2A, 0x2F8B6829,
    0x82F63B78, 0x709DB87B, 0x63CD4B8F, 0x91A6C88C, 0x456CAC67, 0xB7072F64, 0xA457DC90, 0x563C5F93,
    0x082F63B7, 0xFA44E0B4, 0xE9141340, 0x1B7F9043, 0xCFB5F4A8, 0x3DDE77AB, 0x2E8E845F, 0xDCE5075C,
    0x92A8FC17, 0x60C37F14, 0x73938CE0, 0x81F80FE3, 0x55326B08, 0xA759E80B, 0xB4091BFF, 0x466298FC,
    0x1871A4D8, 0xEA1A27DB, 0xF94AD42F, 0x0B21572C, 0xDFEB33C7, 0x2D80B0C4, 0x3ED04330, 0xCCBBC033,
    0xA24BB5A6, 0x502036A5, 0x4370C551, 0xB11B4652, 0x65D122B9, 0x97BAA1BA, 0x84EA524E, 0x7681D14D,
    0x2892ED69, 0xDAF96E6A, 0xC9A99D9E, 0x3BC21E9D, 0xEF087A76, 0x1D63F975, 0x0E330A81, 0xFC588982,
    0xB21572C9, 0x407EF1CA, 0x532E023E, 0xA145813D, 0x758FE5D6, 0x87E466D5, 0x94B49521, 0x66DF1622,
    0x38CC2A06, 0xCAA7A905, 0xD9F75AF1, 0x2B9CD9F2, 0xFF56BD19, 0x0D3D3E1A, 0x1E6DCDEE, 0xEC064EED,
    0xC38D26C4, 0x31E6A5C7, 0x22B65633, 0xD0DDD530, 0x0417B1DB, 0xF67C32D8, 0xE52CC12C, 0x1747422F,
    0x49547E0B, 0xBB3FFD08, 0xA86F0EFC, 0x5A048DFF, 0x8ECEE914, 0x7CA56A17, 0x6FF599E3, 0x9D9E1AE0,
    0xD3D3E1AB, 0x21B862A8, 0x32E8915C, 0xC083125F, 0x144976B4, 0xE622F5B7, 0xF5720643, 0x07198540,
    0x590AB964, 0xAB613A67, 0xB831C993, 0x4A5A4A90, 0x9E902E7B, 0x6CFBAD78, 0x7FAB5E8C, 0x8DC0DD8F,
    0xE330A81A, 0x115B2B19, 0x020BD8ED, 0xF0605BEE, 0x24AA3F05, 0xD6C1BC06, 0xC5914FF2, 0x37FACCF1,
    0x69E9F0D5, 0x9B8273D6, 0x88D28022, 0x7AB90321, 0xAE7367CA, 0x5C18E4C9, 0x4F48173D, 0xBD23943E,
    0xF36E6F75, 0x0105EC76, 0x12551F82, 0xE03E9C81, 0x34F4F86A, 0xC69F7B69, 0xD5CF889D, 0x27A40B9E,
    0x79B737BA, 0x8BDCB4B9, 0x988C474D, 0x6AE7C44E, 0xBE2DA0A5, 0x4C4623A6, 0x5F16D052, 0xAD7D5351,
};

bui_id_t bui_hash_string(const char *str)
{
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; str[i] != '\0'; i++) {
        uint8_t index = (crc ^ (uint8_t) str[i]) & 0xFF;
        crc = (crc >> 8) ^ _crc32_table[index];
    }
    return ~crc;
}

void bui_push_id(bui_wctx_t *wctx, bui_id_t id, bui_widget_t *widget)
{
    if (wctx->key_pairs_capacity == 0) {
        wctx->key_pairs_capacity = 16;
        wctx->key_pairs = malloc(wctx->key_pairs_capacity * sizeof(bui_key_pair_t));
    } else if (wctx->key_pairs_count == wctx->key_pairs_capacity) {
        wctx->key_pairs_capacity *= 2;
        wctx->key_pairs
            = realloc(wctx->key_pairs, wctx->key_pairs_capacity * sizeof(bui_key_pair_t));
    }
    widget->id = id;
    bui_widget_t *prev = _bui_find_widget_by_id(wctx->prev_widget_tree, id);
    if (prev) {
        widget->edit = prev->edit;
        widget->scroll_x = prev->scroll_x;
        widget->scroll_y = prev->scroll_y;
    }
    wctx->key_pairs[wctx->key_pairs_count++] = (bui_key_pair_t){widget, id};
}

bui_widget_t *bui_get_by_id(bui_wctx_t *wctx, bui_id_t id)
{
    for (size_t i = 0; i < wctx->key_pairs_count; i++) {
        if (wctx->key_pairs[i].id == id)
            return wctx->key_pairs[i].widget;
    }
    return NULL;
}

static bui_widget_t *_bui_find_widget_by_id(bui_widget_t *widget, bui_id_t id)
{
    if (widget == NULL)
        return NULL;
    if (widget->id == id)
        return widget;
    for (bui_widget_t *child = widget->first_child; child; child = child->next_sibling) {
        bui_widget_t *found = _bui_find_widget_by_id(child, id);
        if (found)
            return found;
    }
    return NULL;
}
