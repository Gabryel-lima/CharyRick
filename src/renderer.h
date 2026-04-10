#pragma once

#include <stdbool.h>

#include <SDL2/SDL_ttf.h>

#include "config.h"

typedef struct GlyphCache {
    SDL_Texture *texture;
    int w;
    int h;
    bool loaded;
} GlyphCache;

typedef enum RendererFontKind {
    RENDERER_FONT_GRID = 0,
    RENDERER_FONT_SPRITE,
} RendererFontKind;

typedef struct Renderer {
    SDL_Renderer *sdl_renderer;
    TTF_Font *font;
    TTF_Font *sprite_font;
    int cell_w;
    int cell_h;
    int sprite_cell_w;
    int sprite_cell_h;
    int window_w;
    int window_h;
    int origin_x;
    int origin_y;
    GlyphCache glyphs[128];
} Renderer;

bool renderer_init(Renderer *renderer, SDL_Renderer *sdl_renderer);
bool renderer_update_layout(Renderer *renderer);
void renderer_shutdown(Renderer *renderer);
void renderer_clear(Renderer *renderer, SDL_Color color);
void renderer_present(Renderer *renderer);
void renderer_draw_char(Renderer *renderer, int cell_x, int cell_y, char glyph, SDL_Color color);
void renderer_draw_text(Renderer *renderer, int cell_x, int cell_y, const char *text, SDL_Color color);
void renderer_get_font_metrics(const Renderer *renderer, RendererFontKind kind, int *cell_w,
                               int *cell_h);
void renderer_draw_utf8_pixels(Renderer *renderer, int pixel_x, int pixel_y, const char *text,
                               SDL_Color color, RendererFontKind kind);
void renderer_draw_box(Renderer *renderer, int cell_x, int cell_y, int cell_w, int cell_h,
                       SDL_Color border, SDL_Color fill);
