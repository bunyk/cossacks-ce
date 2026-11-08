<p align="center">
    <img src="docs/logo.png" alt="Cossacks: Back to War" width="223"/>
</p>

# Cossacks Linux Port

This project is an effort to migrate the classic game **Cossacks: Back to War** to Linux. The goal is to refactor and update the original source code to ensure compatibility with contemporary hardware and software environments while preserving the original gameplay experience.

## Motivation

**Cossacks: Back to War** is one of the greates RTS games ever made.

## Changes
- Some dead code removed
- ASM sections replaced with C++ code

## TODO:
- Missions and campaigns (loading works, 2/106 mission modules ported - see docs/missions.md)
- Networking
- Ticks tick to fast in scenarios (fixed in replay, but made game way to slow).
- Full screen loading is slower than window. Maybe go for maximized window by default?
- Python for AI and missions? 

## Building

### Install some dependencies and tools

```bash
sudo apt install libboost-all-dev
sudo apt-get install cmake
```

Clone SDL3, need to buid it from source

```bash
git clone https://github.com/libsdl-org/SDL.git vendored/SDL
git clone https://github.com/libsdl-org/SDL_image.git vendored/SDL_image
```


### Build
Configure:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
```

Release build is configured without debug symbols:
```
cmake -S . -B build
```

Build

```bash
cmake --build build
```

### Run

In debugger:
```bash
gdb --args build/Debug/cossacks --window
```

Or run directly:
```bash
build/Debug/cossacks --window
```

## Game data

The game data (`gamedata/`, from a legally obtained copy of the game) is
expected in the working directory; the binary is run from the repo root or
`gamedata` itself (see `docs/missions.md` for how the game finds its files).

### Extract mission maps (required for campaigns/tutorials)

Each mission under `gamedata/Missions/` ships its map packed in a
per-mission `.rar` (e.g. `Tutorial/Tutorial1/tutorial1.rar` holds
`tutorial1.m3d`). The original engine extracted these on demand via
`unrar.dll`; this port expects them extracted once as loose files next to
the archive. A missing map fails silently in the engine and shows up as a
black screen on mission start (a `Load3DMap: can't open map` warning is
printed to stderr since the 2026-11 port).

```bash
sudo apt install unrar
find gamedata/Missions -name '*.rar' -execdir unrar x -o+ {} \;
```

## Contributing

Contributions from the community are welcomed! If you are interested in helping with the migration or have any suggestions, please feel free to open an issue or submit a pull request.

## Disclaimer

This project is an unofficial modification of **Cossacks: Back to War** aimed at improving compatibility with modern systems. The original game and its assets are copyrighted by their respective owners.

- This repository does not distribute the original game.
- This project does not grant permission to redistribute, modify, or use the code for commercial purposes.
- The modifications and improvements in this repository are intended only for personal use.
- The modifications provided here are meant to be applied to a legally obtained copy of the game.
- I do not encourage or support any unauthorized distribution of the original game or its source code.
