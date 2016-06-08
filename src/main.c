#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#if __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define MAX(a,b) ((a) > (b) ? a : b)
#define MIN(a,b) ((a) < (b) ? a : b)

typedef enum {
    FALSE = (1 == 0),
    TRUE = (!FALSE)
} bool;

const int SCREEN_WIDTH = 550;
const int SCREEN_HEIGHT = 600;

const unsigned int SHIP_SPEED = 360; // pixels/second
const unsigned int MAX_BULLETS = 256; // max. amounf of simultaneous sprites
const unsigned int BULLET_SPEED = 480 ; // pixels/second

typedef enum {
    IMG_BACKGROUND,
    IMG_SHIP,
    IMG_BULLET,
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

typedef struct bullet_t {
    Sprite sprite;
} Bullet;


// =============================================================================
// GLOBALS
// =============================================================================

SDL_Window *g_window = NULL;
SDL_Surface *g_screen = NULL;

unsigned int g_last_timestamp = 0;

SDL_Surface *g_images[MAX_IMAGE_INDEX] = {NULL};

Ship g_ship;
Bullet g_bullets[MAX_BULLETS];

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
    if (sprite->image == NULL || !sprite->alive) return;

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


void update_ship(Ship *ship, const float delta, const Uint8 *keyboard) {
    // move ship depending on keyboard
    if (keyboard[SDL_SCANCODE_LEFT]) {
        ship->sprite.x -= SHIP_SPEED * delta;
    }
    else if (keyboard[SDL_SCANCODE_RIGHT]) {
        ship->sprite.x += SHIP_SPEED * delta;
    }
    // clamp x position
    ship->sprite.x = MAX(0, MIN(ship->sprite.x, SCREEN_WIDTH));
}

// =============================================================================
// BULLETS
// =============================================================================

void spawn_bullet(Bullet *bullet, int x, int y) {
    init_sprite(&(bullet->sprite));

    bullet->sprite.x = x;
    bullet->sprite.y = y;
    bullet->sprite.image = g_images[IMG_BULLET];
    bullet->sprite.alive = TRUE;
}

void update_bullet(Bullet *bullet, const float delta) {
    if (!bullet->sprite.alive) return;

    // move upwards
    bullet->sprite.y -= BULLET_SPEED * delta;

    // kill sprite if gone off screen
    if (bullet->sprite.y < -bullet->sprite.image->h) {
        bullet->sprite.alive = FALSE;
    }
}

// =============================================================================
// PLAY STATE
// =============================================================================

void play_init() {
    // load assets
    g_images[IMG_BACKGROUND] = load_image("assets/images/background.png");
    g_images[IMG_SHIP] = load_image("assets/images/captain.png");
    g_images[IMG_BULLET] = load_image("assets/images/laser.png");

    // init sprite lists
    for (unsigned int i = 0; i < MAX_BULLETS; i++) {
        init_sprite(&(g_bullets[i].sprite));
    }
}

void play_create() {
    spawn_ship(&g_ship, SCREEN_WIDTH/2, 500, g_images[IMG_SHIP]);
    spawn_bullet(&(g_bullets[0]), g_ship.sprite.x, g_ship.sprite.y);
}

void play_render() {
    draw_image(g_images[IMG_BACKGROUND], 0, 0);
    draw_sprite(&(g_ship.sprite));
    for (unsigned int i = 0; i < MAX_BULLETS; i++) {
        draw_sprite(&(g_bullets[i].sprite));
    }
}

// returns TRUE if the state must end
bool play_update(float delta) {
    const Uint8 *keyboard = SDL_GetKeyboardState(NULL);

    if (keyboard[SDL_SCANCODE_ESCAPE]) {
        return TRUE;
    }

    update_ship(&g_ship, delta, keyboard);
    for (unsigned int i = 0; i < MAX_BULLETS; i++) {
        update_bullet(&(g_bullets[i]), delta);
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
            SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
        g_screen = SDL_GetWindowSurface(g_window);
        SDL_GL_SetSwapInterval(1);
        return TRUE;
    }
}

#if __EMSCRIPTEN__
void tick() {
#else
bool tick() {
#endif
    unsigned int current_timestamp = SDL_GetTicks();
    float delta = MIN( // clamp max. delta time to 250ms
        (current_timestamp - g_last_timestamp) / 1000.0, 0.25);

    bool res = play_update(delta);
    play_render();
    SDL_UpdateWindowSurface(g_window);

    g_last_timestamp = current_timestamp;

    #if !__EMSCRIPTEN__
    return res;
    #endif
}

void main_loop() {
    g_last_timestamp = SDL_GetTicks();
#if __EMSCRIPTEN__
    emscripten_set_main_loop(tick, -1, 1);
#else
    bool shall_quit = FALSE;
    SDL_Event event;

    while (!shall_quit) {
        // handle input
        while (SDL_PollEvent(&event) != 0) {
            shall_quit = event.type == SDL_QUIT;
        }

        shall_quit = tick();
    }
#endif
}

int main (int argc, char **argv) {
    SDL_Surface *screen = NULL;
    SDL_Window *window = NULL;

    if (startup("Space Shooter")) {
        play_init();
        play_create();
        main_loop();
        play_cleanup();
    }
    else {
        printf("Could not initialise SDL\n");
    }

    SDL_DestroyWindow(g_window);
    SDL_Quit();
    return 0;
}
