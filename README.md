# space-shooter-wasm

Sample C / Webassembly game.

## Compilation

### Native

These are instructions for Mac OS. In other UNIX environments, it should be similar but changing the way to install SDL itself.

1. Install SDL 2 via [brew](http://brew.sh/): `brew install sdl2`
2. Get the flags we need to compile: `sdl2-config --cflags --libs`
3. Note that Emscripten needs to include SDL with `#include <SDL2/SDL.h>`, so remove the `SDL2` from your include paths.
4. Compile `main.c` with `gcc` and the flags from `sdl2-config`. Example:

```
gcc main.c -I/usr/local/include -D_THREAD_SAFE -L/usr/local/lib -lSDL2 -o main
```

### Emscripten

1. Setup Emscripten as indicated in [its wiki](http://kripken.github.io/emscripten-site/docs/getting_started/downloads.html).
2. Compile `src/main.c` adding the SDL2 and SDL2_image ports:

```
cd src
emcc main.c -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS='["png"]' -o ../dist/index.html -O2 --preload-file assets
```
