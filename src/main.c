#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

typedef enum {
    FALSE = (1 == 0),
    TRUE = (!FALSE)
} bool;

const unsigned int SCREEN_WIDTH = 550;
const unsigned int SCREEN_HEIGHT = 600;

bool startup(char *title, SDL_Window **w, SDL_Surface **s) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        return FALSE;
    }
    else {
        *w = SDL_CreateWindow(
            title,
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            SCREEN_WIDTH, SCREEN_HEIGHT,
            SDL_WINDOW_SHOWN);
        *s = SDL_GetWindowSurface(*w);
        return TRUE;
    }
}

SDL_Surface* load_image(char *filename) {
    SDL_Surface *image = IMG_Load(filename);
    return image;
}

int main (int argc, char **argv) {
    SDL_Surface *screen = NULL;
    SDL_Window *window = NULL;
    SDL_Surface *background = NULL;

    if (startup("Space Shooter", &window, &screen)) {
        background = load_image("assets/images/background.png");
        SDL_BlitSurface(background, NULL, screen, NULL);
        SDL_UpdateWindowSurface(window);

        SDL_Delay(2000);
    }
    else {
        printf("Could not initialise SDL\n");
    }

    SDL_FreeSurface(background);

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
