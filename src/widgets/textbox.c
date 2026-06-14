/*
 * Copyright (c) 2026, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <ctype.h>
#include <libui/backend.h>
#include <libui/libui.h>
#include <libui/widgets/textbox.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define STB_TEXTEDIT_STRING bui_widget_t
#define STB_TEXTEDIT_STRINGLEN(obj) _textbox_string_length((obj)->string)
#define STB_TEXTEDIT_GETCHAR(obj, i) ((obj)->string[(i)])
#define STB_TEXTEDIT_NEWLINE '\n'
#define STB_TEXTEDIT_LAYOUTROW(row, obj, n) _textbox_layout_row((row), (obj), (n))
#define STB_TEXTEDIT_GETWIDTH(obj, n, i) _textbox_get_width((obj), (n), (i))
#define STB_TEXTEDIT_KEYTOTEXT(key) (((key) > 0 && (key) < 0x100) ? (key) : 0)
#define STB_TEXTEDIT_DELETECHARS(obj, pos, n) _textbox_delete_chars((obj), (pos), (n))
#define STB_TEXTEDIT_INSERTCHARS(obj, pos, chars, n) \
    _textbox_insert_chars((obj), (pos), (chars), (n))
#define STB_TEXTEDIT_K_SHIFT 0x400
#define STB_TEXTEDIT_K_LEFT 0x100
#define STB_TEXTEDIT_K_RIGHT 0x101
#define STB_TEXTEDIT_K_UP 0x102
#define STB_TEXTEDIT_K_DOWN 0x103
#define STB_TEXTEDIT_K_LINESTART 0x104
#define STB_TEXTEDIT_K_LINEEND 0x105
#define STB_TEXTEDIT_K_TEXTSTART 0x106
#define STB_TEXTEDIT_K_TEXTEND 0x107
#define STB_TEXTEDIT_K_DELETE 0x108
#define STB_TEXTEDIT_K_BACKSPACE 0x109
#define STB_TEXTEDIT_K_UNDO 0x10A
#define STB_TEXTEDIT_K_REDO 0x10B
#define STB_TEXTEDIT_K_WORDLEFT 0x10C
#define STB_TEXTEDIT_K_WORDRIGHT 0x10D
#define STB_TEXTEDIT_K_PGUP 0x10E
#define STB_TEXTEDIT_K_PGDOWN 0x10F
#define STB_TEXTEDIT_IS_SPACE(ch) isspace((unsigned char) (ch))

static int _textbox_string_length(const char *string)
{
    return string ? (int) strlen(string) : 0;
}

static void _textbox_terminate_string(char *string, size_t size)
{
    if (size > 0)
        string[size - 1] = '\0';
}

static uint32_t _textbox_char_width(bui_widget_t *widget, char ch)
{
    bui_font_t *font = widget->theme->font;
    if (!font || ch < font->first_char || ch > font->last_char)
        return 0;
    return font->glyphs[(unsigned char) ch - font->first_char].x_advance;
}

static uint32_t _textbox_text_width(bui_widget_t *widget, int start, int count)
{
    uint32_t width = 0;
    for (int i = 0; i < count && widget->string[start + i] != '\0'; i++)
        width += _textbox_char_width(widget, widget->string[start + i]);
    return width;
}

static void _textbox_layout_row(StbTexteditRow *row, bui_widget_t *widget, int start)
{
    int count = _textbox_string_length(widget->string) - start;
    if (count < 0)
        count = 0;

    row->x0 = 0.0f;
    row->x1 = (float) _textbox_text_width(widget, start, count);
    row->baseline_y_delta = (float) widget->theme->font->height;
    row->ymin = 0.0f;
    row->ymax = row->baseline_y_delta;
    row->num_chars = count;
}

static float _textbox_get_width(bui_widget_t *widget, int start, int char_index)
{
    int index = start + char_index;
    if (index < 0 || index >= _textbox_string_length(widget->string))
        return 0.0f;
    return (float) _textbox_char_width(widget, widget->string[index]);
}

static void _textbox_delete_chars(bui_widget_t *widget, int pos, int count)
{
    int length = _textbox_string_length(widget->string);
    if (pos < 0 || count <= 0 || pos >= length)
        return;
    if (pos + count > length)
        count = length - pos;

    memmove(widget->string + pos, widget->string + pos + count, (size_t) (length - pos - count + 1));
}

static int _textbox_insert_chars(bui_widget_t *widget, int pos, const char *chars, int count)
{
    int length = _textbox_string_length(widget->string);
    if (pos < 0 || pos > length || count <= 0 || widget->string_size == 0)
        return 0;
    if ((size_t) length + (size_t) count + 1 > widget->string_size)
        return 0;

    memmove(widget->string + pos + count, widget->string + pos, (size_t) (length - pos + 1));
    memcpy(widget->string + pos, chars, (size_t) count);
    return 1;
}

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#define STB_TEXTEDIT_IMPLEMENTATION
#include "../../vendor/stb/stb_textedit.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

static uint32_t _textbox_visible_width(bui_widget_t *widget)
{
    uint32_t padding = widget->theme->inner_padding.l + widget->theme->inner_padding.r;
    return widget->computed_area.width > padding ? widget->computed_area.width - padding : 0;
}

static void _textbox_clamp(bui_widget_t *widget)
{
    int length = _textbox_string_length(widget->string);
    if (widget->edit.cursor > length)
        widget->edit.cursor = length;
    if (widget->edit.select_start > length)
        widget->edit.select_start = length;
    if (widget->edit.select_end > length)
        widget->edit.select_end = length;
}

static void _textbox_scroll_to_cursor(bui_widget_t *widget)
{
    uint32_t visible_width = _textbox_visible_width(widget);
    int length = _textbox_string_length(widget->string);
    uint32_t text_width = _textbox_text_width(widget, 0, length);
    uint32_t cursor_x = _textbox_text_width(widget, 0, widget->edit.cursor);

    if (visible_width == 0 || text_width <= visible_width) {
        widget->scroll_x = 0;
        return;
    }

    uint32_t margin = _textbox_char_width(widget, 'M');
    uint32_t max_scroll = text_width - visible_width;
    if (margin > visible_width / 2)
        margin = visible_width / 2;

    if (cursor_x < widget->scroll_x + margin)
        widget->scroll_x = cursor_x > margin ? cursor_x - margin : 0;
    else if (cursor_x + margin > widget->scroll_x + visible_width)
        widget->scroll_x = cursor_x + margin - visible_width;
    if (widget->scroll_x > max_scroll)
        widget->scroll_x = max_scroll;
}

static int _textbox_first_visible_char(bui_widget_t *widget, uint32_t *char_x)
{
    uint32_t x = 0;
    int length = _textbox_string_length(widget->string);
    for (int i = 0; i < length; i++) {
        uint32_t width = _textbox_char_width(widget, widget->string[i]);
        if (x + width > widget->scroll_x) {
            *char_x = x;
            return i;
        }
        x += width;
    }
    *char_x = x;
    return length;
}

static int _textbox_stb_key(bui_wctx_t *ctx, bui_keyboard_scancode_t key)
{
    bool shift = bui_is_key_down(ctx, BUI_KEYBOARD_SCANCODE_LSHIFT)
                 || bui_is_key_down(ctx, BUI_KEYBOARD_SCANCODE_RSHIFT);
    bool ctrl = bui_is_key_down(ctx, BUI_KEYBOARD_SCANCODE_LCTRL);
    int mod = shift ? STB_TEXTEDIT_K_SHIFT : 0;

    switch (key) {
    case BUI_KEYBOARD_SCANCODE_LEFT:
        return (ctrl ? STB_TEXTEDIT_K_WORDLEFT : STB_TEXTEDIT_K_LEFT) | mod;
    case BUI_KEYBOARD_SCANCODE_RIGHT:
        return (ctrl ? STB_TEXTEDIT_K_WORDRIGHT : STB_TEXTEDIT_K_RIGHT) | mod;
    case BUI_KEYBOARD_SCANCODE_UP:
        return STB_TEXTEDIT_K_UP | mod;
    case BUI_KEYBOARD_SCANCODE_DOWN:
        return STB_TEXTEDIT_K_DOWN | mod;
    case BUI_KEYBOARD_SCANCODE_HOME:
        return (ctrl ? STB_TEXTEDIT_K_TEXTSTART : STB_TEXTEDIT_K_LINESTART) | mod;
    case BUI_KEYBOARD_SCANCODE_END:
        return (ctrl ? STB_TEXTEDIT_K_TEXTEND : STB_TEXTEDIT_K_LINEEND) | mod;
    case BUI_KEYBOARD_SCANCODE_BACKSPACE:
        return STB_TEXTEDIT_K_BACKSPACE;
    case BUI_KEYBOARD_SCANCODE_DELETE:
        return STB_TEXTEDIT_K_DELETE;
    case BUI_KEYBOARD_SCANCODE_Z:
        return ctrl ? STB_TEXTEDIT_K_UNDO : 0;
    case BUI_KEYBOARD_SCANCODE_Y:
        return ctrl ? STB_TEXTEDIT_K_REDO : 0;
    default:
        return 0;
    }
}

static void _textbox_insert_text(bui_widget_t *widget, const char *text)
{
    char ascii[BUI_TEXT_INPUT_MAX];
    int count = 0;
    for (const unsigned char *ch = (const unsigned char *) text; *ch && count < (int) sizeof(ascii);
         ch++) {
        if (*ch >= 0x20 && *ch < 0x7F)
            ascii[count++] = (char) *ch;
    }
    if (count > 0)
        stb_textedit_paste(widget, &widget->edit, ascii, count);
}

static float _textbox_mouse_x(bui_wctx_t *ctx, bui_widget_t *widget)
{
    int32_t text_x = (int32_t) widget->computed_area.x + (int32_t) widget->theme->inner_padding.l;
    int32_t mouse_x = (int32_t) ctx->mouse_state.pos_x;
    float local_x = mouse_x > text_x ? (float) (mouse_x - text_x) : 0.0f;
    return local_x + (float) widget->scroll_x;
}

static bool _textbox_submitted(bui_wctx_t *ctx, bui_id_t id)
{
    return ctx->focused_widget_id == id && ctx->input_event.type == BUI_EVENT_KEY_DOWN
           && ctx->input_event.keyboard == BUI_KEYBOARD_SCANCODE_ENTER;
}

static void _textbox_handle_input(bui_wctx_t *ctx, bui_widget_t *widget)
{
    bui_event_type_t type = ctx->input_event.type;

    if (type == BUI_EVENT_MOUSE_BUTTON_DOWN && ctx->active_widget_id == widget->id) {
        ctx->focused_widget_id = widget->id;
        stb_textedit_click(widget, &widget->edit, _textbox_mouse_x(ctx, widget), 0.0f);
    } else if (
        type == BUI_EVENT_MOUSE_MOVE && ctx->active_widget_id == widget->id
        && bui_is_mouse_button_down(ctx, BUI_MOUSE_BUTTON_LEFT)) {
        stb_textedit_drag(widget, &widget->edit, _textbox_mouse_x(ctx, widget), 0.0f);
    }

    if (ctx->focused_widget_id != widget->id)
        return;

    if (type == BUI_EVENT_TEXT_INPUT) {
        _textbox_insert_text(widget, ctx->input_event.text_input.text);
    } else if (type == BUI_EVENT_KEY_DOWN || type == BUI_EVENT_KEY_HOLD) {
        bool ctrl = bui_is_key_down(ctx, BUI_KEYBOARD_SCANCODE_LCTRL);
        if (ctrl && ctx->input_event.keyboard == BUI_KEYBOARD_SCANCODE_A) {
            int length = _textbox_string_length(widget->string);
            widget->edit.cursor = length;
            widget->edit.select_start = 0;
            widget->edit.select_end = length;
        } else {
            int key = _textbox_stb_key(ctx, ctx->input_event.keyboard);
            if (key != 0)
                stb_textedit_key(widget, &widget->edit, key);
        }
    }

    _textbox_clamp(widget);
    _textbox_scroll_to_cursor(widget);
}

static void _draw_textbox(bui_wctx_t *ctx, bui_widget_t *widget)
{
    _textbox_handle_input(ctx, widget);
    _textbox_scroll_to_cursor(widget);

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
        widget->theme->background_color);
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

    uint32_t text_x = widget->computed_area.x + widget->theme->inner_padding.l;
    uint32_t text_y = widget->computed_area.y + widget->theme->inner_padding.t;
    uint32_t text_bottom = widget->computed_area.y + widget->computed_area.height
                           - widget->theme->inner_padding.b;
    if (text_bottom < text_y)
        text_bottom = text_y;

    bui_area_t clip = {
        .x = text_x,
        .y = widget->computed_area.y + widget->theme->border_thickness,
        .width = _textbox_visible_width(widget),
        .height = widget->computed_area.height > widget->theme->border_thickness * 2
                      ? widget->computed_area.height - widget->theme->border_thickness * 2
                      : 0,
    };
    bui_set_clip(ctx, clip);

    if (widget->edit.select_start != widget->edit.select_end) {
        int sel_start = widget->edit.select_start < widget->edit.select_end
                            ? widget->edit.select_start
                            : widget->edit.select_end;
        int sel_end = widget->edit.select_start > widget->edit.select_end
                          ? widget->edit.select_start
                          : widget->edit.select_end;
        int32_t x0 = (int32_t) text_x + (int32_t) _textbox_text_width(widget, 0, sel_start)
                     - (int32_t) widget->scroll_x;
        int32_t x1 = x0 + (int32_t) _textbox_text_width(widget, sel_start, sel_end - sel_start);
        int32_t clip_x0 = (int32_t) clip.x;
        int32_t clip_x1 = (int32_t) (clip.x + clip.width);
        if (x0 < clip_x0)
            x0 = clip_x0;
        if (x1 > clip_x1)
            x1 = clip_x1;
        if (x1 > x0) {
            bui_draw_filled_rect(
                ctx,
                (bui_rect_t){
                    .x = (uint32_t) x0,
                    .y = text_y,
                    .width = (uint32_t) (x1 - x0),
                    .height = text_bottom - text_y,
                },
                widget->theme->shadow_color);
        }
    }

    if (widget->string && widget->string[0] != '\0') {
        uint32_t first_char_x = 0;
        int first_char = _textbox_first_visible_char(widget, &first_char_x);
        uint32_t hidden_px = widget->scroll_x > first_char_x ? widget->scroll_x - first_char_x : 0;
        bui_draw_text(
            ctx,
            widget->theme->font,
            (bui_pos_t){.x = text_x > hidden_px ? text_x - hidden_px : text_x, .y = text_y},
            widget->theme->foreground_color,
            widget->string + first_char);
    }

    if (ctx->focused_widget_id == widget->id) {
        int32_t cursor_x = (int32_t) text_x
                           + (int32_t) _textbox_text_width(widget, 0, widget->edit.cursor)
                           - (int32_t) widget->scroll_x;
        int32_t clip_x0 = (int32_t) clip.x;
        int32_t clip_x1 = (int32_t) (clip.x + clip.width);
        if (cursor_x < clip_x0)
            cursor_x = clip_x0;
        if (cursor_x >= clip_x1)
            cursor_x = clip_x1 > clip_x0 ? clip_x1 - 1 : clip_x0;
        bui_draw_line(
            ctx,
            (bui_line_t){
                .x1 = (uint32_t) cursor_x,
                .x2 = (uint32_t) cursor_x,
                .y1 = text_y,
                .y2 = text_bottom,
                .thickness = 1,
            },
            widget->theme->foreground_color);
    }

    bui_reset_clip(ctx);
}

bool bui_textbox(bui_wctx_t *wctx, const char *label, char *buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0)
        return false;

    bui_id_t id = bui_hash_string(label ? label : "");
    bui_theme_t *theme = bui_get_current_theme(wctx);
    bool submitted = _textbox_submitted(wctx, id);
    _textbox_terminate_string(buffer, buffer_size);

    if (!wctx->hot_state) {
        bui_widget_t widget = {
            .flags = BUI_WIDGET_FLAG_CLICKABLE,
            .label = label,
            .string = buffer,
            .string_size = buffer_size,
            .theme = theme,
            .semantic_size = {
                [BUI_AXIS_X] = {
                    .type = BUI_WIDGET_SIZE_TYPE_PERCENTAGE,
                    .value = 1.0f,
                    .strictness = 0.0f,
                },
                [BUI_AXIS_Y] = {
                    .type = BUI_WIDGET_SIZE_TYPE_FIXED,
                    .value = bui_get_text_height(theme->font, "M") + theme->inner_padding.t
                             + theme->inner_padding.b,
                    .strictness = 0.0f,
                },
            },
            .draw = _draw_textbox,
        };
        stb_textedit_initialize_state(&widget.edit, 1);
        bui_widget_t *textbox = bui_push_new_child(wctx, &widget);
        if (textbox)
            bui_push_id(wctx, id, textbox);
        return submitted;
    }

    bui_widget_t *textbox = bui_get_by_id(wctx, id);
    if (textbox == NULL)
        return submitted;
    textbox->label = label;
    textbox->string = buffer;
    textbox->string_size = buffer_size;
    _textbox_terminate_string(buffer, buffer_size);
    return submitted;
}
