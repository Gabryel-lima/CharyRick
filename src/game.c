#include "game.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "config.h"
#include "entity.h"
#include "input.h"
#include "renderer.h"
#include "world.h"

typedef enum GameMode {
    GAME_MODE_MAIN_MENU = 0,
    GAME_MODE_PLAYING,
    GAME_MODE_PAUSED,
    GAME_MODE_GAME_OVER,
} GameMode;

typedef enum ActionResult {
    ACTION_NONE = 0,
    ACTION_TAKEN,
    ACTION_NEXT_FLOOR,
} ActionResult;

struct Game {
    SDL_Window *window;
    SDL_Renderer *sdl_renderer;
    Renderer renderer;
    World world;
    Entity player;
    Entity enemies[MAX_ENEMIES];
    int enemy_count;
    GameMode mode;
    bool running;
    int menu_selection;
    int pause_selection;
    int game_over_selection;
    int floor_number;
    int turn_counter;
    unsigned int seed;
    char message[MAX_MESSAGE_LENGTH];
    char death_message[MAX_MESSAGE_LENGTH];
};

static const char *const MAIN_MENU_OPTIONS[] = {
    "Start Run",
    "Quit\n",
};

static const char *const PAUSE_MENU_OPTIONS[] = {
    "Resume",
    "New Run",
    "Quit\n",
};

static const char *const GAME_OVER_OPTIONS[] = {
    "Retry",
    "Main Menu",
    "Quit\n",
};

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

static int sign_int(int value) {
    if (value < 0) {
        return -1;
    }

    if (value > 0) {
        return 1;
    }

    return 0;
}

static void game_clear_enemies(Game *game) {
    if (game == NULL) {
        return;
    }

    for (int index = 0; index < MAX_ENEMIES; ++index) {
        memset(&game->enemies[index], 0, sizeof(game->enemies[index]));
    }

    game->enemy_count = 0;
}

static void game_spawn_enemies(Game *game) {
    if (game == NULL) {
        return;
    }

    game_clear_enemies(game);

    int target_count = 4 + game->floor_number * 2;
    if (target_count > MAX_ENEMIES) {
        target_count = MAX_ENEMIES;
    }

    int attempts = 0;
    while (game->enemy_count < target_count && attempts < 2000) {
        ++attempts;

        int x = rand() % GRID_COLS;
        int y = rand() % GRID_ROWS;

        if (!world_is_walkable(&game->world, x, y)) {
            continue;
        }

        if (x == game->world.spawn_x && y == game->world.spawn_y) {
            continue;
        }

        if (x == game->world.stairs_x && y == game->world.stairs_y) {
            continue;
        }

        if (game_is_occupied(game, x, y, -1)) {
            continue;
        }

        Entity *enemy = &game->enemies[game->enemy_count];
        int hp = 4 + game->floor_number;
        int attack = 2 + game->floor_number / 2;
        int defense = game->floor_number / 4;
        entity_init(enemy, ENTITY_ENEMY, "Goblin", ENEMY_GLYPH, x, y, hp, attack, defense, true);
        ++game->enemy_count;
    }
}

static void game_generate_floor(Game *game) {
    if (game == NULL) {
        return;
    }

    world_generate(&game->world, game->seed + (unsigned int)(game->floor_number * 1337));
    game->player.x = game->world.spawn_x;
    game->player.y = game->world.spawn_y;
    game_spawn_enemies(game);
}

static void game_start_new_run(Game *game) {
    if (game == NULL) {
        return;
    }

    game->floor_number = 1;
    game->turn_counter = 0;
    game->seed = (unsigned int)time(NULL) ^ 0xA511E9B3u;
    entity_init(&game->player, ENTITY_PLAYER, "Rick", PLAYER_GLYPH, 0, 0, 20, 5, 1, true);
    game_generate_floor(game);
    game->menu_selection = 0;
    game->pause_selection = 0;
    game->game_over_selection = 0;
    game->mode = GAME_MODE_PLAYING;
    game_set_message(game, "Rick enters the dungeon.");
    game_set_death_message(game, "");
}

static void game_advance_floor(Game *game) {
    if (game == NULL) {
        return;
    }

    ++game->floor_number;
    ++game->seed;

    if (game->player.hp < game->player.max_hp) {
        game->player.hp += 2;
        if (game->player.hp > game->player.max_hp) {
            game->player.hp = game->player.max_hp;
        }
    }

    game_generate_floor(game);
    game_set_message(game, "Rick descends to floor %d.", game->floor_number);
}

static void game_set_game_over(Game *game, const char *format, ...) {
    if (game == NULL || format == NULL) {
        return;
    }

    va_list arguments;
    va_start(arguments, format);
    vsnprintf(game->death_message, sizeof(game->death_message), format, arguments);
    va_end(arguments);

    game->mode = GAME_MODE_GAME_OVER;
    game->game_over_selection = 0;
}

static SDL_Color game_color_for_tile(char tile) {
    switch (tile) {
        case WALL_GLYPH:
            return COLOR_WALL;
        case FLOOR_GLYPH:
            return COLOR_FLOOR;
        case STAIRS_GLYPH:
            return COLOR_STAIRS;
        default:
            return COLOR_DIM;
    }
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

        ++current;
    }

    if (current > longest) {
        longest = current;
    }

    return longest;
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

static void game_draw_menu_frame(Game *game, const char *title, const char *subtitle,
                                 const char *const *options, int option_count, int selected,
                                 SDL_Color selected_color, SDL_Color text_color) {
    if (game == NULL) {
        return;
    }

    const char *footer = "\nArrows/WASD move. Enter confirms. Esc returns.";

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
    if (box_w < 40) {
        box_w = 40;
    }
    if (box_w > GRID_COLS - 4) {
        box_w = GRID_COLS - 4;
    }

    int box_h = option_count + 8;
    if (box_h < 10) {
        box_h = 10;
    }

    const int box_x = (GRID_COLS - box_w) / 2;
    const int box_y = (GRID_ROWS - box_h) / 2;

    renderer_draw_box(&game->renderer, box_x, box_y, box_w, box_h, COLOR_TEXT, COLOR_PANEL);
    game_draw_centered_text(&game->renderer, box_x, box_w, box_y + 1, title, text_color);

    if (subtitle != NULL && subtitle[0] != '\0') {
        game_draw_centered_text(&game->renderer, box_x, box_w, box_y + 3, subtitle, COLOR_DIM);
    }

    int option_row = box_y + 6;
    for (int index = 0; index < option_count; ++index) {
        char line[64];
        snprintf(line, sizeof(line), "%c %s", index == selected ? '>' : ' ', options[index]);
        game_draw_centered_text(&game->renderer, box_x, box_w, option_row + index, line,
                                index == selected ? selected_color : COLOR_TEXT);
    }

    game_draw_centered_text(&game->renderer, box_x, box_w, box_y + box_h - 2,
                            footer,
                            COLOR_DIM);
}

static void game_draw_playfield(Game *game) {
    if (game == NULL) {
        return;
    }

    for (int y = 0; y < GRID_ROWS; ++y) {
        for (int x = 0; x < GRID_COLS; ++x) {
            char tile = world_tile_at(&game->world, x, y);
            renderer_draw_char(&game->renderer, x, y, tile, game_color_for_tile(tile));
        }
    }

    for (int index = 0; index < game->enemy_count; ++index) {
        const Entity *enemy = &game->enemies[index];
        if (!enemy->alive) {
            continue;
        }

        renderer_draw_char(&game->renderer, enemy->x, enemy->y, enemy->glyph, COLOR_ENEMY);
    }

    if (game->player.alive) {
        renderer_draw_char(&game->renderer, game->player.x, game->player.y, game->player.glyph, COLOR_PLAYER);
    }
}

static void game_draw_hud(Game *game) {
    if (game == NULL) {
        return;
    }

    renderer_draw_box(&game->renderer, 0, GRID_ROWS, GRID_COLS, HUD_LINES, COLOR_DIM, COLOR_PANEL);

    int alive_enemies = 0;
    for (int index = 0; index < game->enemy_count; ++index) {
        if (game->enemies[index].alive) {
            ++alive_enemies;
        }
    }

    char line1[128];
    char line2[128];
    char line3[128];
    char line4[128];

    snprintf(line1, sizeof(line1), "Rick  HP %d/%d  ATK %d  DEF %d  FLOOR %d  TURN %d",
             game->player.hp, game->player.max_hp, game->player.attack, game->player.defense,
             game->floor_number, game->turn_counter);
    snprintf(line2, sizeof(line2), "Enemies left: %d", alive_enemies);
    snprintf(line3, sizeof(line3), "%s", game->message[0] != '\0' ? game->message : "Explore the dungeon and reach the stairs.");
    snprintf(line4, sizeof(line4), "Move with WASD or arrows. Space waits. Esc pauses.");

    SDL_Color hp_color = game->player.hp <= 5 ? COLOR_ALERT : COLOR_TEXT;
    renderer_draw_text(&game->renderer, 1, GRID_ROWS + 0, line1, hp_color);
    renderer_draw_text(&game->renderer, 1, GRID_ROWS + 1, line2, COLOR_TEXT);
    renderer_draw_text(&game->renderer, 1, GRID_ROWS + 2, line3, COLOR_HIGHLIGHT);
    renderer_draw_text(&game->renderer, 1, GRID_ROWS + 3, line4, COLOR_DIM);
}

static void game_render_main_menu(Game *game) {
    game_draw_menu_frame(game, GAME_TITLE, "A character-only dungeon crawl starring Rick.",
                         MAIN_MENU_OPTIONS, 2, game->menu_selection, COLOR_HIGHLIGHT, COLOR_TEXT);
}

static void game_render_pause_menu(Game *game) {
    game_draw_menu_frame(game, "PAUSED", "Rick is waiting in the dark.",
                         PAUSE_MENU_OPTIONS, 3, game->pause_selection, COLOR_HIGHLIGHT, COLOR_TEXT);
}

static void game_render_game_over(Game *game) {
    game_draw_menu_frame(game, "GAME OVER", game->death_message,
                         GAME_OVER_OPTIONS, 3, game->game_over_selection, COLOR_ALERT, COLOR_TEXT);
}

static void game_render_playing(Game *game) {
    game_draw_playfield(game);
    game_draw_hud(game);
}

static void game_render(Game *game) {
    if (game == NULL) {
        return;
    }

    renderer_update_layout(&game->renderer);
    renderer_clear(&game->renderer, COLOR_BG);

    switch (game->mode) {
        case GAME_MODE_MAIN_MENU:
            game_render_main_menu(game);
            break;
        case GAME_MODE_PLAYING:
            game_render_playing(game);
            break;
        case GAME_MODE_PAUSED:
            game_render_playing(game);
            game_render_pause_menu(game);
            break;
        case GAME_MODE_GAME_OVER:
            game_render_playing(game);
            game_render_game_over(game);
            break;
        default:
            break;
    }

    renderer_present(&game->renderer);
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

static ActionResult game_try_player_move(Game *game, int dx, int dy) {
    if (game == NULL || dx == 0 && dy == 0) {
        return ACTION_NONE;
    }

    int target_x = game->player.x + dx;
    int target_y = game->player.y + dy;

    if (!world_in_bounds(target_x, target_y)) {
        return ACTION_NONE;
    }

    int enemy_index = game_find_enemy_at(game, target_x, target_y);
    if (enemy_index >= 0) {
        Entity *enemy = &game->enemies[enemy_index];
        int damage = entity_damage_to(&game->player, enemy);
        entity_apply_damage(enemy, damage);

        if (enemy->alive) {
            game_set_message(game, "Rick strikes %s for %d.", enemy->name, damage);
        } else {
            game_set_message(game, "Rick defeats %s.", enemy->name);
            game->player.x = target_x;
            game->player.y = target_y;
        }

        return ACTION_TAKEN;
    }

    if (!world_is_walkable(&game->world, target_x, target_y)) {
        return ACTION_NONE;
    }

    if (game_is_occupied(game, target_x, target_y, -1)) {
        return ACTION_NONE;
    }

    game->player.x = target_x;
    game->player.y = target_y;

    if (target_x == game->world.stairs_x && target_y == game->world.stairs_y) {
        game_advance_floor(game);
        return ACTION_NEXT_FLOOR;
    }

    game_set_message(game, "Rick moves through the dungeon.");
    return ACTION_TAKEN;
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

        int distance_x = game->player.x - enemy->x;
        int distance_y = game->player.y - enemy->y;
        int manhattan = abs(distance_x) + abs(distance_y);

        if (manhattan == 1) {
            int damage = entity_damage_to(enemy, &game->player);
            entity_apply_damage(&game->player, damage);

            if (!game->player.alive) {
                game_set_death_message(game, "%s overwhelms Rick on floor %d.", enemy->name, game->floor_number);
                game_set_game_over(game, "%s overwhelms Rick on floor %d.", enemy->name, game->floor_number);
                return;
            }

            game_set_message(game, "%s hits Rick for %d.", enemy->name, damage);
            continue;
        }

        if (manhattan > 8) {
            continue;
        }

        int step_x = 0;
        int step_y = 0;

        if (abs(distance_x) >= abs(distance_y)) {
            step_x = sign_int(distance_x);
        } else {
            step_y = sign_int(distance_y);
        }

        int target_x = enemy->x + step_x;
        int target_y = enemy->y + step_y;

        if (target_x == game->player.x && target_y == game->player.y) {
            int damage = entity_damage_to(enemy, &game->player);
            entity_apply_damage(&game->player, damage);

            if (!game->player.alive) {
                game_set_death_message(game, "%s catches Rick on floor %d.", enemy->name, game->floor_number);
                game_set_game_over(game, "%s catches Rick on floor %d.", enemy->name, game->floor_number);
                return;
            }

            game_set_message(game, "%s hits Rick for %d.", enemy->name, damage);
            continue;
        }

        bool moved = false;
        if (world_is_walkable(&game->world, target_x, target_y) && !game_is_occupied(game, target_x, target_y, index)) {
            enemy->x = target_x;
            enemy->y = target_y;
            moved = true;
        } else {
            int alternate_x = enemy->x + sign_int(distance_x);
            int alternate_y = enemy->y + sign_int(distance_y);

            if (alternate_x != enemy->x && world_is_walkable(&game->world, alternate_x, enemy->y) &&
                !game_is_occupied(game, alternate_x, enemy->y, index)) {
                enemy->x = alternate_x;
                moved = true;
            } else if (alternate_y != enemy->y && world_is_walkable(&game->world, enemy->x, alternate_y) &&
                       !game_is_occupied(game, enemy->x, alternate_y, index)) {
                enemy->y = alternate_y;
                moved = true;
            }
        }

        if (moved) {
            continue;
        }
    }
}

static void game_handle_main_menu(Game *game, const InputState *input) {
    if (game == NULL || input == NULL) {
        return;
    }

    if (input->menu_delta != 0) {
        game->menu_selection += input->menu_delta;
        if (game->menu_selection < 0) {
            game->menu_selection = 1;
        } else if (game->menu_selection > 1) {
            game->menu_selection = 0;
        }
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

static void game_handle_playing(Game *game, const InputState *input) {
    if (game == NULL || input == NULL) {
        return;
    }

    if (input->cancel) {
        game->mode = GAME_MODE_PAUSED;
        game->pause_selection = 0;
        return;
    }

    if (input->confirm) {
        game_set_message(game, "Rick waits for the dungeon to move.");
        game_enemy_turn(game);
        ++game->turn_counter;
        return;
    }

    if (input->move_x != 0 || input->move_y != 0) {
        ActionResult result = game_try_player_move(game, input->move_x, input->move_y);
        if (result == ACTION_TAKEN) {
            if (game->mode == GAME_MODE_PLAYING) {
                game_enemy_turn(game);
                ++game->turn_counter;
            }
        } else if (result == ACTION_NEXT_FLOOR) {
            ++game->turn_counter;
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
        game->pause_selection += input->menu_delta;
        if (game->pause_selection < 0) {
            game->pause_selection = 2;
        } else if (game->pause_selection > 2) {
            game->pause_selection = 0;
        }
    }

    if (input->confirm) {
        switch (game->pause_selection) {
            case 0:
                game->mode = GAME_MODE_PLAYING;
                break;
            case 1:
                game_start_new_run(game);
                break;
            case 2:
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
        game->game_over_selection += input->menu_delta;
        if (game->game_over_selection < 0) {
            game->game_over_selection = 2;
        } else if (game->game_over_selection > 2) {
            game->game_over_selection = 0;
        }
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

    game->window = SDL_CreateWindow(
        GAME_TITLE,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        960,
        720,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (game->window == NULL) {
        game_shutdown_runtime(game);
        return false;
    }

    game->sdl_renderer = SDL_CreateRenderer(game->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
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
    int window_height = (GRID_ROWS + HUD_LINES) * game->renderer.cell_h;
    SDL_SetWindowSize(game->window, window_width, window_height);
    SDL_SetWindowMinimumSize(game->window, window_width, window_height);
    SDL_SetWindowTitle(game->window, GAME_TITLE);
    renderer_update_layout(&game->renderer);

    world_init(&game->world);
    game->running = true;
    game->mode = GAME_MODE_MAIN_MENU;
    game->menu_selection = 0;
    game->pause_selection = 0;
    game->game_over_selection = 0;
    game->floor_number = 1;
    game->turn_counter = 0;
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

        switch (game->mode) {
            case GAME_MODE_MAIN_MENU:
                game_handle_main_menu(game, &input);
                break;
            case GAME_MODE_PLAYING:
                game_handle_playing(game, &input);
                break;
            case GAME_MODE_PAUSED:
                game_handle_pause(game, &input);
                break;
            case GAME_MODE_GAME_OVER:
                game_handle_game_over(game, &input);
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
