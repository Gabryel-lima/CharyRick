#pragma once

#include <SDL2/SDL.h>

#define GAME_TITLE "CharyRick"

#define GRID_COLS 80
#define GRID_ROWS 24
#define HUD_LINES 4

#define FONT_POINT_SIZE 18
#define MAX_ENEMIES 24
#define MAX_MESSAGE_LENGTH 128

#define PLAYER_GLYPH '@'
#define WALL_GLYPH '#'
#define FLOOR_GLYPH '.'
#define STAIRS_GLYPH '>'
#define ENEMY_GLYPH 'g'

static const SDL_Color COLOR_BG = { 8, 12, 14, 255 };
static const SDL_Color COLOR_PANEL = { 16, 20, 24, 255 };
static const SDL_Color COLOR_TEXT = { 220, 235, 225, 255 };
static const SDL_Color COLOR_DIM = { 128, 148, 138, 255 };
static const SDL_Color COLOR_PLAYER = { 240, 245, 255, 255 };
static const SDL_Color COLOR_ENEMY = { 240, 100, 90, 255 };
static const SDL_Color COLOR_FLOOR = { 46, 58, 54, 255 };
static const SDL_Color COLOR_WALL = { 72, 84, 92, 255 };
static const SDL_Color COLOR_STAIRS = { 235, 200, 80, 255 };
static const SDL_Color COLOR_HIGHLIGHT = { 105, 190, 210, 255 };
static const SDL_Color COLOR_ALERT = { 240, 130, 130, 255 };
