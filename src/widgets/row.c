/*
 * Copyright (c) 2026, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <libui/libui.h>
#include <libui/widgets/row.h>

static void _draw_row(bui_wctx_t *wctx, bui_widget_t *widget)
{
    bui_widget_t *child = widget->first_child;
    while (child != NULL) {
        child->draw(wctx, child);
        child = child->next_sibling;
    }
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
        .strictness = 0.0f,
    };
}

void bui_begin_row(bui_wctx_t *wctx, uint32_t width, uint32_t height)
{
    if (!wctx->hot_state) {
        bui_widget_t widget = {
            .theme = bui_get_current_theme(wctx),
            .layout_axis = BUI_AXIS_X,
            .semantic_size = {
                [BUI_AXIS_X] = _size_to_semantic(width),
                [BUI_AXIS_Y] = _size_to_semantic(height),
            },
            .draw = _draw_row,
        };
        bui_push_new_parent(wctx, &widget);
    }
}

void bui_end_row(bui_wctx_t *wctx)
{
    bui_pop_widget(wctx);
}
