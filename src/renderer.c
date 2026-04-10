#include "renderer.h"

#include <stdio.h>
#include <string.h>

static const char *const FONT_PATHS[] = {
    "assets/fonts/DejaVuSansMono.ttf",
    "../assets/fonts/DejaVuSansMono.ttf",
    "../../assets/fonts/DejaVuSansMono.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
    "/usr/share/fonts/truetype/liberation2/LiberationMono-Regular.ttf",
    "/usr/share/fonts/liberation/LiberationMono-Regular.ttf",
};

static const size_t FONT_PATH_COUNT = sizeof(FONT_PATHS) / sizeof(FONT_PATHS[0]);

static TTF_Font *renderer_font_for_kind(const Renderer *renderer, RendererFontKind kind) {
    if (renderer == NULL) {
        return NULL;
    }

    return kind == RENDERER_FONT_SPRITE ? renderer->sprite_font : renderer->font;
}

static int renderer_font_cell_width(TTF_Font *font) {
    if (font == NULL) {
        return 0;
    }

    int min_x = 0;
    int max_x = 0;
    int min_y = 0;
    int max_y = 0;
    int advance = 0;
    if (TTF_GlyphMetrics(font, 'M', &min_x, &max_x, &min_y, &max_y, &advance) == 0 &&
        advance > 0) {
        return advance;
    }

    int text_w = 0;
    int text_h = 0;
    if (TTF_SizeText(font, "M", &text_w, &text_h) == 0 && text_w > 0) {
        return text_w;
    }

    return TTF_FontHeight(font) / 2;
}

static const char *renderer_find_font_path(void) {
    for (size_t index = 0; index < FONT_PATH_COUNT; ++index) {
        SDL_RWops *file = SDL_RWFromFile(FONT_PATHS[index], "rb");
        if (file != NULL) {
            SDL_RWclose(file);
            return FONT_PATHS[index];
        }
    }

    return NULL;
}

static GlyphCache *renderer_cache_glyph(Renderer *renderer, unsigned char glyph) {
    if (renderer == NULL || renderer->font == NULL || glyph >= 128) {
        return NULL;
    }

    GlyphCache *cache = &renderer->glyphs[glyph];
    if (cache->loaded) {
        return cache;
    }

    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Surface *surface = TTF_RenderGlyph_Blended(renderer->font, glyph, white);
    if (surface == NULL) {
        cache->loaded = true;
        return cache;
    }

    cache->texture = SDL_CreateTextureFromSurface(renderer->sdl_renderer, surface);
    cache->w = surface->w;
    cache->h = surface->h;
    cache->loaded = true;

    if (cache->texture != NULL) {
        SDL_SetTextureBlendMode(cache->texture, SDL_BLENDMODE_BLEND);
    }

    SDL_FreeSurface(surface);
    return cache;
}

bool renderer_update_layout(Renderer *renderer) {
    if (renderer == NULL || renderer->sdl_renderer == NULL) {
        return false;
    }

    int output_w = 0;
    int output_h = 0;
    if (SDL_GetRendererOutputSize(renderer->sdl_renderer, &output_w, &output_h) != 0) {
        return false;
    }

    renderer->window_w = output_w;
    renderer->window_h = output_h;

    int content_w = GRID_COLS * renderer->cell_w;
    int content_h = (GRID_ROWS + HUD_LINES) * renderer->cell_h;

    renderer->origin_x = output_w > content_w ? (output_w - content_w) / 2 : 0;
    renderer->origin_y = output_h > content_h ? (output_h - content_h) / 2 : 0;
    return true;
}

bool renderer_init(Renderer *renderer, SDL_Renderer *sdl_renderer) {
    if (renderer == NULL || sdl_renderer == NULL) {
        return false;
    }

    memset(renderer, 0, sizeof(*renderer));
    renderer->sdl_renderer = sdl_renderer;

    const char *font_path = renderer_find_font_path();
    if (font_path == NULL) {
        SDL_SetError("could not locate a monospace font");
        return false;
    }

    renderer->font = TTF_OpenFont(font_path, FONT_POINT_SIZE);
    if (renderer->font == NULL) {
        return false;
    }

    renderer->sprite_font = TTF_OpenFont(font_path, SPRITE_FONT_POINT_SIZE);
    if (renderer->sprite_font == NULL) {
        renderer_shutdown(renderer);
        return false;
    }

    TTF_SetFontStyle(renderer->font, TTF_STYLE_NORMAL);
    TTF_SetFontHinting(renderer->font, TTF_HINTING_MONO);
    TTF_SetFontStyle(renderer->sprite_font, TTF_STYLE_NORMAL);
    TTF_SetFontHinting(renderer->sprite_font, TTF_HINTING_MONO);

    renderer->cell_h = TTF_FontHeight(renderer->font);
    renderer->cell_w = renderer_font_cell_width(renderer->font);
    renderer->sprite_cell_h = TTF_FontHeight(renderer->sprite_font);
    renderer->sprite_cell_w = renderer_font_cell_width(renderer->sprite_font);

    if (renderer->cell_w <= 0) {
        renderer->cell_w = renderer->cell_h / 2;
    }
    if (renderer->sprite_cell_w <= 0) {
        renderer->sprite_cell_w = renderer->sprite_cell_h / 2;
    }

    if (renderer->cell_h <= 0) {
        renderer->cell_h = FONT_POINT_SIZE + 4;
    }
    if (renderer->sprite_cell_h <= 0) {
        renderer->sprite_cell_h = SPRITE_FONT_POINT_SIZE + 2;
    }

    SDL_SetRenderDrawBlendMode(renderer->sdl_renderer, SDL_BLENDMODE_BLEND);
    return renderer_update_layout(renderer);
}

void renderer_shutdown(Renderer *renderer) {
    if (renderer == NULL) {
        return;
    }

    for (size_t index = 0; index < 128; ++index) {
        if (renderer->glyphs[index].texture != NULL) {
            SDL_DestroyTexture(renderer->glyphs[index].texture);
            renderer->glyphs[index].texture = NULL;
        }
    }

    if (renderer->font != NULL) {
        TTF_CloseFont(renderer->font);
        renderer->font = NULL;
    }

    if (renderer->sprite_font != NULL) {
        TTF_CloseFont(renderer->sprite_font);
        renderer->sprite_font = NULL;
    }
}

void renderer_clear(Renderer *renderer, SDL_Color color) {
    if (renderer == NULL) {
        return;
    }

    SDL_SetRenderDrawColor(renderer->sdl_renderer, color.r, color.g, color.b, color.a);
    SDL_RenderClear(renderer->sdl_renderer);
}

void renderer_present(Renderer *renderer) {
    if (renderer == NULL) {
        return;
    }

    SDL_RenderPresent(renderer->sdl_renderer);
}

void renderer_draw_char(Renderer *renderer, int cell_x, int cell_y, char glyph, SDL_Color color) {
    if (renderer == NULL || glyph == '\0' || glyph == ' ') {
        return;
    }

    unsigned char code = (unsigned char)glyph;
    if (code >= 128) {
        code = '?';
    }

    GlyphCache *cache = renderer_cache_glyph(renderer, code);
    if (cache == NULL || cache->texture == NULL) {
        return;
    }

    SDL_SetTextureColorMod(cache->texture, color.r, color.g, color.b);
    SDL_SetTextureAlphaMod(cache->texture, color.a);

    SDL_Rect destination = {
        renderer->origin_x + cell_x * renderer->cell_w + (renderer->cell_w - cache->w) / 2,
        renderer->origin_y + cell_y * renderer->cell_h + (renderer->cell_h - cache->h) / 2,
        cache->w,
        cache->h,
    };

    SDL_RenderCopy(renderer->sdl_renderer, cache->texture, NULL, &destination);
}

void renderer_draw_text(Renderer *renderer, int cell_x, int cell_y, const char *text, SDL_Color color) {
    if (renderer == NULL || text == NULL) {
        return;
    }

    int current_x = cell_x;
    int current_y = cell_y;

    for (const char *cursor = text; *cursor != '\0'; ++cursor) {
        if (*cursor == '\n') {
            current_y += 1;
            current_x = cell_x;
            continue;
        }

        renderer_draw_char(renderer, current_x, current_y, *cursor, color);
        current_x += 1;
    }
}

void renderer_get_font_metrics(const Renderer *renderer, RendererFontKind kind, int *cell_w,
                               int *cell_h) {
    if (cell_w != NULL) {
        *cell_w = kind == RENDERER_FONT_SPRITE ? renderer->sprite_cell_w : renderer->cell_w;
    }
    if (cell_h != NULL) {
        *cell_h = kind == RENDERER_FONT_SPRITE ? renderer->sprite_cell_h : renderer->cell_h;
    }
}

void renderer_draw_utf8_pixels(Renderer *renderer, int pixel_x, int pixel_y, const char *text,
                               SDL_Color color, RendererFontKind kind) {
    if (renderer == NULL || text == NULL || text[0] == '\0') {
        return;
    }

    TTF_Font *font = renderer_font_for_kind(renderer, kind);
    if (font == NULL) {
        return;
    }

    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text, color);
    if (surface == NULL) {
        return;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer->sdl_renderer, surface);
    if (texture == NULL) {
        SDL_FreeSurface(surface);
        return;
    }

    SDL_Rect destination = {
        renderer->origin_x + pixel_x,
        renderer->origin_y + pixel_y,
        surface->w,
        surface->h,
    };

    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_RenderCopy(renderer->sdl_renderer, texture, NULL, &destination);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

void renderer_draw_box(Renderer *renderer, int cell_x, int cell_y, int cell_w, int cell_h,
                       SDL_Color border, SDL_Color fill) {
    if (renderer == NULL || cell_w <= 0 || cell_h <= 0) {
        return;
    }

    SDL_Rect fill_rect = {
        renderer->origin_x + cell_x * renderer->cell_w,
        renderer->origin_y + cell_y * renderer->cell_h,
        cell_w * renderer->cell_w,
        cell_h * renderer->cell_h,
    };

    SDL_SetRenderDrawColor(renderer->sdl_renderer, fill.r, fill.g, fill.b, fill.a);
    SDL_RenderFillRect(renderer->sdl_renderer, &fill_rect);

    if (cell_w < 2 || cell_h < 2) {
        return;
    }

    for (int x = 0; x < cell_w; ++x) {
        renderer_draw_char(renderer, cell_x + x, cell_y, '-', border);
        renderer_draw_char(renderer, cell_x + x, cell_y + cell_h - 1, '-', border);
    }

    for (int y = 0; y < cell_h; ++y) {
        renderer_draw_char(renderer, cell_x, cell_y + y, '|', border);
        renderer_draw_char(renderer, cell_x + cell_w - 1, cell_y + y, '|', border);
    }

    renderer_draw_char(renderer, cell_x, cell_y, '+', border);
    renderer_draw_char(renderer, cell_x + cell_w - 1, cell_y, '+', border);
    renderer_draw_char(renderer, cell_x, cell_y + cell_h - 1, '+', border);
    renderer_draw_char(renderer, cell_x + cell_w - 1, cell_y + cell_h - 1, '+', border);
}
