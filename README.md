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
| Remove DirectPlay | ✅ Done | Hidden under the `NODPLAY` definition (lobby works, but the gameplay was not tested) |
| Migrate Music system (MCI CD audio) to SDL | ✅ Done ||
| Remove Windows-specific symbols and types | ⏳ In Progress | Things left to be migrated: DLL loading, FS management, registry management, misc. |
| Migrate window management from Win32 to SDL | ✅ Done | DirectPlay still seems to need an HWND pointer |
| Migrate from Winsock to cross-platform sockets | 🚧 Not Started ||
| Remove or replace binary dependencies | 🚧 Not Started | Known so far: `Pinger.lib`, `unrar.dll`, `gw_server.dll`, `dplayx.lib` (when `NODPLAY` is not defined) |
| Recreate the AI | ⏳ In Progress ||
| Rewrite assembler code to C++ | 🚧 Not Started | Over 200 places with inline assembly 😢, related mostly to rendering |

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
