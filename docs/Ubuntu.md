## How to build on Ubuntu

### Porting


```
Maybe instead of commenting windows specific code, I could:

#ifdef __linux__ 
    //linux code goes here
#elif _WIN32
    // windows code goes here
#endif
```

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
cmake -S . -B build
```

Build

```bash
cmake --build build
```
