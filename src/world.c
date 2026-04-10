#include "world.h"

#include <string.h>

static unsigned int world_rng_next(unsigned int *state) {
    *state = *state * 1664525u + 1013904223u;
    return *state;
}

static int world_rng_range(unsigned int *state, int min_value, int max_value) {
    if (max_value <= min_value) {
        return min_value;
    }

    unsigned int span = (unsigned int)(max_value - min_value + 1);
    return min_value + (int)(world_rng_next(state) % span);
}

static bool world_rng_roll(unsigned int *state, int percent) {
    if (percent <= 0) {
        return false;
    }
    if (percent >= 100) {
        return true;
    }

    return (int)(world_rng_next(state) % 100u) < percent;
}

static void world_fill(World *world, char tile) {
    for (int y = 0; y < GRID_ROWS; ++y) {
        for (int x = 0; x < GRID_COLS; ++x) {
            world->tiles[y][x] = tile;
        }
    }
}

static void world_reset(World *world) {
    memset(world, 0, sizeof(*world));
    world_fill(world, TILE_WALL);
    world->spawn_x = 1;
    world->spawn_y = 1;
    world->stairs_x = -1;
    world->stairs_y = -1;
    world->shop_x = -1;
    world->shop_y = -1;
    world->portal_x = -1;
    world->portal_y = -1;
    world->boss_x = -1;
    world->boss_y = -1;
}

static int world_room_center_x(const Room *room) {
    return room->left + room->width / 2;
}

static int world_room_center_y(const Room *room) {
    return room->top + room->height / 2;
}

static bool world_room_overlaps(const Room *a, const Room *b) {
    return a->left - 1 <= b->left + b->width &&
           a->left + a->width + 1 >= b->left &&
           a->top - 1 <= b->top + b->height &&
           a->top + a->height + 1 >= b->top;
}

static void world_carve_room(World *world, const Room *room) {
    for (int y = room->top; y < room->top + room->height; ++y) {
        for (int x = room->left; x < room->left + room->width; ++x) {
            world_set_tile(world, x, y, TILE_FLOOR);
        }
    }
}

static void world_carve_horizontal(World *world, int x0, int x1, int y) {
    int start = x0 < x1 ? x0 : x1;
    int end = x0 > x1 ? x0 : x1;
    for (int x = start; x <= end; ++x) {
        world_set_tile(world, x, y, TILE_FLOOR);
    }
}

static void world_carve_vertical(World *world, int x, int y0, int y1) {
    int start = y0 < y1 ? y0 : y1;
    int end = y0 > y1 ? y0 : y1;
    for (int y = start; y <= end; ++y) {
        world_set_tile(world, x, y, TILE_FLOOR);
    }
}

static void world_connect_rooms(World *world, const Room *a, const Room *b) {
    int ax = world_room_center_x(a);
    int ay = world_room_center_y(a);
    int bx = world_room_center_x(b);
    int by = world_room_center_y(b);

    world_carve_horizontal(world, ax, bx, ay);
    world_carve_vertical(world, bx, ay, by);
}

static void world_place_room_torches(World *world, const Room *room) {
    int center_x = world_room_center_x(room);
    int center_y = world_room_center_y(room);

    if (world_in_bounds(center_x, room->top - 1)) {
        world_set_tile(world, center_x, room->top - 1, TILE_TORCH);
    }
    if (world_in_bounds(center_x, room->top + room->height)) {
        world_set_tile(world, center_x, room->top + room->height, TILE_TORCH);
    }
    if (world_in_bounds(room->left - 1, center_y)) {
        world_set_tile(world, room->left - 1, center_y, TILE_TORCH);
    }
    if (world_in_bounds(room->left + room->width, center_y)) {
        world_set_tile(world, room->left + room->width, center_y, TILE_TORCH);
    }
}

static void world_place_room_door(World *world, const Room *room) {
    int door_y = world_room_center_y(room);
    int door_x = room->left;
    if (world_in_bounds(door_x, door_y) && world_tile_at(world, door_x, door_y) == TILE_FLOOR) {
        world_set_tile(world, door_x, door_y, TILE_DOOR);
    }
}

static void world_record_enemy_spawn(World *world, int x, int y) {
    if (world == NULL || world->enemy_spawn_count >= MAX_ENEMIES) {
        return;
    }
    if (!world_in_bounds(x, y) || world_tile_at(world, x, y) != TILE_FLOOR) {
        return;
    }

    for (int index = 0; index < world->enemy_spawn_count; ++index) {
        if (world->enemy_spawns[index].x == x && world->enemy_spawns[index].y == y) {
            return;
        }
    }

    world->enemy_spawns[world->enemy_spawn_count].x = x;
    world->enemy_spawns[world->enemy_spawn_count].y = y;
    world->enemy_spawn_count += 1;
}

static void world_add_room_spawns(World *world, const Room *room, unsigned int *rng, int count) {
    int center_x = world_room_center_x(room);
    int center_y = world_room_center_y(room);

    for (int attempt = 0; attempt < 24 && count > 0; ++attempt) {
        int x = world_rng_range(rng, room->left + 1, room->left + room->width - 2);
        int y = world_rng_range(rng, room->top + 1, room->top + room->height - 2);
        if (x == center_x && y == center_y) {
            continue;
        }
        if (world_tile_at(world, x, y) != TILE_FLOOR) {
            continue;
        }

        world_record_enemy_spawn(world, x, y);
        count -= 1;
    }
}

static void world_paint_pool(World *world, const Room *room, unsigned int *rng, char tile) {
    int center_x = world_rng_range(rng, room->left + 2, room->left + room->width - 3);
    int center_y = world_rng_range(rng, room->top + 1, room->top + room->height - 2);
    int radius_x = room->width > 10 ? 2 : 1;
    int radius_y = room->height > 6 ? 1 : 0;

    for (int y = center_y - radius_y; y <= center_y + radius_y; ++y) {
        for (int x = center_x - radius_x; x <= center_x + radius_x; ++x) {
            if (world_tile_at(world, x, y) == TILE_FLOOR) {
                world_set_tile(world, x, y, tile);
            }
        }
    }
}

static void world_paint_traps(World *world, const Room *room, unsigned int *rng) {
    int y = world_rng_range(rng, room->top + 1, room->top + room->height - 2);
    for (int x = room->left + 2; x < room->left + room->width - 2; x += 2) {
        if (world_tile_at(world, x, y) == TILE_FLOOR) {
            world_set_tile(world, x, y, TILE_TRAP);
        }
    }
}

static void world_generate_standard_floor(World *world, unsigned int *rng) {
    int target_rooms = 6 + world_rng_range(rng, 0, 2);

    for (int attempt = 0; attempt < MAX_ROOM_ATTEMPTS && world->room_count < target_rooms; ++attempt) {
        Room candidate;
        candidate.width = world_rng_range(rng, 8, 13);
        candidate.height = world_rng_range(rng, 5, 8);
        candidate.left = world_rng_range(rng, 2, GRID_COLS - candidate.width - 3);
        candidate.top = world_rng_range(rng, 2, GRID_ROWS - candidate.height - 3);
        candidate.type = ROOM_TYPE_COMBAT;

        bool overlaps = false;
        for (int index = 0; index < world->room_count; ++index) {
            if (world_room_overlaps(&candidate, &world->rooms[index])) {
                overlaps = true;
                break;
            }
        }

        if (overlaps) {
            continue;
        }

        world->rooms[world->room_count] = candidate;
        world_carve_room(world, &candidate);

        if (world->room_count > 0) {
            world_connect_rooms(world, &world->rooms[world->room_count - 1], &candidate);
        }

        world->room_count += 1;
    }

    if (world->room_count == 0) {
        Room fallback = { 8, 6, 20, 8, ROOM_TYPE_START };
        world->rooms[0] = fallback;
        world->room_count = 1;
        world_carve_room(world, &fallback);
    }

    world->rooms[0].type = ROOM_TYPE_START;
    world->spawn_x = world_room_center_x(&world->rooms[0]);
    world->spawn_y = world_room_center_y(&world->rooms[0]);

    int stairs_room = world->room_count - 1;
    world->stairs_x = world_room_center_x(&world->rooms[stairs_room]);
    world->stairs_y = world_room_center_y(&world->rooms[stairs_room]);
    world_set_tile(world, world->stairs_x, world->stairs_y, TILE_STAIRS_DOWN);

    int treasure_room = -1;
    int shop_room = -1;
    if (world->room_count > 3) {
        treasure_room = world_rng_range(rng, 1, world->room_count - 2);
        world->rooms[treasure_room].type = ROOM_TYPE_TREASURE;
    }
    if (world->room_count > 4) {
        shop_room = world_rng_range(rng, 1, world->room_count - 2);
        if (shop_room == treasure_room) {
            shop_room = shop_room == world->room_count - 2 ? 1 : shop_room + 1;
        }
        world->rooms[shop_room].type = ROOM_TYPE_SHOP;
        world->has_shop = true;
        world->shop_x = world_room_center_x(&world->rooms[shop_room]);
        world->shop_y = world_room_center_y(&world->rooms[shop_room]);
        world_set_tile(world, world->shop_x, world->shop_y, TILE_SHOP);
        world_place_room_door(world, &world->rooms[shop_room]);
    }
    if (treasure_room >= 0) {
        int chest_x = world_room_center_x(&world->rooms[treasure_room]);
        int chest_y = world_room_center_y(&world->rooms[treasure_room]);
        world_set_tile(world, chest_x, chest_y, TILE_CHEST);
        world_place_room_door(world, &world->rooms[treasure_room]);
    }

    int water_room = -1;
    int trap_room = -1;
    if (world->room_count > 4) {
        water_room = world_rng_range(rng, 1, world->room_count - 1);
        if (water_room == treasure_room || water_room == shop_room) {
            water_room = stairs_room > 1 ? stairs_room - 1 : 1;
        }
        trap_room = world_rng_range(rng, 1, world->room_count - 1);
        if (trap_room == water_room || trap_room == treasure_room || trap_room == shop_room) {
            trap_room = 1;
        }
    }

    for (int index = 0; index < world->room_count; ++index) {
        Room *room = &world->rooms[index];
        world_place_room_torches(world, room);

        if (index == water_room) {
            world_paint_pool(world, room, rng, world->floor_number >= 2 ? TILE_LAVA : TILE_WATER);
        }
        if (index == trap_room) {
            world_paint_traps(world, room, rng);
        }

        if (room->type == ROOM_TYPE_START || room->type == ROOM_TYPE_SHOP) {
            continue;
        }

        int spawn_count = room->type == ROOM_TYPE_TREASURE ? 1 : 1 + world_rng_range(rng, 0, world->floor_number > 1 ? 2 : 1);
        world_add_room_spawns(world, room, rng, spawn_count);
    }

    world_set_tile(world, world->spawn_x, world->spawn_y, TILE_FLOOR);
}

static void world_generate_boss_floor(World *world, unsigned int *rng) {
    (void)rng;

    world->boss_floor = true;
    world->has_portal = true;
    world->portal_open = false;

    Room arena = { 8, 2, GRID_COLS - 16, GRID_ROWS - 5, ROOM_TYPE_BOSS };
    world->rooms[0] = arena;
    world->room_count = 1;
    world_carve_room(world, &arena);

    int center_x = world_room_center_x(&arena);
    int center_y = world_room_center_y(&arena);
    world->spawn_x = center_x;
    world->spawn_y = arena.top + arena.height - 3;
    world->boss_x = center_x;
    world->boss_y = center_y - 1;
    world->portal_x = center_x;
    world->portal_y = arena.top + 1;
    world->stairs_x = world->spawn_x;
    world->stairs_y = world->spawn_y;

    world_set_tile(world, center_x, arena.top + arena.height - 1, TILE_DOOR);
    world_set_tile(world, world->portal_x, world->portal_y, TILE_PORTAL);
    world_set_tile(world, world->spawn_x, world->spawn_y, TILE_FLOOR);

    for (int x = center_x - 4; x <= center_x + 4; x += 2) {
        world_set_tile(world, x, center_y + 2, TILE_TRAP);
    }

    for (int y = arena.top + 2; y <= arena.top + 4; ++y) {
        for (int x = arena.left + 3; x <= arena.left + 6; ++x) {
            world_set_tile(world, x, y, TILE_LAVA);
        }
        for (int x = arena.left + arena.width - 7; x <= arena.left + arena.width - 4; ++x) {
            world_set_tile(world, x, y, TILE_LAVA);
        }
    }

    world_place_room_torches(world, &arena);
}

void world_init(World *world) {
    if (world == NULL) {
        return;
    }

    world_reset(world);
}

void world_generate(World *world, unsigned int seed, int floor_number) {
    if (world == NULL) {
        return;
    }

    world_reset(world);
    world->floor_number = floor_number;

    unsigned int rng = seed ^ (unsigned int)(floor_number * 2654435761u);
    if (floor_number >= 3) {
        world_generate_boss_floor(world, &rng);
    } else {
        world_generate_standard_floor(world, &rng);
    }
}

bool world_in_bounds(int x, int y) {
    return x >= 0 && x < GRID_COLS && y >= 0 && y < GRID_ROWS;
}

bool world_is_walkable(const World *world, int x, int y) {
    if (world == NULL || !world_in_bounds(x, y)) {
        return false;
    }

    char tile = world->tiles[y][x];
    switch (tile) {
        case TILE_FLOOR:
        case TILE_DOOR:
        case TILE_DOOR_OPEN:
        case TILE_STAIRS_DOWN:
        case TILE_STAIRS_UP:
        case TILE_WATER:
        case TILE_LAVA:
        case TILE_TRAP:
        case TILE_CHEST:
        case TILE_SHOP:
            return true;
        case TILE_PORTAL:
            return world->portal_open;
        default:
            return false;
    }
}

char world_tile_at(const World *world, int x, int y) {
    if (world == NULL || !world_in_bounds(x, y)) {
        return TILE_WALL;
    }

    return world->tiles[y][x];
}

void world_set_tile(World *world, int x, int y, char tile) {
    if (world == NULL || !world_in_bounds(x, y)) {
        return;
    }

    world->tiles[y][x] = tile;
}

void world_open_portal(World *world) {
    if (world == NULL) {
        return;
    }

    world->portal_open = true;
}

int world_tile_count(const World *world, char tile) {
    if (world == NULL) {
        return 0;
    }

    int count = 0;
    for (int y = 0; y < GRID_ROWS; ++y) {
        for (int x = 0; x < GRID_COLS; ++x) {
            if (world->tiles[y][x] == tile) {
                count += 1;
            }
        }
    }

    return count;
}
