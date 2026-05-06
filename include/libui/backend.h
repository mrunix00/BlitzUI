/*
 * Copyright (c) 2026, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#pragma once

#include <libui/libui.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Initialize the graphics context for the given window.
 */
bool bui_gfx_init(bui_wctx_t *wctx);

/*
 * Destroy the graphics context for the given window.
 */
void bui_gfx_destroy(bui_wctx_t *wctx);

/*
 * Destroy the graphics context for the given window.
 */
bool bui_poll_events(bui_event_t *event);

/*
 * Delay for the given number of milliseconds.
 */
void bui_delay(uint32_t ms);

/*
 * Begin a new frame for the given window context.
 */
void bui_begin_frame(bui_wctx_t *wctx);

/*
 * End the current frame for the given window context.
 */
void bui_end_frame(bui_wctx_t *wctx);

/*
 * Set the clip region for the given window context.
 */
void bui_set_clip(bui_wctx_t *wctx, bui_area_t rect);

/*
 * Reset the clip region for the given window context.
 */
void bui_reset_clip(bui_wctx_t *wctx);

/*
 * Draw a rectangle outline.
 */
void bui_draw_rect(bui_wctx_t *wctx, bui_rect_t rect);

/*
 * Draw a filled rectangle with the given color.
 */
void bui_draw_filled_rect(bui_wctx_t *wctx, bui_rect_t rect, bui_color_t color);

/*
 * Draw a line with the given color.
 */
void bui_draw_line(bui_wctx_t *wctx, bui_line_t line, bui_color_t color);

/*
 * Draw text with the given font and color at the given position.
 */
void bui_draw_text(bui_wctx_t *wctx, bui_font_t *font, bui_pos_t, bui_color_t, const char *text);

/*
 * Get the area occupied by the given text with the given font.
 */
bui_area_t bui_get_text_area(bui_font_t *font, const char *text);

/*
 * Get the width of the given text with the given font.
 */
uint32_t bui_get_text_width(bui_font_t *font, const char *text);

/*
 * Get the height of the given text with the given font.
 */
uint32_t bui_get_text_height(bui_font_t *font, const char *text);

#ifdef __cplusplus
}
#endif
