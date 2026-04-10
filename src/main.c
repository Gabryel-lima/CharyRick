#include "game.h"

#include <stdio.h>

int main(void) {
    Game *game = game_create();
    if (game == NULL) {
        fprintf(stderr, "Failed to start CharyRick.\n");
        return 1;
    }

    game_run(game);
    game_destroy(game);
    return 0;
}
