/*
 * Copyright (c) 2026, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <libui/font.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "../vendor/stb/stb_truetype.h"

static bui_font_t _default_font;
static bool _default_font_loaded;

static unsigned char *_read_file(const char *path, size_t *size)
{
    FILE *file = fopen(path, "rb");
    if (!file)
        return NULL;

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }

    long length = ftell(file);
    if (length <= 0) {
        fclose(file);
        return NULL;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }

    unsigned char *data = malloc((size_t) length);
    if (!data) {
        fclose(file);
        return NULL;
    }

    size_t read = fread(data, 1, (size_t) length, file);
    fclose(file);
    if (read != (size_t) length) {
        free(data);
        return NULL;
    }

    if (size)
        *size = (size_t) length;
    return data;
}

bool bui_font_load_from_file(bui_font_t *font, const char *path, float pixel_height)
{
    if (!font || !path || pixel_height <= 0.0f)
        return false;

    unsigned char *font_data = _read_file(path, NULL);
    if (!font_data)
        return false;

    stbtt_fontinfo font_info;
    int font_offset = stbtt_GetFontOffsetForIndex(font_data, 0);
    if (font_offset < 0 || !stbtt_InitFont(&font_info, font_data, font_offset)) {
        free(font_data);
        return false;
    }

    uint8_t *atlas = calloc(BUI_FONT_ATLAS_WIDTH * BUI_FONT_ATLAS_HEIGHT, sizeof(*atlas));
    bui_glyph_t *glyphs = calloc(BUI_FONT_LAST_CHAR - BUI_FONT_FIRST_CHAR + 1, sizeof(*glyphs));
    stbtt_bakedchar *baked_chars
        = calloc(BUI_FONT_LAST_CHAR - BUI_FONT_FIRST_CHAR + 1, sizeof(*baked_chars));
    if (!atlas || !glyphs || !baked_chars) {
        free(baked_chars);
        free(glyphs);
        free(atlas);
        free(font_data);
        return false;
    }

    int bake_result = stbtt_BakeFontBitmap(
        font_data,
        font_offset,
        pixel_height,
        atlas,
        BUI_FONT_ATLAS_WIDTH,
        BUI_FONT_ATLAS_HEIGHT,
        BUI_FONT_FIRST_CHAR,
        BUI_FONT_LAST_CHAR - BUI_FONT_FIRST_CHAR + 1,
        baked_chars);
    if (bake_result <= 0) {
        free(baked_chars);
        free(glyphs);
        free(atlas);
        free(font_data);
        return false;
    }

    int ascent = 0;
    int descent = 0;
    int line_gap = 0;
    float scale = stbtt_ScaleForPixelHeight(&font_info, pixel_height);
    stbtt_GetFontVMetrics(&font_info, &ascent, &descent, &line_gap);

    for (uint32_t i = 0; i <= BUI_FONT_LAST_CHAR - BUI_FONT_FIRST_CHAR; ++i) {
        stbtt_bakedchar ch = baked_chars[i];
        glyphs[i] = (bui_glyph_t){
            .x = (uint32_t) ch.x0,
            .y = (uint32_t) ch.y0,
            .width = (uint32_t) (ch.x1 - ch.x0),
            .height = (uint32_t) (ch.y1 - ch.y0),
            .x_advance = (uint32_t) (ch.xadvance + 0.5f),
            .x_offset = (int32_t) ch.xoff,
            .y_offset = (int32_t) ch.yoff,
        };
    }

    *font = (bui_font_t){
        .width = (uint32_t) pixel_height,
        .height = (uint32_t) ((float) (ascent - descent + line_gap) * scale + 0.5f),
        .first_char = BUI_FONT_FIRST_CHAR,
        .last_char = BUI_FONT_LAST_CHAR,
        .atlas = atlas,
        .atlas_width = BUI_FONT_ATLAS_WIDTH,
        .atlas_height = BUI_FONT_ATLAS_HEIGHT,
        .glyphs = glyphs,
    };

    free(baked_chars);
    free(font_data);
    return true;
}

void bui_font_free(bui_font_t *font)
{
    if (!font)
        return;

    free((void *) font->atlas);
    free((void *) font->glyphs);
    memset(font, 0, sizeof(*font));
}

bui_font_t *bui_get_default_font(void)
{
    return &_default_font;
}

bool bui_font_init_default(void)
{
    if (_default_font_loaded)
        return true;

    const char *paths[] = {
        BUI_DEFAULT_FONT_PATH,
        "../" BUI_DEFAULT_FONT_PATH,
    };
    for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i) {
        if (bui_font_load_from_file(&_default_font, paths[i], BUI_DEFAULT_FONT_SIZE)) {
            _default_font_loaded = true;
            return true;
        }
    }

    return false;
}
