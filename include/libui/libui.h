/*
 * Copyright (c) 2026, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#pragma once

#include <libgfx.h>
#include <libui/input.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UI_WINDOW_STATE_ERROR = -1,
    UI_WINDOW_STATE_PENDING = 0,
    UI_WINDOW_STATE_ACTIVE = 1,
} ui_window_state;

typedef struct
{
    bool resizable;
} window_flags_t;

typedef struct
{
    uint32_t r, t, l, b;
} ui_rtlb_t;

typedef enum {
    UI_LAYOUT_VERTICAL,
    UI_LAYOUT_HORIZONTAL,
} ui_layout_type_t;

typedef struct
{
    ui_layout_type_t type;
    gfx_area_t size;
    ui_rtlb_t inner_margin;
    ui_rtlb_t outer_margin;
    uint32_t cursor_pos_x, cursor_pos_y;
    uint32_t content_width, content_height;
    uint32_t padding_x, padding_y;
    bool fit_width, fit_height;
} ui_layout_t;

typedef struct
{
    uint32_t pos_x, pos_y;
    uint8_t buttons_state;
} ui_mouse_state_t;

typedef struct _ui_wctx ui_wctx_t;
struct _ui_wctx
{
    ui_wctx_t *next;
    ui_wctx_t *prev;
    const char *title;
    gfx_context_t framebuffer;
    int width;
    int height;
    uint32_t create_sequence;
    ui_window_state state;
    uint32_t window_id;
    ui_layout_t *layout_stack;
    size_t layout_count;
    size_t layout_capacity;
    ui_mouse_state_t mouse_state;
    input_keyboard_action_t keyboard_keys[256];
    input_event_t last_event;
    bool close_requested;
};

typedef struct
{
    ui_wctx_t *first_window;
    bool running;
} ui_ctx_t;

bool ui_init_context(ui_ctx_t *ctx);
void ui_deinit_context(ui_ctx_t *ctx);
bool ui_pump_events(ui_ctx_t *ctx);

bool ui_is_mouse_button_down(ui_wctx_t *wctx, input_mouse_button_t button);
gfx_pos_t ui_get_mouse_pos(ui_wctx_t *wctx);
bool ui_is_mouse_in_area(ui_wctx_t *wctx, gfx_area_t area);

bool ui_is_key_down(ui_wctx_t *wctx, input_keyboard_scancode_t keycode);
bool ui_is_key_up(ui_wctx_t *wctx, input_keyboard_scancode_t keycode);

ui_wctx_t *ui_new_window(ui_ctx_t *, const char *title, int width, int height, window_flags_t);
void ui_destroy_window(ui_ctx_t *, ui_wctx_t *);

bool ui_begin_window(ui_wctx_t *wctx);
bool ui_end_window(ui_wctx_t *wctx);

bool ui_push_layout(ui_wctx_t *wctx, ui_layout_t layout);
ui_layout_t *ui_get_layout(ui_wctx_t *wctx);
void ui_advance_layout(ui_wctx_t *wctx, uint32_t w, uint32_t h);
ui_layout_t ui_pop_layout(ui_wctx_t *wctx);

void ui_begin_column(ui_wctx_t *wctx, ui_rtlb_t margin);
void ui_end_column(ui_wctx_t *wctx);
void ui_begin_row(ui_wctx_t *wctx, ui_rtlb_t margin);
void ui_end_row(ui_wctx_t *wctx);
void ui_begin_container(
    ui_wctx_t *wctx, uint32_t width, uint32_t height, ui_rtlb_t outer_margin, ui_rtlb_t inner_margin);
void ui_end_container(ui_wctx_t *wctx);

void ui_label(ui_wctx_t *wctx, const char *label);
bool ui_button(ui_wctx_t *wctx, const char *label);

typedef struct
{
    char *buffer;
    size_t buffer_size;
    bool focused;
} ui_textbox_state_t;
ui_textbox_state_t ui_new_textbox_state(char *buffer, size_t buffer_size);
void ui_textbox(ui_wctx_t *wctx, ui_textbox_state_t *state);

#ifdef __cplusplus
}
#endif
