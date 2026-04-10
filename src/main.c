#include "game.h"

#include <SDL2/SDL.h>

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ProgramOptions {
    bool debug_snapshot;
    bool debug_capture;
    bool debug_overlay;
    bool show_help;
    bool seed_set;
    bool floor_set;
    unsigned int seed;
    int floor_number;
    const char *capture_path;
    const char *debug_view;
} ProgramOptions;

static void program_options_init(ProgramOptions *options) {
    if (options == NULL) {
        return;
    }

    memset(options, 0, sizeof(*options));
    options->seed = 12345u;
    options->floor_number = 1;
    options->debug_view = "playing";
}

static bool parse_unsigned_arg(const char *text, unsigned int *value) {
    if (text == NULL || value == NULL) {
        return false;
    }

    char *end = NULL;
    unsigned long parsed = strtoul(text, &end, 10);
    if (end == text || *end != '\0' || parsed > UINT_MAX) {
        return false;
    }

    *value = (unsigned int)parsed;
    return true;
}

static bool parse_floor_arg(const char *text, int *value) {
    if (text == NULL || value == NULL) {
        return false;
    }

    char *end = NULL;
    long parsed = strtol(text, &end, 10);
    if (end == text || *end != '\0' || parsed < 1 || parsed > INT_MAX) {
        return false;
    }

    *value = (int)parsed;
    return true;
}

static void print_usage(const char *program_name) {
    fprintf(stderr, "Usage: %s [--debug-overlay] [--debug-snapshot] [--debug-capture FILE] [--debug-view NAME] [--seed N] [--floor N]\n",
            program_name != NULL ? program_name : "CharyRick");
    fputs("\n", stderr);
    fputs("  --debug-overlay   Start with the debug HUD overlay enabled.\n", stderr);
    fputs("  --debug-snapshot  Print a deterministic layout snapshot and exit.\n", stderr);
    fputs("  --debug-capture   Render a frame to a BMP file and exit.\n", stderr);
    fputs("  --debug-view      menu, class, playing, inventory, shop, pause, gameover, victory.\n", stderr);
    fputs("  --seed N          Seed used by --debug-snapshot. Default: 12345.\n", stderr);
    fputs("  --floor N         Floor used by --debug-snapshot. Default: 1.\n", stderr);
}

int main(int argc, char **argv) {
    ProgramOptions options;
    program_options_init(&options);

    for (int index = 1; index < argc; ++index) {
        const char *argument = argv[index];

        if (strcmp(argument, "--debug-overlay") == 0) {
            options.debug_overlay = true;
        } else if (strcmp(argument, "--debug-snapshot") == 0) {
            options.debug_snapshot = true;
        } else if (strcmp(argument, "--debug-capture") == 0) {
            if (index + 1 >= argc) {
                print_usage(argv[0]);
                return 2;
            }

            options.debug_capture = true;
            options.capture_path = argv[++index];
        } else if (strcmp(argument, "--debug-view") == 0) {
            if (index + 1 >= argc) {
                print_usage(argv[0]);
                return 2;
            }

            options.debug_view = argv[++index];
        } else if (strcmp(argument, "--seed") == 0) {
            if (index + 1 >= argc || !parse_unsigned_arg(argv[++index], &options.seed)) {
                print_usage(argv[0]);
                return 2;
            }

            options.seed_set = true;
        } else if (strcmp(argument, "--floor") == 0) {
            if (index + 1 >= argc || !parse_floor_arg(argv[++index], &options.floor_number)) {
                print_usage(argv[0]);
                return 2;
            }

            options.floor_set = true;
        } else if (strcmp(argument, "--help") == 0 || strcmp(argument, "-h") == 0) {
            options.show_help = true;
        } else {
            fprintf(stderr, "Unknown argument: %s\n", argument);
            print_usage(argv[0]);
            return 2;
        }
    }

    if (options.show_help) {
        print_usage(argv[0]);
        return 0;
    }

    if (options.debug_snapshot) {
        SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
        SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);
    }

    Game *game = game_create();
    if (game == NULL) {
        fprintf(stderr, "Failed to start CharyRick.\n");
        return 1;
    }

    unsigned int seed = options.seed_set ? options.seed : 12345u;
    int floor_number = options.floor_set ? options.floor_number : 1;

    if (options.debug_snapshot) {

        if (!game_prepare_debug_run(game, seed, floor_number)) {
            fprintf(stderr, "Failed to prepare debug snapshot.\n");
            game_destroy(game);
            return 1;
        }

        game_set_debug_overlay(game, true);

        if (!game_dump_snapshot(game, stdout)) {
            fprintf(stderr, "Failed to write debug snapshot.\n");
            game_destroy(game);
            return 1;
        }
    } else if (options.debug_capture) {
        if (!game_prepare_debug_view(game, options.debug_view, seed, floor_number)) {
            fprintf(stderr, "Failed to prepare debug view: %s\n", options.debug_view);
            game_destroy(game);
            return 1;
        }

        if (options.debug_overlay) {
            game_set_debug_overlay(game, true);
        }

        if (!game_capture_frame(game, options.capture_path)) {
            fprintf(stderr, "Failed to capture debug frame.\n");
            game_destroy(game);
            return 1;
        }
    } else {
        if (options.debug_overlay) {
            game_set_debug_overlay(game, true);
        }

        game_run(game);
    }

    game_destroy(game);
    return 0;
}
