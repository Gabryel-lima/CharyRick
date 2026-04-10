#include "game.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ascii_art.h"
#include "config.h"
#include "entity.h"
#include "input.h"
#include "renderer.h"
#include "world.h"

typedef enum GameMode {
    GAME_MODE_MAIN_MENU = 0,
    GAME_MODE_CLASS_SELECT,
    GAME_MODE_PLAYING,
    GAME_MODE_INVENTORY,
    GAME_MODE_SHOP,
    GAME_MODE_PAUSED,
    GAME_MODE_GAME_OVER,
    GAME_MODE_VICTORY,
} GameMode;

typedef enum ActionResult {
    ACTION_NONE = 0,
    ACTION_TURN,
    ACTION_SKIP_ENEMY,
} ActionResult;

typedef struct ShopOffer {
    bool weapon_offer;
    WeaponType weapon;
    ItemType item;
    int price;
    bool sold;
} ShopOffer;

struct Game {
    SDL_Window *window;
    SDL_Renderer *sdl_renderer;
    Renderer renderer;
    World world;
    Entity player;
    Entity enemies[MAX_ENEMIES];
    ItemType inventory[MAX_INVENTORY_SLOTS];
    ShopOffer shop_offers[MAX_SHOP_OFFERS];
    int enemy_count;
    GameMode mode;
    bool running;
    int menu_selection;
    int class_selection;
    int pause_selection;
    int inventory_selection;
    int shop_selection;
    int game_over_selection;
    int victory_selection;
    int floor_number;
    int turn_counter;
    int kills;
    int revive_charges;
    unsigned int seed;
    bool debug_overlay;
    char message[MAX_MESSAGE_LENGTH];
    char death_message[MAX_MESSAGE_LENGTH];
    AsciiArtPose player_pose;
    Uint32 player_pose_expires_at;
    int player_pose_frame;
};

static const char *const MAIN_MENU_OPTIONS[] = {
    "Start Run",
    "Quit",
};

static const char *const PAUSE_MENU_OPTIONS[] = {
    "Resume",
    "Inventory",
    "Main Menu",
    "Quit",
};

static const char *const GAME_OVER_OPTIONS[] = {
    "Retry",
    "Main Menu",
    "Quit",
};

static const char *const VICTORY_OPTIONS[] = {
    "New Run",
    "Main Menu",
    "Quit",
};

static const PlayerClass CLASS_SELECTION_ORDER[PLAYER_CLASS_COUNT] = {
    PLAYER_CLASS_WARRIOR,
    PLAYER_CLASS_MAGE,
    PLAYER_CLASS_ARCHER,
    PLAYER_CLASS_ROGUE,
};

static void game_shutdown_runtime(Game *game);
static void game_generate_floor(Game *game);
static void game_begin_run(Game *game, PlayerClass player_class, unsigned int seed,
                           int floor_number);
static void game_start_new_run(Game *game);
static void game_advance_floor(Game *game);
static void game_complete_turn(Game *game);
static void game_open_shop(Game *game);
static bool game_open_chest(Game *game);
static void game_set_game_over(Game *game, const char *format, ...);
static void game_set_victory(Game *game, const char *subtitle);
static void game_render_scene(Game *game);
static void game_render(Game *game);
static void game_reset_player_pose(Game *game);
static void game_set_player_pose(Game *game, AsciiArtPose pose, Uint32 duration_ms);
static AsciiArtPose game_player_render_pose(const Game *game);
static int game_player_render_frame(const Game *game);
static int game_class_selection_index(PlayerClass player_class);
static bool game_parse_debug_class_view(const char *view_name, const char *prefix,
                                        PlayerClass *player_class);

static bool game_view_name_is(const char *view_name, const char *candidate) {
    return view_name != NULL && candidate != NULL && strcmp(view_name, candidate) == 0;
}

static int game_class_selection_index(PlayerClass player_class) {
    for (int index = 0; index < PLAYER_CLASS_COUNT; ++index) {
        if (CLASS_SELECTION_ORDER[index] == player_class) {
            return index;
        }
    }

    return 0;
}

static bool game_parse_debug_class_view(const char *view_name, const char *prefix,
                                        PlayerClass *player_class) {
    if (view_name == NULL || prefix == NULL || player_class == NULL) {
        return false;
    }

    size_t prefix_length = strlen(prefix);
    if (strncmp(view_name, prefix, prefix_length) != 0) {
        return false;
    }

    const char *suffix = view_name + prefix_length;
    if (strcmp(suffix, "warrior") == 0) {
        *player_class = PLAYER_CLASS_WARRIOR;
        return true;
    }
    if (strcmp(suffix, "mage") == 0) {
        *player_class = PLAYER_CLASS_MAGE;
        return true;
    }
    if (strcmp(suffix, "archer") == 0) {
        *player_class = PLAYER_CLASS_ARCHER;
        return true;
    }
    if (strcmp(suffix, "rogue") == 0) {
        *player_class = PLAYER_CLASS_ROGUE;
        return true;
    }

    return false;
}

static int game_visible_rows(void) {
    return GRID_ROWS + HUD_LINES;
}

static int sign_int(int value) {
    if (value < 0) {
        return -1;
    }
    if (value > 0) {
        return 1;
    }
    return 0;
}

static void game_reset_player_pose(Game *game) {
    if (game == NULL) {
        return;
    }

    game->player_pose = ASCII_ART_POSE_IDLE;
    game->player_pose_expires_at = 0;
    game->player_pose_frame = 0;
}

static void game_set_player_pose(Game *game, AsciiArtPose pose, Uint32 duration_ms) {
    if (game == NULL) {
        return;
    }

    game->player_pose = pose;
    game->player_pose_frame = (game->player_pose_frame + 1) % 4;
    game->player_pose_expires_at = duration_ms > 0 ? SDL_GetTicks() + duration_ms : 0;
}

static AsciiArtPose game_player_render_pose(const Game *game) {
    if (game == NULL) {
        return ASCII_ART_POSE_IDLE;
    }

    if (!game->player.alive || game->mode == GAME_MODE_GAME_OVER) {
        return ASCII_ART_POSE_DEAD;
    }

    if (game->mode == GAME_MODE_VICTORY) {
        return ASCII_ART_POSE_GUARD;
    }

    if (game->player_pose_expires_at > SDL_GetTicks()) {
        return game->player_pose;
    }

    if (entity_has_status(&game->player, STATUS_GUARD)) {
        return ASCII_ART_POSE_GUARD;
    }

    return ASCII_ART_POSE_IDLE;
}

static int game_player_render_frame(const Game *game) {
    if (game == NULL) {
        return 0;
    }

    return game->player_pose_frame;
}

static void game_set_message(Game *game, const char *format, ...) {
    if (game == NULL || format == NULL) {
        return;
    }

    va_list arguments;
    va_start(arguments, format);
    vsnprintf(game->message, sizeof(game->message), format, arguments);
    va_end(arguments);
}

static void game_set_death_message(Game *game, const char *format, ...) {
    if (game == NULL || format == NULL) {
        return;
    }

    va_list arguments;
    va_start(arguments, format);
    vsnprintf(game->death_message, sizeof(game->death_message), format, arguments);
    va_end(arguments);
}

static int game_text_width(const char *text) {
    if (text == NULL) {
        return 0;
    }

    int longest = 0;
    int current = 0;
    for (const char *cursor = text; *cursor != '\0'; ++cursor) {
        if (*cursor == '\n') {
            if (current > longest) {
                longest = current;
            }
            current = 0;
            continue;
        }
        current += 1;
    }

    return current > longest ? current : longest;
}

static int game_option_line_width(const char *const *options, int option_count) {
    int longest = 0;
    for (int index = 0; index < option_count; ++index) {
        int width = 2 + game_text_width(options[index]);
        if (width > longest) {
            longest = width;
        }
    }
    return longest;
}

static void game_draw_centered_text(Renderer *renderer, int start_x, int width, int row,
                                    const char *text, SDL_Color color) {
    if (renderer == NULL || text == NULL) {
        return;
    }

    int text_width = (int)strlen(text);
    int x = start_x + (width - text_width) / 2;
    if (x < start_x) {
        x = start_x;
    }
    renderer_draw_text(renderer, x, row, text, color);
}

static const char *game_mode_name(GameMode mode) {
    switch (mode) {
        case GAME_MODE_MAIN_MENU:
            return "main_menu";
        case GAME_MODE_CLASS_SELECT:
            return "class_select";
        case GAME_MODE_PLAYING:
            return "playing";
        case GAME_MODE_INVENTORY:
            return "inventory";
        case GAME_MODE_SHOP:
            return "shop";
        case GAME_MODE_PAUSED:
            return "paused";
        case GAME_MODE_GAME_OVER:
            return "game_over";
        case GAME_MODE_VICTORY:
            return "victory";
        default:
            return "unknown";
    }
}

static int game_alive_enemy_count(const Game *game) {
    if (game == NULL) {
        return 0;
    }

    int alive_enemies = 0;
    for (int index = 0; index < game->enemy_count; ++index) {
        if (game->enemies[index].alive) {
            alive_enemies += 1;
        }
    }
    return alive_enemies;
}

static int game_find_enemy_at(const Game *game, int x, int y) {
    if (game == NULL) {
        return -1;
    }

    for (int index = 0; index < game->enemy_count; ++index) {
        const Entity *enemy = &game->enemies[index];
        if (enemy->alive && enemy->x == x && enemy->y == y) {
            return index;
        }
    }

    return -1;
}

static bool game_is_occupied(const Game *game, int x, int y, int ignore_enemy_index) {
    if (game == NULL) {
        return false;
    }

    if (game->player.alive && game->player.x == x && game->player.y == y) {
        return true;
    }

    for (int index = 0; index < game->enemy_count; ++index) {
        if (index == ignore_enemy_index) {
            continue;
        }

        const Entity *enemy = &game->enemies[index];
        if (enemy->alive && enemy->x == x && enemy->y == y) {
            return true;
        }
    }

    return false;
}

static void game_clear_enemies(Game *game) {
    if (game == NULL) {
        return;
    }

    memset(game->enemies, 0, sizeof(game->enemies));
    game->enemy_count = 0;
}

static void game_clear_inventory(Game *game) {
    if (game == NULL) {
        return;
    }

    for (int slot = 0; slot < MAX_INVENTORY_SLOTS; ++slot) {
        game->inventory[slot] = ITEM_NONE;
    }
    game->inventory_selection = 0;
}

static void game_clear_shop(Game *game) {
    if (game == NULL) {
        return;
    }

    memset(game->shop_offers, 0, sizeof(game->shop_offers));
    game->shop_selection = 0;
}

static int game_inventory_count(const Game *game) {
    if (game == NULL) {
        return 0;
    }

    int count = 0;
    for (int slot = 0; slot < MAX_INVENTORY_SLOTS; ++slot) {
        if (game->inventory[slot] != ITEM_NONE) {
            count += 1;
        }
    }
    return count;
}

static void game_compact_inventory(Game *game) {
    if (game == NULL) {
        return;
    }

    ItemType compacted[MAX_INVENTORY_SLOTS] = { ITEM_NONE };
    int next_slot = 0;
    for (int slot = 0; slot < MAX_INVENTORY_SLOTS; ++slot) {
        if (game->inventory[slot] != ITEM_NONE) {
            compacted[next_slot++] = game->inventory[slot];
        }
    }
    memcpy(game->inventory, compacted, sizeof(compacted));

    if (game->inventory_selection >= MAX_INVENTORY_SLOTS) {
        game->inventory_selection = MAX_INVENTORY_SLOTS - 1;
    }
    if (game->inventory_selection < 0) {
        game->inventory_selection = 0;
    }
}

static bool game_try_revive_player(Game *game) {
    if (game == NULL || game->player.alive || game->revive_charges <= 0) {
        return false;
    }

    game->revive_charges -= 1;
    game->player.alive = true;
    game->player.hp = game->player.max_hp / 2;
    if (game->player.hp < 1) {
        game->player.hp = 1;
    }
    game->player.mp = game->player.max_mp / 2;
    game_set_player_pose(game, ASCII_ART_POSE_HURT, 220);
    game_set_message(game, "The Crystal Heart revives Rick.");
    return true;
}

static void game_set_game_over(Game *game, const char *format, ...) {
    if (game == NULL || format == NULL) {
        return;
    }

    va_list arguments;
    va_start(arguments, format);
    vsnprintf(game->death_message, sizeof(game->death_message), format, arguments);
    va_end(arguments);
    game_set_player_pose(game, ASCII_ART_POSE_DEAD, 0);
    game->mode = GAME_MODE_GAME_OVER;
    game->game_over_selection = 0;
}

static void game_set_victory(Game *game, const char *subtitle) {
    if (game == NULL) {
        return;
    }

    game_set_death_message(game, "%s", subtitle != NULL ? subtitle : "Rick escapes the crypt.");
    game_set_player_pose(game, ASCII_ART_POSE_GUARD, 0);
    game->mode = GAME_MODE_VICTORY;
    game->victory_selection = 0;
}

static void game_apply_relic(Game *game, ItemType item_type) {
    if (game == NULL) {
        return;
    }

    switch (item_type) {
        case ITEM_RELIC_TITAN_RING:
            game->player.max_hp += 10;
            game->player.hp += 10;
            if (game->player.hp > game->player.max_hp) {
                game->player.hp = game->player.max_hp;
            }
            game_set_message(game, "Titan Ring fortifies Rick.");
            break;
        case ITEM_RELIC_DEMON_EYE:
            game->player.vision += 2;
            game_set_message(game, "Demon Eye sharpens Rick's sight.");
            break;
        case ITEM_RELIC_CRYSTAL_HEART:
            game->revive_charges += 1;
            game_set_message(game, "Crystal Heart stores one revive.");
            break;
        default:
            break;
    }
}

static bool game_add_item(Game *game, ItemType item_type) {
    if (game == NULL || item_type == ITEM_NONE) {
        return false;
    }

    const ItemProfile *profile = entity_item_profile(item_type);
    if (profile->relic) {
        game_apply_relic(game, item_type);
        return true;
    }

    for (int slot = 0; slot < MAX_INVENTORY_SLOTS; ++slot) {
        if (game->inventory[slot] == ITEM_NONE) {
            game->inventory[slot] = item_type;
            game_set_message(game, "%s added to the bag.", profile->name);
            return true;
        }
    }

    game_set_message(game, "The bag is full.");
    return false;
}

static void game_seed_starting_inventory(Game *game) {
    if (game == NULL) {
        return;
    }

    game_clear_inventory(game);
    game_add_item(game, ITEM_POTION_HEALTH);

    switch (game->player.player_class) {
        case PLAYER_CLASS_WARRIOR:
            game_add_item(game, ITEM_POTION_HEALTH);
            break;
        case PLAYER_CLASS_MAGE:
            game_add_item(game, ITEM_POTION_MANA);
            game_add_item(game, ITEM_SCROLL_FIRE);
            break;
        case PLAYER_CLASS_ARCHER:
            game_add_item(game, ITEM_POTION_MANA);
            game_add_item(game, ITEM_SCROLL_ICE);
            break;
        case PLAYER_CLASS_ROGUE:
            game_add_item(game, ITEM_POTION_MANA);
            break;
        default:
            break;
    }
}

static void game_apply_floor_scaling(Game *game, int floor_number) {
    if (game == NULL) {
        return;
    }

    for (int floor = 1; floor < floor_number; ++floor) {
        game->player.level += 1;
        game->player.max_hp += 5;
        game->player.max_mp += 4;
        game->player.attack += 2;
        game->player.defense += 1;
        game->player.hp = game->player.max_hp;
        game->player.mp = game->player.max_mp;
        game->player.gold += 25;
        game->player.exp_to_level += 20;
    }

    if (floor_number >= 2) {
        game->player.weapon = entity_recommended_upgrade(game->player.player_class, floor_number);
        game_add_item(game, ITEM_POTION_HEALTH);
    }
}

static EnemyType game_random_common_enemy(int floor_number) {
    static const EnemyType FLOOR_ONE_POOL[] = {
        ENEMY_GOBLIN,
        ENEMY_SKELETON,
        ENEMY_ZOMBIE,
        ENEMY_BAT,
        ENEMY_SLIME,
        ENEMY_SPIDER,
        ENEMY_FUNGUS,
    };
    static const EnemyType FLOOR_TWO_POOL[] = {
        ENEMY_GOBLIN,
        ENEMY_SKELETON,
        ENEMY_ZOMBIE,
        ENEMY_BAT,
        ENEMY_SPIDER,
        ENEMY_SHADOW_KNIGHT,
        ENEMY_NECROMANCER,
        ENEMY_ASSASSIN,
        ENEMY_WITCH,
    };

    if (floor_number <= 1) {
        size_t count = sizeof(FLOOR_ONE_POOL) / sizeof(FLOOR_ONE_POOL[0]);
        return FLOOR_ONE_POOL[rand() % (int)count];
    }

    size_t count = sizeof(FLOOR_TWO_POOL) / sizeof(FLOOR_TWO_POOL[0]);
    return FLOOR_TWO_POOL[rand() % (int)count];
}

static EnemyType game_random_elite_enemy(void) {
    static const EnemyType ELITE_POOL[] = {
        ENEMY_SHADOW_KNIGHT,
        ENEMY_NECROMANCER,
        ENEMY_STONE_GOLEM,
        ENEMY_ASSASSIN,
        ENEMY_WITCH,
    };

    size_t count = sizeof(ELITE_POOL) / sizeof(ELITE_POOL[0]);
    return ELITE_POOL[rand() % (int)count];
}

static void game_spawn_enemies(Game *game) {
    if (game == NULL) {
        return;
    }

    game_clear_enemies(game);

    if (game->world.boss_floor) {
        entity_init_enemy(&game->enemies[0], ENEMY_CRYPT_GUARDIAN, game->floor_number,
                          game->world.boss_x, game->world.boss_y);
        game->enemy_count = 1;
        return;
    }

    for (int index = 0; index < game->world.enemy_spawn_count && game->enemy_count < MAX_ENEMIES;
         ++index) {
        Point spawn = game->world.enemy_spawns[index];
        EnemyType type = game_random_common_enemy(game->floor_number);

        if (game->floor_number >= 2 && index % 4 == 3) {
            type = game_random_elite_enemy();
        }

        entity_init_enemy(&game->enemies[game->enemy_count], type, game->floor_number,
                          spawn.x, spawn.y);
        game->enemy_count += 1;
    }
}

static void game_set_shop_offer_item(ShopOffer *offer, ItemType item, int price) {
    if (offer == NULL) {
        return;
    }

    memset(offer, 0, sizeof(*offer));
    offer->item = item;
    offer->price = price;
}

static void game_set_shop_offer_weapon(ShopOffer *offer, WeaponType weapon, int price) {
    if (offer == NULL) {
        return;
    }

    memset(offer, 0, sizeof(*offer));
    offer->weapon_offer = true;
    offer->weapon = weapon;
    offer->price = price;
}

static void game_generate_shop_offers(Game *game) {
    if (game == NULL) {
        return;
    }

    game_clear_shop(game);
    if (!game->world.has_shop) {
        return;
    }

    WeaponType upgrade = entity_recommended_upgrade(game->player.player_class, game->floor_number);
    game_set_shop_offer_weapon(&game->shop_offers[0], upgrade,
                               game->floor_number >= 2 ? 65 : 45);

    if (game->player.player_class == PLAYER_CLASS_MAGE ||
        game->player.player_class == PLAYER_CLASS_ARCHER) {
        game_set_shop_offer_item(&game->shop_offers[1], ITEM_POTION_MANA, 18);
    } else {
        game_set_shop_offer_item(&game->shop_offers[1], ITEM_POTION_HEALTH, 18);
    }

    if (game->floor_number >= 2) {
        game_set_shop_offer_item(&game->shop_offers[2], ITEM_SCROLL_FIRE, 30);
    } else {
        game_set_shop_offer_item(&game->shop_offers[2], ITEM_SCROLL_ICE, 24);
    }
}

static void game_award_enemy_defeat(Game *game, Entity *enemy) {
    if (game == NULL || enemy == NULL) {
        return;
    }

    game->player.exp += enemy->exp_reward;
    game->player.gold += enemy->gold_reward;
    game->kills += 1;

    enemy->exp_reward = 0;
    enemy->gold_reward = 0;
    enemy->blocks = false;

    while (game->player.exp >= game->player.exp_to_level) {
        game->player.exp -= game->player.exp_to_level;
        game->player.level += 1;
        game->player.exp_to_level += 24;
        game->player.max_hp += 5;
        game->player.max_mp += 4;
        game->player.attack += 2;
        game->player.defense += 1;
        game->player.hp = game->player.max_hp;
        game->player.mp = game->player.max_mp;
        game_set_message(game, "Rick rises to level %d.", game->player.level);
    }

    if (enemy->boss) {
        world_open_portal(&game->world);
        game->player.gold += 120;
        game_set_message(game, "The Crypt Guardian falls. The portal opens.");
    }
}

static bool game_handle_player_death(Game *game, const char *format, ...) {
    if (game == NULL) {
        return false;
    }

    if (game_try_revive_player(game)) {
        return true;
    }

    va_list arguments;
    va_start(arguments, format);
    vsnprintf(game->death_message, sizeof(game->death_message), format, arguments);
    va_end(arguments);
    game->mode = GAME_MODE_GAME_OVER;
    game->game_over_selection = 0;
    return false;
}

static int game_find_nearest_enemy(const Game *game, int max_distance) {
    if (game == NULL) {
        return -1;
    }

    int best_index = -1;
    int best_distance = max_distance + 1;
    for (int index = 0; index < game->enemy_count; ++index) {
        const Entity *enemy = &game->enemies[index];
        if (!enemy->alive) {
            continue;
        }

        int distance = abs(enemy->x - game->player.x) + abs(enemy->y - game->player.y);
        if (distance < best_distance) {
            best_distance = distance;
            best_index = index;
        }
    }

    return best_index;
}

static int game_find_enemy_in_direction(const Game *game, int dx, int dy, int range) {
    if (game == NULL || range <= 0) {
        return -1;
    }

    for (int step = 1; step <= range; ++step) {
        int x = game->player.x + dx * step;
        int y = game->player.y + dy * step;
        if (!world_in_bounds(x, y)) {
            break;
        }

        int enemy_index = game_find_enemy_at(game, x, y);
        if (enemy_index >= 0) {
            return enemy_index;
        }

        if (!world_is_walkable(&game->world, x, y)) {
            break;
        }
    }

    return -1;
}

static bool game_player_attack_enemy(Game *game, int enemy_index, int bonus_damage,
                                     bool ignore_defense, const char *verb,
                                     StatusFlags applied_status, int status_turns) {
    if (game == NULL || enemy_index < 0 || enemy_index >= game->enemy_count) {
        return false;
    }

    Entity *enemy = &game->enemies[enemy_index];
    if (!enemy->alive) {
        return false;
    }

    const WeaponProfile *weapon = entity_weapon_profile(game->player.weapon);
    int damage = entity_attack_damage(&game->player, enemy, bonus_damage, ignore_defense);
    bool crit = weapon->crit_bonus > 0 && (rand() % 100) < weapon->crit_bonus;
    if (crit) {
        damage += 2 + weapon->attack_bonus / 2;
    }

    entity_apply_damage(enemy, damage);

    if (enemy->alive && applied_status != 0) {
        entity_apply_status(enemy, applied_status, status_turns);
    }
    if (enemy->alive && game->player.weapon == WEAPON_FLAME_BLADE) {
        entity_apply_status(enemy, STATUS_BURN, 2);
    }
    if (enemy->alive && game->player.poison_blade_turns > 0 &&
        entity_is_adjacent(&game->player, enemy)) {
        entity_apply_status(enemy, STATUS_POISON, 3);
        game->player.poison_blade_turns = 0;
    }

    if (!enemy->alive) {
        game_award_enemy_defeat(game, enemy);
        if (!enemy->boss) {
            game_set_message(game, "%s %s %s.",
                             entity_player_class_name(game->player.player_class), verb,
                             enemy->name);
        }
    } else {
        game_set_message(game, "%s %s %s for %d%s.",
                         entity_player_class_name(game->player.player_class), verb, enemy->name,
                         damage, crit ? " critical" : "");
    }

    game_set_player_pose(game, ASCII_ART_POSE_ATTACK, 180);

    return true;
}

static bool game_enemy_attack_player(Game *game, Entity *enemy) {
    if (game == NULL || enemy == NULL || !enemy->alive || !game->player.alive) {
        return false;
    }

    if (entity_has_status(&game->player, STATUS_INVIS)) {
        return false;
    }

    int damage = entity_attack_damage(enemy, &game->player, 0, false);
    entity_apply_damage(&game->player, damage);
    game_set_player_pose(game, ASCII_ART_POSE_HURT, 180);

    if (enemy->enemy_type == ENEMY_SPIDER && game->player.alive) {
        entity_apply_status(&game->player, STATUS_POISON, 3);
    }
    if (enemy->enemy_type == ENEMY_WITCH && game->player.alive) {
        entity_apply_status(&game->player, STATUS_CURSE, 3);
    }
    if (enemy->boss && game->player.alive && (rand() % 100) < 25) {
        entity_apply_status(&game->player, STATUS_STUN, 1);
    }

    if (!game->player.alive) {
        return game_handle_player_death(game, "%s crushes Rick on floor %d.", enemy->name,
                                        game->floor_number);
    }

    game_set_message(game, "%s hits Rick for %d.", enemy->name, damage);
    return true;
}

static void game_enemy_turn(Game *game) {
    if (game == NULL || game->mode != GAME_MODE_PLAYING) {
        return;
    }

    for (int index = 0; index < game->enemy_count; ++index) {
        Entity *enemy = &game->enemies[index];
        if (!enemy->alive) {
            continue;
        }

        StatusTickResult tick = entity_tick_statuses(enemy);
        if (!tick.alive) {
            game_award_enemy_defeat(game, enemy);
            continue;
        }
        entity_tick_cooldowns(enemy);

        if (entity_has_status(enemy, STATUS_FREEZE) || entity_has_status(enemy, STATUS_STUN)) {
            continue;
        }

        int dx = game->player.x - enemy->x;
        int dy = game->player.y - enemy->y;
        int distance = abs(dx) + abs(dy);

        if (!entity_has_status(&game->player, STATUS_INVIS) && distance <= 1) {
            if (!game_enemy_attack_player(game, enemy)) {
                return;
            }
            continue;
        }

        int move_x = 0;
        int move_y = 0;
        if (enemy->hp * 5 < enemy->max_hp && !enemy->boss && distance <= 3) {
            move_x = -sign_int(dx);
            move_y = -sign_int(dy);
            enemy->ai_state = AI_RETREAT;
        } else if (!entity_has_status(&game->player, STATUS_INVIS) && distance <= enemy->vision) {
            if (abs(dx) >= abs(dy)) {
                move_x = sign_int(dx);
            } else {
                move_y = sign_int(dy);
            }
            enemy->ai_state = AI_CHASE;
        } else {
            move_x = sign_int(enemy->spawn_x - enemy->x);
            move_y = sign_int(enemy->spawn_y - enemy->y);
            enemy->ai_state = AI_PATROL;
        }

        int target_x = enemy->x + move_x;
        int target_y = enemy->y + move_y;

        if (target_x == game->player.x && target_y == game->player.y) {
            if (!game_enemy_attack_player(game, enemy)) {
                return;
            }
            continue;
        }

        if (world_is_walkable(&game->world, target_x, target_y) &&
            !game_is_occupied(game, target_x, target_y, index)) {
            enemy->x = target_x;
            enemy->y = target_y;
            continue;
        }

        if (move_x != 0) {
            int alternate_y = enemy->y + sign_int(dy);
            if (world_is_walkable(&game->world, enemy->x, alternate_y) &&
                !game_is_occupied(game, enemy->x, alternate_y, index)) {
                enemy->y = alternate_y;
            }
        } else if (move_y != 0) {
            int alternate_x = enemy->x + sign_int(dx);
            if (world_is_walkable(&game->world, alternate_x, enemy->y) &&
                !game_is_occupied(game, alternate_x, enemy->y, index)) {
                enemy->x = alternate_x;
            }
        }
    }
}

static ItemType game_roll_chest_item(Game *game) {
    if (game == NULL) {
        return ITEM_POTION_HEALTH;
    }

    int roll = rand() % 100;
    if (game->floor_number >= 2 && roll < 18) {
        return ITEM_RELIC_CRYSTAL_HEART;
    }
    if (roll < 28) {
        return ITEM_RELIC_TITAN_RING;
    }
    if (roll < 42) {
        return ITEM_RELIC_DEMON_EYE;
    }
    if (roll < 62) {
        return ITEM_POTION_MANA;
    }
    if (roll < 80) {
        return ITEM_SCROLL_FIRE;
    }
    return ITEM_POTION_HEALTH;
}

static bool game_open_chest(Game *game) {
    if (game == NULL || world_tile_at(&game->world, game->player.x, game->player.y) != TILE_CHEST) {
        return false;
    }

    if (game->floor_number >= 2 &&
        game->player.weapon != entity_recommended_upgrade(game->player.player_class,
                                                          game->floor_number) &&
        (rand() % 100) < 40) {
        game->player.weapon = entity_recommended_upgrade(game->player.player_class,
                                                         game->floor_number);
        world_set_tile(&game->world, game->player.x, game->player.y, TILE_FLOOR);
        game_set_message(game, "The chest holds a %s.",
                         entity_weapon_profile(game->player.weapon)->name);
        return true;
    }

    ItemType reward = game_roll_chest_item(game);
    world_set_tile(&game->world, game->player.x, game->player.y, TILE_FLOOR);
    if (!game_add_item(game, reward)) {
        game->player.gold += 20;
        game_set_message(game, "The chest breaks into 20 gold.");
    }
    return true;
}

static ActionResult game_process_player_tile(Game *game) {
    if (game == NULL) {
        return ACTION_NONE;
    }

    char tile = world_tile_at(&game->world, game->player.x, game->player.y);

    if (tile == TILE_CHEST) {
        game_open_chest(game);
    }

    if (tile == TILE_WATER) {
        entity_restore_mp(&game->player, 1);
    } else if (tile == TILE_LAVA) {
        entity_apply_damage(&game->player, 4);
        if (!game->player.alive && !game_handle_player_death(game, "Rick burned in the lava.")) {
            return ACTION_SKIP_ENEMY;
        }
        game_set_message(game, "Lava scorches Rick's boots.");
    } else if (tile == TILE_TRAP) {
        entity_apply_damage(&game->player, 2);
        world_set_tile(&game->world, game->player.x, game->player.y, TILE_FLOOR);
        if (!game->player.alive && !game_handle_player_death(game, "Rick fell to a trap.")) {
            return ACTION_SKIP_ENEMY;
        }
        game_set_message(game, "A trap snaps beneath Rick.");
    }

    if (tile == TILE_STAIRS_DOWN && !game->world.boss_floor) {
        game_advance_floor(game);
        return ACTION_SKIP_ENEMY;
    }

    if (tile == TILE_PORTAL && game->world.portal_open) {
        game_set_victory(game, "Rick escapes through the awakened portal.");
        return ACTION_SKIP_ENEMY;
    }

    if (tile == TILE_SHOP) {
        game_set_message(game, "Press Enter to trade with the shopkeeper.");
    }

    return ACTION_TURN;
}

static bool game_use_inventory_item(Game *game, int slot) {
    if (game == NULL || slot < 0 || slot >= MAX_INVENTORY_SLOTS) {
        return false;
    }

    ItemType item_type = game->inventory[slot];
    const ItemProfile *profile = entity_item_profile(item_type);
    if (item_type == ITEM_NONE || !profile->consumable) {
        return false;
    }

    switch (item_type) {
        case ITEM_POTION_HEALTH:
            entity_restore_hp(&game->player, profile->power);
            game_set_message(game, "Rick drinks a health potion.");
            break;
        case ITEM_POTION_MANA:
            entity_restore_mp(&game->player, profile->power);
            game_set_message(game, "Rick drinks a mana potion.");
            break;
        case ITEM_SCROLL_FIRE:
            for (int hit = 0; hit < 3; ++hit) {
                int enemy_index = game_find_nearest_enemy(game, 10);
                if (enemy_index < 0) {
                    break;
                }
                game_player_attack_enemy(game, enemy_index, profile->power, false, "burns",
                                         STATUS_BURN, 2);
            }
            break;
        case ITEM_SCROLL_ICE:
            for (int index = 0; index < game->enemy_count; ++index) {
                Entity *enemy = &game->enemies[index];
                if (!enemy->alive) {
                    continue;
                }
                int distance = abs(enemy->x - game->player.x) + abs(enemy->y - game->player.y);
                if (distance <= 3) {
                    game_player_attack_enemy(game, index, profile->power / 2, false, "freezes",
                                             STATUS_FREEZE, 2);
                }
            }
            break;
        case ITEM_SCROLL_HEALING:
            entity_restore_hp(&game->player, profile->power);
            entity_restore_mp(&game->player, profile->power / 2);
            game_set_message(game, "A healing scroll restores Rick.");
            break;
        default:
            return false;
    }

    game->inventory[slot] = ITEM_NONE;
    game_compact_inventory(game);
    return true;
}

static bool game_use_ability(Game *game, int slot) {
    if (game == NULL || slot < 0 || slot >= 3) {
        return false;
    }

    AbilityType ability = game->player.abilities[slot];
    const AbilityProfile *profile = entity_ability_profile(ability);
    if (ability == ABILITY_NONE || profile->mana_cost > game->player.mp ||
        game->player.ability_cooldowns[slot] > 0) {
        return false;
    }

    bool used = false;
    switch (ability) {
        case ABILITY_BRUTAL_STRIKE: {
            int target = game_find_nearest_enemy(game, 1);
            if (target >= 0) {
                used = game_player_attack_enemy(game, target, game->player.attack, false,
                                                "crushes", 0, 0);
            }
            break;
        }
        case ABILITY_WAR_CRY:
            game->player.attack_buff_turns = 4;
            game_set_message(game, "Rick roars and surges forward.");
            used = true;
            break;
        case ABILITY_SHIELD_WALL:
            entity_apply_status(&game->player, STATUS_GUARD, 2);
            game_set_message(game, "A shield wall locks into place.");
            used = true;
            break;
        case ABILITY_FIREBALL: {
            int target = game_find_nearest_enemy(game, game->player.vision + 2);
            if (target >= 0) {
                used = game_player_attack_enemy(game, target, 8, false, "blasts",
                                                STATUS_BURN, 2);
                if (used) {
                    Entity *center = &game->enemies[target];
                    for (int index = 0; index < game->enemy_count; ++index) {
                        if (index == target || !game->enemies[index].alive) {
                            continue;
                        }
                        int distance = abs(game->enemies[index].x - center->x) +
                                       abs(game->enemies[index].y - center->y);
                        if (distance <= 1) {
                            game_player_attack_enemy(game, index, 3, false, "scorches",
                                                     STATUS_BURN, 2);
                        }
                    }
                }
            }
            break;
        }
        case ABILITY_BLIZZARD:
            for (int index = 0; index < game->enemy_count; ++index) {
                Entity *enemy = &game->enemies[index];
                if (!enemy->alive) {
                    continue;
                }
                int distance = abs(enemy->x - game->player.x) + abs(enemy->y - game->player.y);
                if (distance <= 3) {
                    used = true;
                    game_player_attack_enemy(game, index, 4, false, "freezes", STATUS_FREEZE,
                                             2);
                }
            }
            break;
        case ABILITY_LIGHTNING: {
            int target = game_find_nearest_enemy(game, game->player.vision + 3);
            if (target >= 0) {
                used = game_player_attack_enemy(game, target, 12, true, "shocks", STATUS_STUN,
                                                1);
            }
            break;
        }
        case ABILITY_ARROW_RAIN: {
            int hits = 0;
            for (int pass = 0; pass < 3; ++pass) {
                int target = game_find_nearest_enemy(game, game->player.vision + 4);
                if (target < 0) {
                    break;
                }
                if (game_player_attack_enemy(game, target, 5, false, "pierces", 0, 0)) {
                    hits += 1;
                }
            }
            used = hits > 0;
            break;
        }
        case ABILITY_PIERCING_SHOT: {
            int best_target = -1;
            int best_distance = 1000;
            for (int index = 0; index < game->enemy_count; ++index) {
                Entity *enemy = &game->enemies[index];
                if (!enemy->alive) {
                    continue;
                }
                if (enemy->x != game->player.x && enemy->y != game->player.y) {
                    continue;
                }
                int distance = abs(enemy->x - game->player.x) + abs(enemy->y - game->player.y);
                if (distance < best_distance) {
                    best_distance = distance;
                    best_target = index;
                }
            }
            if (best_target >= 0) {
                used = game_player_attack_enemy(game, best_target, 9, true, "skewers", 0, 0);
            }
            break;
        }
        case ABILITY_TRAP_KIT:
            if (world_tile_at(&game->world, game->player.x, game->player.y) == TILE_FLOOR) {
                world_set_tile(&game->world, game->player.x, game->player.y, TILE_TRAP);
                game_set_message(game, "Rick sets a trap underfoot.");
                used = true;
            }
            break;
        case ABILITY_EVASION:
            entity_apply_status(&game->player, STATUS_INVIS, 1);
            entity_apply_status(&game->player, STATUS_HASTE, 2);
            game_set_message(game, "Rick slips into the shadows.");
            used = true;
            break;
        case ABILITY_POISON_BLADE:
            game->player.poison_blade_turns = 3;
            game_set_message(game, "Rick coats the blade with poison.");
            used = true;
            break;
        case ABILITY_SMOKE_BOMB: {
            bool moved = false;
            for (int radius = 2; radius <= 4 && !moved; ++radius) {
                for (int y = game->player.y - radius; y <= game->player.y + radius && !moved; ++y) {
                    for (int x = game->player.x - radius; x <= game->player.x + radius; ++x) {
                        if (!world_in_bounds(x, y) || !world_is_walkable(&game->world, x, y) ||
                            game_is_occupied(game, x, y, -1)) {
                            continue;
                        }
                        game->player.x = x;
                        game->player.y = y;
                        entity_apply_status(&game->player, STATUS_INVIS, 1);
                        game_set_message(game, "Smoke hides Rick's escape.");
                        moved = true;
                        used = true;
                        break;
                    }
                }
            }
            break;
        }
        default:
            break;
    }

    if (!used) {
        return false;
    }

    if (!entity_spend_mp(&game->player, profile->mana_cost)) {
        return false;
    }

    game->player.ability_cooldowns[slot] = profile->cooldown;

    switch (ability) {
        case ABILITY_SHIELD_WALL:
            game_set_player_pose(game, ASCII_ART_POSE_GUARD, 240);
            break;
        case ABILITY_EVASION:
        case ABILITY_SMOKE_BOMB:
        case ABILITY_TRAP_KIT:
            game_set_player_pose(game, ASCII_ART_POSE_WALK, 180);
            break;
        default:
            game_set_player_pose(game, ASCII_ART_POSE_ATTACK, 180);
            break;
    }

    return true;
}

static ActionResult game_try_player_move(Game *game, int dx, int dy) {
    if (game == NULL || (dx == 0 && dy == 0)) {
        return ACTION_NONE;
    }

    int weapon_range = entity_weapon_range(&game->player);
    int enemy_index = game_find_enemy_in_direction(game, dx, dy, weapon_range);
    if (enemy_index >= 0) {
        if (game_player_attack_enemy(game, enemy_index, 0, false, "strikes", 0, 0)) {
            return ACTION_TURN;
        }
    }

    int target_x = game->player.x + dx;
    int target_y = game->player.y + dy;
    if (!world_in_bounds(target_x, target_y) || !world_is_walkable(&game->world, target_x, target_y) ||
        game_is_occupied(game, target_x, target_y, -1)) {
        return ACTION_NONE;
    }

    game->player.x = target_x;
    game->player.y = target_y;
    game_set_player_pose(game, ASCII_ART_POSE_WALK, 150);

    ActionResult tile_result = game_process_player_tile(game);
    if (tile_result == ACTION_TURN) {
        game_set_message(game, "Rick advances deeper into the dungeon.");
    }
    return tile_result;
}

static void game_complete_turn(Game *game) {
    if (game == NULL || game->mode != GAME_MODE_PLAYING) {
        return;
    }

    game_enemy_turn(game);
    if (game->mode != GAME_MODE_PLAYING) {
        return;
    }

    StatusTickResult player_tick = entity_tick_statuses(&game->player);
    entity_tick_cooldowns(&game->player);
    if (!game->player.alive && !game_handle_player_death(game, "Rick falls on floor %d.",
                                                         game->floor_number)) {
        return;
    }

    if (player_tick.damage > 0 && game->mode == GAME_MODE_PLAYING) {
        game_set_message(game, "Status effects chew through Rick for %d.", player_tick.damage);
    }

    game->turn_counter += 1;
}

static void game_generate_floor(Game *game) {
    if (game == NULL) {
        return;
    }

    unsigned int floor_seed = game->seed + (unsigned int)(game->floor_number * 1337u);
    srand(floor_seed);
    world_generate(&game->world, floor_seed, game->floor_number);
    game->player.x = game->world.spawn_x;
    game->player.y = game->world.spawn_y;
    game_spawn_enemies(game);
    game_generate_shop_offers(game);
}

static void game_begin_run(Game *game, PlayerClass player_class, unsigned int seed,
                           int floor_number) {
    if (game == NULL) {
        return;
    }

    if (floor_number < 1) {
        floor_number = 1;
    }

    game->floor_number = floor_number;
    game->turn_counter = 0;
    game->kills = 0;
    game->seed = seed;
    game->revive_charges = 0;
    entity_init_player(&game->player, player_class);
    game_reset_player_pose(game);
    game_seed_starting_inventory(game);
    game_apply_floor_scaling(game, floor_number);
    game_generate_floor(game);
    game->menu_selection = 0;
    game->pause_selection = 0;
    game->inventory_selection = 0;
    game->shop_selection = 0;
    game->game_over_selection = 0;
    game->victory_selection = 0;
    game->mode = GAME_MODE_PLAYING;
    game_set_message(game, "Rick enters the dungeon as %s.",
                     entity_player_class_name(player_class));
    game_set_death_message(game, "");
}

static void game_start_new_run(Game *game) {
    if (game == NULL) {
        return;
    }

    game->class_selection = 0;
    game->mode = GAME_MODE_CLASS_SELECT;
    game_set_message(game, "Choose a class for Rick.");
}

static void game_advance_floor(Game *game) {
    if (game == NULL) {
        return;
    }

    game->floor_number += 1;
    game->seed += 1;
    entity_restore_hp(&game->player, 8);
    entity_restore_mp(&game->player, 6);
    game_generate_floor(game);
    game_set_message(game, "Rick descends to floor %d.", game->floor_number);
}

static SDL_Color game_color_for_tile(char tile) {
    switch (tile) {
        case TILE_WALL:
            return COLOR_WALL;
        case TILE_FLOOR:
            return COLOR_FLOOR;
        case TILE_DOOR:
        case TILE_DOOR_OPEN:
            return COLOR_DOOR;
        case TILE_STAIRS_DOWN:
        case TILE_STAIRS_UP:
            return COLOR_STAIRS;
        case TILE_WATER:
            return COLOR_WATER;
        case TILE_LAVA:
            return COLOR_LAVA;
        case TILE_TRAP:
            return COLOR_TRAP;
        case TILE_CHEST:
            return COLOR_CHEST;
        case TILE_TORCH:
            return COLOR_TORCH;
        case TILE_PORTAL:
            return COLOR_PORTAL;
        case TILE_SHOP:
            return COLOR_SHOP;
        default:
            return COLOR_DIM;
    }
}

static SDL_Color game_color_for_enemy(const Entity *enemy) {
    if (enemy == NULL) {
        return COLOR_ENEMY;
    }
    if (enemy->boss) {
        return COLOR_BOSS;
    }
    if (enemy->elite) {
        return COLOR_ELITE;
    }
    return COLOR_ENEMY;
}

static const char *game_short_ability_name(AbilityType ability) {
    switch (ability) {
        case ABILITY_BRUTAL_STRIKE:
            return "Brutal";
        case ABILITY_WAR_CRY:
            return "WarCry";
        case ABILITY_SHIELD_WALL:
            return "Guard";
        case ABILITY_FIREBALL:
            return "Fire";
        case ABILITY_BLIZZARD:
            return "Blizz";
        case ABILITY_LIGHTNING:
            return "Bolt";
        case ABILITY_ARROW_RAIN:
            return "Rain";
        case ABILITY_PIERCING_SHOT:
            return "Pierce";
        case ABILITY_TRAP_KIT:
            return "Trap";
        case ABILITY_EVASION:
            return "Evade";
        case ABILITY_POISON_BLADE:
            return "Poison";
        case ABILITY_SMOKE_BOMB:
            return "Smoke";
        default:
            return "None";
    }
}

static void game_build_normal_hud_lines(const Game *game, char line1[128], char line2[128],
                                        char line3[128], char line4[128]) {
    const WeaponProfile *weapon = entity_weapon_profile(game->player.weapon);
    snprintf(line1, 128, "%s HP %d/%d MP %d/%d LV %d EXP %d/%d",
             entity_player_class_name(game->player.player_class), game->player.hp,
             game->player.max_hp, game->player.mp, game->player.max_mp, game->player.level,
             game->player.exp, game->player.exp_to_level);
    snprintf(line2, 128, "Gold %d Kills %d Floor B%d Weapon %s", game->player.gold, game->kills,
             game->floor_number, weapon->name);
    snprintf(line3, 128, "1:%s[%d] 2:%s[%d] 3:%s[%d] I:bag Esc:pause F2:debug",
             game_short_ability_name(game->player.abilities[0]), game->player.ability_cooldowns[0],
             game_short_ability_name(game->player.abilities[1]), game->player.ability_cooldowns[1],
             game_short_ability_name(game->player.abilities[2]), game->player.ability_cooldowns[2]);
    snprintf(line4, 128, "%s",
             game->message[0] != '\0' ? game->message : "Explore, loot, and survive.");
}

static void game_build_debug_hud_lines(const Game *game, char line1[128], char line2[128],
                                       char line3[128], char line4[128]) {
    int content_w = GRID_COLS * game->renderer.cell_w;
    int content_h = (GRID_ROWS + HUD_LINES) * game->renderer.cell_h;
    bool fits = game->renderer.window_w >= content_w && game->renderer.window_h >= content_h;

    snprintf(line1, 128, "WIN %dx%d ORIG %d,%d", game->renderer.window_w,
             game->renderer.window_h, game->renderer.origin_x, game->renderer.origin_y);
    snprintf(line2, 128, "CELL %dx%d CONT %dx%d FIT %s", game->renderer.cell_w,
             game->renderer.cell_h, content_w, content_h, fits ? "full" : "clip");
    snprintf(line3, 128, "MODE %s FLOOR %d TURN %d CLASS %s", game_mode_name(game->mode),
             game->floor_number, game->turn_counter,
             entity_player_class_name(game->player.player_class));
    snprintf(line4, 128, "PLAYER %d,%d HP %d/%d ENEMIES %d/%d", game->player.x, game->player.y,
             game->player.hp, game->player.max_hp, game_alive_enemy_count(game),
             game->enemy_count);
}

static void game_build_hud_lines(const Game *game, char line1[128], char line2[128],
                                 char line3[128], char line4[128]) {
    if (game != NULL && game->debug_overlay) {
        game_build_debug_hud_lines(game, line1, line2, line3, line4);
        return;
    }
    game_build_normal_hud_lines(game, line1, line2, line3, line4);
}

static void game_fill_snapshot_rows(const Game *game,
                                    char rows[GRID_ROWS + HUD_LINES][GRID_COLS + 1]) {
    if (game == NULL) {
        return;
    }

    char hud_line1[128];
    char hud_line2[128];
    char hud_line3[128];
    char hud_line4[128];
    game_build_hud_lines(game, hud_line1, hud_line2, hud_line3, hud_line4);

    const char *hud_lines[HUD_LINES] = { hud_line1, hud_line2, hud_line3, hud_line4 };
    for (int row = 0; row < HUD_LINES; ++row) {
        memset(rows[row], ' ', GRID_COLS);
        rows[row][GRID_COLS] = '\0';
        size_t length = strlen(hud_lines[row]);
        if (length > (size_t)(GRID_COLS - 1)) {
            length = (size_t)(GRID_COLS - 1);
        }
        memcpy(rows[row] + 1, hud_lines[row], length);
    }

    for (int y = 0; y < GRID_ROWS; ++y) {
        for (int x = 0; x < GRID_COLS; ++x) {
            rows[HUD_LINES + y][x] = world_tile_at(&game->world, x, y);
        }
        rows[HUD_LINES + y][GRID_COLS] = '\0';
    }

    for (int index = 0; index < game->enemy_count; ++index) {
        const Entity *enemy = &game->enemies[index];
        if (enemy->alive && world_in_bounds(enemy->x, enemy->y)) {
            rows[HUD_LINES + enemy->y][enemy->x] = enemy->glyph;
        }
    }

    if (game->player.alive && world_in_bounds(game->player.x, game->player.y)) {
        rows[HUD_LINES + game->player.y][game->player.x] = game->player.glyph;
    }
}

static void game_draw_menu_frame(Game *game, const char *title, const char *subtitle,
                                 const char *const *options, int option_count, int selected,
                                 SDL_Color selected_color, SDL_Color text_color,
                                 const char *footer) {
    if (game == NULL) {
        return;
    }

    int box_w = game_text_width(title);
    int subtitle_w = game_text_width(subtitle);
    int options_w = game_option_line_width(options, option_count);
    int footer_w = game_text_width(footer);

    if (subtitle_w > box_w) {
        box_w = subtitle_w;
    }
    if (options_w > box_w) {
        box_w = options_w;
    }
    if (footer_w > box_w) {
        box_w = footer_w;
    }

    box_w += 4;
    if (box_w < 42) {
        box_w = 42;
    }
    if (box_w > GRID_COLS - 4) {
        box_w = GRID_COLS - 4;
    }

    int box_h = option_count + 8;
    int box_x = (GRID_COLS - box_w) / 2;
    int box_y = (game_visible_rows() - box_h) / 2;

    renderer_draw_box(&game->renderer, box_x, box_y, box_w, box_h, COLOR_TEXT, COLOR_PANEL);
    game_draw_centered_text(&game->renderer, box_x, box_w, box_y + 1, title, text_color);
    if (subtitle != NULL && subtitle[0] != '\0') {
        game_draw_centered_text(&game->renderer, box_x, box_w, box_y + 3, subtitle, COLOR_DIM);
    }

    for (int index = 0; index < option_count; ++index) {
        char line[80];
        snprintf(line, sizeof(line), "%c %s", index == selected ? '>' : ' ', options[index]);
        game_draw_centered_text(&game->renderer, box_x, box_w, box_y + 5 + index, line,
                                index == selected ? selected_color : COLOR_TEXT);
    }

    game_draw_centered_text(&game->renderer, box_x, box_w, box_y + box_h - 2, footer, COLOR_DIM);
}

static void game_draw_playfield(Game *game) {
    if (game == NULL) {
        return;
    }

    for (int y = 0; y < GRID_ROWS; ++y) {
        for (int x = 0; x < GRID_COLS; ++x) {
            char tile = world_tile_at(&game->world, x, y);
            char draw_tile = tile;
            if (tile == TILE_WALL) {
                draw_tile = ascii_art_wall_glyph(&game->world, x, y);
            } else if (ascii_art_structure_sprite(tile) != NULL) {
                draw_tile = TILE_FLOOR;
            }

            renderer_draw_char(&game->renderer, x, y + HUD_LINES, draw_tile,
                               game_color_for_tile(tile));
        }
    }

    for (int y = 0; y < GRID_ROWS; ++y) {
        for (int x = 0; x < GRID_COLS; ++x) {
            char tile = world_tile_at(&game->world, x, y);
            const AsciiSprite *structure_sprite = ascii_art_structure_sprite(tile);
            if (structure_sprite != NULL) {
                ascii_art_draw_sprite(&game->renderer, structure_sprite, x, y, HUD_LINES,
                                      game_color_for_tile(tile));
            }
        }
    }

    for (int index = 0; index < game->enemy_count; ++index) {
        const Entity *enemy = &game->enemies[index];
        if (!enemy->alive) {
            continue;
        }

        const AsciiSprite *enemy_sprite = ascii_art_enemy_sprite(enemy->enemy_type);
        if (enemy_sprite != NULL) {
            ascii_art_draw_sprite(&game->renderer, enemy_sprite, enemy->x, enemy->y, HUD_LINES,
                                  game_color_for_enemy(enemy));
        } else {
            renderer_draw_char(&game->renderer, enemy->x, enemy->y + HUD_LINES, enemy->glyph,
                               game_color_for_enemy(enemy));
        }
    }

    if (game->player.alive || game->mode == GAME_MODE_GAME_OVER) {
        const AsciiSprite *player_sprite =
            ascii_art_player_sprite(game->player.player_class, game_player_render_pose(game),
                                    game_player_render_frame(game));
        if (player_sprite != NULL) {
            ascii_art_draw_sprite(&game->renderer, player_sprite, game->player.x, game->player.y,
                                  HUD_LINES, COLOR_PLAYER);
        } else {
            renderer_draw_char(&game->renderer, game->player.x, game->player.y + HUD_LINES,
                               game->player.glyph, COLOR_PLAYER);
        }
    }
}

static void game_draw_hud(Game *game) {
    if (game == NULL) {
        return;
    }

    char line1[128];
    char line2[128];
    char line3[128];
    char line4[128];
    game_build_hud_lines(game, line1, line2, line3, line4);

    SDL_Color border_color = game->debug_overlay ? COLOR_HIGHLIGHT : COLOR_DIM;
    renderer_draw_box(&game->renderer, 0, 0, GRID_COLS, HUD_LINES, border_color, COLOR_PANEL);

    renderer_draw_text(&game->renderer, 1, 0, line1,
                       game->player.hp <= game->player.max_hp / 4 ? COLOR_ALERT : COLOR_TEXT);
    renderer_draw_text(&game->renderer, 1, 1, line2, COLOR_TEXT);
    renderer_draw_text(&game->renderer, 1, 2, line3,
                       game->debug_overlay ? COLOR_TEXT : COLOR_HIGHLIGHT);
    renderer_draw_text(&game->renderer, 1, 3, line4,
                       game->debug_overlay ? COLOR_TEXT : COLOR_DIM);
}

static void game_render_main_menu(Game *game) {
    game_draw_menu_frame(game, GAME_TITLE, "ASCII roguelike prototype from the design plan.",
                         MAIN_MENU_OPTIONS, 2, game->menu_selection, COLOR_HIGHLIGHT, COLOR_TEXT,
                         "Arrows move. Enter confirms. Esc quits.");
}

static void game_render_class_select(Game *game) {
    if (game == NULL) {
        return;
    }

    int box_w = 60;
    int box_h = 16;
    int box_x = (GRID_COLS - box_w) / 2;
    int box_y = (game_visible_rows() - box_h) / 2;
    int divider_x = box_x + 34;
    int preview_x = box_x + 47;
    int preview_y = box_y + 10;
    PlayerClass selected_class = CLASS_SELECTION_ORDER[game->class_selection];

    renderer_draw_box(&game->renderer, box_x, box_y, box_w, box_h, COLOR_TEXT, COLOR_PANEL);
    game_draw_centered_text(&game->renderer, box_x, box_w, box_y + 1, "CHOOSE CLASS",
                            COLOR_HIGHLIGHT);
    game_draw_centered_text(&game->renderer, box_x, box_w, box_y + 3,
                            entity_player_class_summary(selected_class), COLOR_DIM);

    for (int row = box_y + 5; row < box_y + box_h - 2; ++row) {
        renderer_draw_char(&game->renderer, divider_x, row, '|', COLOR_DIM);
    }

    renderer_draw_text(&game->renderer, box_x + 4, box_y + 5, "Classes", COLOR_DIM);
    for (int index = 0; index < PLAYER_CLASS_COUNT; ++index) {
        char option[32];
        snprintf(option, sizeof(option), "%c %s",
                 index == game->class_selection ? '>' : ' ',
                 entity_player_class_name(CLASS_SELECTION_ORDER[index]));
        renderer_draw_text(&game->renderer, box_x + 4, box_y + 7 + index, option,
                           index == game->class_selection ? COLOR_HIGHLIGHT : COLOR_TEXT);
    }

    game_draw_centered_text(&game->renderer, divider_x + 1, box_w - (divider_x - box_x) - 1,
                            box_y + 5, "Preview", COLOR_DIM);
    renderer_draw_text(&game->renderer, divider_x + 3, box_y + 12,
                       entity_player_class_name(selected_class), COLOR_TEXT);

    const AsciiSprite *preview_sprite =
        ascii_art_player_sprite(selected_class, ASCII_ART_POSE_IDLE, 0);
    if (preview_sprite != NULL) {
        ascii_art_draw_sprite(&game->renderer, preview_sprite, preview_x, preview_y, 0,
                              COLOR_PLAYER);
    }

    game_draw_centered_text(&game->renderer, box_x, box_w, box_y + box_h - 2,
                            "Enter begins the run. Esc returns.", COLOR_DIM);
}

static void game_render_inventory(Game *game) {
    if (game == NULL) {
        return;
    }

    int box_w = 58;
    int box_h = 16;
    int box_x = (GRID_COLS - box_w) / 2;
    int box_y = (game_visible_rows() - box_h) / 2;

    renderer_draw_box(&game->renderer, box_x, box_y, box_w, box_h, COLOR_TEXT, COLOR_PANEL);
    game_draw_centered_text(&game->renderer, box_x, box_w, box_y + 1, "INVENTORY - RICK",
                            COLOR_HIGHLIGHT);

    char line[96];
    char weapon_label[48];
    ascii_art_format_weapon_label(game->player.weapon, weapon_label, sizeof(weapon_label));
    snprintf(line, sizeof(line), "Class: %s  Weapon: %s",
             entity_player_class_name(game->player.player_class), weapon_label);
    renderer_draw_text(&game->renderer, box_x + 2, box_y + 3, line, COLOR_TEXT);

    snprintf(line, sizeof(line), "Gold: %d  Revives: %d  Bag: %d/%d", game->player.gold,
             game->revive_charges, game_inventory_count(game), MAX_INVENTORY_SLOTS);
    renderer_draw_text(&game->renderer, box_x + 2, box_y + 4, line, COLOR_TEXT);

    for (int slot = 0; slot < MAX_INVENTORY_SLOTS; ++slot) {
        char item_label[40];
        if (game->inventory[slot] == ITEM_NONE) {
            snprintf(item_label, sizeof(item_label), "[ ] Empty");
        } else {
            ascii_art_format_item_label(game->inventory[slot], item_label, sizeof(item_label));
        }

        snprintf(line, sizeof(line), "%c [%d] %-24s", slot == game->inventory_selection ? '>' : ' ',
                 slot + 1, item_label);
        renderer_draw_text(&game->renderer, box_x + 2, box_y + 6 + slot, line,
                           slot == game->inventory_selection ? COLOR_HIGHLIGHT : COLOR_TEXT);
    }

    renderer_draw_text(&game->renderer, box_x + 2, box_y + box_h - 2,
                       "Enter uses. I or Esc closes.", COLOR_DIM);
}

static void game_render_shop(Game *game) {
    if (game == NULL) {
        return;
    }

    int box_w = 64;
    int box_h = 11;
    int box_x = (GRID_COLS - box_w) / 2;
    int box_y = (game_visible_rows() - box_h) / 2;

    renderer_draw_box(&game->renderer, box_x, box_y, box_w, box_h, COLOR_SHOP, COLOR_PANEL);
    game_draw_centered_text(&game->renderer, box_x, box_w, box_y + 1, "SHOP", COLOR_SHOP);

    char line[96];
    snprintf(line, sizeof(line), "Gold on hand: %d", game->player.gold);
    renderer_draw_text(&game->renderer, box_x + 2, box_y + 3, line, COLOR_TEXT);

    for (int index = 0; index < MAX_SHOP_OFFERS; ++index) {
        const ShopOffer *offer = &game->shop_offers[index];
        char offer_label[48];
        if (offer->weapon_offer) {
            ascii_art_format_weapon_label(offer->weapon, offer_label, sizeof(offer_label));
        } else {
            ascii_art_format_item_label(offer->item, offer_label, sizeof(offer_label));
        }

        snprintf(line, sizeof(line), "%c %-30s %s %3dg",
                 index == game->shop_selection ? '>' : ' ', offer_label,
                 offer->sold ? "SOLD" : "BUY ", offer->price);
        renderer_draw_text(&game->renderer, box_x + 2, box_y + 5 + index, line,
                           index == game->shop_selection ? COLOR_HIGHLIGHT : COLOR_TEXT);
    }

    renderer_draw_text(&game->renderer, box_x + 2, box_y + 8,
                       game->shop_selection == MAX_SHOP_OFFERS ? "> Leave" : "  Leave",
                       game->shop_selection == MAX_SHOP_OFFERS ? COLOR_HIGHLIGHT : COLOR_TEXT);
    renderer_draw_text(&game->renderer, box_x + 2, box_y + box_h - 2,
                       "Enter buys. Esc leaves the shop.", COLOR_DIM);
}

static void game_render_pause_menu(Game *game) {
    game_draw_menu_frame(game, "PAUSED", "Rick holds position in the dark.",
                         PAUSE_MENU_OPTIONS, 4, game->pause_selection, COLOR_HIGHLIGHT,
                         COLOR_TEXT, "Esc resumes the run.");
}

static void game_render_game_over(Game *game) {
    game_draw_menu_frame(game, "GAME OVER", game->death_message, GAME_OVER_OPTIONS, 3,
                         game->game_over_selection, COLOR_ALERT, COLOR_TEXT,
                         "Retry the run or return to the menu.");
}

static void game_render_victory(Game *game) {
    game_draw_menu_frame(game, "YOU WIN", game->death_message, VICTORY_OPTIONS, 3,
                         game->victory_selection, COLOR_HIGHLIGHT, COLOR_TEXT,
                         "Rick made it out alive.");
}

static void game_render_playing(Game *game) {
    game_draw_playfield(game);
    game_draw_hud(game);
}

static void game_render_scene(Game *game) {
    if (game == NULL) {
        return;
    }

    switch (game->mode) {
        case GAME_MODE_MAIN_MENU:
            game_render_main_menu(game);
            break;
        case GAME_MODE_CLASS_SELECT:
            game_render_class_select(game);
            break;
        case GAME_MODE_PLAYING:
            game_render_playing(game);
            break;
        case GAME_MODE_INVENTORY:
            game_render_playing(game);
            game_render_inventory(game);
            break;
        case GAME_MODE_SHOP:
            game_render_playing(game);
            game_render_shop(game);
            break;
        case GAME_MODE_PAUSED:
            game_render_playing(game);
            game_render_pause_menu(game);
            break;
        case GAME_MODE_GAME_OVER:
            game_render_playing(game);
            game_render_game_over(game);
            break;
        case GAME_MODE_VICTORY:
            game_render_playing(game);
            game_render_victory(game);
            break;
        default:
            break;
    }
}

static void game_render(Game *game) {
    if (game == NULL) {
        return;
    }

    renderer_update_layout(&game->renderer);
    renderer_clear(&game->renderer, COLOR_BG);
    game_render_scene(game);

    renderer_present(&game->renderer);
}

static void game_open_shop(Game *game) {
    if (game == NULL || world_tile_at(&game->world, game->player.x, game->player.y) != TILE_SHOP) {
        return;
    }

    game->shop_selection = 0;
    game->mode = GAME_MODE_SHOP;
}

static bool game_buy_shop_offer(Game *game, int index) {
    if (game == NULL || index < 0 || index >= MAX_SHOP_OFFERS) {
        return false;
    }

    ShopOffer *offer = &game->shop_offers[index];
    if (offer->sold || offer->price <= 0) {
        return false;
    }
    if (game->player.gold < offer->price) {
        game_set_message(game, "Rick cannot afford that.");
        return false;
    }

    if (offer->weapon_offer) {
        game->player.weapon = offer->weapon;
        game->player.gold -= offer->price;
        offer->sold = true;
        game_set_message(game, "Rick buys %s.", entity_weapon_profile(offer->weapon)->name);
        return true;
    }

    if (!game_add_item(game, offer->item)) {
        return false;
    }

    game->player.gold -= offer->price;
    offer->sold = true;
    return true;
}

static void game_handle_main_menu(Game *game, const InputState *input) {
    if (game == NULL || input == NULL) {
        return;
    }

    if (input->menu_delta != 0) {
        game->menu_selection = (game->menu_selection + input->menu_delta + 2) % 2;
    }
    if (input->confirm) {
        if (game->menu_selection == 0) {
            game_start_new_run(game);
        } else {
            game->running = false;
        }
    }
    if (input->cancel) {
        game->running = false;
    }
}

static void game_handle_class_select(Game *game, const InputState *input) {
    if (game == NULL || input == NULL) {
        return;
    }

    if (input->menu_delta != 0) {
        int count = PLAYER_CLASS_COUNT;
        game->class_selection = (game->class_selection + input->menu_delta + count) % count;
    }

    if (input->confirm) {
        unsigned int seed = (unsigned int)time(NULL) ^ 0xA511E9B3u;
        game_begin_run(game, CLASS_SELECTION_ORDER[game->class_selection], seed, 1);
    }
    if (input->cancel) {
        game->mode = GAME_MODE_MAIN_MENU;
        game->menu_selection = 0;
    }
}

static void game_handle_playing(Game *game, const InputState *input) {
    if (game == NULL || input == NULL) {
        return;
    }

    if (input->cancel) {
        game->pause_selection = 0;
        game->mode = GAME_MODE_PAUSED;
        return;
    }

    if (input->open_inventory) {
        game->mode = GAME_MODE_INVENTORY;
        return;
    }

    if (input->ability_slot >= 0) {
        if (game_use_ability(game, input->ability_slot)) {
            game_complete_turn(game);
        }
        return;
    }

    if (input->confirm) {
        char tile = world_tile_at(&game->world, game->player.x, game->player.y);
        if (tile == TILE_SHOP) {
            game_open_shop(game);
            return;
        }
        if (tile == TILE_CHEST) {
            if (game_open_chest(game)) {
                game_complete_turn(game);
            }
            return;
        }
        if (tile == TILE_STAIRS_DOWN && !game->world.boss_floor) {
            game_advance_floor(game);
            return;
        }
        if (tile == TILE_PORTAL && game->world.portal_open) {
            game_set_victory(game, "Rick escapes through the awakened portal.");
            return;
        }

        game_set_message(game, "Rick waits and listens.");
        game_complete_turn(game);
        return;
    }

    if (input->move_x != 0 || input->move_y != 0) {
        ActionResult result = game_try_player_move(game, input->move_x, input->move_y);
        if (result == ACTION_TURN) {
            game_complete_turn(game);
        }
    }
}

static void game_handle_inventory(Game *game, const InputState *input) {
    if (game == NULL || input == NULL) {
        return;
    }

    if (input->cancel || input->open_inventory) {
        game->mode = GAME_MODE_PLAYING;
        return;
    }

    if (input->menu_delta != 0) {
        int count = MAX_INVENTORY_SLOTS;
        game->inventory_selection = (game->inventory_selection + input->menu_delta + count) % count;
    }

    if (input->confirm && game_use_inventory_item(game, game->inventory_selection)) {
        game->mode = GAME_MODE_PLAYING;
        game_complete_turn(game);
    }
}

static void game_handle_shop(Game *game, const InputState *input) {
    if (game == NULL || input == NULL) {
        return;
    }

    if (input->cancel) {
        game->mode = GAME_MODE_PLAYING;
        return;
    }

    if (input->menu_delta != 0) {
        int count = MAX_SHOP_OFFERS + 1;
        game->shop_selection = (game->shop_selection + input->menu_delta + count) % count;
    }

    if (input->confirm) {
        if (game->shop_selection == MAX_SHOP_OFFERS) {
            game->mode = GAME_MODE_PLAYING;
        } else {
            game_buy_shop_offer(game, game->shop_selection);
        }
    }
}

static void game_handle_pause(Game *game, const InputState *input) {
    if (game == NULL || input == NULL) {
        return;
    }

    if (input->cancel) {
        game->mode = GAME_MODE_PLAYING;
        return;
    }

    if (input->menu_delta != 0) {
        int count = 4;
        game->pause_selection = (game->pause_selection + input->menu_delta + count) % count;
    }

    if (input->confirm) {
        switch (game->pause_selection) {
            case 0:
                game->mode = GAME_MODE_PLAYING;
                break;
            case 1:
                game->mode = GAME_MODE_INVENTORY;
                break;
            case 2:
                game->mode = GAME_MODE_MAIN_MENU;
                game->menu_selection = 0;
                break;
            case 3:
                game->running = false;
                break;
            default:
                break;
        }
    }
}

static void game_handle_game_over(Game *game, const InputState *input) {
    if (game == NULL || input == NULL) {
        return;
    }

    if (input->cancel) {
        game->mode = GAME_MODE_MAIN_MENU;
        game->menu_selection = 0;
        return;
    }

    if (input->menu_delta != 0) {
        int count = 3;
        game->game_over_selection =
            (game->game_over_selection + input->menu_delta + count) % count;
    }

    if (input->confirm) {
        switch (game->game_over_selection) {
            case 0:
                game_start_new_run(game);
                break;
            case 1:
                game->mode = GAME_MODE_MAIN_MENU;
                game->menu_selection = 0;
                break;
            case 2:
                game->running = false;
                break;
            default:
                break;
        }
    }
}

static void game_handle_victory(Game *game, const InputState *input) {
    if (game == NULL || input == NULL) {
        return;
    }

    if (input->cancel) {
        game->mode = GAME_MODE_MAIN_MENU;
        game->menu_selection = 0;
        return;
    }

    if (input->menu_delta != 0) {
        int count = 3;
        game->victory_selection = (game->victory_selection + input->menu_delta + count) % count;
    }

    if (input->confirm) {
        switch (game->victory_selection) {
            case 0:
                game_start_new_run(game);
                break;
            case 1:
                game->mode = GAME_MODE_MAIN_MENU;
                game->menu_selection = 0;
                break;
            case 2:
                game->running = false;
                break;
            default:
                break;
        }
    }
}

static void game_shutdown_runtime(Game *game) {
    if (game == NULL) {
        return;
    }

    renderer_shutdown(&game->renderer);
    if (game->sdl_renderer != NULL) {
        SDL_DestroyRenderer(game->sdl_renderer);
        game->sdl_renderer = NULL;
    }
    if (game->window != NULL) {
        SDL_DestroyWindow(game->window);
        game->window = NULL;
    }

    if (TTF_WasInit() != 0) {
        TTF_Quit();
    }
    if (SDL_WasInit(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        SDL_Quit();
    }
}

static bool game_init_internal(Game *game) {
    if (game == NULL) {
        return false;
    }

    memset(game, 0, sizeof(*game));

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        return false;
    }
    if (TTF_Init() != 0) {
        game_shutdown_runtime(game);
        return false;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    game->window = SDL_CreateWindow(GAME_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                    960, 720, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (game->window == NULL) {
        game_shutdown_runtime(game);
        return false;
    }

    game->sdl_renderer = SDL_CreateRenderer(
        game->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (game->sdl_renderer == NULL) {
        game->sdl_renderer = SDL_CreateRenderer(game->window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (game->sdl_renderer == NULL) {
        game_shutdown_runtime(game);
        return false;
    }

    if (!renderer_init(&game->renderer, game->sdl_renderer)) {
        game_shutdown_runtime(game);
        return false;
    }

    int window_width = GRID_COLS * game->renderer.cell_w;
    int window_height = game_visible_rows() * game->renderer.cell_h;
    SDL_SetWindowSize(game->window, window_width, window_height);
    SDL_SetWindowMinimumSize(game->window, window_width, window_height);
    renderer_update_layout(&game->renderer);

    world_init(&game->world);
    game->running = true;
    game->mode = GAME_MODE_MAIN_MENU;
    game->menu_selection = 0;
    game->class_selection = 0;
    game->pause_selection = 0;
    game->inventory_selection = 0;
    game->shop_selection = 0;
    game->game_over_selection = 0;
    game->victory_selection = 0;
    game->floor_number = 1;
    game->turn_counter = 0;
    game->kills = 0;
    game->revive_charges = 0;
    game_reset_player_pose(game);
    game_clear_inventory(game);
    game_clear_shop(game);
    game_set_message(game, "");
    game_set_death_message(game, "");
    return true;
}

Game *game_create(void) {
    Game *game = (Game *)calloc(1, sizeof(*game));
    if (game == NULL) {
        return NULL;
    }

    if (!game_init_internal(game)) {
        game_shutdown_runtime(game);
        free(game);
        return NULL;
    }

    return game;
}

void game_run(Game *game) {
    if (game == NULL) {
        return;
    }

    while (game->running) {
        InputState input;
        input_reset(&input);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            input_handle_event(&input, &event);
        }

        if (input.quit) {
            game->running = false;
            break;
        }
        if (input.toggle_debug) {
            game->debug_overlay = !game->debug_overlay;
        }

        switch (game->mode) {
            case GAME_MODE_MAIN_MENU:
                game_handle_main_menu(game, &input);
                break;
            case GAME_MODE_CLASS_SELECT:
                game_handle_class_select(game, &input);
                break;
            case GAME_MODE_PLAYING:
                game_handle_playing(game, &input);
                break;
            case GAME_MODE_INVENTORY:
                game_handle_inventory(game, &input);
                break;
            case GAME_MODE_SHOP:
                game_handle_shop(game, &input);
                break;
            case GAME_MODE_PAUSED:
                game_handle_pause(game, &input);
                break;
            case GAME_MODE_GAME_OVER:
                game_handle_game_over(game, &input);
                break;
            case GAME_MODE_VICTORY:
                game_handle_victory(game, &input);
                break;
            default:
                break;
        }

        game_render(game);
        SDL_Delay(1);
    }
}

void game_destroy(Game *game) {
    if (game == NULL) {
        return;
    }

    game_shutdown_runtime(game);
    free(game);
}

void game_set_debug_overlay(Game *game, bool enabled) {
    if (game == NULL) {
        return;
    }

    game->debug_overlay = enabled;
}

bool game_prepare_debug_run(Game *game, unsigned int seed, int floor_number) {
    if (game == NULL) {
        return false;
    }

    game_begin_run(game, PLAYER_CLASS_WARRIOR, seed, floor_number);
    return true;
}

bool game_dump_snapshot(Game *game, FILE *stream) {
    if (game == NULL || stream == NULL) {
        return false;
    }

    bool layout_ok = renderer_update_layout(&game->renderer);
    int content_w = GRID_COLS * game->renderer.cell_w;
    int content_h = game_visible_rows() * game->renderer.cell_h;
    bool fits = game->renderer.window_w >= content_w && game->renderer.window_h >= content_h;

    fprintf(stream, "=== CHARYRICK DEBUG SNAPSHOT ===\n");
    fprintf(stream, "layout=%s mode=%s floor=%d turn=%d seed=%u\n",
            layout_ok ? "ok" : "stale", game_mode_name(game->mode), game->floor_number,
            game->turn_counter, game->seed);
    fprintf(stream, "window=%dx%d content=%dx%d origin=%d,%d fit=%s\n",
            game->renderer.window_w, game->renderer.window_h, content_w, content_h,
            game->renderer.origin_x, game->renderer.origin_y, fits ? "full" : "clip");
    fprintf(stream, "cell=%dx%d grid=%dx%d hud=%d\n", game->renderer.cell_w,
            game->renderer.cell_h, GRID_COLS, GRID_ROWS, HUD_LINES);
    fprintf(stream, "spawn=%d,%d stairs=%d,%d player=%d,%d hp=%d/%d enemies=%d/%d\n",
            game->world.spawn_x, game->world.spawn_y, game->world.stairs_x, game->world.stairs_y,
            game->player.x, game->player.y, game->player.hp, game->player.max_hp,
            game_alive_enemy_count(game), game->enemy_count);
    if (game->message[0] != '\0') {
        fprintf(stream, "message=%s\n", game->message);
    }

    fputs("x=0         10        20        30        40        50        60        70\n",
          stream);

    char rows[GRID_ROWS + HUD_LINES][GRID_COLS + 1];
    game_fill_snapshot_rows(game, rows);
    for (int row = 0; row < GRID_ROWS + HUD_LINES; ++row) {
        fprintf(stream, "%02d |%s|\n", row, rows[row]);
    }

    return true;
}

bool game_prepare_debug_view(Game *game, const char *view_name, unsigned int seed,
                             int floor_number) {
    if (game == NULL || view_name == NULL) {
        return false;
    }

    PlayerClass debug_class;

    if (game_view_name_is(view_name, "menu")) {
        game->mode = GAME_MODE_MAIN_MENU;
        game->menu_selection = 0;
        return true;
    }

    if (game_view_name_is(view_name, "class")) {
        game_start_new_run(game);
        return true;
    }

    if (game_parse_debug_class_view(view_name, "class-", &debug_class)) {
        game_start_new_run(game);
        game->class_selection = game_class_selection_index(debug_class);
        return true;
    }

    if (game_parse_debug_class_view(view_name, "playing-", &debug_class)) {
        game_begin_run(game, debug_class, seed, floor_number);
        return true;
    }

    if (!game_prepare_debug_run(game, seed, floor_number)) {
        return false;
    }

    if (game_view_name_is(view_name, "playing")) {
        return true;
    }

    if (game_view_name_is(view_name, "inventory")) {
        game->mode = GAME_MODE_INVENTORY;
        return true;
    }

    if (game_view_name_is(view_name, "shop")) {
        if (!game->world.has_shop || !world_in_bounds(game->world.shop_x, game->world.shop_y)) {
            return false;
        }
        game->player.x = game->world.shop_x;
        game->player.y = game->world.shop_y;
        game->mode = GAME_MODE_SHOP;
        return true;
    }

    if (game_view_name_is(view_name, "pause")) {
        game->mode = GAME_MODE_PAUSED;
        game->pause_selection = 0;
        return true;
    }

    if (game_view_name_is(view_name, "gameover")) {
        game_set_game_over(game, "Debug defeat on floor %d.", game->floor_number);
        return true;
    }

    if (game_view_name_is(view_name, "victory")) {
        world_open_portal(&game->world);
        if (world_in_bounds(game->world.portal_x, game->world.portal_y)) {
            game->player.x = game->world.portal_x;
            game->player.y = game->world.portal_y;
        }
        game_set_victory(game, "Rick escapes through the awakened portal.");
        return true;
    }

    return false;
}

bool game_capture_frame(Game *game, const char *file_path) {
    if (game == NULL || file_path == NULL || file_path[0] == '\0') {
        return false;
    }

    renderer_update_layout(&game->renderer);
    renderer_clear(&game->renderer, COLOR_BG);
    game_render_scene(game);

    int width = 0;
    int height = 0;
    if (SDL_GetRendererOutputSize(game->sdl_renderer, &width, &height) != 0 || width <= 0 ||
        height <= 0) {
        return false;
    }

    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32,
                                                          SDL_PIXELFORMAT_RGBA32);
    if (surface == NULL) {
        return false;
    }

    bool ok = SDL_RenderReadPixels(game->sdl_renderer, NULL, SDL_PIXELFORMAT_RGBA32,
                                   surface->pixels, surface->pitch) == 0 &&
              SDL_SaveBMP(surface, file_path) == 0;
    SDL_FreeSurface(surface);
    return ok;
}
