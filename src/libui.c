/*
 * Copyright (c) 2026, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <libui/backend.h>
#include <libui/libui.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "font.h"

#define INITIAL_STACK_CAPACITY 16

#define DEFAULT_PADDING 7
#define BACKGROUND_COLOR (bui_color_t){0xFF, 0x21, 0x21, 0x21}
#define SECONDARY_BACKGROUND_COLOR (bui_color_t){0xFF, 0x18, 0x18, 0x18}
#define TEXT_COLOR (bui_color_t){0xFF, 0xB3, 0xB3, 0xB3}
#define BORDER_COLOR (bui_color_t){0xFF, 0x00, 0x00, 0x00}
#define BORDER_THICKNESS 1
#define BORDER_SHADOW (bui_color_t){0x33, 0xFF, 0xFF, 0xFF}

static bui_font_t _default_font = {
    .atlas = font_atlas,
    .atlas_width = FONT_ATLAS_WIDTH,
    .atlas_height = FONT_ATLAS_HEIGHT,
    .glyphs = font_glyphs,
    .first_char = FONT_FIRST_CHAR,
    .last_char = FONT_LAST_CHAR,
    .width = 13,
    .height = FONT_LINE_HEIGHT,
};

static inline uint32_t _sub_or_zero(uint32_t lhs, uint32_t rhs)
{
    return lhs > rhs ? lhs - rhs : 0;
}

static ui_wctx_t *_ui_find_window_by_id(ui_ctx_t *ctx, uint32_t window_id)
{
    ui_wctx_t *current = ctx->first_window;
    while (current) {
        if (current->window_id == window_id)
            return current;
        current = current->next;
    }
    return NULL;
}

static void _ui_clear_transient_events(ui_ctx_t *ctx)
{
    for (ui_wctx_t *wctx = ctx->first_window; wctx; wctx = wctx->next)
        wctx->last_event = (bui_event_t){0};
}

bool ui_init_context(ui_ctx_t *ctx)
{
    memset(ctx, 0, sizeof(ui_ctx_t));
    ctx->running = true;
    return true;
}

void ui_deinit_context(ui_ctx_t *ctx)
{
    ui_wctx_t *current = ctx->first_window;
    while (current) {
        ui_wctx_t *next = current->next;
        ui_destroy_window(ctx, current);
        current = next;
    }
}

ui_wctx_t *ui_new_window(ui_ctx_t *ctx, const char *title, int w, int h, window_flags_t flags)
{
    ui_wctx_t *wctx = calloc(1, sizeof(ui_wctx_t));
    if (!wctx)
        return NULL;

    wctx->title = title;
    wctx->flags = flags;
    wctx->width = w;
    wctx->height = h;
    wctx->state = UI_WINDOW_STATE_ACTIVE;

    if (!bui_gfx_init(wctx)) {
        free(wctx);
        return NULL;
    }

    wctx->layout_stack = malloc(INITIAL_STACK_CAPACITY * sizeof(bui_layout_t));
    if (wctx->layout_stack == NULL) {
        bui_gfx_destroy(wctx);
        free(wctx);
        return NULL;
    }
    wctx->layout_count = 0;
    wctx->layout_capacity = INITIAL_STACK_CAPACITY;

    // for (size_t i = 0; i < 256; i++)
    //     wctx->keyboard_keys[i] = BUI_INPUT_KEYACTION_UP;
    // wctx->mouse_state = (ui_mouse_state_t){0};

    wctx->next = ctx->first_window;
    if (ctx->first_window)
        ctx->first_window->prev = wctx;
    ctx->first_window = wctx;

    return wctx;
}

void ui_destroy_window(ui_ctx_t *ctx, ui_wctx_t *wctx)
{
    if (!wctx)
        return;
    if (wctx->prev)
        wctx->prev->next = wctx->next;
    if (wctx->next)
        wctx->next->prev = wctx->prev;
    if (ctx->first_window == wctx)
        ctx->first_window = wctx->next;

    bui_gfx_destroy(wctx);
    free(wctx->layout_stack);
    free(wctx);
}

bool ui_pump_events(ui_ctx_t *ctx)
{
    if (!ctx || !ctx->running || ctx->first_window == NULL)
        return false;

    _ui_clear_transient_events(ctx);

    bui_event_t event = {0};
    if (!bui_poll_events(&event)) {
        bui_delay(1);
        return true;
    }

    ui_wctx_t *wctx = _ui_find_window_by_id(ctx, event.window_id);
    if (!wctx && event.type != BUI_EVENT_WINDOW_CLOSED)
        return true;

    if (wctx)
        wctx->last_event = event;

    if (event.type == BUI_EVENT_WINDOW_CLOSED) {
        if (wctx) {
            wctx->close_requested = true;
            wctx->state = UI_WINDOW_STATE_PENDING;
        }
        ctx->running = false;
        return false;
    }

    if (!wctx)
        return true;

    switch (event.type) {
    case BUI_EVENT_WINDOW_CREATED:
    case BUI_EVENT_WINDOW_RESIZED:
        wctx->width = event.window_resized.width;
        wctx->height = event.window_resized.height;
        break;
    case BUI_EVENT_KEY_DOWN:
        if (event.keyboard < 256)
            wctx->keyboard_keys[event.keyboard] = BUI_KEY_STATE_DOWN;
        break;
    case BUI_EVENT_KEY_HOLD:
        if (event.keyboard < 256)
            wctx->keyboard_keys[event.keyboard] = BUI_KEY_STATE_HOLD;
        break;
    case BUI_EVENT_KEY_UP:
        if (event.keyboard < 256)
            wctx->keyboard_keys[event.keyboard] = BUI_KEY_STATE_UP;
        break;
    case BUI_EVENT_MOUSE_MOVE:
        wctx->mouse_state.pos_x = event.mouse_move.pos_x;
        wctx->mouse_state.pos_y = event.mouse_move.pos_y;
        break;
    case BUI_EVENT_MOUSE_BUTTON_DOWN:
        wctx->mouse_state.pos_x = event.mouse_button.pos_x;
        wctx->mouse_state.pos_y = event.mouse_button.pos_y;
        wctx->mouse_state.buttons_state |= event.mouse_button.button;
        break;
    case BUI_EVENT_MOUSE_BUTTON_UP:
        wctx->mouse_state.pos_x = event.mouse_button.pos_x;
        wctx->mouse_state.pos_y = event.mouse_button.pos_y;
        wctx->mouse_state.buttons_state &= (uint8_t) ~event.mouse_button.button;
        break;
    default:
        break;
    }

    return true;
}

bool ui_begin_window(ui_wctx_t *wctx)
{
    if (!wctx || wctx->state != UI_WINDOW_STATE_ACTIVE)
        return false;

    wctx->layout_count = 0;
    bui_begin_frame(wctx);
    bui_reset_clip(wctx);
    bui_draw_filled_rect(
        wctx,
        (bui_rect_t){.x = 0, .y = 0, .width = wctx->width, .height = wctx->height},
        BACKGROUND_COLOR);
    ui_push_layout(
        wctx,
        (bui_layout_t){
            .type = UI_LAYOUT_VERTICAL,
            .size = (bui_area_t){0, 0, wctx->width, wctx->height},
            .cursor_pos_x = 0,
            .cursor_pos_y = 0,
            .padding_x = 0,
            .padding_y = 0,
            .inner_margin = {0},
            .outer_margin = {0},
        });
    return true;
}

bool ui_end_window(ui_wctx_t *wctx)
{
    if (!wctx || wctx->state != UI_WINDOW_STATE_ACTIVE)
        return false;
    bui_end_frame(wctx);
    return true;
}

bool ui_push_layout(ui_wctx_t *wctx, bui_layout_t layout)
{
    if (wctx->layout_count >= wctx->layout_capacity) {
        size_t new_capacity = wctx->layout_capacity * 2;
        bui_layout_t *new_stack = realloc(wctx->layout_stack, new_capacity * sizeof(bui_layout_t));
        if (new_stack == NULL)
            return false;
        wctx->layout_stack = new_stack;
        wctx->layout_capacity = new_capacity;
    }
    wctx->layout_stack[wctx->layout_count++] = layout;
    return true;
}

bui_layout_t *ui_get_layout(ui_wctx_t *wctx)
{
    if (!wctx || wctx->layout_count == 0)
        return NULL;
    return &wctx->layout_stack[wctx->layout_count - 1];
}

void ui_advance_layout(ui_wctx_t *wctx, uint32_t w, uint32_t h)
{
    bui_layout_t *current = ui_get_layout(wctx);
    if (!current)
        return;
    if (current->type == UI_LAYOUT_HORIZONTAL) {
        uint32_t item_right
            = _sub_or_zero(current->cursor_pos_x, current->size.x + current->inner_margin.l) + w;
        if (item_right > current->content_width)
            current->content_width = item_right;
        if (h > current->content_height)
            current->content_height = h;
        current->cursor_pos_x += w + current->padding_x;
    } else if (current->type == UI_LAYOUT_VERTICAL) {
        uint32_t item_bottom
            = _sub_or_zero(current->cursor_pos_y, current->size.y + current->inner_margin.t) + h;
        if (w > current->content_width)
            current->content_width = w;
        if (item_bottom > current->content_height)
            current->content_height = item_bottom;
        current->cursor_pos_y += h + current->padding_y;
    }
}

bui_layout_t ui_pop_layout(ui_wctx_t *wctx)
{
    if (!wctx || wctx->layout_count == 0)
        return (bui_layout_t){0};
    bui_layout_t result = wctx->layout_stack[--wctx->layout_count];
    if (result.fit_width)
        result.size.width = result.content_width + result.inner_margin.l + result.inner_margin.r;
    if (result.fit_height)
        result.size.height = result.content_height + result.inner_margin.t + result.inner_margin.b;
    return result;
}

static uint32_t _ui_get_available_space(ui_wctx_t *wctx, bui_layout_type_t layout_type)
{
    bui_layout_t *current = ui_get_layout(wctx);
    if (!current)
        return 0;
    if (layout_type == UI_LAYOUT_VERTICAL) {
        uint32_t height
            = _sub_or_zero(current->size.height, current->inner_margin.t + current->inner_margin.b);
        uint32_t used
            = _sub_or_zero(current->cursor_pos_y, current->size.y + current->inner_margin.t);
        return _sub_or_zero(height, used);
    } else if (layout_type == UI_LAYOUT_HORIZONTAL) {
        uint32_t width
            = _sub_or_zero(current->size.width, current->inner_margin.l + current->inner_margin.r);
        uint32_t used
            = _sub_or_zero(current->cursor_pos_x, current->size.x + current->inner_margin.l);
        return _sub_or_zero(width, used);
    }
    return 0;
}

void ui_begin_column(ui_wctx_t *wctx, bui_rtlb_t margin)
{
    bui_layout_t *current = ui_get_layout(wctx);
    if (!current)
        return;
    uint32_t available_width
        = _sub_or_zero(_ui_get_available_space(wctx, UI_LAYOUT_HORIZONTAL), margin.l + margin.r);
    uint32_t available_height
        = _sub_or_zero(_ui_get_available_space(wctx, UI_LAYOUT_VERTICAL), margin.t + margin.b);

    ui_push_layout(
        wctx,
        (bui_layout_t){
            .type = UI_LAYOUT_VERTICAL,
            .size = (bui_area_t){
                .x = current->cursor_pos_x,
                .y = current->cursor_pos_y,
                .width = available_width + margin.l + margin.r,
                .height = available_height + margin.t + margin.b,
            },
            .inner_margin = margin,
            .cursor_pos_x = current->cursor_pos_x + margin.l,
            .cursor_pos_y = current->cursor_pos_y + margin.t,
            .padding_x = 0,
            .padding_y = DEFAULT_PADDING,
            .fit_width = true,
            .fit_height = true,
        });
}

void ui_end_column(ui_wctx_t *wctx)
{
    bui_layout_t column_layout = ui_pop_layout(wctx);
    ui_advance_layout(
        wctx,
        column_layout.size.width + column_layout.outer_margin.l + column_layout.outer_margin.r,
        column_layout.size.height + column_layout.outer_margin.t + column_layout.outer_margin.b);
}

void ui_begin_row(ui_wctx_t *wctx, bui_rtlb_t margin)
{
    bui_layout_t *current = ui_get_layout(wctx);
    if (!current)
        return;
    uint32_t available_width
        = _sub_or_zero(_ui_get_available_space(wctx, UI_LAYOUT_HORIZONTAL), margin.l + margin.r);
    uint32_t available_height
        = _sub_or_zero(_ui_get_available_space(wctx, UI_LAYOUT_VERTICAL), margin.t + margin.b);

    ui_push_layout(
        wctx,
        (bui_layout_t){
            .type = UI_LAYOUT_HORIZONTAL,
            .size = (bui_area_t){
                .x = current->cursor_pos_x,
                .y = current->cursor_pos_y,
                .width = available_width + margin.l + margin.r,
                .height = available_height + margin.t + margin.b,
            },
            .inner_margin = margin,
            .cursor_pos_x = current->cursor_pos_x + margin.l,
            .cursor_pos_y = current->cursor_pos_y + margin.t,
            .padding_x = DEFAULT_PADDING,
            .padding_y = 0,
            .fit_width = true,
            .fit_height = true,
        });
}

void ui_end_row(ui_wctx_t *wctx)
{
    bui_layout_t row_layout = ui_pop_layout(wctx);
    ui_advance_layout(
        wctx,
        row_layout.size.width + row_layout.outer_margin.l + row_layout.outer_margin.r,
        row_layout.size.height + row_layout.outer_margin.t + row_layout.outer_margin.b);
}

void ui_begin_container(
    ui_wctx_t *wctx,
    uint32_t width,
    uint32_t height,
    bui_rtlb_t outer_margin,
    bui_rtlb_t inner_margin)
{
    bui_layout_t *current = ui_get_layout(wctx);
    if (!current)
        return;
    uint32_t available_width = _sub_or_zero(
        _ui_get_available_space(wctx, UI_LAYOUT_HORIZONTAL), outer_margin.l + outer_margin.r);
    uint32_t available_height = _sub_or_zero(
        _ui_get_available_space(wctx, UI_LAYOUT_VERTICAL), outer_margin.t + outer_margin.b);
    uint32_t layout_width = width == UINT32_MAX ? available_width : width;
    uint32_t layout_height = height == UINT32_MAX ? available_height : height;
    bool fit_width = width == 0;
    bool fit_height = height == 0;

    if (fit_width || layout_width > available_width)
        layout_width = available_width;
    if (fit_height || layout_height > available_height)
        layout_height = available_height;

    ui_push_layout(
        wctx,
        (bui_layout_t){
            .type = UI_LAYOUT_VERTICAL,
            .size = (bui_area_t){
                .x = current->cursor_pos_x + outer_margin.l,
                .y = current->cursor_pos_y + outer_margin.t,
                .width = layout_width,
                .height = layout_height,
            },
            .inner_margin = inner_margin,
            .outer_margin = outer_margin,
            .cursor_pos_x = current->cursor_pos_x + outer_margin.l + inner_margin.l,
            .cursor_pos_y = current->cursor_pos_y + outer_margin.t + inner_margin.t,
            .padding_x = 0,
            .padding_y = DEFAULT_PADDING,
            .fit_width = fit_width,
            .fit_height = fit_height,
        });
}

void ui_end_container(ui_wctx_t *wctx)
{
    bui_layout_t container_layout = ui_pop_layout(wctx);
    bui_area_t size = container_layout.size;

    bui_draw_rect(
        wctx,
        (bui_rect_t){
            .x = size.x,
            .y = size.y,
            .width = size.width,
            .height = size.height,
            .border_color = BORDER_COLOR,
            .border_thickness = BORDER_THICKNESS,
        });

    if (size.width > BORDER_THICKNESS * 2 && size.height > BORDER_THICKNESS * 2) {
        bui_draw_rect(
            wctx,
            (bui_rect_t){
                .x = size.x + BORDER_THICKNESS,
                .y = size.y + BORDER_THICKNESS,
                .width = size.width - BORDER_THICKNESS * 2,
                .height = size.height - BORDER_THICKNESS * 2,
                .border_color = BORDER_SHADOW,
                .border_thickness = BORDER_THICKNESS,
            });
    }

    ui_advance_layout(
        wctx,
        container_layout.size.width + container_layout.outer_margin.l
            + container_layout.outer_margin.r,
        container_layout.size.height + container_layout.outer_margin.t
            + container_layout.outer_margin.b);
}

static bool _ui_has_visible_space(ui_wctx_t *wctx)
{
    return _ui_get_available_space(wctx, UI_LAYOUT_HORIZONTAL) > 0
           && _ui_get_available_space(wctx, UI_LAYOUT_VERTICAL) > 0;
}

void ui_label(ui_wctx_t *wctx, const char *label)
{
    bui_layout_t *layout = ui_get_layout(wctx);
    if (!layout)
        return;
    bui_area_t text_area = bui_get_text_area(&_default_font, label);

    if (!_ui_has_visible_space(wctx))
        return;

    bui_set_clip(
        wctx,
        (bui_area_t){
            .x = layout->cursor_pos_x,
            .y = layout->cursor_pos_y,
            .width = _ui_get_available_space(wctx, UI_LAYOUT_HORIZONTAL),
            .height = _ui_get_available_space(wctx, UI_LAYOUT_VERTICAL),
        });
    bui_draw_text(
        wctx,
        &_default_font,
        (bui_pos_t){layout->cursor_pos_x + text_area.x, layout->cursor_pos_y + text_area.y},
        TEXT_COLOR,
        label);
    bui_reset_clip(wctx);

    ui_advance_layout(wctx, text_area.width, text_area.height);
}

bool ui_is_mouse_in_area(ui_wctx_t *wctx, bui_area_t area)
{
    return wctx->mouse_state.pos_x >= area.x && wctx->mouse_state.pos_x < area.x + area.width
           && wctx->mouse_state.pos_y >= area.y && wctx->mouse_state.pos_y < area.y + area.height;
}

bui_pos_t ui_get_mouse_pos(ui_wctx_t *wctx)
{
    return (bui_pos_t){wctx->mouse_state.pos_x, wctx->mouse_state.pos_y};
}

bool ui_is_mouse_button_down(ui_wctx_t *wctx, bui_mouse_button_t button)
{
    return (wctx->mouse_state.buttons_state & button) != 0;
}

bool ui_is_key_down(ui_wctx_t *wctx, bui_keyboard_scancode_t keycode)
{
    return wctx && keycode < 256
           && (wctx->keyboard_keys[keycode] == BUI_KEY_STATE_DOWN
               || wctx->keyboard_keys[keycode] == BUI_KEY_STATE_HOLD);
}

bool ui_is_key_up(ui_wctx_t *wctx, bui_keyboard_scancode_t keycode)
{
    return !wctx || keycode >= 256 || wctx->keyboard_keys[keycode] == BUI_KEY_STATE_UP;
}

bool ui_button(ui_wctx_t *wctx, const char *label)
{
    bui_layout_t *layout = ui_get_layout(wctx);
    if (!layout)
        return false;
    bui_area_t text_area = bui_get_text_area(&_default_font, label);
    bui_area_t button_area = {
        .x = layout->cursor_pos_x,
        .y = layout->cursor_pos_y,
        .width = text_area.width + DEFAULT_PADDING * 2,
        .height = text_area.height + DEFAULT_PADDING * 2,
    };

    if (!_ui_has_visible_space(wctx))
        return false;

    bui_set_clip(
        wctx,
        (bui_area_t){
            .x = layout->cursor_pos_x,
            .y = layout->cursor_pos_y,
            .width = _ui_get_available_space(wctx, UI_LAYOUT_HORIZONTAL),
            .height = _ui_get_available_space(wctx, UI_LAYOUT_VERTICAL),
        });
    bui_draw_filled_rect(
        wctx,
        (bui_rect_t){
            .x = button_area.x,
            .y = button_area.y,
            .width = button_area.width,
            .height = button_area.height,
            .border_color = BORDER_COLOR,
            .border_thickness = BORDER_THICKNESS,
        },
        SECONDARY_BACKGROUND_COLOR);
    if (button_area.width > BORDER_THICKNESS * 2 && button_area.height > BORDER_THICKNESS * 2) {
        bui_draw_rect(
            wctx,
            (bui_rect_t){
                .x = button_area.x + BORDER_THICKNESS,
                .y = button_area.y + BORDER_THICKNESS,
                .width = button_area.width - BORDER_THICKNESS * 2,
                .height = button_area.height - BORDER_THICKNESS * 2,
                .border_color = BORDER_SHADOW,
                .border_thickness = BORDER_THICKNESS,
            });
    }
    bui_draw_text(
        wctx,
        &_default_font,
        (bui_pos_t){
            .x = button_area.x + text_area.x + DEFAULT_PADDING,
            .y = button_area.y + text_area.y + DEFAULT_PADDING,
        },
        TEXT_COLOR,
        label);
    bui_reset_clip(wctx);

    ui_advance_layout(wctx, button_area.width, button_area.height);

    return wctx->last_event.type == BUI_EVENT_MOUSE_BUTTON_DOWN
           && wctx->last_event.mouse_button.button == BUI_MOUSE_BUTTON_LEFT
           && ui_is_mouse_in_area(wctx, button_area);
}

ui_textbox_state_t ui_new_textbox_state(char *buffer, size_t buffer_size)
{
    return (ui_textbox_state_t){.buffer = buffer, .buffer_size = buffer_size, .focused = false};
}

static void _ui_textbox_append(ui_textbox_state_t *state, const char *text)
{
    if (!state || !state->buffer || state->buffer_size == 0 || !text)
        return;
    size_t len = strlen(state->buffer);
    if (len >= state->buffer_size - 1)
        return;
    size_t remaining = state->buffer_size - 1 - len;
    strncat(state->buffer, text, remaining);
}

static char _ui_textbox_char_from_key(ui_wctx_t *wctx, bui_keyboard_scancode_t key)
{
    if (ui_is_key_down(wctx, BUI_KEYBOARD_SCANCODE_LCTRL)
        || ui_is_key_down(wctx, BUI_KEYBOARD_SCANCODE_LALT))
        return '\0';

    bool shifted = ui_is_key_down(wctx, BUI_KEYBOARD_SCANCODE_LSHIFT)
                   || ui_is_key_down(wctx, BUI_KEYBOARD_SCANCODE_RSHIFT);

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

void ui_textbox(ui_wctx_t *wctx, ui_textbox_state_t *state)
{
    bui_layout_t *layout = ui_get_layout(wctx);
    if (!layout || !state)
        return;
    bui_area_t textbox_area = {
        .x = layout->cursor_pos_x,
        .y = layout->cursor_pos_y,
        .width = _ui_get_available_space(wctx, UI_LAYOUT_HORIZONTAL),
        .height = 13 + DEFAULT_PADDING * 2,
    };

    if (!_ui_has_visible_space(wctx))
        return;

    bui_set_clip(
        wctx,
        (bui_area_t){
            .x = layout->cursor_pos_x,
            .y = layout->cursor_pos_y,
            .width = _ui_get_available_space(wctx, UI_LAYOUT_HORIZONTAL),
            .height = _ui_get_available_space(wctx, UI_LAYOUT_VERTICAL),
        });
    bui_draw_filled_rect(
        wctx,
        (bui_rect_t){
            .x = textbox_area.x,
            .y = textbox_area.y,
            .width = textbox_area.width,
            .height = textbox_area.height,
            .border_color = BORDER_COLOR,
            .border_thickness = BORDER_THICKNESS,
        },
        SECONDARY_BACKGROUND_COLOR);
    if (textbox_area.width > BORDER_THICKNESS * 2 && textbox_area.height > BORDER_THICKNESS * 2) {
        bui_draw_rect(
            wctx,
            (bui_rect_t){
                .x = textbox_area.x + BORDER_THICKNESS,
                .y = textbox_area.y + BORDER_THICKNESS,
                .width = textbox_area.width - BORDER_THICKNESS * 2,
                .height = textbox_area.height - BORDER_THICKNESS * 2,
                .border_color = BORDER_SHADOW,
                .border_thickness = BORDER_THICKNESS,
            });
    }

    if (wctx->last_event.type == BUI_EVENT_MOUSE_BUTTON_DOWN
        && wctx->last_event.mouse_button.button == BUI_MOUSE_BUTTON_LEFT)
        state->focused = ui_is_mouse_in_area(wctx, textbox_area);

    if (state->focused
        && (wctx->last_event.type == BUI_EVENT_KEY_DOWN
            || wctx->last_event.type == BUI_EVENT_KEY_HOLD)) {
        char text[2] = {_ui_textbox_char_from_key(wctx, wctx->last_event.keyboard), '\0'};
        if (text[0] != '\0')
            _ui_textbox_append(state, text);
    }

    if (state->focused
        && (wctx->last_event.type == BUI_EVENT_KEY_DOWN
            || wctx->last_event.type == BUI_EVENT_KEY_HOLD)
        && wctx->last_event.keyboard == BUI_KEYBOARD_SCANCODE_BACKSPACE && state->buffer
        && state->buffer[0] != '\0')
        state->buffer[strlen(state->buffer) - 1] = '\0';

    if (state->buffer && state->buffer[0] != '\0') {
        bui_area_t text_area = bui_get_text_area(&_default_font, state->buffer);
        bui_draw_text(
            wctx,
            &_default_font,
            (bui_pos_t){
                .x = textbox_area.x + 5 + text_area.x,
                .y = textbox_area.y + DEFAULT_PADDING + text_area.y,
            },
            TEXT_COLOR,
            state->buffer);
    }

    if (state->focused) {
        uint32_t caret_x = textbox_area.x + 5;
        if (state->buffer)
            caret_x += bui_get_text_width(&_default_font, state->buffer);
        bui_draw_line(
            wctx,
            (bui_line_t){
                .x1 = caret_x,
                .y1 = textbox_area.y + 4,
                .x2 = caret_x,
                .y2 = textbox_area.y + textbox_area.height - 6,
                .thickness = BORDER_THICKNESS,
            },
            TEXT_COLOR);
    }
    bui_reset_clip(wctx);

    ui_advance_layout(wctx, textbox_area.width, textbox_area.height);
}
