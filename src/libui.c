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
#define SCROLLBAR_COLOR (bui_color_t){0x80, 0xB3, 0xB3, 0xB3}
#define SCROLLBAR_WIDTH 6
#define SCROLL_STEP 20

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

static inline uint32_t _add_or_zero(uint32_t value, int32_t delta)
{
    return delta < 0 ? _sub_or_zero(value, (uint32_t) -delta) : value + (uint32_t) delta;
}

static bui_area_t _bui_layout_clip_area(bui_layout_t *layout)
{
    uint32_t width
        = _sub_or_zero(layout->size.width, layout->inner_margin.l + layout->inner_margin.r);
    uint32_t height
        = _sub_or_zero(layout->size.height, layout->inner_margin.t + layout->inner_margin.b);
    if (layout->scroll_y)
        width = _sub_or_zero(width, SCROLLBAR_WIDTH);
    if (layout->scroll_x)
        height = _sub_or_zero(height, SCROLLBAR_WIDTH);
    return (bui_area_t){
        .x = layout->size.x + layout->inner_margin.l,
        .y = layout->size.y + layout->inner_margin.t,
        .width = width,
        .height = height,
    };
}

static bool _bui_area_intersects(bui_area_t lhs, bui_area_t rhs)
{
    return lhs.x < rhs.x + rhs.width && lhs.x + lhs.width > rhs.x && lhs.y < rhs.y + rhs.height
           && lhs.y + lhs.height > rhs.y;
}

static bool _bui_area_contains(bui_area_t area, uint32_t x, uint32_t y)
{
    return x >= area.x && x < area.x + area.width && y >= area.y && y < area.y + area.height;
}

static uint32_t _bui_scroll_from_thumb_pos(
    uint32_t thumb_pos,
    uint32_t track_pos,
    uint32_t track_size,
    uint32_t thumb_size,
    uint32_t max_scroll)
{
    uint32_t travel = _sub_or_zero(track_size, thumb_size);
    if (travel == 0 || max_scroll == 0)
        return 0;
    if (thumb_pos < track_pos)
        thumb_pos = track_pos;
    if (thumb_pos > track_pos + travel)
        thumb_pos = track_pos + travel;
    return (uint32_t) (((uint64_t) (thumb_pos - track_pos) * max_scroll) / travel);
}

static uint32_t _bui_scroll_thumb_size(uint32_t track_size, uint32_t content_size)
{
    uint32_t thumb_size = track_size * track_size / content_size;
    if (thumb_size < SCROLLBAR_WIDTH)
        thumb_size = SCROLLBAR_WIDTH;
    if (thumb_size > track_size)
        thumb_size = track_size;
    return thumb_size;
}

static uint32_t _bui_scroll_thumb_pos(
    uint32_t track_pos,
    uint32_t track_size,
    uint32_t thumb_size,
    uint32_t scroll,
    uint32_t max_scroll)
{
    if (max_scroll == 0)
        return track_pos;
    return track_pos + (uint32_t) (((uint64_t) scroll * (track_size - thumb_size)) / max_scroll);
}

static uint32_t _bui_get_pos_x(bui_layout_t *layout)
{
    return _sub_or_zero(layout->cursor_pos_x, layout->scroll_offset_x);
}

static uint32_t _bui_get_pos_y(bui_layout_t *layout)
{
    return _sub_or_zero(layout->cursor_pos_y, layout->scroll_offset_y);
}

static uint32_t _bui_remaining_clip_width(bui_area_t clip, uint32_t x)
{
    if (x <= clip.x)
        return clip.width;
    return _sub_or_zero(clip.x + clip.width, x);
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

static void _bui_clear_transient_events(bui_ctx_t *ctx)
{
    for (bui_wctx_t *wctx = ctx->first_window; wctx; wctx = wctx->next)
        wctx->last_event = (bui_event_t){0};
}

bool bui_init_context(bui_ctx_t *ctx)
{
    memset(ctx, 0, sizeof(bui_ctx_t));
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

    wctx->layout_stack = malloc(INITIAL_STACK_CAPACITY * sizeof(bui_layout_t));
    if (wctx->layout_stack == NULL) {
        bui_gfx_destroy(wctx);
        free(wctx);
        return NULL;
    }
    wctx->layout_count = 0;
    wctx->layout_capacity = INITIAL_STACK_CAPACITY;

    wctx->next = ctx->first_window;
    if (ctx->first_window)
        ctx->first_window->prev = wctx;
    ctx->first_window = wctx;

    return wctx;
}

void bui_destroy_window(bui_ctx_t *ctx, bui_wctx_t *wctx)
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

bool bui_pump_events(bui_ctx_t *ctx)
{
    if (!ctx || !ctx->running || ctx->first_window == NULL)
        return false;

    _bui_clear_transient_events(ctx);

    bui_event_t event = {0};
    if (!bui_poll_events(&event)) {
        bui_delay(1);
        return true;
    }

    bui_wctx_t *wctx = _bui_find_window_by_id(ctx, event.window_id);
    if (!wctx && event.type != BUI_EVENT_WINDOW_CLOSED)
        return true;

    if (wctx)
        wctx->last_event = event;

    if (event.type == BUI_EVENT_WINDOW_CLOSED) {
        if (wctx) {
            wctx->close_requested = true;
            wctx->state = BUI_WINDOW_STATE_PENDING;
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
    case BUI_EVENT_MOUSE_WHEEL:
        wctx->mouse_state.pos_x = event.mouse_wheel.pos_x;
        wctx->mouse_state.pos_y = event.mouse_wheel.pos_y;
        break;
    default:
        break;
    }

    return true;
}

bool bui_begin_window(bui_wctx_t *wctx)
{
    if (!wctx || wctx->state != BUI_WINDOW_STATE_ACTIVE)
        return false;

    wctx->layout_count = 0;
    bui_begin_frame(wctx);
    bui_reset_clip(wctx);
    bui_draw_filled_rect(
        wctx,
        (bui_rect_t){.x = 0, .y = 0, .width = wctx->width, .height = wctx->height},
        BACKGROUND_COLOR);
    bui_push_layout(
        wctx,
        (bui_layout_t){
            .type = BUI_LAYOUT_VERTICAL,
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

bool bui_end_window(bui_wctx_t *wctx)
{
    if (!wctx || wctx->state != BUI_WINDOW_STATE_ACTIVE)
        return false;
    bui_end_frame(wctx);
    return true;
}

bool bui_push_layout(bui_wctx_t *wctx, bui_layout_t layout)
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

bui_layout_t *bui_get_layout(bui_wctx_t *wctx)
{
    if (!wctx || wctx->layout_count == 0)
        return NULL;
    return &wctx->layout_stack[wctx->layout_count - 1];
}

void bui_advance_layout(bui_wctx_t *wctx, uint32_t w, uint32_t h)
{
    bui_layout_t *current = bui_get_layout(wctx);
    if (!current)
        return;
    if (current->type == BUI_LAYOUT_HORIZONTAL) {
        uint32_t item_right
            = _sub_or_zero(current->cursor_pos_x, current->size.x + current->inner_margin.l) + w;
        if (item_right > current->content_width)
            current->content_width = item_right;
        if (h > current->content_height)
            current->content_height = h;
        current->cursor_pos_x += w + current->padding_x;
    } else if (current->type == BUI_LAYOUT_VERTICAL) {
        uint32_t item_bottom
            = _sub_or_zero(current->cursor_pos_y, current->size.y + current->inner_margin.t) + h;
        if (w > current->content_width)
            current->content_width = w;
        if (item_bottom > current->content_height)
            current->content_height = item_bottom;
        current->cursor_pos_y += h + current->padding_y;
    }
}

bui_layout_t bui_pop_layout(bui_wctx_t *wctx)
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

uint32_t bui_get_available_space(bui_wctx_t *wctx, bui_layout_type_t layout_type)
{
    bui_layout_t *current = bui_get_layout(wctx);
    if (!current)
        return 0;
    if (layout_type == BUI_LAYOUT_VERTICAL) {
        if (current->scroll_y)
            return UINT32_MAX / 4;
        uint32_t height
            = _sub_or_zero(current->size.height, current->inner_margin.t + current->inner_margin.b);
        uint32_t used
            = _sub_or_zero(current->cursor_pos_y, current->size.y + current->inner_margin.t);
        return _sub_or_zero(height, used);
    } else if (layout_type == BUI_LAYOUT_HORIZONTAL) {
        if (current->scroll_x)
            return UINT32_MAX / 4;
        uint32_t width
            = _sub_or_zero(current->size.width, current->inner_margin.l + current->inner_margin.r);
        uint32_t used
            = _sub_or_zero(current->cursor_pos_x, current->size.x + current->inner_margin.l);
        return _sub_or_zero(width, used);
    }
    return 0;
}

void bui_begin_column(bui_wctx_t *wctx, bui_rtlb_t margin)
{
    bui_layout_t *current = bui_get_layout(wctx);
    if (!current)
        return;
    uint32_t available_width
        = _sub_or_zero(bui_get_available_space(wctx, BUI_LAYOUT_HORIZONTAL), margin.l + margin.r);
    uint32_t available_height
        = _sub_or_zero(bui_get_available_space(wctx, BUI_LAYOUT_VERTICAL), margin.t + margin.b);

    bui_push_layout(
        wctx,
        (bui_layout_t){
            .type = BUI_LAYOUT_VERTICAL,
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

void bui_end_column(bui_wctx_t *wctx)
{
    bui_layout_t column_layout = bui_pop_layout(wctx);
    bui_advance_layout(
        wctx,
        column_layout.size.width + column_layout.outer_margin.l + column_layout.outer_margin.r,
        column_layout.size.height + column_layout.outer_margin.t + column_layout.outer_margin.b);
}

void bui_begin_row(bui_wctx_t *wctx, bui_rtlb_t margin)
{
    bui_layout_t *current = bui_get_layout(wctx);
    if (!current)
        return;
    uint32_t available_width
        = _sub_or_zero(bui_get_available_space(wctx, BUI_LAYOUT_HORIZONTAL), margin.l + margin.r);
    uint32_t available_height
        = _sub_or_zero(bui_get_available_space(wctx, BUI_LAYOUT_VERTICAL), margin.t + margin.b);

    bui_push_layout(
        wctx,
        (bui_layout_t){
            .type = BUI_LAYOUT_HORIZONTAL,
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

void bui_end_row(bui_wctx_t *wctx)
{
    bui_layout_t row_layout = bui_pop_layout(wctx);
    bui_advance_layout(
        wctx,
        row_layout.size.width + row_layout.outer_margin.l + row_layout.outer_margin.r,
        row_layout.size.height + row_layout.outer_margin.t + row_layout.outer_margin.b);
}

void bui_begin_container(
    bui_wctx_t *wctx,
    uint32_t width,
    uint32_t height,
    bui_rtlb_t outer_margin,
    bui_rtlb_t inner_margin,
    uint32_t *scroll_x,
    uint32_t *scroll_y)
{
    bui_layout_t *current = bui_get_layout(wctx);
    if (!current)
        return;
    uint32_t available_width = _sub_or_zero(
        bui_get_available_space(wctx, BUI_LAYOUT_HORIZONTAL), outer_margin.l + outer_margin.r);
    uint32_t available_height = _sub_or_zero(
        bui_get_available_space(wctx, BUI_LAYOUT_VERTICAL), outer_margin.t + outer_margin.b);
    uint32_t layout_width = width == UINT32_MAX ? available_width : width;
    uint32_t layout_height = height == UINT32_MAX ? available_height : height;
    bool fit_width = width == 0;
    bool fit_height = height == 0;

    if (fit_width || layout_width > available_width)
        layout_width = available_width;
    if (fit_height || layout_height > available_height)
        layout_height = available_height;

    bui_area_t size = {
        .x = current->cursor_pos_x + outer_margin.l,
        .y = current->cursor_pos_y + outer_margin.t,
        .width = layout_width,
        .height = layout_height,
    };

    if ((scroll_x || scroll_y) && wctx->last_event.type == BUI_EVENT_MOUSE_WHEEL
        && bui_is_mouse_in_area(wctx, size)) {
        if (scroll_x)
            *scroll_x = _add_or_zero(*scroll_x, -wctx->last_event.mouse_wheel.delta_x * SCROLL_STEP);
        if (scroll_y)
            *scroll_y = _add_or_zero(*scroll_y, -wctx->last_event.mouse_wheel.delta_y * SCROLL_STEP);
    }

    bui_push_layout(
        wctx,
        (bui_layout_t){
            .type = BUI_LAYOUT_VERTICAL,
            .size = size,
            .inner_margin = inner_margin,
            .outer_margin = outer_margin,
            .cursor_pos_x = size.x + inner_margin.l,
            .cursor_pos_y = size.y + inner_margin.t,
            .padding_x = 0,
            .padding_y = DEFAULT_PADDING,
            .scroll_x = scroll_x,
            .scroll_y = scroll_y,
            .scroll_offset_x = scroll_x ? *scroll_x : 0,
            .scroll_offset_y = scroll_y ? *scroll_y : 0,
            .fit_width = fit_width,
            .fit_height = fit_height,
        });
}

static void _bui_draw_scrollbar(bui_wctx_t *ctx, bui_layout_t *layout, bui_area_t clip, bool vertical)
{
    uint32_t *scroll = vertical ? layout->scroll_y : layout->scroll_x;
    if (!scroll)
        return;

    uint32_t content_size = vertical ? layout->content_height : layout->content_width;
    uint32_t track_pos = vertical ? clip.y : clip.x;
    uint32_t track_size = vertical ? clip.height : clip.width;
    uint32_t max_scroll = _sub_or_zero(content_size, track_size);
    if (*scroll > max_scroll)
        *scroll = max_scroll;
    if (max_scroll == 0 || track_size == 0)
        return;

    uint32_t bar_size = _bui_scroll_thumb_size(track_size, content_size);
    uint32_t bar_pos = _bui_scroll_thumb_pos(track_pos, track_size, bar_size, *scroll, max_scroll);
    bui_area_t bar_area
        = vertical
              ? (bui_area_t){.x = clip.x + clip.width, .y = bar_pos, .width = SCROLLBAR_WIDTH, .height = bar_size}
              : (bui_area_t){
                    .x = bar_pos,
                    .y = clip.y + clip.height,
                    .width = bar_size,
                    .height = SCROLLBAR_WIDTH};

    uint32_t mouse_pos = vertical ? ctx->mouse_state.pos_y : ctx->mouse_state.pos_x;
    if (ctx->last_event.type == BUI_EVENT_MOUSE_BUTTON_DOWN
        && ctx->last_event.mouse_button.button == BUI_MOUSE_BUTTON_LEFT
        && _bui_area_contains(bar_area, ctx->mouse_state.pos_x, ctx->mouse_state.pos_y)) {
        ctx->scrollbar_drag_scroll = scroll;
        ctx->scrollbar_drag_vertical = vertical;
        ctx->scrollbar_drag_offset = mouse_pos - bar_pos;
    }

    if (ctx->scrollbar_drag_scroll == scroll && ctx->scrollbar_drag_vertical == vertical) {
        if (bui_is_mouse_button_down(ctx, BUI_MOUSE_BUTTON_LEFT)) {
            uint32_t desired_thumb_pos = _sub_or_zero(mouse_pos, ctx->scrollbar_drag_offset);
            *scroll = _bui_scroll_from_thumb_pos(
                desired_thumb_pos, track_pos, track_size, bar_size, max_scroll);
            bar_pos = _bui_scroll_thumb_pos(track_pos, track_size, bar_size, *scroll, max_scroll);
            if (vertical)
                bar_area.y = bar_pos;
            else
                bar_area.x = bar_pos;
        } else {
            ctx->scrollbar_drag_scroll = NULL;
            ctx->scrollbar_drag_offset = 0;
        }
    }

    bui_draw_filled_rect(
        ctx,
        (bui_rect_t){
            .x = bar_area.x,
            .y = bar_area.y,
            .width = bar_area.width,
            .height = bar_area.height,
        },
        SCROLLBAR_COLOR);
}

void bui_end_container(bui_wctx_t *wctx)
{
    bui_layout_t container_layout = bui_pop_layout(wctx);
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

    bui_area_t clip = _bui_layout_clip_area(&container_layout);
    _bui_draw_scrollbar(wctx, &container_layout, clip, true);
    _bui_draw_scrollbar(wctx, &container_layout, clip, false);

    bui_advance_layout(
        wctx,
        container_layout.size.width + container_layout.outer_margin.l
            + container_layout.outer_margin.r,
        container_layout.size.height + container_layout.outer_margin.t
            + container_layout.outer_margin.b);
}

static bool _bui_has_visible_space(bui_wctx_t *wctx)
{
    return bui_get_available_space(wctx, BUI_LAYOUT_HORIZONTAL) > 0
           && bui_get_available_space(wctx, BUI_LAYOUT_VERTICAL) > 0;
}

void bui_label(bui_wctx_t *wctx, const char *label)
{
    bui_layout_t *layout = bui_get_layout(wctx);
    if (!layout)
        return;
    bui_area_t text_area = bui_get_text_area(&_default_font, label);
    bui_area_t clip = _bui_layout_clip_area(layout);
    bui_area_t item_area = {
        .x = _bui_get_pos_x(layout),
        .y = _bui_get_pos_y(layout),
        .width = text_area.width,
        .height = text_area.height,
    };

    if (!_bui_has_visible_space(wctx) || !_bui_area_intersects(item_area, clip)) {
        bui_advance_layout(wctx, text_area.width, text_area.height);
        return;
    }

    bui_set_clip(wctx, clip);
    bui_draw_text(
        wctx,
        &_default_font,
        (bui_pos_t){item_area.x + text_area.x, item_area.y + text_area.y},
        TEXT_COLOR,
        label);
    bui_reset_clip(wctx);

    bui_advance_layout(wctx, text_area.width, text_area.height);
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
    return wctx && keycode < 256
           && (wctx->keyboard_keys[keycode] == BUI_KEY_STATE_DOWN
               || wctx->keyboard_keys[keycode] == BUI_KEY_STATE_HOLD);
}

bool bui_is_key_up(bui_wctx_t *wctx, bui_keyboard_scancode_t keycode)
{
    return !wctx || keycode >= 256 || wctx->keyboard_keys[keycode] == BUI_KEY_STATE_UP;
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

bool bui_button(bui_wctx_t *wctx, const char *label)
{
    bui_layout_t *layout = bui_get_layout(wctx);
    if (!layout)
        return false;
    bui_area_t text_area = bui_get_text_area(&_default_font, label);
    bui_area_t button_area = {
        .x = _bui_get_pos_x(layout),
        .y = _bui_get_pos_y(layout),
        .width = text_area.width + DEFAULT_PADDING * 2,
        .height = _default_font.height + DEFAULT_PADDING,
    };
    bui_area_t clip = _bui_layout_clip_area(layout);

    if (!_bui_has_visible_space(wctx) || !_bui_area_intersects(button_area, clip)) {
        bui_advance_layout(wctx, button_area.width, button_area.height);
        return false;
    }

    bui_set_clip(wctx, clip);
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

    bui_advance_layout(wctx, button_area.width, button_area.height);

    return wctx->last_event.type == BUI_EVENT_MOUSE_BUTTON_DOWN
           && wctx->last_event.mouse_button.button == BUI_MOUSE_BUTTON_LEFT
           && bui_is_mouse_in_area(wctx, button_area);
}

bui_textbox_state_t bui_new_textbox_state(char *buffer, size_t size)
{
    return (bui_textbox_state_t){.buffer = buffer, .size = size, .cursor = 0, .focused = false};
}

void bui_reset_textbox(bui_textbox_state_t *state)
{
    if (!state)
        return;
    state->cursor = 0;
    state->buffer[0] = '\0';
}

static void _bui_textbox_append(bui_textbox_state_t *state, char c)
{
    if (!state || !state->buffer || state->size == 0 || c == '\0')
        return;
    if (state->cursor >= state->size - 1)
        return;
    state->buffer[state->cursor++] = c;
    state->buffer[state->cursor] = '\0';
}

bool bui_textbox(bui_wctx_t *wctx, bui_textbox_state_t *state, uint32_t width)
{
    bui_layout_t *layout = bui_get_layout(wctx);
    if (!layout || !state)
        return false;
    bui_area_t clip = _bui_layout_clip_area(layout);
    uint32_t x = _bui_get_pos_x(layout);
    bui_area_t textbox_area = {
        .x = x,
        .y = _bui_get_pos_y(layout),
        .width = width == UINT32_MAX ? _bui_remaining_clip_width(clip, x) : width,
        .height = _default_font.height + DEFAULT_PADDING,
    };

    if (!_bui_has_visible_space(wctx) || !_bui_area_intersects(textbox_area, clip)) {
        bui_advance_layout(wctx, textbox_area.width, textbox_area.height);
        return false;
    }

    bui_set_clip(wctx, clip);
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
        state->focused = bui_is_mouse_in_area(wctx, textbox_area);

    if (state->focused
        && (wctx->last_event.type == BUI_EVENT_KEY_DOWN
            || wctx->last_event.type == BUI_EVENT_KEY_HOLD)) {
        _bui_textbox_append(state, bui_char_from_key_scancode(wctx, wctx->last_event.keyboard));
    }

    if (state->focused
        && (wctx->last_event.type == BUI_EVENT_KEY_DOWN
            || wctx->last_event.type == BUI_EVENT_KEY_HOLD)
        && wctx->last_event.keyboard == BUI_KEYBOARD_SCANCODE_BACKSPACE && state->buffer
        && state->cursor > 0)
        state->buffer[--state->cursor] = '\0';

    if (state->buffer && state->cursor > 0) {
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

    bui_advance_layout(wctx, textbox_area.width, textbox_area.height);

    return state->focused && wctx->last_event.type == BUI_EVENT_KEY_DOWN
           && wctx->last_event.keyboard == BUI_KEYBOARD_SCANCODE_ENTER;
}
