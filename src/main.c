#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#if __EMSCRIPTEN__
    #include <emscripten.h>
    #include <SDL/SDL_mixer.h>
#else
    #include <SDL2/SDL_mixer.h>
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
const unsigned int MAX_BULLETS = 128; // max. amounf of simultaneous sprites
const unsigned int BULLET_SPEED = 480 ; // pixels/second

const unsigned int MAX_ALIENS = 128; // max. amount of simultaneous sprites
const unsigned int MAX_ALIEN_SPEED_X = 240; // pixels/second
const unsigned int MAX_ALIEN_SPEED_Y = 480; // pixels/second
const unsigned int MIN_ALIEN_SPEED_Y = 180; // pixels/second
const unsigned int ALIEN_ANIM_HZ = 3; // times/second

const unsigned int MAX_EXPLOSIONS = 128; // max. amount of simultaneous sprites
const float EXPLOSION_LIFESPAN = 1.0;

typedef enum {
    IMG_BACKGROUND,
    IMG_SHIP,
    IMG_BULLET,
    IMG_ALIEN,
    IMG_EXPLOSION,
    MAX_IMAGE_INDEX
} ImageIndex;

typedef enum {
    SFX_BOOM,
    SFX_SHOOT,
    MAX_SFX_INDEX
} SfxIndex;

// =============================================================================
// TYPE DEFS
// =============================================================================

typedef struct sprite_t {
    int x;
    int y;
    bool alive;
    SDL_Surface *image;
    int n_frames;
    int frame_index;
} Sprite;

typedef struct ship_t {
    Sprite sprite;
    bool wasSpaceDown;
} Ship;

typedef struct bullet_t {
    Sprite sprite;
} Bullet;

typedef struct alien_t {
    Sprite sprite;
    float timestamp;
    int speed_x;
    int speed_y;
} Alien;

typedef struct explosion_t {
    Sprite sprite;
    float lifespan;
    float timestamp;
} Explosion;


// =============================================================================
// GLOBALS
// =============================================================================

SDL_Window *g_window = NULL;
SDL_Surface *g_screen = NULL;

unsigned int g_last_timestamp = 0;

SDL_Surface *g_images[MAX_IMAGE_INDEX] = {NULL};
Mix_Chunk *g_sfx[MAX_SFX_INDEX] = {NULL};

Ship g_ship;
Bullet g_bullets[MAX_BULLETS];
Alien g_aliens[MAX_ALIENS];
Explosion g_explosions[MAX_EXPLOSIONS];

// =============================================================================
// UTILS
// =============================================================================

int rand_between(int min, int max) {
    // NOTE: this is not a good random generator, but for this game it's enough
    return (rand() % (max + 1 - min)) + min;
}

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

    // for BMP, mask transparent color 0xff00ff
    if (strstr(filename, ".bmp") != NULL) {
        Uint32 color_key = SDL_MapRGB(optimized->format, 0xFF, 0, 0xFF);
        SDL_SetColorKey(optimized, SDL_TRUE, color_key);
    }

    return optimized;
}

Mix_Chunk* load_sfx(char *filename) {
    Mix_Chunk *sfx = Mix_LoadWAV(filename);
    if (sfx == NULL) {
        fprintf(stderr, "Unable to load sfx %s - %s", filename, SDL_GetError());
    }

    return sfx;
}

void draw_image(SDL_Surface *image, int x, int y) {
    SDL_Rect rect = {x, y, image->w, image->h};
    SDL_BlitSurface(image, NULL, g_screen, &rect);
}

void draw_image_frame(SDL_Surface *image, int x, int y, int n_frames, int i) {
    // assume spritesheets are horizontal strips only
    int frame_w = image->w / n_frames;
    SDL_Rect screen_rect = {x, y, frame_w, image->h};
    SDL_Rect image_rect = {i * frame_w, 0, frame_w, image->h};
    SDL_BlitSurface(image, &image_rect, g_screen, &screen_rect);
}

void play_sfx(Mix_Chunk *sfx) {
    Mix_PlayChannel(-1, sfx, 0);
}

// =============================================================================
// SPRITE UTILS
// =============================================================================
#define S_WIDTH(s) ((s->n_frames > 1) ? s->image->w/s->n_frames : s->image->w)

void init_sprite(Sprite *sprite) {
    sprite->x = 0;
    sprite->y = 0;
    sprite->image = NULL;
    sprite->alive = FALSE;
    sprite->n_frames = 1;
    sprite->frame_index = 0;
}

void draw_sprite(Sprite *sprite) {
    if (sprite->image == NULL || !sprite->alive) return;

    int x = sprite->x - S_WIDTH(sprite) / 2;
    int y = sprite->y - sprite->image->h / 2;

    if (sprite->n_frames > 1) {
        draw_image_frame(
            sprite->image, x, y,
            sprite->n_frames, sprite->frame_index);
    }
    else {
        draw_image(sprite->image, x, y);
    }
}


bool sprite_intersect(Sprite *a, Sprite *b) {
    int a_width = S_WIDTH(a);
    int a_min_x = a->x - a_width / 2;
    int a_max_x = a->x + a_width / 2;
    int a_min_y = a->y - a->image->h / 2;
    int a_max_y = a->y + a->image->h / 2;

    int b_width = S_WIDTH(b);
    int b_min_x = b->x - b_width / 2;
    int b_max_x = b->x + b_width / 2;
    int b_min_y = b->y - b->image->h / 2;
    int b_max_y = b->y + b->image->h / 2;

    return (a_min_x <= b_max_x && a_max_x >= b_min_x) &&
           (a_min_y <= b_max_y && a_max_y >= b_min_y);
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

void shoot_bullet(int x, int y) {
    Bullet *bullet = NULL;
    // get a free sprite in the group
    for (unsigned int i = 0; i < MAX_BULLETS; i++) {
        if (!(g_bullets[i].sprite.alive)) {
            bullet = &(g_bullets[i]);
            break;
        }
    }

    if (bullet != NULL) {
        spawn_bullet(bullet, x, y);
    }
    else {
        fprintf(stderr, "ERROR. Could not find a free Bullet sprite\n");
    }
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
// SHIP
// =============================================================================

void spawn_ship(Ship *ship, int x, int y, SDL_Surface *image) {
    init_sprite(&(ship->sprite));

    ship->sprite.x = x;
    ship->sprite.y = y;
    ship->sprite.image = image;
    ship->sprite.alive = TRUE;

    ship->wasSpaceDown = FALSE;
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

    // shoot
    if (!ship->wasSpaceDown && keyboard[SDL_SCANCODE_SPACE]) {
        shoot_bullet(ship->sprite.x, ship->sprite.y);
        play_sfx(g_sfx[SFX_SHOOT]);
    }

    ship->wasSpaceDown = keyboard[SDL_SCANCODE_SPACE];
}

// =============================================================================
// ALIENS
// =============================================================================

void spawn_alien(Alien *alien, int x, int y, SDL_Surface *image) {
    init_sprite(&(alien->sprite));

    alien->sprite.x = x;
    alien->sprite.y = y;
    alien->sprite.image = image;
    alien->sprite.alive = TRUE;
    alien->sprite.n_frames = 2;

    alien->timestamp = 0;
    alien->speed_x = rand_between(-MAX_ALIEN_SPEED_X, MAX_ALIEN_SPEED_X);
    alien->speed_y = rand_between(MIN_ALIEN_SPEED_Y, MAX_ALIEN_SPEED_Y);
}

void generate_alien(int x, int y) {
    Alien *alien = NULL;
    // get a free sprite in the group
    for (unsigned int i = 0; i < MAX_BULLETS; i++) {
        if (!(g_aliens[i].sprite.alive)) {
            alien = &(g_aliens[i]);
            break;
        }
    }

    if (alien != NULL) {
        spawn_alien(alien, x, y, g_images[IMG_ALIEN]);
    }
    else {
        fprintf(stderr, "ERROR. Could not find a free Alien sprite\n");
    }
}

void update_alien(Alien *alien, const float delta) {
    if (!alien->sprite.alive) return;

    // animate
    alien->timestamp += delta;
    if (alien->timestamp >= 1.0 / ALIEN_ANIM_HZ) {
        alien->sprite.frame_index =
            (alien->sprite.frame_index + 1) % alien->sprite.n_frames;
        alien->timestamp -= 1.0 / ALIEN_ANIM_HZ;
    }

    // move
    alien->sprite.x += alien->speed_x * delta;
    alien->sprite.y += alien->speed_y * delta;

    // kill when out of screen
    if (alien->sprite.y > SCREEN_HEIGHT + alien->sprite.image->h) {
        alien->sprite.alive = FALSE;
    }
}

// =============================================================================
// EXPLOSIONS
// =============================================================================

void spawn_explosion(Explosion *explosion, int x, int y, SDL_Surface *image) {
    init_sprite(&(explosion->sprite));

    explosion->sprite.x = x;
    explosion->sprite.y = y;
    explosion->sprite.image = image;
    explosion->sprite.alive = TRUE;
    explosion->sprite.n_frames = 12;

    explosion->timestamp = 0;
    explosion->lifespan = 0;
}

void update_explosion(Explosion *explosion, const float delta) {
    if (!explosion->sprite.alive) return;

    // animate
    explosion->timestamp += delta;
    if (explosion->timestamp >= EXPLOSION_LIFESPAN/explosion->sprite.n_frames) {
        explosion->sprite.frame_index += 1;
        if (explosion->sprite.frame_index >= explosion->sprite.n_frames) {
            explosion->sprite.alive = FALSE;
        }
    }
}

void explode(int x, int y) {
    Explosion *explosion = NULL;
    // get a free sprite in the group
    for (unsigned int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (!(g_explosions[i].sprite.alive)) {
            explosion = &(g_explosions[i]);
            break;
        }
    }

    if (explosion != NULL) {
        spawn_explosion(explosion, x, y, g_images[IMG_EXPLOSION]);
    }
    else {
        fprintf(stderr, "ERROR. Could not find a free Explosion sprite\n");
    }
}

// =============================================================================
// COLLISIONS
// =============================================================================

void collide_bullets_vs_aliens() {
    Alien *alien = NULL;
    Bullet *bullet = NULL;

    for (int i = 0; i < MAX_BULLETS; i++) {
        // check for alive
        bullet = &(g_bullets[i]);
        if (!bullet->sprite.alive) continue;

        for (int j = 0; j < MAX_ALIENS; j++) {
            alien = &(g_aliens[j]);
            if (!alien->sprite.alive) continue;

            if (sprite_intersect(&(bullet->sprite), &(alien->sprite))) {
                explode(alien->sprite.x, alien->sprite.y);
                play_sfx(g_sfx[SFX_BOOM]);

                alien->sprite.alive = FALSE;
                bullet->sprite.alive = FALSE;
            }
        }
    }
}

// =============================================================================
// PLAY STATE
// =============================================================================

void play_init() {
    // load assets
    g_images[IMG_BACKGROUND] = load_image("assets/images/background.png");
    // g_images[IMG_SHIP] = load_image("assets/images/captain.png");
    g_images[IMG_SHIP] = load_image("assets/images/ship.bmp");
    g_images[IMG_BULLET] = load_image("assets/images/laser.png");
    g_images[IMG_ALIEN] = load_image("assets/images/alien.bmp");
    g_images[IMG_EXPLOSION] = load_image("assets/images/explosion.png");

    g_sfx[SFX_SHOOT] = load_sfx("assets/audio/shoot.wav");
    g_sfx[SFX_BOOM] = load_sfx("assets/audio/explosion.wav");

    // init sprite lists
    for (unsigned int i = 0; i < MAX_BULLETS; i++) {
        init_sprite(&(g_bullets[i].sprite));
    }
    for (unsigned int i = 0; i < MAX_ALIENS; i++) {
        init_sprite(&(g_aliens[i].sprite));
    }
}

void play_create() {
    spawn_ship(&g_ship, SCREEN_WIDTH/2, 500, g_images[IMG_SHIP]);
    spawn_alien(&(g_aliens[0]), 100, 100, g_images[IMG_ALIEN]);
}

void play_render() {
    // background
    draw_image(g_images[IMG_BACKGROUND], 0, 0);
    // bullets
    for (unsigned int i = 0; i < MAX_BULLETS; i++) {
        draw_sprite(&(g_bullets[i].sprite));
    }
    // aliens
    for (unsigned int i = 0; i < MAX_ALIENS; i++) {
        draw_sprite(&(g_aliens[i].sprite));
    }
    // explosions
    for (unsigned int i = 0; i < MAX_EXPLOSIONS; i++) {
        draw_sprite(&(g_explosions[i].sprite));
    }
    // ship
    draw_sprite(&(g_ship.sprite));
}

// returns TRUE if the state must end
bool play_update(float delta) {
    const Uint8 *keyboard = SDL_GetKeyboardState(NULL);

    if (keyboard[SDL_SCANCODE_ESCAPE]) {
        return TRUE;
    }

    // randomly spawn new aliens
    if (rand_between(0, 100) < 10) {
        generate_alien(rand_between(0, SCREEN_WIDTH), -50);
    };

    update_ship(&g_ship, delta, keyboard);
    for (unsigned int i = 0; i < MAX_BULLETS; i++) {
        update_bullet(&(g_bullets[i]), delta);
    }
    for (unsigned int i = 0; i < MAX_ALIENS; i++) {
        update_alien(&(g_aliens[i]), delta);
    }
    for (unsigned int i = 0; i < MAX_EXPLOSIONS; i++) {
        update_explosion(&(g_explosions[i]), delta);
    }

    collide_bullets_vs_aliens();

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

    // unload sfx
    for (int i = 0; i < MAX_SFX_INDEX; i++) {
        if (g_sfx[i] != NULL) {
            Mix_FreeChunk(g_sfx[i]);
            g_sfx[i] = NULL;
        }
    }
}


// =============================================================================
// MAIN
// =============================================================================

bool startup(char *title) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        return FALSE;
    }
    else {
         // initi PNG loading
        if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
            fprintf(stderr, "SDL_image init error: %s\n", IMG_GetError());
            return FALSE;
        } // init SDL mixer
        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
            fprintf(stderr, "SDL_mixer init error %s\n", Mix_GetError());
            return FALSE;
        }

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
    srand(time(NULL));

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
