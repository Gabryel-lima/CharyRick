#pragma once

#include <stdbool.h>

#include "config.h"

typedef struct Point {
    int x;
    int y;
} Point;

typedef enum RoomType {
    ROOM_TYPE_START = 0,
    ROOM_TYPE_COMBAT,
    ROOM_TYPE_TREASURE,
    ROOM_TYPE_SHOP,
    ROOM_TYPE_BOSS,
} RoomType;

typedef struct Room {
    int left;
    int top;
    int width;
    int height;
    RoomType type;
} Room;

typedef struct World {
    char tiles[GRID_ROWS][GRID_COLS];
    Room rooms[MAX_ROOMS];
    int room_count;
    Point enemy_spawns[MAX_ENEMIES];
    int enemy_spawn_count;
    int spawn_x;
    int spawn_y;
    int stairs_x;
    int stairs_y;
    int shop_x;
    int shop_y;
    int portal_x;
    int portal_y;
    int boss_x;
    int boss_y;
    int floor_number;
    bool has_shop;
    bool has_portal;
    bool portal_open;
    bool boss_floor;
} World;

void world_init(World *world);
void world_generate(World *world, unsigned int seed, int floor_number);
bool world_in_bounds(int x, int y);
bool world_is_walkable(const World *world, int x, int y);
char world_tile_at(const World *world, int x, int y);
void world_set_tile(World *world, int x, int y, char tile);
void world_open_portal(World *world);
int world_tile_count(const World *world, char tile);
