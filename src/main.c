#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

typedef enum {
    FALSE = (1 == 0),
    TRUE = (!FALSE)
} bool;

const unsigned int SCREEN_WIDTH = 550;
const unsigned int SCREEN_HEIGHT = 600;

typedef enum {
    IMG_BACKGROUND,
    IMG_SHIP,
    MAX_IMAGE_INDEX
} ImageIndex;

// =============================================================================
// TYPE DEFS
// =============================================================================

typedef struct sprite_t {
    int x;
    int y;
    bool alive;
    SDL_Surface *image;
} Sprite;

typedef struct ship_t {
    Sprite sprite;
} Ship;


// =============================================================================
// GLOBALS
// =============================================================================

SDL_Window *g_window = NULL;
SDL_Surface *g_screen = NULL;

SDL_Surface *g_images[MAX_IMAGE_INDEX] = {NULL};

Ship g_ship;

// =============================================================================
// UTILS
// =============================================================================

SDL_Surface* load_image(char *filename) {
    SDL_Surface *optimized = NULL;
    SDL_Surface *raw = IMG_Load(filename);

    if (raw == NULL) {
        fprintf(stderr, "Unable to load image %s - %s\n", filename,
            SDL_GetError());
    }
    else {
        optimized = SDL_ConvertSurface(raw, g_screen->format, 0);
        if (optimized == NULL) {
            fprintf(stderr, "Unable to optimize image %s - %s", filename,
                SDL_GetError());
        }
    }
    return optimized;
}

// =============================================================================
// SPRITE UTILS
// =============================================================================

void init_sprite(Sprite *sprite) {
    sprite->x = 0;
    sprite->y = 0;
    sprite->image = NULL;
    sprite->alive = FALSE;
}

void draw_sprite(Sprite *sprite) {
    SDL_Rect rect = {
        sprite->x - (sprite->image->w / 2),
        sprite->y - (sprite->image->h / 2),
        sprite->image->w,
        sprite->image->h
    };
    SDL_BlitSurface(sprite->image, NULL, g_screen, &rect);
}

// =============================================================================
// SHIP
// =============================================================================


void spawn_ship(Ship *ship, int x, int y, SDL_Surface *image) {
    init_sprite(&(ship->sprite));

    ship->sprite.x = x;
    ship->sprite.y = y;
    ship->sprite.image = image;
    ship->sprite.alive = TRUE;
}


// =============================================================================
// PLAY STATE
// =============================================================================

void play_init() {
    // load assets
    g_images[IMG_BACKGROUND] = load_image("assets/images/background.png");
    g_images[IMG_SHIP] = load_image("assets/images/captain.png");
}

void play_create() {
    spawn_ship(&g_ship, SCREEN_WIDTH/2, SCREEN_HEIGHT/2, g_images[IMG_SHIP]);
}

void play_render() {

}

void play_cleanup() {
    // unload images
    for (int i = 0; i < MAX_IMAGE_INDEX; i++) {
        if (g_images[i] != NULL) {
            SDL_FreeSurface(g_images[i]);
            g_images[i] = NULL;
        }
    }
}


// =============================================================================
// MAIN
// =============================================================================

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


int main (int argc, char **argv) {
    SDL_Surface *screen = NULL;
    SDL_Window *window = NULL;
    SDL_Surface *background = NULL;
    SDL_Surface *img_ship = NULL;

    if (startup("Space Shooter")) {
        play_init();
        play_create();

        SDL_BlitSurface(g_images[IMG_BACKGROUND], NULL, g_screen, NULL);
        draw_sprite(&g_ship.sprite);
        SDL_UpdateWindowSurface(g_window);


        SDL_Delay(4000);

        play_cleanup();
    }
    else {
        printf("Could not initialise SDL\n");
    }


    SDL_DestroyWindow(g_window);
    SDL_Quit();
    return 0;
}
