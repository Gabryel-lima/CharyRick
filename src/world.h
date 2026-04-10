#pragma once

#include <stdbool.h>

#include "config.h"

typedef struct World {
    char tiles[GRID_ROWS][GRID_COLS];
    int spawn_x;
    int spawn_y;
    int stairs_x;
    int stairs_y;
} World;

void world_init(World *world);
void world_generate(World *world, unsigned int seed);
bool world_in_bounds(int x, int y);
bool world_is_walkable(const World *world, int x, int y);
char world_tile_at(const World *world, int x, int y);
