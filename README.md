# CharyRick

CharyRick is a character-based roguelike prototype written in C with SDL2.
The game opens its own window, renders every visible element as text, and uses
a terminal-like menu style without relying on a real terminal.

## Current direction

- Language: C
- Windowing/rendering: SDL2
- Text rendering: SDL2_ttf with monospace font fallback search
- Visual style: ASCII / character grid only
- Main character: Rick

## Build

See [docs/BUILD.md](docs/BUILD.md) for build prerequisites and commands.

## Controls

- Arrow keys or WASD: move Rick / navigate menus
- Enter or Space: confirm / wait in gameplay
- Escape: pause or go back

## Notes

The implementation starts with a minimal, playable scaffold:

- a title screen
- a grid-based dungeon
- Rick as the main playable character
- simple enemies and stairs
- all gameplay elements represented with characters
