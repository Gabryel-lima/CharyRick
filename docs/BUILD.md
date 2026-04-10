# Build

## Linux dependencies

Install the C toolchain, CMake, SDL2, SDL2_ttf, and a monospace font package.

Example on Debian / Ubuntu:

```sh
sudo apt install build-essential cmake libsdl2-dev libsdl2-ttf-dev fonts-dejavu-core
```

## Configure and build

```sh
cmake -S . -B build
cmake --build build
```

## Run

```sh
./build/CharyRick
```

## Debug snapshot

To inspect the generated dungeon and HUD layout without interacting with the SDL window, run:

```sh
./build/CharyRick --debug-snapshot --seed 12345 --floor 2
```

This prints a deterministic text snapshot that the automated smoke test also uses.

## Font lookup

The renderer searches for a monospace font in a few common locations, including:

- `assets/fonts/DejaVuSansMono.ttf`
- `../assets/fonts/DejaVuSansMono.ttf`
- `../../assets/fonts/DejaVuSansMono.ttf`
- common system font paths under `/usr/share/fonts`

If you prefer a bundled font, place it in `assets/fonts/` using one of the
paths above.
