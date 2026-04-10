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

typedef struct Renderer {
    SDL_Renderer *sdl_renderer;
    TTF_Font *font;
    int cell_w;
    int cell_h;
    GlyphCache glyphs[128];
} Renderer;

bool renderer_init(Renderer *renderer, SDL_Renderer *sdl_renderer);
void renderer_shutdown(Renderer *renderer);
void renderer_clear(Renderer *renderer, SDL_Color color);
void renderer_present(Renderer *renderer);
void renderer_draw_char(Renderer *renderer, int cell_x, int cell_y, char glyph, SDL_Color color);
void renderer_draw_text(Renderer *renderer, int cell_x, int cell_y, const char *text, SDL_Color color);
void renderer_draw_box(Renderer *renderer, int cell_x, int cell_y, int cell_w, int cell_h,
                       SDL_Color border, SDL_Color fill);
