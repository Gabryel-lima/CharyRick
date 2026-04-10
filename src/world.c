#include "world.h"

#include <stdlib.h>
#include <string.h>

static void world_fill(World *world, char tile) {
    for (int y = 0; y < GRID_ROWS; ++y) {
        for (int x = 0; x < GRID_COLS; ++x) {
            world->tiles[y][x] = tile;
        }
    }
}

static void world_carve_room(World *world, int left, int top, int width, int height) {
    for (int y = top; y < top + height && y < GRID_ROWS - 1; ++y) {
        if (y <= 0) {
            continue;
        }

        for (int x = left; x < left + width && x < GRID_COLS - 1; ++x) {
            if (x <= 0) {
                continue;
            }

            world->tiles[y][x] = FLOOR_GLYPH;
        }
    }
}

static void world_carve_hallway_horizontal(World *world, int x0, int x1, int y) {
    if (y <= 0 || y >= GRID_ROWS - 1) {
        return;
    }

    int start = x0 < x1 ? x0 : x1;
    int end = x0 > x1 ? x0 : x1;

    if (start < 1) {
        start = 1;
    }

    if (end > GRID_COLS - 2) {
        end = GRID_COLS - 2;
    }

    for (int x = start; x <= end; ++x) {
        world->tiles[y][x] = FLOOR_GLYPH;
    }
}

static void world_carve_hallway_vertical(World *world, int x, int y0, int y1) {
    if (x <= 0 || x >= GRID_COLS - 1) {
        return;
    }

    int start = y0 < y1 ? y0 : y1;
    int end = y0 > y1 ? y0 : y1;

    if (start < 1) {
        start = 1;
    }

    if (end > GRID_ROWS - 2) {
        end = GRID_ROWS - 2;
    }

    for (int y = start; y <= end; ++y) {
        world->tiles[y][x] = FLOOR_GLYPH;
    }
}

void world_init(World *world) {
    if (world == NULL) {
        return;
    }

    memset(world, 0, sizeof(*world));
    world_fill(world, WALL_GLYPH);
    world->spawn_x = 1;
    world->spawn_y = 1;
    world->stairs_x = GRID_COLS - 2;
    world->stairs_y = GRID_ROWS - 2;
}

void world_generate(World *world, unsigned int seed) {
    if (world == NULL) {
        return;
    }

    srand(seed);
    world_fill(world, WALL_GLYPH);

    int previous_center_x = 0;
    int previous_center_y = 0;
    bool have_previous_room = false;

    int room_total = 5 + rand() % 3;
    for (int room = 0; room < room_total; ++room) {
        int room_width = 6 + rand() % 12;
        int room_height = 4 + rand() % 6;
        int room_left = 1 + rand() % (GRID_COLS - room_width - 2);
        int room_top = 1 + rand() % (GRID_ROWS - room_height - 2);

        world_carve_room(world, room_left, room_top, room_width, room_height);

        int center_x = room_left + room_width / 2;
        int center_y = room_top + room_height / 2;

        if (!have_previous_room) {
            world->spawn_x = center_x;
            world->spawn_y = center_y;
            have_previous_room = true;
        } else {
            world_carve_hallway_horizontal(world, previous_center_x, center_x, previous_center_y);
            world_carve_hallway_vertical(world, center_x, previous_center_y, center_y);
        }

        previous_center_x = center_x;
        previous_center_y = center_y;
    }

    world->stairs_x = previous_center_x;
    world->stairs_y = previous_center_y;

    if (world_in_bounds(world->spawn_x, world->spawn_y)) {
        world->tiles[world->spawn_y][world->spawn_x] = FLOOR_GLYPH;
    }

    if (world_in_bounds(world->stairs_x, world->stairs_y)) {
        world->tiles[world->stairs_y][world->stairs_x] = STAIRS_GLYPH;
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
    return tile == FLOOR_GLYPH || tile == STAIRS_GLYPH;
}

char world_tile_at(const World *world, int x, int y) {
    if (world == NULL || !world_in_bounds(x, y)) {
        return WALL_GLYPH;
    }

    return world->tiles[y][x];
}
