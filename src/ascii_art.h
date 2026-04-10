#pragma once

#include <stddef.h>

#include "entity.h"
#include "renderer.h"
#include "world.h"

typedef enum AsciiArtPose {
    ASCII_ART_POSE_IDLE = 0,
    ASCII_ART_POSE_WALK,
    ASCII_ART_POSE_ATTACK,
    ASCII_ART_POSE_HURT,
    ASCII_ART_POSE_GUARD,
    ASCII_ART_POSE_DEAD,
} AsciiArtPose;

typedef struct AsciiSprite {
    const char *utf8_text;
    int origin_x;
    int origin_y;
    int pixel_offset_x;
    int pixel_offset_y;
    int line_step;
    RendererFontKind font_kind;
} AsciiSprite;

char ascii_art_wall_glyph(const World *world, int x, int y);
const AsciiSprite *ascii_art_player_sprite(PlayerClass player_class, AsciiArtPose pose, int frame);
const AsciiSprite *ascii_art_enemy_sprite(EnemyType enemy_type);
const AsciiSprite *ascii_art_structure_sprite(char tile);
void ascii_art_draw_sprite(Renderer *renderer, const AsciiSprite *sprite, int tile_x, int tile_y,
                           int row_offset, SDL_Color color);
void ascii_art_format_item_label(ItemType item_type, char *buffer, size_t buffer_size);
void ascii_art_format_weapon_label(WeaponType weapon_type, char *buffer, size_t buffer_size);
