/*
 * Copyright (c) 2026, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <SDL3/SDL.h>
#include <libui/libui.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "font.h"

#define INITIAL_STACK_CAPACITY 16

#define DEFAULT_PADDING 7
#define BACKGROUND_COLOR (gfx_color_t){0xFF, 0x21, 0x21, 0x21}
#define SECONDARY_BACKGROUND_COLOR (gfx_color_t){0xFF, 0x18, 0x18, 0x18}
#define TEXT_COLOR (gfx_color_t){0xFF, 0xB3, 0xB3, 0xB3}
#define BORDER_COLOR (gfx_color_t){0xFF, 0x00, 0x00, 0x00}
#define BORDER_THICKNESS 1
#define BORDER_SHADOW (gfx_color_t){0x33, 0xFF, 0xFF, 0xFF}

static gfx_font_t _default_font = {
    .atlas = font_atlas,
    .atlas_size = {FONT_ATLAS_WIDTH, FONT_ATLAS_HEIGHT},
    .glyphs = font_glyphs,
    .first_char = FONT_FIRST_CHAR,
    .last_char = FONT_LAST_CHAR,
    .size = {13, FONT_LINE_HEIGHT},
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

static input_keyboard_scancode_t _ui_translate_scancode(SDL_Scancode scancode)
{
    switch (scancode) {
    case SDL_SCANCODE_ESCAPE:
        return INPUT_KEYBOARD_SCANCODE_ESCAPE;
    case SDL_SCANCODE_1:
        return INPUT_KEYBOARD_SCANCODE_1;
    case SDL_SCANCODE_2:
        return INPUT_KEYBOARD_SCANCODE_2;
    case SDL_SCANCODE_3:
        return INPUT_KEYBOARD_SCANCODE_3;
    case SDL_SCANCODE_4:
        return INPUT_KEYBOARD_SCANCODE_4;
    case SDL_SCANCODE_5:
        return INPUT_KEYBOARD_SCANCODE_5;
    case SDL_SCANCODE_6:
        return INPUT_KEYBOARD_SCANCODE_6;
    case SDL_SCANCODE_7:
        return INPUT_KEYBOARD_SCANCODE_7;
    case SDL_SCANCODE_8:
        return INPUT_KEYBOARD_SCANCODE_8;
    case SDL_SCANCODE_9:
        return INPUT_KEYBOARD_SCANCODE_9;
    case SDL_SCANCODE_0:
        return INPUT_KEYBOARD_SCANCODE_0;
    case SDL_SCANCODE_MINUS:
        return INPUT_KEYBOARD_SCANCODE_MINUS;
    case SDL_SCANCODE_EQUALS:
        return INPUT_KEYBOARD_SCANCODE_EQUALS;
    case SDL_SCANCODE_BACKSPACE:
        return INPUT_KEYBOARD_SCANCODE_BACKSPACE;
    case SDL_SCANCODE_TAB:
        return INPUT_KEYBOARD_SCANCODE_TAB;
    case SDL_SCANCODE_Q:
        return INPUT_KEYBOARD_SCANCODE_Q;
    case SDL_SCANCODE_W:
        return INPUT_KEYBOARD_SCANCODE_W;
    case SDL_SCANCODE_E:
        return INPUT_KEYBOARD_SCANCODE_E;
    case SDL_SCANCODE_R:
        return INPUT_KEYBOARD_SCANCODE_R;
    case SDL_SCANCODE_T:
        return INPUT_KEYBOARD_SCANCODE_T;
    case SDL_SCANCODE_Y:
        return INPUT_KEYBOARD_SCANCODE_Y;
    case SDL_SCANCODE_U:
        return INPUT_KEYBOARD_SCANCODE_U;
    case SDL_SCANCODE_I:
        return INPUT_KEYBOARD_SCANCODE_I;
    case SDL_SCANCODE_O:
        return INPUT_KEYBOARD_SCANCODE_O;
    case SDL_SCANCODE_P:
        return INPUT_KEYBOARD_SCANCODE_P;
    case SDL_SCANCODE_LEFTBRACKET:
        return INPUT_KEYBOARD_SCANCODE_LBRACKET;
    case SDL_SCANCODE_RIGHTBRACKET:
        return INPUT_KEYBOARD_SCANCODE_RBRACKET;
    case SDL_SCANCODE_RETURN:
        return INPUT_KEYBOARD_SCANCODE_ENTER;
    case SDL_SCANCODE_LCTRL:
        return INPUT_KEYBOARD_SCANCODE_LCTRL;
    case SDL_SCANCODE_A:
        return INPUT_KEYBOARD_SCANCODE_A;
    case SDL_SCANCODE_S:
        return INPUT_KEYBOARD_SCANCODE_S;
    case SDL_SCANCODE_D:
        return INPUT_KEYBOARD_SCANCODE_D;
    case SDL_SCANCODE_F:
        return INPUT_KEYBOARD_SCANCODE_F;
    case SDL_SCANCODE_G:
        return INPUT_KEYBOARD_SCANCODE_G;
    case SDL_SCANCODE_H:
        return INPUT_KEYBOARD_SCANCODE_H;
    case SDL_SCANCODE_J:
        return INPUT_KEYBOARD_SCANCODE_J;
    case SDL_SCANCODE_K:
        return INPUT_KEYBOARD_SCANCODE_K;
    case SDL_SCANCODE_L:
        return INPUT_KEYBOARD_SCANCODE_L;
    case SDL_SCANCODE_SEMICOLON:
        return INPUT_KEYBOARD_SCANCODE_SEMICOLON;
    case SDL_SCANCODE_APOSTROPHE:
        return INPUT_KEYBOARD_SCANCODE_APOSTROPHE;
    case SDL_SCANCODE_GRAVE:
        return INPUT_KEYBOARD_SCANCODE_BACKTICK;
    case SDL_SCANCODE_LSHIFT:
        return INPUT_KEYBOARD_SCANCODE_LSHIFT;
    case SDL_SCANCODE_BACKSLASH:
        return INPUT_KEYBOARD_SCANCODE_BACKSLASH;
    case SDL_SCANCODE_Z:
        return INPUT_KEYBOARD_SCANCODE_Z;
    case SDL_SCANCODE_X:
        return INPUT_KEYBOARD_SCANCODE_X;
    case SDL_SCANCODE_C:
        return INPUT_KEYBOARD_SCANCODE_C;
    case SDL_SCANCODE_V:
        return INPUT_KEYBOARD_SCANCODE_V;
    case SDL_SCANCODE_B:
        return INPUT_KEYBOARD_SCANCODE_B;
    case SDL_SCANCODE_N:
        return INPUT_KEYBOARD_SCANCODE_N;
    case SDL_SCANCODE_M:
        return INPUT_KEYBOARD_SCANCODE_M;
    case SDL_SCANCODE_COMMA:
        return INPUT_KEYBOARD_SCANCODE_COMMA;
    case SDL_SCANCODE_PERIOD:
        return INPUT_KEYBOARD_SCANCODE_PERIOD;
    case SDL_SCANCODE_SLASH:
        return INPUT_KEYBOARD_SCANCODE_SLASH;
    case SDL_SCANCODE_RSHIFT:
        return INPUT_KEYBOARD_SCANCODE_RSHIFT;
    case SDL_SCANCODE_LALT:
        return INPUT_KEYBOARD_SCANCODE_LALT;
    case SDL_SCANCODE_SPACE:
        return INPUT_KEYBOARD_SCANCODE_SPACE;
    case SDL_SCANCODE_CAPSLOCK:
        return INPUT_KEYBOARD_SCANCODE_CAPSLOCK;
    case SDL_SCANCODE_F1:
        return INPUT_KEYBOARD_SCANCODE_F1;
    case SDL_SCANCODE_F2:
        return INPUT_KEYBOARD_SCANCODE_F2;
    case SDL_SCANCODE_F3:
        return INPUT_KEYBOARD_SCANCODE_F3;
    case SDL_SCANCODE_F4:
        return INPUT_KEYBOARD_SCANCODE_F4;
    case SDL_SCANCODE_F5:
        return INPUT_KEYBOARD_SCANCODE_F5;
    case SDL_SCANCODE_F6:
        return INPUT_KEYBOARD_SCANCODE_F6;
    case SDL_SCANCODE_F7:
        return INPUT_KEYBOARD_SCANCODE_F7;
    case SDL_SCANCODE_F8:
        return INPUT_KEYBOARD_SCANCODE_F8;
    case SDL_SCANCODE_F9:
        return INPUT_KEYBOARD_SCANCODE_F9;
    case SDL_SCANCODE_F10:
        return INPUT_KEYBOARD_SCANCODE_F10;
    case SDL_SCANCODE_F11:
        return INPUT_KEYBOARD_SCANCODE_F11;
    case SDL_SCANCODE_F12:
        return INPUT_KEYBOARD_SCANCODE_F12;
    case SDL_SCANCODE_UP:
        return INPUT_KEYBOARD_SCANCODE_UP;
    case SDL_SCANCODE_LEFT:
        return INPUT_KEYBOARD_SCANCODE_LEFT;
    case SDL_SCANCODE_RIGHT:
        return INPUT_KEYBOARD_SCANCODE_RIGHT;
    case SDL_SCANCODE_DOWN:
        return INPUT_KEYBOARD_SCANCODE_DOWN;
    case SDL_SCANCODE_DELETE:
        return INPUT_KEYBOARD_SCANCODE_DELETE;
    default:
        return INPUT_KEYBOARD_SCANCODE_NULL;
    }
}

static void _ui_clear_transient_events(ui_ctx_t *ctx)
{
    for (ui_wctx_t *wctx = ctx->first_window; wctx; wctx = wctx->next)
        wctx->last_event = (input_event_t){0};
}

static bool _ui_resize_window(ui_wctx_t *wctx, int width, int height)
{
    if (width <= 0 || height <= 0)
        return false;

    uint32_t *new_backbuffer
        = realloc(wctx->framebuffer.backbuffer, (size_t) width * (size_t) height * sizeof(uint32_t));
    if (!new_backbuffer)
        return false;

    SDL_Texture *old_texture = (SDL_Texture *) wctx->framebuffer.native_texture;
    if (old_texture)
        SDL_DestroyTexture(old_texture);

    SDL_Texture *texture = SDL_CreateTexture(
        (SDL_Renderer *) wctx->framebuffer.native_renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        width,
        height);
    if (!texture)
        return false;

    wctx->width = width;
    wctx->height = height;
    wctx->framebuffer.backbuffer = new_backbuffer;
    wctx->framebuffer.framebuffer = new_backbuffer;
    wctx->framebuffer.width = (size_t) width;
    wctx->framebuffer.height = (size_t) height;
    wctx->framebuffer.pitch = (size_t) width * sizeof(uint32_t);
    wctx->framebuffer.native_texture = texture;
    gfx_reset_clip(&wctx->framebuffer);
    return true;
}

bool ui_init_context(ui_ctx_t *ctx)
{
    memset(ctx, 0, sizeof(ui_ctx_t));
    if (!SDL_Init(SDL_INIT_VIDEO))
        return false;
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
    SDL_Quit();
}

ui_wctx_t *ui_new_window(ui_ctx_t *ctx, const char *title, int w, int h, window_flags_t flags)
{
    ui_wctx_t *wctx = calloc(1, sizeof(ui_wctx_t));
    if (!wctx)
        return NULL;

    SDL_WindowFlags sdl_flags = flags.resizable ? SDL_WINDOW_RESIZABLE : 0;
    SDL_Window *window = SDL_CreateWindow(title, w, h, sdl_flags);
    if (!window) {
        free(wctx);
        return NULL;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        SDL_DestroyWindow(window);
        free(wctx);
        return NULL;
    }

    SDL_Texture *texture
        = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
    if (!texture) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        free(wctx);
        return NULL;
    }

    uint32_t *backbuffer = calloc((size_t) w * (size_t) h, sizeof(uint32_t));
    if (!backbuffer) {
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        free(wctx);
        return NULL;
    }

    wctx->title = title;
    wctx->width = w;
    wctx->height = h;
    wctx->create_sequence = (uint32_t) SDL_GetWindowID(window);
    wctx->state = UI_WINDOW_STATE_ACTIVE;
    wctx->window_id = (uint32_t) SDL_GetWindowID(window);
    wctx->framebuffer = (gfx_context_t){
        .framebuffer = backbuffer,
        .backbuffer = backbuffer,
        .width = (size_t) w,
        .height = (size_t) h,
        .pitch = (size_t) w * sizeof(uint32_t),
        .bpp = 32,
        .memory_model = 1,
        .red_mask_size = 8,
        .red_mask_shift = 16,
        .green_mask_size = 8,
        .green_mask_shift = 8,
        .blue_mask_size = 8,
        .blue_mask_shift = 0,
        .fps_last_update_ticks = SDL_GetTicks(),
        .clip_rect = {0, 0, (uint32_t) w, (uint32_t) h},
        .native_window = window,
        .native_renderer = renderer,
        .native_texture = texture,
    };

    wctx->layout_stack = malloc(INITIAL_STACK_CAPACITY * sizeof(ui_layout_t));
    if (wctx->layout_stack == NULL) {
        free(backbuffer);
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        free(wctx);
        return NULL;
    }
    wctx->layout_count = 0;
    wctx->layout_capacity = INITIAL_STACK_CAPACITY;

    for (size_t i = 0; i < 256; i++)
        wctx->keyboard_keys[i] = INPUT_KEYACTION_UP;
    wctx->mouse_state = (ui_mouse_state_t){0};

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

    SDL_Texture *texture = (SDL_Texture *) wctx->framebuffer.native_texture;
    SDL_Renderer *renderer = (SDL_Renderer *) wctx->framebuffer.native_renderer;
    SDL_Window *window = (SDL_Window *) wctx->framebuffer.native_window;
    if (texture)
        SDL_DestroyTexture(texture);
    if (renderer)
        SDL_DestroyRenderer(renderer);
    if (window)
        SDL_DestroyWindow(window);
    free(wctx->framebuffer.backbuffer);
    free(wctx->layout_stack);
    free(wctx);
}

bool ui_pump_events(ui_ctx_t *ctx)
{
    if (!ctx || !ctx->running || ctx->first_window == NULL)
        return false;

    _ui_clear_transient_events(ctx);

    SDL_Event event;
    if (!SDL_PollEvent(&event)) {
        SDL_Delay(1);
        return true;
    }

    if (event.type == SDL_EVENT_QUIT) {
        ctx->running = false;
        return false;
    }

    uint32_t window_id = 0;
    if (event.type >= SDL_EVENT_WINDOW_FIRST && event.type <= SDL_EVENT_WINDOW_LAST)
        window_id = event.window.windowID;
    else if (event.type == SDL_EVENT_MOUSE_MOTION)
        window_id = event.motion.windowID;
    else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP)
        window_id = event.button.windowID;
    else if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP)
        window_id = event.key.windowID;
    else if (event.type == SDL_EVENT_TEXT_INPUT)
        window_id = event.text.windowID;

    ui_wctx_t *wctx = window_id ? _ui_find_window_by_id(ctx, window_id) : ctx->first_window;
    if (!wctx)
        return true;

    if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
        wctx->close_requested = true;
        wctx->state = UI_WINDOW_STATE_PENDING;
        ctx->running = false;
        return false;
    }

    if (event.type == SDL_EVENT_WINDOW_RESIZED) {
        return _ui_resize_window(wctx, event.window.data1, event.window.data2);
    }

    if (event.type == SDL_EVENT_MOUSE_MOTION) {
        wctx->mouse_state.pos_x = (uint32_t) event.motion.x;
        wctx->mouse_state.pos_y = (uint32_t) event.motion.y;
        wctx->last_event.type = INPUT_EVENT_MOUSE;
        wctx->last_event.data.mouse.x = (int32_t) event.motion.x;
        wctx->last_event.data.mouse.y = (int32_t) event.motion.y;
        wctx->last_event.data.mouse.delta_x = (int8_t) event.motion.xrel;
        wctx->last_event.data.mouse.delta_y = (int8_t) event.motion.yrel;
        wctx->last_event.data.mouse.buttons = wctx->mouse_state.buttons_state;
        return true;
    }

    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
        uint8_t bit = 0;
        if (event.button.button == SDL_BUTTON_LEFT)
            bit = INPUT_MOUSE_BUTTON_LEFT;
        else if (event.button.button == SDL_BUTTON_RIGHT)
            bit = INPUT_MOUSE_BUTTON_RIGHT;
        else if (event.button.button == SDL_BUTTON_MIDDLE)
            bit = INPUT_MOUSE_BUTTON_MIDDLE;

        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            wctx->mouse_state.buttons_state |= bit;
        else
            wctx->mouse_state.buttons_state &= (uint8_t) ~bit;

        wctx->mouse_state.pos_x = (uint32_t) event.button.x;
        wctx->mouse_state.pos_y = (uint32_t) event.button.y;
        wctx->last_event.type = INPUT_EVENT_MOUSE;
        wctx->last_event.data.mouse.x = (int32_t) event.button.x;
        wctx->last_event.data.mouse.y = (int32_t) event.button.y;
        wctx->last_event.data.mouse.buttons = wctx->mouse_state.buttons_state;
        return true;
    }

    if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
        input_keyboard_scancode_t scancode = _ui_translate_scancode(event.key.scancode);
        if (scancode != INPUT_KEYBOARD_SCANCODE_NULL) {
            input_keyboard_action_t action = event.type == SDL_EVENT_KEY_UP
                                                 ? INPUT_KEYACTION_UP
                                                 : (event.key.repeat ? INPUT_KEYACTION_HOLD
                                                                     : INPUT_KEYACTION_DOWN);
            wctx->keyboard_keys[scancode] = action;
            wctx->last_event.type = INPUT_EVENT_KEYBOARD;
            wctx->last_event.data.keyboard.scancode = scancode;
            wctx->last_event.data.keyboard.action = action;
        }
        return true;
    }

    if (event.type == SDL_EVENT_TEXT_INPUT) {
        wctx->last_event.type = INPUT_EVENT_TEXT;
        SDL_strlcpy(wctx->last_event.data.text, event.text.text, sizeof(wctx->last_event.data.text));
        return true;
    }

    return true;
}

bool ui_begin_window(ui_wctx_t *wctx)
{
    if (!wctx || wctx->state != UI_WINDOW_STATE_ACTIVE)
        return false;

    wctx->layout_count = 0;
    gfx_context_t *framebuffer = &wctx->framebuffer;
    gfx_begin_frame(framebuffer);
    gfx_reset_clip(framebuffer);
    gfx_draw_filled_rect(
        framebuffer,
        (gfx_rect_t){.x = 0, .y = 0, .width = framebuffer->width, .height = framebuffer->height},
        BACKGROUND_COLOR);
    ui_push_layout(
        wctx,
        (ui_layout_t){
            .type = UI_LAYOUT_VERTICAL,
            .size = (gfx_area_t){0, 0, framebuffer->width, framebuffer->height},
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
    gfx_end_frame(&wctx->framebuffer);
    return true;
}

bool ui_push_layout(ui_wctx_t *wctx, ui_layout_t layout)
{
    if (wctx->layout_count >= wctx->layout_capacity) {
        size_t new_capacity = wctx->layout_capacity * 2;
        ui_layout_t *new_stack = realloc(wctx->layout_stack, new_capacity * sizeof(ui_layout_t));
        if (new_stack == NULL)
            return false;
        wctx->layout_stack = new_stack;
        wctx->layout_capacity = new_capacity;
    }
    wctx->layout_stack[wctx->layout_count++] = layout;
    return true;
}

ui_layout_t *ui_get_layout(ui_wctx_t *wctx)
{
    if (!wctx || wctx->layout_count == 0)
        return NULL;
    return &wctx->layout_stack[wctx->layout_count - 1];
}

void ui_advance_layout(ui_wctx_t *wctx, uint32_t w, uint32_t h)
{
    ui_layout_t *current = ui_get_layout(wctx);
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

ui_layout_t ui_pop_layout(ui_wctx_t *wctx)
{
    if (!wctx || wctx->layout_count == 0)
        return (ui_layout_t){0};
    ui_layout_t result = wctx->layout_stack[--wctx->layout_count];
    if (result.fit_width)
        result.size.width = result.content_width + result.inner_margin.l + result.inner_margin.r;
    if (result.fit_height)
        result.size.height = result.content_height + result.inner_margin.t + result.inner_margin.b;
    return result;
}

static uint32_t _ui_get_available_space(ui_wctx_t *wctx, ui_layout_type_t layout_type)
{
    ui_layout_t *current = ui_get_layout(wctx);
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

void ui_begin_column(ui_wctx_t *wctx, ui_rtlb_t margin)
{
    ui_layout_t *current = ui_get_layout(wctx);
    if (!current)
        return;
    uint32_t available_width
        = _sub_or_zero(_ui_get_available_space(wctx, UI_LAYOUT_HORIZONTAL), margin.l + margin.r);
    uint32_t available_height
        = _sub_or_zero(_ui_get_available_space(wctx, UI_LAYOUT_VERTICAL), margin.t + margin.b);

    ui_push_layout(
        wctx,
        (ui_layout_t){
            .type = UI_LAYOUT_VERTICAL,
            .size = (gfx_area_t){
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
    ui_layout_t column_layout = ui_pop_layout(wctx);
    ui_advance_layout(
        wctx,
        column_layout.size.width + column_layout.outer_margin.l + column_layout.outer_margin.r,
        column_layout.size.height + column_layout.outer_margin.t + column_layout.outer_margin.b);
}

void ui_begin_row(ui_wctx_t *wctx, ui_rtlb_t margin)
{
    ui_layout_t *current = ui_get_layout(wctx);
    if (!current)
        return;
    uint32_t available_width
        = _sub_or_zero(_ui_get_available_space(wctx, UI_LAYOUT_HORIZONTAL), margin.l + margin.r);
    uint32_t available_height
        = _sub_or_zero(_ui_get_available_space(wctx, UI_LAYOUT_VERTICAL), margin.t + margin.b);

    ui_push_layout(
        wctx,
        (ui_layout_t){
            .type = UI_LAYOUT_HORIZONTAL,
            .size = (gfx_area_t){
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
    ui_layout_t row_layout = ui_pop_layout(wctx);
    ui_advance_layout(
        wctx,
        row_layout.size.width + row_layout.outer_margin.l + row_layout.outer_margin.r,
        row_layout.size.height + row_layout.outer_margin.t + row_layout.outer_margin.b);
}

void ui_begin_container(
    ui_wctx_t *wctx, uint32_t width, uint32_t height, ui_rtlb_t outer_margin, ui_rtlb_t inner_margin)
{
    ui_layout_t *current = ui_get_layout(wctx);
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
        (ui_layout_t){
            .type = UI_LAYOUT_VERTICAL,
            .size = (gfx_area_t){
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
    ui_layout_t container_layout = ui_pop_layout(wctx);
    gfx_area_t size = container_layout.size;

    gfx_draw_rect(
        &wctx->framebuffer,
        (gfx_rect_t){
            .x = size.x,
            .y = size.y,
            .width = size.width,
            .height = size.height,
            .border_color = BORDER_COLOR,
            .border_thickness = BORDER_THICKNESS,
        });

    if (size.width > BORDER_THICKNESS * 2 && size.height > BORDER_THICKNESS * 2) {
        gfx_draw_rect(
            &wctx->framebuffer,
            (gfx_rect_t){
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
    ui_layout_t *layout = ui_get_layout(wctx);
    if (!layout)
        return;
    gfx_area_t text_area = gfx_get_text_area(&_default_font, label);

    if (!_ui_has_visible_space(wctx))
        return;

    gfx_set_clip(
        &wctx->framebuffer,
        (gfx_area_t){
            .x = layout->cursor_pos_x,
            .y = layout->cursor_pos_y,
            .width = _ui_get_available_space(wctx, UI_LAYOUT_HORIZONTAL),
            .height = _ui_get_available_space(wctx, UI_LAYOUT_VERTICAL),
        });
    gfx_draw_text(
        &wctx->framebuffer,
        &_default_font,
        (gfx_pos_t){layout->cursor_pos_x + text_area.x, layout->cursor_pos_y + text_area.y},
        TEXT_COLOR,
        label);
    gfx_reset_clip(&wctx->framebuffer);

    ui_advance_layout(wctx, text_area.width, text_area.height);
}

bool ui_is_mouse_in_area(ui_wctx_t *wctx, gfx_area_t area)
{
    return wctx->mouse_state.pos_x >= area.x && wctx->mouse_state.pos_x < area.x + area.width
           && wctx->mouse_state.pos_y >= area.y && wctx->mouse_state.pos_y < area.y + area.height;
}

gfx_pos_t ui_get_mouse_pos(ui_wctx_t *wctx)
{
    return (gfx_pos_t){wctx->mouse_state.pos_x, wctx->mouse_state.pos_y};
}

bool ui_is_mouse_button_down(ui_wctx_t *wctx, input_mouse_button_t button)
{
    return (wctx->mouse_state.buttons_state & button) != 0;
}

bool ui_is_key_down(ui_wctx_t *wctx, input_keyboard_scancode_t keycode)
{
    return wctx->keyboard_keys[keycode] == INPUT_KEYACTION_DOWN
           || wctx->keyboard_keys[keycode] == INPUT_KEYACTION_HOLD;
}

bool ui_is_key_up(ui_wctx_t *wctx, input_keyboard_scancode_t keycode)
{
    return wctx->keyboard_keys[keycode] == INPUT_KEYACTION_UP;
}

bool ui_button(ui_wctx_t *wctx, const char *label)
{
    ui_layout_t *layout = ui_get_layout(wctx);
    if (!layout)
        return false;
    gfx_area_t text_area = gfx_get_text_area(&_default_font, label);
    gfx_area_t button_area = {
        .x = layout->cursor_pos_x,
        .y = layout->cursor_pos_y,
        .width = text_area.width + DEFAULT_PADDING * 2,
        .height = text_area.height + DEFAULT_PADDING * 2,
    };

    if (!_ui_has_visible_space(wctx))
        return false;

    gfx_set_clip(
        &wctx->framebuffer,
        (gfx_area_t){
            .x = layout->cursor_pos_x,
            .y = layout->cursor_pos_y,
            .width = _ui_get_available_space(wctx, UI_LAYOUT_HORIZONTAL),
            .height = _ui_get_available_space(wctx, UI_LAYOUT_VERTICAL),
        });
    gfx_draw_filled_rect(
        &wctx->framebuffer,
        (gfx_rect_t){
            .x = button_area.x,
            .y = button_area.y,
            .width = button_area.width,
            .height = button_area.height,
            .border_color = BORDER_COLOR,
            .border_thickness = BORDER_THICKNESS,
        },
        SECONDARY_BACKGROUND_COLOR);
    if (button_area.width > BORDER_THICKNESS * 2 && button_area.height > BORDER_THICKNESS * 2) {
        gfx_draw_rect(
            &wctx->framebuffer,
            (gfx_rect_t){
                .x = button_area.x + BORDER_THICKNESS,
                .y = button_area.y + BORDER_THICKNESS,
                .width = button_area.width - BORDER_THICKNESS * 2,
                .height = button_area.height - BORDER_THICKNESS * 2,
                .border_color = BORDER_SHADOW,
                .border_thickness = BORDER_THICKNESS,
            });
    }
    gfx_draw_text(
        &wctx->framebuffer,
        &_default_font,
        (gfx_pos_t){
            .x = button_area.x + text_area.x + DEFAULT_PADDING,
            .y = button_area.y + text_area.y + DEFAULT_PADDING,
        },
        TEXT_COLOR,
        label);
    gfx_reset_clip(&wctx->framebuffer);

    ui_advance_layout(wctx, button_area.width, button_area.height);

    return ui_is_mouse_in_area(wctx, button_area)
           && ui_is_mouse_button_down(wctx, INPUT_MOUSE_BUTTON_LEFT)
           && wctx->last_event.type == INPUT_EVENT_MOUSE;
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

void ui_textbox(ui_wctx_t *wctx, ui_textbox_state_t *state)
{
    ui_layout_t *layout = ui_get_layout(wctx);
    if (!layout || !state)
        return;
    gfx_area_t textbox_area = {
        .x = layout->cursor_pos_x,
        .y = layout->cursor_pos_y,
        .width = _ui_get_available_space(wctx, UI_LAYOUT_HORIZONTAL),
        .height = 12 + DEFAULT_PADDING * 2,
    };

    if (!_ui_has_visible_space(wctx))
        return;

    gfx_set_clip(
        &wctx->framebuffer,
        (gfx_area_t){
            .x = layout->cursor_pos_x,
            .y = layout->cursor_pos_y,
            .width = _ui_get_available_space(wctx, UI_LAYOUT_HORIZONTAL),
            .height = _ui_get_available_space(wctx, UI_LAYOUT_VERTICAL),
        });
    gfx_draw_filled_rect(
        &wctx->framebuffer,
        (gfx_rect_t){
            .x = textbox_area.x,
            .y = textbox_area.y,
            .width = textbox_area.width,
            .height = textbox_area.height,
            .border_color = BORDER_COLOR,
            .border_thickness = BORDER_THICKNESS,
        },
        SECONDARY_BACKGROUND_COLOR);
    if (textbox_area.width > BORDER_THICKNESS * 2 && textbox_area.height > BORDER_THICKNESS * 2) {
        gfx_draw_rect(
            &wctx->framebuffer,
            (gfx_rect_t){
                .x = textbox_area.x + BORDER_THICKNESS,
                .y = textbox_area.y + BORDER_THICKNESS,
                .width = textbox_area.width - BORDER_THICKNESS * 2,
                .height = textbox_area.height - BORDER_THICKNESS * 2,
                .border_color = BORDER_SHADOW,
                .border_thickness = BORDER_THICKNESS,
            });
    }

    if (!state->focused && wctx->last_event.type == INPUT_EVENT_MOUSE
        && ui_is_mouse_button_down(wctx, INPUT_MOUSE_BUTTON_LEFT)
        && ui_is_mouse_in_area(wctx, textbox_area))
        state->focused = true;

    if (state->focused && wctx->last_event.type == INPUT_EVENT_MOUSE
        && ui_is_mouse_button_down(wctx, INPUT_MOUSE_BUTTON_LEFT)
        && !ui_is_mouse_in_area(wctx, textbox_area))
        state->focused = false;

    if (state->focused) {
        if (wctx->last_event.type == INPUT_EVENT_TEXT)
            _ui_textbox_append(state, wctx->last_event.data.text);
        else if (
            wctx->last_event.type == INPUT_EVENT_KEYBOARD
            && wctx->last_event.data.keyboard.action == INPUT_KEYACTION_DOWN
            && wctx->last_event.data.keyboard.scancode == INPUT_KEYBOARD_SCANCODE_BACKSPACE
            && state->buffer && state->buffer[0] != '\0')
            state->buffer[strlen(state->buffer) - 1] = '\0';
    }

    if (state->buffer && state->buffer[0] != '\0') {
        gfx_area_t text_area = gfx_get_text_area(&_default_font, state->buffer);
        gfx_draw_text(
            &wctx->framebuffer,
            &_default_font,
            (gfx_pos_t){
                textbox_area.x + 5 + text_area.x, textbox_area.y + DEFAULT_PADDING + text_area.y},
            TEXT_COLOR,
            state->buffer);
    }

    if (state->focused) {
        uint32_t caret_x = textbox_area.x + 5;
        if (state->buffer)
            caret_x += gfx_get_text_width(&_default_font, state->buffer);
        gfx_draw_line(
            &wctx->framebuffer,
            (gfx_line_t){
                .x1 = caret_x,
                .y1 = textbox_area.y + 4,
                .x2 = caret_x,
                .y2 = textbox_area.y + textbox_area.height - 6,
                .thickness = BORDER_THICKNESS,
            },
            TEXT_COLOR);
    }
    gfx_reset_clip(&wctx->framebuffer);

    ui_advance_layout(wctx, textbox_area.width, textbox_area.height);
}
