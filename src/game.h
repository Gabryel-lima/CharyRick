#pragma once

typedef struct Game Game;

Game *game_create(void);
void game_run(Game *game);
void game_destroy(Game *game);
