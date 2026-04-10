# Architecture

The initial implementation is organized around a small set of modules:

- `renderer` handles SDL2_ttf setup, font lookup, glyph caching, and character drawing.
- `world` stores and generates the dungeon grid.
- `entity` defines Rick and enemy stats.
- `input` converts SDL events into a compact frame input state.
- `game` owns the window, the main loop, menu state, combat, and floor transitions.

## Design goals

- Keep the entire visible game as text / characters.
- Keep the window fixed-size and easy to port.
- Avoid mixing menu logic with combat logic where possible.
- Stay small enough that the first iteration is easy to debug.

## First playable loop

1. Start at the main menu.
2. Begin a run as Rick.
3. Move through a dungeon made of character tiles.
4. Fight enemies by moving into them.
5. Reach the stairs to generate a new floor.
