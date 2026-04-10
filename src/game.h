#pragma once

#include <stdbool.h>
#include <stdio.h>

#include "entity.h"

typedef struct Game Game;

Game *game_create(void);
void game_run(Game *game);
void game_destroy(Game *game);

bool game_prepare_debug_run(Game *game, unsigned int seed, int floor_number);
bool game_dump_snapshot(Game *game, FILE *stream);
bool game_prepare_debug_view(Game *game, const char *view_name, unsigned int seed,
							 int floor_number);
bool game_capture_frame(Game *game, const char *file_path);
void game_set_debug_overlay(Game *game, bool enabled);
