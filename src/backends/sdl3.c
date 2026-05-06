/*
 * Copyright (c) 2026, Ibrahim KAIKAA <ibrahimkaikaa@gmail.com>
 * SPDX-License-Identifier: GPL-3.0
 */

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_timer.h>
#include <libui/backend.h>
#include <stdlib.h>

typedef struct
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *font_texture;
} bui_sdl3_context_t;

static SDL_Color _bui_to_sdl_color(bui_color_t color)
{
    return (SDL_Color){.r = color.r, .g = color.g, .b = color.b, .a = color.a};
}

static void _set_draw_color(SDL_Renderer *renderer, bui_color_t color)
{
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

static bui_sdl3_context_t *_ctx(bui_wctx_t *wctx)
{
    return wctx ? (bui_sdl3_context_t *) wctx->gfx_context : NULL;
}

static void _fill_window_size(SDL_Window *window, bui_window_resized_event_t *size)
{
    int pixel_width = 0;
    int pixel_height = 0;
    if (window)
        SDL_GetWindowSizeInPixels(window, &pixel_width, &pixel_height);
    size->width = (uint32_t) pixel_width;
    size->height = (uint32_t) pixel_height;
}

static bool _is_sdl3_initialized = false;

bool bui_gfx_init(bui_wctx_t *wctx)
{
    if (!_is_sdl3_initialized) {
        if (!SDL_Init(SDL_INIT_VIDEO))
            return false;
        _is_sdl3_initialized = true;
    }
    SDL_WindowFlags flags = 0;
    if (wctx->flags.resizable)
        flags |= SDL_WINDOW_RESIZABLE;
    if (wctx->flags.fullscreen)
        flags |= SDL_WINDOW_FULLSCREEN;
    SDL_Window *window = SDL_CreateWindow(wctx->title, wctx->width, wctx->height, flags);
    if (window == NULL)
        return false;

    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    if (renderer == NULL) {
        SDL_DestroyWindow(window);
        return false;
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    bui_sdl3_context_t *ctx = calloc(1, sizeof(*ctx));
    if (ctx == NULL) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        return false;
    }
    ctx->window = window;
    ctx->renderer = renderer;

    wctx->gfx_context = ctx;
    wctx->window_id = SDL_GetWindowID(window);
    bui_window_resized_event_t initial_size = {0};
    _fill_window_size(window, &initial_size);
    wctx->width = (int) initial_size.width;
    wctx->height = (int) initial_size.height;
    return true;
}

void bui_gfx_destroy(bui_wctx_t *wctx)
{
    bui_sdl3_context_t *ctx = _ctx(wctx);
    if (!ctx)
        return;
    if (ctx->font_texture)
        SDL_DestroyTexture(ctx->font_texture);
    if (ctx->renderer)
        SDL_DestroyRenderer(ctx->renderer);
    if (ctx->window)
        SDL_DestroyWindow(ctx->window);
    free(ctx);
    wctx->gfx_context = NULL;
}

static bui_keyboard_scancode_t _ui_translate_scancode(SDL_Scancode scancode)
{
    switch (scancode) {
    case SDL_SCANCODE_ESCAPE:
        return BUI_KEYBOARD_SCANCODE_ESCAPE;
    case SDL_SCANCODE_1:
        return BUI_KEYBOARD_SCANCODE_1;
    case SDL_SCANCODE_2:
        return BUI_KEYBOARD_SCANCODE_2;
    case SDL_SCANCODE_3:
        return BUI_KEYBOARD_SCANCODE_3;
    case SDL_SCANCODE_4:
        return BUI_KEYBOARD_SCANCODE_4;
    case SDL_SCANCODE_5:
        return BUI_KEYBOARD_SCANCODE_5;
    case SDL_SCANCODE_6:
        return BUI_KEYBOARD_SCANCODE_6;
    case SDL_SCANCODE_7:
        return BUI_KEYBOARD_SCANCODE_7;
    case SDL_SCANCODE_8:
        return BUI_KEYBOARD_SCANCODE_8;
    case SDL_SCANCODE_9:
        return BUI_KEYBOARD_SCANCODE_9;
    case SDL_SCANCODE_0:
        return BUI_KEYBOARD_SCANCODE_0;
    case SDL_SCANCODE_MINUS:
        return BUI_KEYBOARD_SCANCODE_MINUS;
    case SDL_SCANCODE_EQUALS:
        return BUI_KEYBOARD_SCANCODE_EQUALS;
    case SDL_SCANCODE_BACKSPACE:
        return BUI_KEYBOARD_SCANCODE_BACKSPACE;
    case SDL_SCANCODE_TAB:
        return BUI_KEYBOARD_SCANCODE_TAB;
    case SDL_SCANCODE_Q:
        return BUI_KEYBOARD_SCANCODE_Q;
    case SDL_SCANCODE_W:
        return BUI_KEYBOARD_SCANCODE_W;
    case SDL_SCANCODE_E:
        return BUI_KEYBOARD_SCANCODE_E;
    case SDL_SCANCODE_R:
        return BUI_KEYBOARD_SCANCODE_R;
    case SDL_SCANCODE_T:
        return BUI_KEYBOARD_SCANCODE_T;
    case SDL_SCANCODE_Y:
        return BUI_KEYBOARD_SCANCODE_Y;
    case SDL_SCANCODE_U:
        return BUI_KEYBOARD_SCANCODE_U;
    case SDL_SCANCODE_I:
        return BUI_KEYBOARD_SCANCODE_I;
    case SDL_SCANCODE_O:
        return BUI_KEYBOARD_SCANCODE_O;
    case SDL_SCANCODE_P:
        return BUI_KEYBOARD_SCANCODE_P;
    case SDL_SCANCODE_LEFTBRACKET:
        return BUI_KEYBOARD_SCANCODE_LBRACKET;
    case SDL_SCANCODE_RIGHTBRACKET:
        return BUI_KEYBOARD_SCANCODE_RBRACKET;
    case SDL_SCANCODE_RETURN:
        return BUI_KEYBOARD_SCANCODE_ENTER;
    case SDL_SCANCODE_LCTRL:
        return BUI_KEYBOARD_SCANCODE_LCTRL;
    case SDL_SCANCODE_A:
        return BUI_KEYBOARD_SCANCODE_A;
    case SDL_SCANCODE_S:
        return BUI_KEYBOARD_SCANCODE_S;
    case SDL_SCANCODE_D:
        return BUI_KEYBOARD_SCANCODE_D;
    case SDL_SCANCODE_F:
        return BUI_KEYBOARD_SCANCODE_F;
    case SDL_SCANCODE_G:
        return BUI_KEYBOARD_SCANCODE_G;
    case SDL_SCANCODE_H:
        return BUI_KEYBOARD_SCANCODE_H;
    case SDL_SCANCODE_J:
        return BUI_KEYBOARD_SCANCODE_J;
    case SDL_SCANCODE_K:
        return BUI_KEYBOARD_SCANCODE_K;
    case SDL_SCANCODE_L:
        return BUI_KEYBOARD_SCANCODE_L;
    case SDL_SCANCODE_SEMICOLON:
        return BUI_KEYBOARD_SCANCODE_SEMICOLON;
    case SDL_SCANCODE_APOSTROPHE:
        return BUI_KEYBOARD_SCANCODE_APOSTROPHE;
    case SDL_SCANCODE_GRAVE:
        return BUI_KEYBOARD_SCANCODE_BACKTICK;
    case SDL_SCANCODE_LSHIFT:
        return BUI_KEYBOARD_SCANCODE_LSHIFT;
    case SDL_SCANCODE_BACKSLASH:
        return BUI_KEYBOARD_SCANCODE_BACKSLASH;
    case SDL_SCANCODE_Z:
        return BUI_KEYBOARD_SCANCODE_Z;
    case SDL_SCANCODE_X:
        return BUI_KEYBOARD_SCANCODE_X;
    case SDL_SCANCODE_C:
        return BUI_KEYBOARD_SCANCODE_C;
    case SDL_SCANCODE_V:
        return BUI_KEYBOARD_SCANCODE_V;
    case SDL_SCANCODE_B:
        return BUI_KEYBOARD_SCANCODE_B;
    case SDL_SCANCODE_N:
        return BUI_KEYBOARD_SCANCODE_N;
    case SDL_SCANCODE_M:
        return BUI_KEYBOARD_SCANCODE_M;
    case SDL_SCANCODE_COMMA:
        return BUI_KEYBOARD_SCANCODE_COMMA;
    case SDL_SCANCODE_PERIOD:
        return BUI_KEYBOARD_SCANCODE_PERIOD;
    case SDL_SCANCODE_SLASH:
        return BUI_KEYBOARD_SCANCODE_SLASH;
    case SDL_SCANCODE_RSHIFT:
        return BUI_KEYBOARD_SCANCODE_RSHIFT;
    case SDL_SCANCODE_LALT:
        return BUI_KEYBOARD_SCANCODE_LALT;
    case SDL_SCANCODE_SPACE:
        return BUI_KEYBOARD_SCANCODE_SPACE;
    case SDL_SCANCODE_CAPSLOCK:
        return BUI_KEYBOARD_SCANCODE_CAPSLOCK;
    case SDL_SCANCODE_F1:
        return BUI_KEYBOARD_SCANCODE_F1;
    case SDL_SCANCODE_F2:
        return BUI_KEYBOARD_SCANCODE_F2;
    case SDL_SCANCODE_F3:
        return BUI_KEYBOARD_SCANCODE_F3;
    case SDL_SCANCODE_F4:
        return BUI_KEYBOARD_SCANCODE_F4;
    case SDL_SCANCODE_F5:
        return BUI_KEYBOARD_SCANCODE_F5;
    case SDL_SCANCODE_F6:
        return BUI_KEYBOARD_SCANCODE_F6;
    case SDL_SCANCODE_F7:
        return BUI_KEYBOARD_SCANCODE_F7;
    case SDL_SCANCODE_F8:
        return BUI_KEYBOARD_SCANCODE_F8;
    case SDL_SCANCODE_F9:
        return BUI_KEYBOARD_SCANCODE_F9;
    case SDL_SCANCODE_F10:
        return BUI_KEYBOARD_SCANCODE_F10;
    case SDL_SCANCODE_F11:
        return BUI_KEYBOARD_SCANCODE_F11;
    case SDL_SCANCODE_F12:
        return BUI_KEYBOARD_SCANCODE_F12;
    case SDL_SCANCODE_UP:
        return BUI_KEYBOARD_SCANCODE_UP;
    case SDL_SCANCODE_LEFT:
        return BUI_KEYBOARD_SCANCODE_LEFT;
    case SDL_SCANCODE_RIGHT:
        return BUI_KEYBOARD_SCANCODE_RIGHT;
    case SDL_SCANCODE_DOWN:
        return BUI_KEYBOARD_SCANCODE_DOWN;
    case SDL_SCANCODE_DELETE:
        return BUI_KEYBOARD_SCANCODE_DELETE;
    default:
        return BUI_KEYBOARD_SCANCODE_NULL;
    }
}

bool bui_poll_events(bui_event_t *event)
{
    SDL_Event sdl_event;
    if (!SDL_PollEvent(&sdl_event))
        return false;

    SDL_Window *window = NULL;
    uint32_t window_id = 0;

    switch (sdl_event.type) {
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
    case SDL_EVENT_WINDOW_RESIZED:
    case SDL_EVENT_WINDOW_SHOWN:
        window_id = sdl_event.window.windowID;
        break;
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
        window_id = sdl_event.key.windowID;
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        window_id = sdl_event.button.windowID;
        break;
    case SDL_EVENT_MOUSE_MOTION:
        window_id = sdl_event.motion.windowID;
        break;
    default:
        return false;
    }

    window = SDL_GetWindowFromID(window_id);

    switch (sdl_event.type) {
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        event->window_id = sdl_event.window.windowID;
        event->type = BUI_EVENT_WINDOW_CLOSED;
        break;
    case SDL_EVENT_WINDOW_RESIZED:
        event->window_id = sdl_event.window.windowID;
        event->type = BUI_EVENT_WINDOW_RESIZED;
        _fill_window_size(window, &event->window_resized);
        break;
    case SDL_EVENT_WINDOW_SHOWN:
        event->type = BUI_EVENT_WINDOW_CREATED;
        event->window_id = sdl_event.window.windowID;
        _fill_window_size(window, &event->window_resized);
        break;
    case SDL_EVENT_KEY_DOWN:
        event->window_id = sdl_event.key.windowID;
        event->type = sdl_event.key.repeat ? BUI_EVENT_KEY_HOLD : BUI_EVENT_KEY_DOWN;
        event->keyboard = _ui_translate_scancode(sdl_event.key.scancode);
        break;
    case SDL_EVENT_KEY_UP:
        event->window_id = sdl_event.key.windowID;
        event->type = BUI_EVENT_KEY_UP;
        event->keyboard = _ui_translate_scancode(sdl_event.key.scancode);
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (sdl_event.button.button == SDL_BUTTON_LEFT)
            event->mouse_button.button = BUI_MOUSE_BUTTON_LEFT;
        else if (sdl_event.button.button == SDL_BUTTON_RIGHT)
            event->mouse_button.button = BUI_MOUSE_BUTTON_RIGHT;
        else if (sdl_event.button.button == SDL_BUTTON_MIDDLE)
            event->mouse_button.button = BUI_MOUSE_BUTTON_MIDDLE;
        else
            return false;
        event->window_id = sdl_event.button.windowID;
        event->mouse_button.pos_x = (uint32_t) sdl_event.button.x;
        event->mouse_button.pos_y = (uint32_t) sdl_event.button.y;
        event->type = BUI_EVENT_MOUSE_BUTTON_DOWN;
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (sdl_event.button.button == SDL_BUTTON_LEFT)
            event->mouse_button.button = BUI_MOUSE_BUTTON_LEFT;
        else if (sdl_event.button.button == SDL_BUTTON_RIGHT)
            event->mouse_button.button = BUI_MOUSE_BUTTON_RIGHT;
        else if (sdl_event.button.button == SDL_BUTTON_MIDDLE)
            event->mouse_button.button = BUI_MOUSE_BUTTON_MIDDLE;
        else
            return false;
        event->window_id = sdl_event.button.windowID;
        event->mouse_button.pos_x = (uint32_t) sdl_event.button.x;
        event->mouse_button.pos_y = (uint32_t) sdl_event.button.y;
        event->type = BUI_EVENT_MOUSE_BUTTON_UP;
        break;
    case SDL_EVENT_MOUSE_MOTION:
        event->window_id = sdl_event.motion.windowID;
        event->type = BUI_EVENT_MOUSE_MOVE;
        event->mouse_move.pos_x = (uint32_t) sdl_event.motion.x;
        event->mouse_move.pos_y = (uint32_t) sdl_event.motion.y;
        event->mouse_move.delta_x = (uint32_t) sdl_event.motion.xrel;
        event->mouse_move.delta_y = (uint32_t) sdl_event.motion.yrel;
        break;
    default:
        return false;
    }

    return true;
}

void bui_delay(uint32_t ms)
{
    SDL_Delay(ms);
}

void bui_begin_frame(bui_wctx_t *wctx)
{
    bui_sdl3_context_t *ctx = _ctx(wctx);
    if (!ctx)
        return;
    SDL_SetRenderClipRect(ctx->renderer, NULL);
}

void bui_end_frame(bui_wctx_t *wctx)
{
    bui_sdl3_context_t *ctx = _ctx(wctx);
    if (!ctx)
        return;
    SDL_RenderPresent(ctx->renderer);
}

void bui_set_clip(bui_wctx_t *wctx, bui_area_t rect)
{
    bui_sdl3_context_t *ctx = _ctx(wctx);
    if (!ctx)
        return;
    SDL_Rect sdl_rect = {
        .x = (int) rect.x,
        .y = (int) rect.y,
        .w = (int) rect.width,
        .h = (int) rect.height,
    };
    SDL_SetRenderClipRect(ctx->renderer, &sdl_rect);
}

void bui_reset_clip(bui_wctx_t *wctx)
{
    bui_sdl3_context_t *ctx = _ctx(wctx);
    if (ctx)
        SDL_SetRenderClipRect(ctx->renderer, NULL);
}

void bui_draw_filled_rect(bui_wctx_t *wctx, bui_rect_t rect, bui_color_t color)
{
    bui_sdl3_context_t *ctx = _ctx(wctx);
    if (!ctx)
        return;
    _set_draw_color(ctx->renderer, color);
    SDL_FRect sdl_rect = {
        .x = (float) rect.x,
        .y = (float) rect.y,
        .w = (float) rect.width,
        .h = (float) rect.height,
    };
    SDL_RenderFillRect(ctx->renderer, &sdl_rect);
    if (rect.border_thickness > 0)
        bui_draw_rect(wctx, rect);
}

void bui_draw_rect(bui_wctx_t *wctx, bui_rect_t rect)
{
    bui_sdl3_context_t *ctx = _ctx(wctx);
    if (!ctx)
        return;
    _set_draw_color(ctx->renderer, rect.border_color);
    for (uint32_t i = 0; i < rect.border_thickness; ++i) {
        if (rect.width <= i * 2 || rect.height <= i * 2)
            break;
        SDL_FRect sdl_rect = {
            .x = (float) (rect.x + i),
            .y = (float) (rect.y + i),
            .w = (float) (rect.width - i * 2),
            .h = (float) (rect.height - i * 2),
        };
        SDL_RenderRect(ctx->renderer, &sdl_rect);
    }
}

void bui_draw_line(bui_wctx_t *wctx, bui_line_t line, bui_color_t color)
{
    bui_sdl3_context_t *ctx = _ctx(wctx);
    if (!ctx)
        return;
    _set_draw_color(ctx->renderer, color);
    uint32_t thickness = line.thickness == 0 ? 1 : line.thickness;
    for (uint32_t i = 0; i < thickness; ++i) {
        float offset = (float) i;
        SDL_RenderLine(
            ctx->renderer,
            (float) line.x1 + offset,
            (float) line.y1,
            (float) line.x2 + offset,
            (float) line.y2);
    }
}

static SDL_Texture *_get_font_texture(bui_sdl3_context_t *ctx, bui_font_t *font, bui_color_t color)
{
    (void) color;
    if (!ctx || !font || !font->atlas)
        return NULL;
    if (ctx->font_texture)
        return ctx->font_texture;

    SDL_Surface *surface = SDL_CreateSurface(
        (int) font->atlas_width, (int) font->atlas_height, SDL_PIXELFORMAT_RGBA32);
    if (!surface)
        return NULL;

    SDL_Color white = _bui_to_sdl_color((bui_color_t){0xFF, 0xFF, 0xFF, 0xFF});
    uint32_t *pixels = surface->pixels;
    for (uint32_t y = 0; y < font->atlas_height; ++y) {
        for (uint32_t x = 0; x < font->atlas_width; ++x) {
            uint8_t alpha = font->atlas[y * font->atlas_width + x];
            pixels[y * font->atlas_width + x]
                = SDL_MapSurfaceRGBA(surface, white.r, white.g, white.b, alpha);
        }
    }

    ctx->font_texture = SDL_CreateTextureFromSurface(ctx->renderer, surface);
    SDL_DestroySurface(surface);
    if (ctx->font_texture)
        SDL_SetTextureBlendMode(ctx->font_texture, SDL_BLENDMODE_BLEND);
    return ctx->font_texture;
}

static void _font_visual_metrics(bui_font_t *font, int32_t *ascent, uint32_t *line_height)
{
    int32_t min_top = 0;
    int32_t max_bottom = 0;
    bool found_visible_glyph = false;

    if (font && font->glyphs) {
        for (uint32_t ch = font->first_char; ch <= font->last_char; ++ch) {
            const bui_glyph_t *glyph = &font->glyphs[ch - font->first_char];
            if (glyph->width == 0 || glyph->height == 0)
                continue;

            int32_t top = (int32_t) glyph->y_offset;
            int32_t bottom = top + (int32_t) glyph->height;
            if (!found_visible_glyph || top < min_top)
                min_top = top;
            if (!found_visible_glyph || bottom > max_bottom)
                max_bottom = bottom;
            found_visible_glyph = true;
        }
    }

    if (!found_visible_glyph || max_bottom <= min_top) {
        *ascent = font ? (int32_t) font->height : 0;
        *line_height = font ? font->height : 0;
        return;
    }

    *ascent = min_top < 0 ? -min_top : 0;
    *line_height = (uint32_t) (max_bottom - min_top);
}

void bui_draw_text(
    bui_wctx_t *wctx, bui_font_t *font, bui_pos_t pos, bui_color_t color, const char *text)
{
    bui_sdl3_context_t *ctx = _ctx(wctx);
    if (!ctx || !font || !text)
        return;
    SDL_Texture *texture = _get_font_texture(ctx, font, color);
    if (!texture)
        return;
    SDL_SetTextureColorMod(texture, color.r, color.g, color.b);
    SDL_SetTextureAlphaMod(texture, color.a);

    int32_t ascent = 0;
    uint32_t line_height = 0;
    _font_visual_metrics(font, &ascent, &line_height);

    float pen_x = (float) pos.x;
    float baseline_y = (float) ((int32_t) pos.y + ascent);
    for (const unsigned char *ch = (const unsigned char *) text; *ch; ++ch) {
        if (*ch == '\n') {
            pen_x = (float) pos.x;
            baseline_y += (float) line_height;
            continue;
        }
        if (*ch < font->first_char || *ch > font->last_char)
            continue;
        const bui_glyph_t *glyph = &font->glyphs[*ch - font->first_char];
        if (glyph->width > 0 && glyph->height > 0) {
            SDL_FRect src = {
                .x = (float) glyph->x,
                .y = (float) glyph->y,
                .w = (float) glyph->width,
                .h = (float) glyph->height,
            };
            SDL_FRect dst = {
                .x = pen_x + (float) glyph->x_offset,
                .y = baseline_y + (float) (int32_t) glyph->y_offset,
                .w = (float) glyph->width,
                .h = (float) glyph->height,
            };
            SDL_RenderTexture(ctx->renderer, texture, &src, &dst);
        }
        pen_x += glyph->x_advance;
    }
}

uint32_t bui_get_text_width(bui_font_t *font, const char *text)
{
    if (!font || !text)
        return 0;
    uint32_t max_width = 0;
    uint32_t line_width = 0;
    for (const unsigned char *ch = (const unsigned char *) text; *ch; ++ch) {
        if (*ch == '\n') {
            if (line_width > max_width)
                max_width = line_width;
            line_width = 0;
            continue;
        }
        if (*ch >= font->first_char && *ch <= font->last_char)
            line_width += font->glyphs[*ch - font->first_char].x_advance;
    }
    return line_width > max_width ? line_width : max_width;
}

uint32_t bui_get_text_height(bui_font_t *font, const char *text)
{
    if (!font || !text || text[0] == '\0')
        return 0;

    int32_t ascent = 0;
    uint32_t line_height = 0;
    _font_visual_metrics(font, &ascent, &line_height);

    uint32_t lines = 1;
    for (const char *ch = text; *ch; ++ch) {
        if (*ch == '\n')
            ++lines;
    }
    return lines * line_height;
}

bui_area_t bui_get_text_area(bui_font_t *font, const char *text)
{
    return (bui_area_t){
        .x = 0,
        .y = 0,
        .width = bui_get_text_width(font, text),
        .height = bui_get_text_height(font, text),
    };
}
