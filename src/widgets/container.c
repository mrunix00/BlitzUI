/*
 * Copyright (c) 2026, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <libui/backend.h>
#include <libui/libui.h>
#include <libui/widgets/container.h>

#define SCROLLBAR_WIDTH 6
#define SCROLL_STEP 20

static uint32_t _sub_or_zero(uint32_t lhs, uint32_t rhs)
{
    return lhs > rhs ? lhs - rhs : 0;
}

static uint32_t _spacing_sum(bui_widget_t *widget, uint32_t children_count)
{
    return children_count <= 1 ? 0 : widget->theme->spacing * (children_count - 1);
}

static bui_area_t _container_viewport(bui_widget_t *widget)
{
    uint32_t horizontal_padding = widget->theme->inner_padding.l + widget->theme->inner_padding.r;
    uint32_t vertical_padding = widget->theme->inner_padding.t + widget->theme->inner_padding.b;

    return (bui_area_t){
        .x = widget->computed_area.x + widget->theme->inner_padding.l,
        .y = widget->computed_area.y + widget->theme->inner_padding.t,
        .width = _sub_or_zero(widget->computed_area.width, horizontal_padding),
        .height = _sub_or_zero(widget->computed_area.height, vertical_padding),
    };
}

static void _translate_widget_tree(bui_widget_t *widget, int32_t dx, int32_t dy)
{
    int64_t x = (int64_t) widget->computed_area.x + dx;
    int64_t y = (int64_t) widget->computed_area.y + dy;
    widget->computed_area.x = x > 0 ? (uint32_t) x : 0;
    widget->computed_area.y = y > 0 ? (uint32_t) y : 0;

    for (bui_widget_t *child = widget->first_child; child != NULL; child = child->next_sibling)
        _translate_widget_tree(child, dx, dy);
}

static uint32_t _container_content_extent(bui_widget_t *widget, bui_axis_t axis)
{
    bui_area_t viewport = _container_viewport(widget);
    uint32_t extent = axis == BUI_AXIS_X ? viewport.width : viewport.height;
    uint32_t content_extent = 0;
    uint32_t children_count = 0;

    for (bui_widget_t *child = widget->first_child; child != NULL; child = child->next_sibling) {
        uint32_t child_size = child->computed_size[axis];
        if (widget->layout_axis == axis)
            content_extent += child_size;
        else if (child_size > content_extent)
            content_extent = child_size;
        children_count++;
    }

    if (widget->layout_axis == axis)
        content_extent += _spacing_sum(widget, children_count);

    return content_extent > extent ? content_extent : extent;
}

static uint32_t _container_max_scroll(bui_widget_t *widget, bui_axis_t axis)
{
    bui_area_t viewport = _container_viewport(widget);
    uint32_t viewport_size = axis == BUI_AXIS_X ? viewport.width : viewport.height;
    uint32_t content_extent = _container_content_extent(widget, axis);
    return _sub_or_zero(content_extent, viewport_size);
}

static void _container_set_scroll(bui_widget_t *widget, uint32_t scroll_x, uint32_t scroll_y)
{
    uint32_t old_scroll_x = widget->scroll_x;
    uint32_t old_scroll_y = widget->scroll_y;

    uint32_t max_scroll_x = _container_max_scroll(widget, BUI_AXIS_X);
    uint32_t max_scroll_y = _container_max_scroll(widget, BUI_AXIS_Y);
    widget->scroll_x = scroll_x > max_scroll_x ? max_scroll_x : scroll_x;
    widget->scroll_y = scroll_y > max_scroll_y ? max_scroll_y : scroll_y;

    int32_t dx = (int32_t) old_scroll_x - (int32_t) widget->scroll_x;
    int32_t dy = (int32_t) old_scroll_y - (int32_t) widget->scroll_y;
    if (dx == 0 && dy == 0)
        return;

    for (bui_widget_t *child = widget->first_child; child != NULL; child = child->next_sibling)
        _translate_widget_tree(child, dx, dy);
}

static uint32_t _scroll_delta(uint32_t scroll, int32_t wheel_delta)
{
    if (wheel_delta > 0) {
        uint32_t delta = (uint32_t) wheel_delta * SCROLL_STEP;
        return scroll > delta ? scroll - delta : 0;
    }

    if (wheel_delta < 0)
        return scroll + (uint32_t) -wheel_delta * SCROLL_STEP;

    return scroll;
}

static bool _area_contains(bui_area_t area, uint32_t x, uint32_t y)
{
    return x >= area.x && x < area.x + area.width && y >= area.y && y < area.y + area.height;
}

static bui_area_t _scrollbar_thumb(
    bui_widget_t *widget, bui_axis_t axis, uint32_t max_scroll, bui_area_t viewport)
{
    if (max_scroll == 0)
        return (bui_area_t){0};

    uint32_t viewport_size = axis == BUI_AXIS_X ? viewport.width : viewport.height;
    uint32_t content_size = viewport_size + max_scroll;
    if (viewport_size == 0 || content_size == 0)
        return (bui_area_t){0};

    uint32_t thumb_size = (uint32_t) (((uint64_t) viewport_size * viewport_size) / content_size);
    if (thumb_size < SCROLLBAR_WIDTH)
        thumb_size = SCROLLBAR_WIDTH;
    if (thumb_size > viewport_size)
        thumb_size = viewport_size;

    uint32_t scroll = axis == BUI_AXIS_X ? widget->scroll_x : widget->scroll_y;
    uint32_t travel = _sub_or_zero(viewport_size, thumb_size);
    uint32_t thumb_pos = max_scroll == 0 ? 0 : (uint32_t) (((uint64_t) scroll * travel) / max_scroll);

    if (axis == BUI_AXIS_X) {
        return (bui_area_t){
            .x = viewport.x + thumb_pos,
            .y = viewport.y + _sub_or_zero(viewport.height, SCROLLBAR_WIDTH),
            .width = thumb_size,
            .height = SCROLLBAR_WIDTH,
        };
    }

    return (bui_area_t){
        .x = viewport.x + _sub_or_zero(viewport.width, SCROLLBAR_WIDTH),
        .y = viewport.y + thumb_pos,
        .width = SCROLLBAR_WIDTH,
        .height = thumb_size,
    };
}

static uint32_t _scroll_from_thumb_pos(
    uint32_t thumb_pos, uint32_t track_pos, uint32_t track_size, uint32_t thumb_size, uint32_t max_scroll)
{
    uint32_t travel = _sub_or_zero(track_size, thumb_size);
    if (travel == 0 || max_scroll == 0)
        return 0;
    uint32_t local_thumb_pos = thumb_pos > track_pos ? thumb_pos - track_pos : 0;
    if (local_thumb_pos > travel)
        local_thumb_pos = travel;
    return (uint32_t) (((uint64_t) local_thumb_pos * max_scroll) / travel);
}

static void _begin_scrollbar_drag(
    bui_wctx_t *wctx,
    bui_widget_t *widget,
    bui_axis_t axis,
    bui_area_t thumb)
{
    uint32_t mouse_pos = axis == BUI_AXIS_X ? wctx->mouse_state.pos_x : wctx->mouse_state.pos_y;
    uint32_t thumb_pos = axis == BUI_AXIS_X ? thumb.x : thumb.y;

    wctx->active_widget_id = widget->id;
    wctx->scrollbar_drag_widget_id = widget->id;
    wctx->scrollbar_drag_axis = axis;
    wctx->scrollbar_drag_offset = mouse_pos > thumb_pos ? mouse_pos - thumb_pos : 0;
}

static bool _handle_scrollbar_drag(
    bui_wctx_t *wctx,
    bui_widget_t *widget,
    bui_axis_t axis,
    uint32_t max_scroll,
    bui_area_t viewport)
{
    bui_area_t thumb = _scrollbar_thumb(widget, axis, max_scroll, viewport);
    if (thumb.width == 0 || thumb.height == 0)
        return false;

    if (wctx->input_event.type == BUI_EVENT_MOUSE_BUTTON_DOWN
        && (wctx->input_event.mouse_button.button & BUI_MOUSE_BUTTON_LEFT)
        && _area_contains(thumb, wctx->mouse_state.pos_x, wctx->mouse_state.pos_y)) {
        _begin_scrollbar_drag(wctx, widget, axis, thumb);
        return true;
    }

    if (wctx->input_event.type != BUI_EVENT_MOUSE_MOVE
        || wctx->scrollbar_drag_widget_id != widget->id
        || wctx->scrollbar_drag_axis != axis
        || !bui_is_mouse_button_down(wctx, BUI_MOUSE_BUTTON_LEFT))
        return false;

    uint32_t mouse_pos = axis == BUI_AXIS_X ? wctx->mouse_state.pos_x : wctx->mouse_state.pos_y;
    uint32_t track_pos = axis == BUI_AXIS_X ? viewport.x : viewport.y;
    uint32_t track_size = axis == BUI_AXIS_X ? viewport.width : viewport.height;
    uint32_t thumb_size = axis == BUI_AXIS_X ? thumb.width : thumb.height;
    uint32_t thumb_pos = mouse_pos > wctx->scrollbar_drag_offset
                             ? mouse_pos - wctx->scrollbar_drag_offset
                             : 0;
    uint32_t scroll = _scroll_from_thumb_pos(thumb_pos, track_pos, track_size, thumb_size, max_scroll);

    if (axis == BUI_AXIS_X)
        _container_set_scroll(widget, scroll, widget->scroll_y);
    else
        _container_set_scroll(widget, widget->scroll_x, scroll);

    return true;
}

static void _handle_container_scroll(bui_wctx_t *wctx, bui_widget_t *widget, bui_area_t viewport)
{
    uint32_t max_scroll_x = _container_max_scroll(widget, BUI_AXIS_X);
    uint32_t max_scroll_y = _container_max_scroll(widget, BUI_AXIS_Y);
    if (_handle_scrollbar_drag(wctx, widget, BUI_AXIS_X, max_scroll_x, viewport))
        return;
    if (_handle_scrollbar_drag(wctx, widget, BUI_AXIS_Y, max_scroll_y, viewport))
        return;

    if (wctx->input_event.type != BUI_EVENT_MOUSE_WHEEL)
        return;
    if (!bui_is_mouse_in_area(wctx, widget->computed_area))
        return;

    _container_set_scroll(
        widget,
        _scroll_delta(widget->scroll_x, wctx->input_event.mouse_wheel.delta_x),
        _scroll_delta(widget->scroll_y, wctx->input_event.mouse_wheel.delta_y));
}

static void _draw_scrollbar(
    bui_wctx_t *wctx,
    bui_widget_t *widget,
    bui_axis_t axis,
    uint32_t max_scroll,
    bui_area_t viewport)
{
    if (max_scroll == 0)
        return;

    bui_area_t thumb = _scrollbar_thumb(widget, axis, max_scroll, viewport);
    if (thumb.width == 0 || thumb.height == 0)
        return;

    bui_draw_filled_rect(
        wctx,
        (bui_rect_t){
            .x = thumb.x,
            .y = thumb.y,
            .width = thumb.width,
            .height = thumb.height,
        },
        widget->theme->foreground_color);
}

static void _draw_container(bui_wctx_t *wctx, bui_widget_t *widget)
{
    bui_area_t viewport = _container_viewport(widget);
    _handle_container_scroll(wctx, widget, viewport);
    _container_set_scroll(widget, widget->scroll_x, widget->scroll_y);

    uint32_t border_thickness = widget->theme->border_thickness;
    uint32_t shadow_thickness = widget->theme->shadow_thickness;

    bui_draw_filled_rect(
        wctx,
        (bui_rect_t){
            .x = widget->computed_area.x,
            .y = widget->computed_area.y,
            .width = widget->computed_area.width,
            .height = widget->computed_area.height,
            .border_thickness = border_thickness,
            .border_color = widget->theme->border_color,
        },
        widget->theme->background_color);
    bui_draw_rect(
        wctx,
        (bui_rect_t){
            .x = widget->computed_area.x + border_thickness,
            .y = widget->computed_area.y + border_thickness,
            .width = _sub_or_zero(widget->computed_area.width, border_thickness * 2),
            .height = _sub_or_zero(widget->computed_area.height, border_thickness * 2),
            .border_thickness = shadow_thickness,
            .border_color = widget->theme->shadow_color,
        });

    bui_set_clip(wctx, viewport);

    bui_widget_t *child = widget->first_child;
    while (child != NULL) {
        child->draw(wctx, child);
        child = child->next_sibling;
    }

    bui_reset_clip(wctx);

    _draw_scrollbar(
        wctx, widget, BUI_AXIS_X, _container_max_scroll(widget, BUI_AXIS_X), viewport);
    _draw_scrollbar(
        wctx, widget, BUI_AXIS_Y, _container_max_scroll(widget, BUI_AXIS_Y), viewport);
}

static bui_widget_size_t _size_to_semantic(uint32_t size)
{
    if (size == 0) {
        return (bui_widget_size_t){
            .type = BUI_WIDGET_SIZE_TYPE_CHILDREN_SUM,
            .strictness = 1.0f,
        };
    } else if (size == UINT32_MAX) {
        return (bui_widget_size_t){
            .type = BUI_WIDGET_SIZE_TYPE_PERCENTAGE,
            .value = 1.0f,
            .strictness = 0.0f,
        };
    }
    return (bui_widget_size_t){
        .type = BUI_WIDGET_SIZE_TYPE_FIXED,
        .value = size,
        .strictness = 1.0f,
    };
}

void bui_begin_container(bui_wctx_t *wctx, uint32_t width, uint32_t height)
{
    if (!wctx->hot_state) {
        bui_widget_t widget = {
            .theme = bui_get_current_theme(wctx),
            .flags = BUI_WIDGET_FLAG_PADDED | BUI_WIDGET_FLAG_SCROLLABLE,
            .layout_axis = BUI_AXIS_Y,
            .semantic_size = {
                [BUI_AXIS_X] = _size_to_semantic(width),
                [BUI_AXIS_Y] = _size_to_semantic(height),
            },
            .draw = _draw_container,
        };
        bui_push_new_parent(wctx, &widget);
    }
}

void bui_end_container(bui_wctx_t *wctx)
{
    bui_pop_widget(wctx);
}
