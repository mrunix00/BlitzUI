/*
 * Copyright (c) 2026, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <libui/backend.h>
#include <libui/libui.h>

static void _text_draw(bui_wctx_t *ctx, bui_widget_t *widget)
{
    bui_draw_text(
        ctx,
        widget->theme->font,
        (bui_pos_t){.x = widget->computed_area.x, .y = widget->computed_area.y},
        widget->theme->foreground_color,
        widget->label);
}

void bui_text(bui_wctx_t *wctx, const char *text)
{
    if (!wctx->hot_state) {
        bui_theme_t *theme = bui_get_current_theme(wctx);
        bui_area_t text_area = bui_get_text_area(theme->font, text);
        bui_widget_t widget = {
            .theme = theme,
            .label = text,
            .semantic_size = {
                [BUI_AXIS_X] = {
                    .type = BUI_WIDGET_SIZE_TYPE_FIXED,
                    .value = text_area.width,
                    .strictness = 1.0f,
                },
                [BUI_AXIS_Y] = {
                    .type = BUI_WIDGET_SIZE_TYPE_FIXED,
                    .value = text_area.height,
                    .strictness = 1.0f,
                },
            },
            .draw = _text_draw,
        };

        bui_push_new_child(wctx, &widget);
    }
}
