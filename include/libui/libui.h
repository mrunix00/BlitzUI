/*
 * Copyright (c) 2026, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#pragma once

#include <libui/events.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BUI_COLUMN(window, padding, body) \
    bui_begin_column(window, (padding)); \
    {body}; \
    bui_end_column(window)

#define BUI_ROW(window, padding, body) \
    bui_begin_row(window, (padding)); \
    {body}; \
    bui_end_row(window)

#define BUI_CONTAINER(window, width, height, outer_margin, inner_margin, body) \
    bui_begin_container(window, width, height, outer_margin, inner_margin); \
    body; \
    bui_end_container(window)

typedef enum {
    UI_WINDOW_STATE_ERROR = -1,
    UI_WINDOW_STATE_PENDING = 0,
    UI_WINDOW_STATE_ACTIVE = 1,
} ui_window_state;

typedef struct
{
    bool resizable : 1;
    bool fullscreen : 1;
} window_flags_t;

typedef struct
{
    uint32_t r, t, l, b;
} bui_rtlb_t;

typedef enum {
    BUI_LAYOUT_VERTICAL,
    BUI_LAYOUT_HORIZONTAL,
} bui_layout_type_t;

typedef struct
{
    uint32_t x, y, width, height;
} bui_area_t;

typedef struct
{
    uint8_t a, r, g, b;
} bui_color_t;

typedef struct
{
    uint32_t x, y, width, height;
    uint32_t border_thickness;
    bui_color_t border_color;
} bui_rect_t;

typedef struct
{
    uint32_t x1, x2;
    uint32_t y1, y2;
    uint32_t thickness;
} bui_line_t;

typedef struct
{
    uint32_t x, y;
} bui_pos_t;

typedef struct
{
    uint32_t x, y, width, height, x_offset, y_offset, x_advance;
} bui_glyph_t;

typedef bui_rtlb_t ui_rtlb_t;
typedef bui_layout_type_t ui_layout_type_t;
typedef bui_area_t ui_area_t;
typedef bui_color_t ui_color_t;
typedef bui_rect_t ui_rect_t;
typedef bui_line_t ui_line_t;
typedef bui_pos_t ui_pos_t;
typedef bui_glyph_t ui_font_glyph_t;

typedef struct
{
    uint32_t width, height;
    uint8_t first_char;
    uint8_t last_char;
    const uint8_t *atlas;
    uint32_t atlas_width, atlas_height;
    const bui_glyph_t *glyphs;
} bui_font_t;

typedef bui_font_t ui_font_t;

typedef struct
{
    bui_layout_type_t type;
    bui_area_t size;
    bui_rtlb_t inner_margin;
    bui_rtlb_t outer_margin;
    uint32_t cursor_pos_x, cursor_pos_y;
    uint32_t content_width, content_height;
    uint32_t padding_x, padding_y;
    bool fit_width, fit_height;
} bui_layout_t;

typedef bui_layout_t ui_layout_t;

typedef struct
{
    uint32_t pos_x, pos_y;
    uint8_t buttons_state;
} bui_mouse_state_t;

typedef bui_mouse_state_t ui_mouse_state_t;

typedef struct _ui_wctx ui_wctx_t;
struct _ui_wctx
{
    ui_wctx_t *next;
    ui_wctx_t *prev;
    const char *title;
    void *gfx_context;
    window_flags_t flags;
    int width;
    int height;
    ui_window_state state;
    uint32_t window_id;
    bui_layout_t *layout_stack;
    size_t layout_count;
    size_t layout_capacity;
    bui_mouse_state_t mouse_state;
    bui_key_state_t keyboard_keys[256];
    bui_event_t last_event;
    bool close_requested;
};

typedef struct
{
    ui_wctx_t *first_window;
    bool running;
} ui_ctx_t;

bool bui_init_context(ui_ctx_t *ctx);
void bui_deinit_context(ui_ctx_t *ctx);
bool bui_pump_events(ui_ctx_t *ctx);

bool bui_is_mouse_button_down(ui_wctx_t *wctx, bui_mouse_button_t button);
bui_pos_t bui_get_mouse_pos(ui_wctx_t *wctx);
bool bui_is_mouse_in_area(ui_wctx_t *wctx, bui_area_t area);

bool bui_is_key_down(ui_wctx_t *wctx, bui_keyboard_scancode_t keycode);
bool bui_is_key_up(ui_wctx_t *wctx, bui_keyboard_scancode_t keycode);
char bui_char_from_key_scancode(ui_wctx_t *wctx, bui_keyboard_scancode_t key);

ui_wctx_t *bui_new_window(ui_ctx_t *, const char *title, int width, int height, window_flags_t);
void bui_destroy_window(ui_ctx_t *, ui_wctx_t *);

bool bui_begin_window(ui_wctx_t *wctx);
bool bui_end_window(ui_wctx_t *wctx);

bool bui_push_layout(ui_wctx_t *wctx, bui_layout_t layout);
bui_layout_t *bui_get_layout(ui_wctx_t *wctx);
void bui_advance_layout(ui_wctx_t *wctx, uint32_t w, uint32_t h);
bui_layout_t bui_pop_layout(ui_wctx_t *wctx);
uint32_t bui_get_available_space(ui_wctx_t *wctx, bui_layout_type_t layout_type);

void bui_begin_column(ui_wctx_t *wctx, bui_rtlb_t margin);
void bui_end_column(ui_wctx_t *wctx);
void bui_begin_row(ui_wctx_t *wctx, bui_rtlb_t margin);
void bui_end_row(ui_wctx_t *wctx);
void bui_begin_container(
    ui_wctx_t *wctx,
    uint32_t width,
    uint32_t height,
    bui_rtlb_t outer_margin,
    bui_rtlb_t inner_margin);
void bui_end_container(ui_wctx_t *wctx);

void bui_label(ui_wctx_t *wctx, const char *label);
bool bui_button(ui_wctx_t *wctx, const char *label);

typedef struct
{
    char *buffer;
    size_t size;
    size_t cursor;
    bool focused;
} ui_textbox_state_t;
ui_textbox_state_t bui_new_textbox_state(char *buffer, size_t buffer_size);
void bui_reset_textbox(ui_textbox_state_t *state);
bool bui_textbox(ui_wctx_t *wctx, ui_textbox_state_t *state, uint32_t width);

#ifdef __cplusplus
}
#endif
