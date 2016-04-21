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

void draw_image(SDL_Surface *image, int x, int y) {
    SDL_Rect rect = {x, y, image->w, image->h};
    SDL_BlitSurface(image, NULL, g_screen, &rect);
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
    draw_image(
        sprite->image,
        sprite->x - sprite->image->w / 2,
        sprite->y - sprite->image->h / 2
    );
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
    draw_image(g_images[IMG_BACKGROUND], 0, 0);
    draw_sprite(&(g_ship.sprite));
}

// returns TRUE if the state must end
bool play_update(const Uint8 *keyboard, unsigned int delta) {
    if (keyboard[SDL_SCANCODE_ESCAPE]) {
        return TRUE;
    }

    return FALSE;
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

        // main loop
        bool shall_quit = FALSE;
        SDL_Event event;
        unsigned int last_timestamp = SDL_GetTicks();
        unsigned int current_timestamp = last_timestamp;

        while (!shall_quit) {
            // handle input
            while (SDL_PollEvent(&event) != 0) {
                shall_quit = event.type == SDL_QUIT;
            }
            const Uint8 *keyboard = SDL_GetKeyboardState(NULL);

            current_timestamp = SDL_GetTicks();
            int delta = current_timestamp - last_timestamp;

            shall_quit = play_update(keyboard, delta);
            play_render();
            SDL_UpdateWindowSurface(g_window);

            last_timestamp = current_timestamp;
        }

        play_cleanup();
    }
    else {
        printf("Could not initialise SDL\n");
    }


    SDL_DestroyWindow(g_window);
    SDL_Quit();
    return 0;
}
