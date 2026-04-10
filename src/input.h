#pragma once

#include <stdbool.h>

#include <SDL2/SDL.h>

typedef struct InputState {
    bool quit;
    bool confirm;
    bool cancel;
    bool toggle_debug;
    bool open_inventory;
    int move_x;
    int move_y;
    int menu_delta;
    int ability_slot;
} InputState;

void input_reset(InputState *input);
void input_handle_event(InputState *input, const SDL_Event *event);
