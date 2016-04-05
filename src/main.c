#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

typedef enum {
    FALSE = (1 == 0),
    TRUE = (!FALSE)
} bool;

const unsigned int SCREEN_WIDTH = 550;
const unsigned int SCREEN_HEIGHT = 600;

SDL_Window *g_window = NULL;
SDL_Surface *g_screen = NULL;

bool startup(char *title) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        return FALSE;
    }
    else {
        g_window = SDL_CreateWindow(
            title,
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            SCREEN_WIDTH, SCREEN_HEIGHT,
            SDL_WINDOW_SHOWN);
        g_screen = SDL_GetWindowSurface(g_window);
        return TRUE;
    }
}

SDL_Surface* load_image(char *filename) {
    SDL_Surface *optimized = NULL;
    SDL_Surface *raw = IMG_Load(filename);

    if (raw == NULL) {
        fprintf(stderr, "Unable to load image %s - %s\n", filename, SDL_GetError());
    }
    else {
        optimized = SDL_ConvertSurface(raw, g_screen->format, 0);
        if (optimized == NULL) {
            fprintf(stderr, "Unable to optimize image %s - %s", filename, SDL_GetError());
        }
    }
    return optimized;
}

int main (int argc, char **argv) {
    SDL_Surface *screen = NULL;
    SDL_Window *window = NULL;
    SDL_Surface *background = NULL;

    if (startup("Space Shooter")) {
        background = load_image("assets/images/background.png");
        SDL_BlitSurface(background, NULL, g_screen, NULL);
        SDL_UpdateWindowSurface(g_window);

        SDL_Delay(2000);
    }
    else {
        printf("Could not initialise SDL\n");
    }

    SDL_FreeSurface(background);

    SDL_DestroyWindow(g_window);
    SDL_Quit();
    return 0;
}
