#include "input.h"

#include <string.h>

void input_reset(InputState *input) {
    if (input == NULL) {
        return;
    }

    memset(input, 0, sizeof(*input));
}

void input_handle_event(InputState *input, const SDL_Event *event) {
    if (input == NULL || event == NULL) {
        return;
    }

    if (event->type == SDL_QUIT) {
        input->quit = true;
        return;
    }

    if (event->type != SDL_KEYDOWN || event->key.repeat != 0) {
        return;
    }

    switch (event->key.keysym.sym) {
        case SDLK_RETURN:
        case SDLK_SPACE:
            input->confirm = true;
            break;
        case SDLK_ESCAPE:
            input->cancel = true;
            break;
        case SDLK_F1:
            input->toggle_help = true;
            break;
        case SDLK_UP:
        case SDLK_w:
            input->move_y = -1;
            input->menu_delta = -1;
            break;
        case SDLK_DOWN:
        case SDLK_s:
            input->move_y = 1;
            input->menu_delta = 1;
            break;
        case SDLK_LEFT:
        case SDLK_a:
            input->move_x = -1;
            break;
        case SDLK_RIGHT:
        case SDLK_d:
            input->move_x = 1;
            break;
        default:
            break;
    }
}
