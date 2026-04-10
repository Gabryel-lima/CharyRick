#include "ascii_art.h"

#include <stdio.h>
#include <string.h>

#define ASCII_ART_LINE_STEP 9

static const AsciiSprite ENEMY_GOBLIN_SPRITE = {
    "  ^\n (o)\n  |\n / \\",
    2,
    3,
    0,
    0,
    ASCII_ART_LINE_STEP,
    RENDERER_FONT_SPRITE,
};

static const AsciiSprite ENEMY_SKELETON_SPRITE = {
    "  o\n /|\\\n | |\n / \\",
    2,
    3,
    0,
    0,
    ASCII_ART_LINE_STEP,
    RENDERER_FONT_SPRITE,
};

static const AsciiSprite ENEMY_ZOMBIE_SPRITE = {
    "  @\n /|~\n / \\",
    2,
    2,
    0,
    1,
    ASCII_ART_LINE_STEP,
    RENDERER_FONT_SPRITE,
};

static const AsciiSprite ENEMY_BAT_SPRITE = {
    "/\\\n(  )\n \\/\n><><",
    1,
    3,
    0,
    -1,
    8,
    RENDERER_FONT_SPRITE,
};

static const AsciiSprite ENEMY_SLIME_SPRITE = {
    " ___\n/   \\\n| ^^ |\n \\___/",
    2,
    3,
    0,
    0,
    8,
    RENDERER_FONT_SPRITE,
};

static const AsciiSprite ENEMY_SPIDER_SPRITE = {
    "  *\n (*)\n /|\n / \\",
    2,
    3,
    0,
    0,
    8,
    RENDERER_FONT_SPRITE,
};

static const AsciiSprite ENEMY_FUNGUS_SPRITE = {
    "  ^\n (^)\n  |",
    2,
    2,
    0,
    2,
    8,
    RENDERER_FONT_SPRITE,
};

static const AsciiSprite ENEMY_SHADOW_KNIGHT_SPRITE = {
    "  Θ\n[/|\\]\n / \\",
    2,
    2,
    0,
    0,
    ASCII_ART_LINE_STEP,
    RENDERER_FONT_SPRITE,
};

static const AsciiSprite ENEMY_NECROMANCER_SPRITE = {
    "  ô\n /|\\\n | \\\n  ~",
    2,
    3,
    0,
    -1,
    8,
    RENDERER_FONT_SPRITE,
};

static const AsciiSprite ENEMY_GOLEM_SPRITE = {
    "███\n█°°█\n█ _ █\n█/ \\",
    2,
    3,
    0,
    -1,
    8,
    RENDERER_FONT_SPRITE,
};

static const AsciiSprite ENEMY_ASSASSIN_SPRITE = {
    "  °\n /|.\n /|_\n  ~",
    2,
    3,
    0,
    -1,
    8,
    RENDERER_FONT_SPRITE,
};

static const AsciiSprite ENEMY_WITCH_SPRITE = {
    "  ö\n /|\\\n | \\\n  ~",
    2,
    3,
    0,
    -1,
    8,
    RENDERER_FONT_SPRITE,
};

static const AsciiSprite ENEMY_CRYPT_GUARDIAN_SPRITE = {
    "  ___\n /   \\\n| o o |\n \\   /\n  | |\n /   \\",
    3,
    5,
    0,
    -3,
    8,
    RENDERER_FONT_SPRITE,
};

static const AsciiSprite STRUCTURE_CHEST_SPRITE = {
    " _____\n|     |\n|_____|",
    3,
    2,
    0,
    2,
    8,
    RENDERER_FONT_SPRITE,
};

static const AsciiSprite STRUCTURE_SHOP_SPRITE = {
    "[$][$]\n NPC ",
    2,
    1,
    0,
    0,
    8,
    RENDERER_FONT_SPRITE,
};

static const AsciiSprite STRUCTURE_PORTAL_SPRITE = {
    "╔≈≈≈╗\n║≈≈≈║\n╚≈≈≈╝",
    2,
    2,
    0,
    0,
    8,
    RENDERER_FONT_SPRITE,
};

static const AsciiSprite STRUCTURE_STAIRS_DOWN_SPRITE = {
    " <<<\n< < <\n<   <",
    2,
    2,
    0,
    1,
    8,
    RENDERER_FONT_SPRITE,
};

static const AsciiSprite STRUCTURE_STAIRS_UP_SPRITE = {
    " >>>\n> > >\n>   >",
    2,
    2,
    0,
    1,
    8,
    RENDERER_FONT_SPRITE,
};

static const AsciiSprite STRUCTURE_TORCH_SPRITE = {
    ")f(\n)|f|(\n~~~",
    1,
    2,
    0,
    0,
    8,
    RENDERER_FONT_SPRITE,
};

static const char *ascii_art_player_head(PlayerClass player_class) {
    switch (player_class) {
        case PLAYER_CLASS_WARRIOR:
            return "Θ";
        case PLAYER_CLASS_MAGE:
            return "ô";
        case PLAYER_CLASS_ARCHER:
            return "o";
        case PLAYER_CLASS_ROGUE:
            return "°";
        default:
            return "o";
    }
}

static bool ascii_art_cell_is_open(const World *world, int x, int y) {
    return world != NULL && world_in_bounds(x, y) && world_tile_at(world, x, y) != TILE_WALL;
}

char ascii_art_wall_glyph(const World *world, int x, int y) {
    bool north_open = ascii_art_cell_is_open(world, x, y - 1);
    bool south_open = ascii_art_cell_is_open(world, x, y + 1);
    bool west_open = ascii_art_cell_is_open(world, x - 1, y);
    bool east_open = ascii_art_cell_is_open(world, x + 1, y);

    if ((north_open || south_open) && !(east_open || west_open)) {
        return '-';
    }
    if ((east_open || west_open) && !(north_open || south_open)) {
        return '|';
    }
    if (north_open || south_open || east_open || west_open) {
        return '+';
    }
    return '#';
}

const AsciiSprite *ascii_art_player_sprite(PlayerClass player_class, AsciiArtPose pose, int frame) {
    static AsciiSprite sprite;
    static char sprite_text[128];
    const char *head = ascii_art_player_head(player_class);

    sprite.origin_x = 2;
    sprite.origin_y = 2;
    sprite.pixel_offset_x = 0;
    sprite.pixel_offset_y = 0;
    sprite.line_step = ASCII_ART_LINE_STEP;
    sprite.font_kind = RENDERER_FONT_SPRITE;

    switch (pose) {
        case ASCII_ART_POSE_WALK:
            switch (frame % 4) {
                case 0:
                    snprintf(sprite_text, sizeof(sprite_text), "  %s\n (|)\n--|--\n  |\n / \\", head);
                    sprite.origin_y = 4;
                    sprite.line_step = 8;
                    break;
                case 1:
                    snprintf(sprite_text, sizeof(sprite_text), "  %s\n (|)\n--|--\n  |\n/ |", head);
                    sprite.origin_y = 4;
                    sprite.line_step = 8;
                    break;
                case 2:
                    snprintf(sprite_text, sizeof(sprite_text), "  %s\n (|)\n--|--\n / \\", head);
                    sprite.origin_y = 3;
                    sprite.line_step = 8;
                    break;
                default:
                    snprintf(sprite_text, sizeof(sprite_text), "  %s\n (|)\n--|--\n \\ /", head);
                    sprite.origin_y = 3;
                    sprite.line_step = 8;
                    break;
            }
            break;
        case ASCII_ART_POSE_ATTACK:
            switch (frame % 4) {
                case 0:
                    snprintf(sprite_text, sizeof(sprite_text), "  %s\n (|)\n--|--\n  |\n / \\", head);
                    sprite.origin_y = 4;
                    sprite.line_step = 8;
                    break;
                case 1:
                    snprintf(sprite_text, sizeof(sprite_text), "  %s\n (|)\n--|--\\\n  |\n / \\", head);
                    sprite.origin_y = 4;
                    sprite.line_step = 8;
                    break;
                case 2:
                    snprintf(sprite_text, sizeof(sprite_text), "  %s\n (|)->\n--|--\n  |\n / \\", head);
                    sprite.origin_y = 4;
                    sprite.line_step = 8;
                    break;
                default:
                    snprintf(sprite_text, sizeof(sprite_text), "  %s\n (|)\n--|--\n  |\n / \\", head);
                    sprite.origin_y = 4;
                    sprite.line_step = 8;
                    break;
            }
            break;
        case ASCII_ART_POSE_HURT:
            snprintf(sprite_text, sizeof(sprite_text), "  @\n \\|/\n  | \\\n / \\");
            sprite.origin_y = 3;
            sprite.line_step = 8;
            break;
        case ASCII_ART_POSE_GUARD:
            snprintf(sprite_text, sizeof(sprite_text), " [%s]\n[/|\\]\n / \\", head);
            break;
        case ASCII_ART_POSE_DEAD:
            snprintf(sprite_text, sizeof(sprite_text), "  x\n--+--\n  | ");
            break;
        case ASCII_ART_POSE_IDLE:
        default:
            switch (player_class) {
                case PLAYER_CLASS_WARRIOR:
                    snprintf(sprite_text, sizeof(sprite_text), "  Θ\n[/|\\]\n / \\");
                    break;
                case PLAYER_CLASS_MAGE:
                    snprintf(sprite_text, sizeof(sprite_text), "  ô\n /|\\\n / \\");
                    break;
                case PLAYER_CLASS_ARCHER:
                    snprintf(sprite_text, sizeof(sprite_text), "  o\n\\|->\n / \\");
                    break;
                case PLAYER_CLASS_ROGUE:
                    snprintf(sprite_text, sizeof(sprite_text), "  °\n*/|\\\n / \\");
                    break;
                default:
                    snprintf(sprite_text, sizeof(sprite_text), "  o\n (|)\n / \\");
                    break;
            }
            break;
    }

    sprite.utf8_text = sprite_text;
    return &sprite;
}

const AsciiSprite *ascii_art_enemy_sprite(EnemyType enemy_type) {
    switch (enemy_type) {
        case ENEMY_GOBLIN:
            return &ENEMY_GOBLIN_SPRITE;
        case ENEMY_SKELETON:
            return &ENEMY_SKELETON_SPRITE;
        case ENEMY_ZOMBIE:
            return &ENEMY_ZOMBIE_SPRITE;
        case ENEMY_BAT:
            return &ENEMY_BAT_SPRITE;
        case ENEMY_SLIME:
            return &ENEMY_SLIME_SPRITE;
        case ENEMY_SPIDER:
            return &ENEMY_SPIDER_SPRITE;
        case ENEMY_FUNGUS:
            return &ENEMY_FUNGUS_SPRITE;
        case ENEMY_SHADOW_KNIGHT:
            return &ENEMY_SHADOW_KNIGHT_SPRITE;
        case ENEMY_NECROMANCER:
            return &ENEMY_NECROMANCER_SPRITE;
        case ENEMY_STONE_GOLEM:
            return &ENEMY_GOLEM_SPRITE;
        case ENEMY_ASSASSIN:
            return &ENEMY_ASSASSIN_SPRITE;
        case ENEMY_WITCH:
            return &ENEMY_WITCH_SPRITE;
        case ENEMY_CRYPT_GUARDIAN:
            return &ENEMY_CRYPT_GUARDIAN_SPRITE;
        default:
            return NULL;
    }
}

const AsciiSprite *ascii_art_structure_sprite(char tile) {
    switch (tile) {
        case TILE_CHEST:
            return &STRUCTURE_CHEST_SPRITE;
        case TILE_SHOP:
            return &STRUCTURE_SHOP_SPRITE;
        case TILE_PORTAL:
            return &STRUCTURE_PORTAL_SPRITE;
        case TILE_STAIRS_DOWN:
            return &STRUCTURE_STAIRS_DOWN_SPRITE;
        case TILE_STAIRS_UP:
            return &STRUCTURE_STAIRS_UP_SPRITE;
        case TILE_TORCH:
            return &STRUCTURE_TORCH_SPRITE;
        default:
            return NULL;
    }
}

void ascii_art_draw_sprite(Renderer *renderer, const AsciiSprite *sprite, int tile_x, int tile_y,
                           int row_offset, SDL_Color color) {
    if (renderer == NULL || sprite == NULL || sprite->utf8_text == NULL) {
        return;
    }

    int glyph_w = 0;
    int glyph_h = 0;
    renderer_get_font_metrics(renderer, sprite->font_kind, &glyph_w, &glyph_h);
    int line_step = sprite->line_step > 0 ? sprite->line_step : glyph_h;

    int start_x = tile_x * renderer->cell_w - sprite->origin_x * glyph_w + sprite->pixel_offset_x;
    int start_y = (tile_y + row_offset) * renderer->cell_h - sprite->origin_y * line_step +
                  sprite->pixel_offset_y;

    const char *cursor = sprite->utf8_text;
    char line[128];
    int row = 0;
    while (*cursor != '\0') {
        const char *line_end = strchr(cursor, '\n');
        size_t length = line_end != NULL ? (size_t)(line_end - cursor) : strlen(cursor);
        if (length >= sizeof(line)) {
            length = sizeof(line) - 1;
        }

        memcpy(line, cursor, length);
        line[length] = '\0';

        renderer_draw_utf8_pixels(renderer, start_x, start_y + row * line_step, line, color,
                                  sprite->font_kind);

        row += 1;
        if (line_end == NULL) {
            break;
        }
        cursor = line_end + 1;
    }
}

void ascii_art_format_item_label(ItemType item_type, char *buffer, size_t buffer_size) {
    if (buffer == NULL || buffer_size == 0) {
        return;
    }

    switch (item_type) {
        case ITEM_POTION_HEALTH:
            snprintf(buffer, buffer_size, "[P] Health Potion");
            break;
        case ITEM_POTION_MANA:
            snprintf(buffer, buffer_size, "[M] Mana Potion");
            break;
        case ITEM_SCROLL_FIRE:
            snprintf(buffer, buffer_size, "[F] Fire Scroll");
            break;
        case ITEM_SCROLL_ICE:
            snprintf(buffer, buffer_size, "[I] Ice Scroll");
            break;
        case ITEM_SCROLL_HEALING:
            snprintf(buffer, buffer_size, "[C] Healing Scroll");
            break;
        case ITEM_RELIC_TITAN_RING:
            snprintf(buffer, buffer_size, "[R] Titan Ring");
            break;
        case ITEM_RELIC_DEMON_EYE:
            snprintf(buffer, buffer_size, "[E] Demon Eye");
            break;
        case ITEM_RELIC_CRYSTAL_HEART:
            snprintf(buffer, buffer_size, "[X] Crystal Heart");
            break;
        case ITEM_CRYPT_KEY:
            snprintf(buffer, buffer_size, "[K] Crypt Key");
            break;
        default:
            snprintf(buffer, buffer_size, "[ ] Empty");
            break;
    }
}

void ascii_art_format_weapon_label(WeaponType weapon_type, char *buffer, size_t buffer_size) {
    if (buffer == NULL || buffer_size == 0) {
        return;
    }

    switch (weapon_type) {
        case WEAPON_IRON_SWORD:
            snprintf(buffer, buffer_size, "[/] Iron Sword");
            break;
        case WEAPON_OAK_STAFF:
            snprintf(buffer, buffer_size, "[|] Oak Staff");
            break;
        case WEAPON_HUNTER_BOW:
            snprintf(buffer, buffer_size, "[)] Hunter Bow");
            break;
        case WEAPON_SHADOW_DAGGER:
            snprintf(buffer, buffer_size, "[*] Shadow Dagger");
            break;
        case WEAPON_FLAME_BLADE:
            snprintf(buffer, buffer_size, "[/] Flame Blade");
            break;
        case WEAPON_THUNDER_HAMMER:
            snprintf(buffer, buffer_size, "[=] Thunder Hammer");
            break;
        case WEAPON_SUPREME_STAFF:
            snprintf(buffer, buffer_size, "[|] Supreme Staff");
            break;
        case WEAPON_ASTRAL_BOW:
            snprintf(buffer, buffer_size, "[)] Astral Bow");
            break;
        case WEAPON_MOON_TWINS:
            snprintf(buffer, buffer_size, "[*] Moon Twins");
            break;
        default:
            snprintf(buffer, buffer_size, "[*] Fists");
            break;
    }
}
