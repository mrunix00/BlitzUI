/*
 * Copyright (c) 2026, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include "libui/types.h"
#include <libui/backend.h>
#include <libui/libui.h>
#include <libui/widgets/button.h>
#include <string.h>

static bui_widget_event_t _consume_button_event(bui_wctx_t *wctx, bui_id_t id)
{
    if (wctx->event_widget_id != id)
        return BUI_WIDGET_EVENT_NONE;
    bui_widget_event_t event = wctx->event_widget_state;
    wctx->event_widget_state &= BUI_WIDGET_EVENT_HOVERED | BUI_WIDGET_EVENT_FOCUSED;
    return event;
}

static void _draw_button(bui_wctx_t *ctx, bui_widget_t *widget)
{
    bui_color_t fill_color = bui_is_mouse_in_area(ctx, widget->computed_area)
                                 ? (bui_color_t){0xFF, 0x30, 0x30, 0x30}
                                 : widget->theme->background_color;
    bui_draw_filled_rect(
        ctx,
        (bui_rect_t){
            .x = widget->computed_area.x,
            .y = widget->computed_area.y,
            .width = widget->computed_area.width,
            .height = widget->computed_area.height,
            .border_thickness = widget->theme->border_thickness,
            .border_color = widget->theme->border_color,
        },
        fill_color);
    bui_draw_rect(
        ctx,
        (bui_rect_t){
            .x = widget->computed_area.x + widget->theme->border_thickness,
            .y = widget->computed_area.y + widget->theme->border_thickness,
            .width = widget->computed_area.width - widget->theme->border_thickness * 2,
            .height = widget->computed_area.height - widget->theme->border_thickness * 2,
            .border_thickness = widget->theme->shadow_thickness,
            .border_color = widget->theme->shadow_color,
        });
    bui_draw_text(
        ctx,
        widget->theme->font,
        (bui_pos_t){
            widget->computed_area.x + widget->theme->inner_padding.l,
            widget->computed_area.y + widget->theme->inner_padding.t,
        },
        widget->theme->foreground_color,
        widget->label);
}

bui_widget_event_t bui_button(bui_wctx_t *wctx, const char *label)
{
    bui_id_t id = bui_hash_string(label);

    if (!wctx->hot_state) {
        bui_theme_t *theme = bui_get_current_theme(wctx);
        bui_area_t text_area = bui_get_text_area(theme->font, label);
        bui_widget_t widget = {
            .flags = BUI_WIDGET_FLAG_CLICKABLE,
            .theme = theme,
            .label = label,
            .semantic_size = {
                [BUI_AXIS_X] = {
                    .type = BUI_WIDGET_SIZE_TYPE_FIXED,
                    .value = text_area.width + theme->inner_padding.l
                             + theme->inner_padding.r,
                    .strictness = 1.0f,
                },
                [BUI_AXIS_Y] = {
                    .type = BUI_WIDGET_SIZE_TYPE_FIXED,
                    .value = text_area.height + theme->inner_padding.t
                             + theme->inner_padding.b,
                    .strictness = 1.0f,
                },
            },
            .draw = _draw_button,
        };

        bui_widget_t *button = bui_push_new_child(wctx, &widget);
        bui_push_id(wctx, id, button);
        return _consume_button_event(wctx, id);
    }

    bui_widget_t *button = bui_get_by_id(wctx, id);
    if (button == NULL)
        return false;
    return _consume_button_event(wctx, id);
}
