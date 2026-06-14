/*
 * Copyright (c) 2026, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#pragma once

#include <libui/events.h>
#include <libui/memory.h>
#include <libui/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BUI_THEME(window, theme, body) \
    bui_push_theme(window, theme); \
    {body}; \
    bui_pop_theme(window)

#define BUI_WINDOW(window, body) \
    bui_begin_window(window); \
    {body}; \
    bui_end_window(window)

bool bui_init_context(bui_ctx_t *ctx);
void bui_deinit_context(bui_ctx_t *ctx);
bool bui_pump_events(bui_ctx_t *ctx);

bool bui_font_load_from_file(bui_font_t *font, const char *path, float pixel_height);
void bui_font_free(bui_font_t *font);
bui_font_t *bui_get_default_font(void);

bool bui_is_mouse_button_down(bui_wctx_t *wctx, bui_mouse_button_t button);
bui_pos_t bui_get_mouse_pos(bui_wctx_t *wctx);
bool bui_is_mouse_in_area(bui_wctx_t *wctx, bui_area_t area);

bool bui_is_key_down(bui_wctx_t *wctx, bui_keyboard_scancode_t keycode);
bool bui_is_key_up(bui_wctx_t *wctx, bui_keyboard_scancode_t keycode);
char bui_char_from_key_scancode(bui_wctx_t *wctx, bui_keyboard_scancode_t key);

bui_widget_t *bui_push_new_child(bui_wctx_t *wctx, bui_widget_t *);
bui_widget_t *bui_push_new_parent(bui_wctx_t *wctx, bui_widget_t *);
void bui_pop_widget(bui_wctx_t *wctx);

bui_wctx_t *bui_new_window(bui_ctx_t *, const char *title, int width, int height, bui_window_flags_t);
void bui_destroy_window(bui_ctx_t *, bui_wctx_t *);

bool bui_begin_window(bui_wctx_t *wctx);
bool bui_end_window(bui_wctx_t *wctx);

void bui_push_theme(bui_wctx_t *ctx, bui_theme_t *theme);
bui_theme_t *bui_pop_theme(bui_wctx_t *ctx);
bui_theme_t *bui_get_current_theme(bui_wctx_t *ctx);

bui_id_t bui_hash_string(const char *str);
void bui_push_id(bui_wctx_t *wctx, bui_id_t id, bui_widget_t *widget);
bui_widget_t *bui_get_by_id(bui_wctx_t *wctx, bui_id_t id);

#ifdef __cplusplus
}
#endif
