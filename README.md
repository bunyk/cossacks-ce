<p align="center">
    <img src="docs/logo.png" alt="Cossacks: Back to War" width="223"/>
</p>

# Cossacks Linux Port

This project is an effort to migrate the classic game **Cossacks: Back to War** to Linux. The goal is to refactor and update the original source code to ensure compatibility with contemporary hardware and software environments while preserving the original gameplay experience.

## Motivation

**Cossacks: Back to War** is the greatest RTS game ever made, in my opinion.

## Roadmap

Done doesn't mean it's perfect, it just means it works.
The list is not exhaustive and will be updated as the project progresses.

| Description | Status | Comments |
|-------------|--------|----------|
| Remove Windows-specific symbols and types | ⏳ In Progress | Things left to be migrated: DLL loading, FS management, registry management, misc. |
| Migrate from Winsock to cross-platform sockets | 🚧 Not Started ||
| Remove or replace binary dependencies | 🚧 Not Started | Known so far: `Pinger.lib`, `unrar.dll`, `gw_server.dll` |
| Recreate the AI | ⏳ In Progress ||
| Rewrite assembler code to C++ | 🚧 In progress | Over 200 places with inline assembly 😢, related mostly to rendering |

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

Release build is configuredwithout debug symbols:
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
gdb --args build/Debug/cossacks /window
```
The repo-root `.gdbinit` sets `solib-search-path` so gdb finds the AI
modules the game dlopens from its data dir (`gamedata/AI/*.so`); without
it you get "Could not load shared library symbols" warnings and no
symbols for AI-module code. On the first run gdb may decline the
`.gdbinit` for security — allow it once with:
```bash
echo "add-auto-load-safe-path $(pwd)/.gdbinit" >> ~/.config/gdb/gdbinit
```

Or run directly:
```bash
build/Debug/cossacks /window
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
