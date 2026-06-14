/*
 * Copyright (c) 2026, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#pragma once

#include <libui/events.h>
#include <libui/memory.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef STB_TEXTEDIT_CHARTYPE
#define STB_TEXTEDIT_CHARTYPE char
#endif
#ifndef STB_TEXTEDIT_POSITIONTYPE
#define STB_TEXTEDIT_POSITIONTYPE int
#endif
#include "../../vendor/stb/stb_textedit.h"

typedef struct _bui_wctx bui_wctx_t;
typedef struct _bui_widget bui_widget_t;
typedef struct _bui_theme bui_theme_t;
typedef uint32_t bui_id_t;

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
    uint32_t r, t, l, b;
} bui_rtlb_t;

typedef enum {
    BUI_AXIS_X = 0,
    BUI_AXIS_Y = 1,
    BUI_AXIS2_COUNT = 2,
} bui_axis_t;

typedef enum {
    BUI_WIDGET_SIZE_TYPE_NONE = 0,
    BUI_WIDGET_SIZE_TYPE_FIXED,
    BUI_WIDGET_SIZE_TYPE_PERCENTAGE,
    BUI_WIDGET_SIZE_TYPE_CHILDREN_SUM,
} bui_widget_size_type_t;

typedef struct
{
    bui_widget_size_type_t type;
    uint32_t value;
    float strictness;
} bui_widget_size_t;

typedef enum {
    BUI_WINDOW_STATE_ERROR = -1,
    BUI_WINDOW_STATE_PENDING = 0,
    BUI_WINDOW_STATE_ACTIVE = 1,
} bui_window_state;

typedef struct
{
    bool resizable : 1;
    bool fullscreen : 1;
} bui_window_flags_t;

typedef enum {
    BUI_LAYOUT_VERTICAL,
    BUI_LAYOUT_HORIZONTAL,
} bui_layout_type_t;

typedef struct
{
    uint32_t x, y, width, height, x_advance;
    int32_t x_offset, y_offset;
} bui_glyph_t;

typedef struct
{
    uint32_t width, height;
    uint8_t first_char;
    uint8_t last_char;
    const uint8_t *atlas;
    uint32_t atlas_width, atlas_height;
    const bui_glyph_t *glyphs;
} bui_font_t;

typedef struct
{
    uint32_t pos_x, pos_y;
    uint8_t buttons_state;
} bui_mouse_state_t;

typedef struct
{
    bui_wctx_t *first_window;
    bool running;
} bui_ctx_t;

struct _bui_theme
{
    bui_theme_t *next;
    bui_theme_t *prev;

    bui_font_t *font;
    bui_rtlb_t inner_padding;
    bui_rtlb_t outer_padding;

    uint32_t border_thickness;
    uint32_t shadow_thickness;
    uint32_t spacing;

    bui_color_t foreground_color;
    bui_color_t background_color;
    bui_color_t border_color;
    bui_color_t shadow_color;
};

typedef enum {
    BUI_WIDGET_FLAG_CLICKABLE = 1 << 0,
    BUI_WIDGET_FLAG_WITH_CHILDREN = 1 << 1,
    BUI_WIDGET_FLAG_FOCUSED = 1 << 2,
    BUI_WIDGET_FLAG_PADDED = 1 << 3,
    BUI_WIDGET_FLAG_SCROLLABLE = 1 << 4,
} bui_widget_flags_t;

typedef struct
{
    bui_widget_t *widget;
    bui_id_t id;
} bui_key_pair_t;

typedef enum : uint32_t {
    BUI_WIDGET_EVENT_NONE = 0,
    BUI_WIDGET_EVENT_HOVERED = 1 << 0,
    BUI_WIDGET_EVENT_FOCUSED = 1 << 1,
    BUI_WIDGET_EVENT_RCLICKED = 1 << 2,
    BUI_WIDGET_EVENT_LCLICKED = 1 << 3,
    BUI_WIDGET_EVENT_MCLICKED = 1 << 4,
    BUI_WIDGET_EVENT_RRELEASED = 1 << 5,
    BUI_WIDGET_EVENT_LRELEASED = 1 << 6,
    BUI_WIDGET_EVENT_MRELEASED = 1 << 7,
    BUI_WIDGET_EVENT_DOUBLE_RCLICKED = 1 << 8,
    BUI_WIDGET_EVENT_DOUBLE_LCLICKED = 1 << 9,
    BUI_WIDGET_EVENT_DOUBLE_MCLICKED = 1 << 10,
    BUI_WIDGET_EVENT_KEYBOARD_PRESSED = 1 << 11,
} bui_widget_event_t;

struct _bui_widget
{
    bui_widget_t *parent;
    bui_widget_t *first_child;
    bui_widget_t *next_sibling;
    bui_widget_t *prev_sibling;
    const char *label;
    char *string;
    size_t string_size;
    bui_theme_t *theme;
    void (*draw)(bui_wctx_t *ctx, bui_widget_t *widget);

    bui_widget_size_t semantic_size[BUI_AXIS2_COUNT];
    uint32_t computed_size[BUI_AXIS2_COUNT];
    bui_area_t computed_area;
    bui_id_t id;
    bui_widget_event_t last_event;
    STB_TexteditState edit;
    uint32_t scroll_x, scroll_y;

    bui_widget_flags_t flags;
    bui_axis_t layout_axis;
};

struct _bui_wctx
{
    bui_wctx_t *next;
    bui_wctx_t *prev;

    bui_arena_t arena;
    bui_arena_t persistent_arena;
    const char *title;
    void *gfx_context;
    bui_id_t scrollbar_drag_widget_id;
    bui_axis_t scrollbar_drag_axis;
    uint32_t scrollbar_drag_offset;
    bui_widget_t *widget_tree;
    bui_widget_t *prev_widget_tree;
    bui_widget_t *current_widget;
    bui_theme_t *themes_head;
    bui_theme_t *themes_tail;

    bui_key_pair_t *key_pairs;
    size_t key_pairs_count;
    size_t key_pairs_capacity;

    bui_mouse_state_t mouse_state;
    bui_key_state_t keyboard_keys[128];
    bui_event_t input_event;
    bui_widget_event_t event_widget_state;
    bui_id_t event_widget_id;
    bui_id_t focused_widget_id;
    bui_id_t active_widget_id;

    uint32_t window_id;
    uint32_t width, height;

    bui_window_state state;
    bui_window_flags_t flags;
    bool close_requested;
    bool hot_state;
};
