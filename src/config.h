#pragma once

#include <SDL2/SDL.h>

#define GAME_TITLE "CharyRick"

#define GRID_COLS 80
#define GRID_ROWS 20
#define HUD_LINES 4

#define FONT_POINT_SIZE 18
#define SPRITE_FONT_POINT_SIZE 11
#define MAX_ENEMIES 32
#define MAX_ROOMS 10
#define MAX_ROOM_ATTEMPTS 128
#define MAX_INVENTORY_SLOTS 8
#define MAX_SHOP_OFFERS 3
#define MAX_MESSAGE_LENGTH 128

#define TILE_EMPTY ' '
#define TILE_WALL '#'
#define TILE_FLOOR '.'
#define TILE_DOOR '+'
#define TILE_DOOR_OPEN '/'
#define TILE_STAIRS_DOWN '>'
#define TILE_STAIRS_UP '<'
#define TILE_WATER '~'
#define TILE_LAVA '='
#define TILE_TRAP '^'
#define TILE_CHEST '$'
#define TILE_TORCH 'f'
#define TILE_PORTAL 'O'
#define TILE_SHOP 'S'

#define PLAYER_GLYPH_WARRIOR 'O'
#define PLAYER_GLYPH_MAGE '@'
#define PLAYER_GLYPH_ARCHER 'o'
#define PLAYER_GLYPH_ROGUE '*'

static const SDL_Color COLOR_BG = { 8, 10, 12, 255 };
static const SDL_Color COLOR_PANEL = { 20, 24, 28, 255 };
static const SDL_Color COLOR_TEXT = { 244, 244, 236, 255 };
static const SDL_Color COLOR_DIM = { 130, 140, 132, 255 };
static const SDL_Color COLOR_HIGHLIGHT = { 255, 208, 64, 255 };
static const SDL_Color COLOR_ALERT = { 230, 92, 76, 255 };

static const SDL_Color COLOR_PLAYER = { 255, 215, 0, 255 };
static const SDL_Color COLOR_ENEMY = { 204, 68, 68, 255 };
static const SDL_Color COLOR_ELITE = { 204, 68, 204, 255 };
static const SDL_Color COLOR_BOSS = { 255, 0, 0, 255 };

static const SDL_Color COLOR_FLOOR = { 64, 64, 64, 255 };
static const SDL_Color COLOR_WALL = { 192, 192, 192, 255 };
static const SDL_Color COLOR_WATER = { 34, 68, 136, 255 };
static const SDL_Color COLOR_LAVA = { 255, 102, 0, 255 };
static const SDL_Color COLOR_TRAP = { 196, 196, 196, 255 };
static const SDL_Color COLOR_DOOR = { 176, 132, 84, 255 };
static const SDL_Color COLOR_STAIRS = { 200, 200, 200, 255 };
static const SDL_Color COLOR_CHEST = { 255, 170, 0, 255 };
static const SDL_Color COLOR_TORCH = { 255, 170, 0, 255 };
static const SDL_Color COLOR_PORTAL = { 68, 196, 220, 255 };
static const SDL_Color COLOR_SHOP = { 90, 210, 130, 255 };
static const SDL_Color COLOR_ITEM_HP = { 255, 102, 102, 255 };
static const SDL_Color COLOR_ITEM_MP = { 102, 102, 255, 255 };
static const SDL_Color COLOR_EXP = { 34, 255, 34, 255 };
