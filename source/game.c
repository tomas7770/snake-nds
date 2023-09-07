#include <nf_lib.h>
#include <stdlib.h>
#include <stdio.h>
#include "nf_extra.h"
#include "game.h"
#include "title.h"

// Snake size doesn't include head
#define INITIAL_SNAKE_SIZE 2
#define MAX_SCORE 9999
#define MAX_SNAKE_TILES 339
// Snake head must start with some margin from the borders
// Ideally > INITIAL_SNAKE_SIZE, or else some code changes will be necessary
#define RANDOM_SNAKE_MARGIN 5
#define MAX_STARS 3

#define FOOD_ANIM_FRAMES 4
#define FOOD_ANIM_SPD 15

#define END_GAME_BLINK_SPD 12
#define END_GAME_TIME 180

#define BG_LAYER 3
#define FG_LAYER 2
#define WINDOW_LAYER 1
#define TEXT_LAYER 0

typedef enum {
    DIR_LEFT,
    DIR_UP,
    DIR_RIGHT,
    DIR_DOWN,
    // Not an actual direction
    NUM_DIRECTIONS,
} SnakeDir;

typedef enum {
    TILE_VOID,
    TILE_BORDER,
    TILE_BORDER_HOR,
    TILE_BORDER_VER,
    TILE_BORDER_CORNER,
    TILE_INVISIBLE,
    TILE_BODY_HOR,
    TILE_BODY_VER,
    TILE_BODY_CORNER,
    TILE_TAIL_HOR,
    TILE_TAIL_VER,
} TileId;

#define TILEBASE_BODYFAT 5

typedef enum {
    TILEDIR_HORIZONTAL,
    TILEDIR_VERTICAL,
    TILEDIR_LEFTUP,
    TILEDIR_RIGHTUP,
    TILEDIR_LEFTDOWN,
    TILEDIR_RIGHTDOWN,
} TileDir;

typedef enum {
    SPRITE_HEAD,
    SPRITE_FOOD,
    SPRITE_ARROW,
} SpriteId;

typedef enum {
    PALETTE_HEAD,
    PALETTE_FOOD,
    PALETTE_ARROW,
} SprPaletteId;

typedef struct {
    bool valid;
    char x;
    char y;
    SnakeDir forward_dir;
} SnakeTile;

typedef enum {
    PAUSE_CONTINUE,
    PAUSE_QUIT,
    // Not an actual selection
    NUM_PAUSE_SELECTIONS,
} PauseSelection;

const char game_screen = 0;

// Grid size in 8x8 pixel cells
const char grid_width = 20;
const char grid_height = 17;

// For each difficulty level, snake moves every X frames
// Speed increases when a star is gained
const char spd_table[4][NUM_DIFFICULTIES] = {
    {15, 12, 10, 8, 6},
    {13, 10, 8, 6, 5},
    {11, 9, 7, 5, 4},
    {10, 8, 6, 4, 3}
};

// Score needed to reach 1 star, 2 stars, 3 stars, and to reset snake size at 3 stars
const int star_threshold[MAX_STARS+1] = {200, 250, 300, MAX_SNAKE_TILES-INITIAL_SNAKE_SIZE};

Difficulty difficulty;
int score;
int stars;
int snake_size;
// Snake tiles don't include head
SnakeTile snake_tiles[MAX_SNAKE_TILES];
int snake_tile_i;
// Snake head position
char head_x, head_y;
SnakeDir direction, prev_direction;
// Food position
char food_x, food_y;
char food_anim_frame;
char food_anim_counter;
char spd_counter;
// If move_lock is true, cannot change snake direction
bool move_lock;
// Whether screen wrap is enabled or not
bool wrap;

bool paused;
PauseSelection pause_selection;
bool game_over;

bool level_beaten;
char end_game_counter;
bool text_shown;

char score_text_buffer[16];

char get_translated_x(char x) {
    return x + 32/2 - grid_width/2;
}

char get_translated_y(char y) {
    return y + 24/2 - grid_height/2;
}

void build_bg_border() {
    // Quite a mess... might clean it up at some point

    // Top and bottom
    for (char y = 1; y < get_translated_y(0)-1; y++) {
        for (char x = 0; x < 32; x++) {
            NF_SetTileOfMap(game_screen, BG_LAYER, x, y, TILE_BORDER);
        }
    }
    for (char y = get_translated_y(grid_height)+1; y < 24; y++) {
        for (char x = 0; x < 32; x++) {
            NF_SetTileOfMap(game_screen, BG_LAYER, x, y, TILE_BORDER);
        }
    }
    {
        // Top edge
        char y = get_translated_y(0)-1;
        for (char x = 0; x < get_translated_x(0)-1; x++) {
            NF_SetTileOfMap(game_screen, BG_LAYER, x, y, TILE_BORDER);
        }
        NF_SetTileOfMap(game_screen, BG_LAYER, get_translated_x(0)-1, y, TILE_BORDER_CORNER);
        for (char x = get_translated_x(0); x < get_translated_x(grid_width); x++) {
            NF_SetTileOfMap(game_screen, BG_LAYER, x, y, TILE_BORDER_VER);
        }
        NF_SetTileOfMap(game_screen, BG_LAYER, get_translated_x(grid_width), y, TILE_BORDER_CORNER);
        NF_SetTileHflip(game_screen, BG_LAYER, get_translated_x(grid_width), y);
        for (char x = get_translated_x(grid_width)+1; x < 32; x++) {
            NF_SetTileOfMap(game_screen, BG_LAYER, x, y, TILE_BORDER);
        }
        // Bottom edge
        y = get_translated_y(grid_height);
        for (char x = 0; x < get_translated_x(0)-1; x++) {
            NF_SetTileOfMap(game_screen, BG_LAYER, x, y, TILE_BORDER);
            NF_SetTileVflip(game_screen, BG_LAYER, x, y);
        }
        NF_SetTileOfMap(game_screen, BG_LAYER, get_translated_x(0)-1, y, TILE_BORDER_CORNER);
        NF_SetTileVflip(game_screen, BG_LAYER, get_translated_x(0)-1, y);
        for (char x = get_translated_x(0); x < get_translated_x(grid_width); x++) {
            NF_SetTileOfMap(game_screen, BG_LAYER, x, y, TILE_BORDER_VER);
            NF_SetTileVflip(game_screen, BG_LAYER, x, y);
        }
        NF_SetTileOfMap(game_screen, BG_LAYER, get_translated_x(grid_width), y, TILE_BORDER_CORNER);
        NF_SetTileHflip(game_screen, BG_LAYER, get_translated_x(grid_width), y);
        NF_SetTileVflip(game_screen, BG_LAYER, get_translated_x(grid_width), y);
        for (char x = get_translated_x(grid_width)+1; x < 32; x++) {
            NF_SetTileOfMap(game_screen, BG_LAYER, x, y, TILE_BORDER);
        }
    }
    // Middle
    for (char y = get_translated_y(0); y < get_translated_y(grid_height); y++) {
        for (char x = 0; x < get_translated_x(0)-1; x++) {
            NF_SetTileOfMap(game_screen, BG_LAYER, x, y, TILE_BORDER);
        }
        NF_SetTileOfMap(game_screen, BG_LAYER, get_translated_x(0)-1, y, TILE_BORDER_HOR);
        NF_SetTileOfMap(game_screen, BG_LAYER, get_translated_x(grid_width), y, TILE_BORDER_HOR);
        NF_SetTileHflip(game_screen, BG_LAYER, get_translated_x(grid_width), y);
        for (char x = get_translated_x(grid_width)+1; x < 32; x++) {
            NF_SetTileOfMap(game_screen, BG_LAYER, x, y, TILE_BORDER);
        }
    }

    NF_UpdateVramMap(game_screen, BG_LAYER);
}

void init_window() {
    for (char y = 0; y < 24; y++) {
        for (char x = 0; x < 32; x++) {
            NF_SetTileOfMap(game_screen, WINDOW_LAYER, x, y, TILE_INVISIBLE);
        }
    }
    for (char y = get_translated_y(0); y < get_translated_y(grid_height); y++) {
        for (char x = get_translated_x(0); x < get_translated_x(grid_width); x++) {
            NF_SetTileOfMap(game_screen, WINDOW_LAYER, x, y, TILE_VOID);
        }
    }
    NF_HideBg(game_screen, WINDOW_LAYER);
    NF_UpdateVramMap(game_screen, WINDOW_LAYER);
}

int get_snake_size() {
    return snake_size;
}

void set_snake_body_tile(char x, char y, TileDir tile_dir) {
    TileId tile_id = TILE_BODY_HOR;
    int frame = stars >= 3 ? TILEBASE_BODYFAT : 0;
    int h_flip = 0, v_flip = 0;
    switch (tile_dir)
    {
    case TILEDIR_VERTICAL:
        tile_id = TILE_BODY_VER;
        break;
    case TILEDIR_LEFTUP:
        tile_id = TILE_BODY_CORNER;
        h_flip = 1;
        v_flip = 1;
        break;
    case TILEDIR_RIGHTUP:
        tile_id = TILE_BODY_CORNER;
        v_flip = 1;
        break;
    case TILEDIR_LEFTDOWN:
        tile_id = TILE_BODY_CORNER;
        h_flip = 1;
        break;
    case TILEDIR_RIGHTDOWN:
        tile_id = TILE_BODY_CORNER;
        break;
    default:
        break;
    }
    NF_SetTileOfMap(game_screen, FG_LAYER, get_translated_x(x), get_translated_y(y), tile_id + frame - TILE_INVISIBLE);
    NF_ForceTileHflip(game_screen, FG_LAYER, get_translated_x(x), get_translated_y(y), h_flip);
    NF_ForceTileVflip(game_screen, FG_LAYER, get_translated_x(x), get_translated_y(y), v_flip);
}

void set_snake_tail_tile(char x, char y, SnakeDir forward_dir) {
    TileId tile_id = TILE_TAIL_HOR;
    int frame = stars >= 3 ? TILEBASE_BODYFAT : 0;
    int h_flip = 0, v_flip = 0;
    switch (forward_dir)
    {
    case DIR_LEFT:
        h_flip = 1;
        break;
    case DIR_UP:
        v_flip = 1;
    case DIR_DOWN:
        tile_id = TILE_TAIL_VER;
        break;
    default:
        break;
    }
    NF_SetTileOfMap(game_screen, FG_LAYER, get_translated_x(x), get_translated_y(y), tile_id + frame - TILE_INVISIBLE);
    NF_ForceTileHflip(game_screen, FG_LAYER, get_translated_x(x), get_translated_y(y), h_flip);
    NF_ForceTileVflip(game_screen, FG_LAYER, get_translated_x(x), get_translated_y(y), v_flip);
}

void new_snake_tile() {
    snake_tiles[snake_tile_i].valid = true;
    snake_tiles[snake_tile_i].x = head_x;
    snake_tiles[snake_tile_i].y = head_y;
    snake_tiles[snake_tile_i].forward_dir = direction;

    TileDir tile_dir = TILEDIR_HORIZONTAL;
    if ((direction == DIR_UP || direction == DIR_DOWN) && direction == prev_direction)
        tile_dir = TILEDIR_VERTICAL;
    else if ((prev_direction == DIR_LEFT && direction == DIR_UP)
    || (prev_direction == DIR_DOWN && direction == DIR_RIGHT))
        tile_dir = TILEDIR_LEFTUP;
    else if ((prev_direction == DIR_RIGHT && direction == DIR_UP)
    || (prev_direction == DIR_DOWN && direction == DIR_LEFT))
        tile_dir = TILEDIR_RIGHTUP;
    else if ((prev_direction == DIR_LEFT && direction == DIR_DOWN)
    || (prev_direction == DIR_UP && direction == DIR_RIGHT))
        tile_dir = TILEDIR_LEFTDOWN;
    else if ((prev_direction == DIR_RIGHT && direction == DIR_DOWN)
    || (prev_direction == DIR_UP && direction == DIR_LEFT))
        tile_dir = TILEDIR_RIGHTDOWN;
    
    prev_direction = direction;

    set_snake_body_tile(head_x, head_y, tile_dir);
}

void randomize_food_pos() {
    food_x = rand() % grid_width;
    food_y = rand() % grid_height;
}

void update_snake_head() {
    NF_MoveSprite(game_screen, SPRITE_HEAD, get_translated_x(head_x)*8, get_translated_y(head_y)*8);

    int frame = stars >= 3 ? 2 : 0;
    bool h_flip = false, v_flip = false;
    switch (direction)
    {
    case DIR_LEFT:
        h_flip = true;
    case DIR_RIGHT:
        NF_SpriteFrame(game_screen, SPRITE_HEAD, frame + 0);
        break;
    case DIR_DOWN:
        v_flip = true;
    case DIR_UP:
        NF_SpriteFrame(game_screen, SPRITE_HEAD, frame + 1);
        break;
    default:
        break;
    }
    NF_HflipSprite(game_screen, SPRITE_HEAD, h_flip);
    NF_VflipSprite(game_screen, SPRITE_HEAD, v_flip);
}

void update_food_spr() {
    NF_MoveSprite(game_screen, SPRITE_FOOD, get_translated_x(food_x)*8, get_translated_y(food_y)*8);
}

void do_food_anim() {
    if (stars >= 3) {
        food_anim_counter = (food_anim_counter + 1) % FOOD_ANIM_SPD;
        if (!food_anim_counter) {
            food_anim_frame = (food_anim_frame + 1) % FOOD_ANIM_FRAMES;
            NF_SpriteFrame(game_screen, SPRITE_FOOD, stars + food_anim_frame);
        }
        NF_SpriteOamSet(game_screen);
    }
}

void update_pause_arrow_position() {
    NF_MoveSprite(game_screen, SPRITE_ARROW, 12*8, (13 + pause_selection)*8);
    NF_SpriteOamSet(game_screen);
}

void update_level_graphics() {
    // Snake palette
    switch (stars)
    {
    case 1:
        // Yellow/orange snake
        NF_BgSetPalColor(game_screen, FG_LAYER, 5, 26, 21, 0);
        NF_BgSetPalColor(game_screen, FG_LAYER, 6, 31, 27, 0);
        NF_BgSetPalColor(game_screen, FG_LAYER, 7, 31, 15, 0);
        NF_SpriteSetPalColor(game_screen, PALETTE_HEAD, 5, 26, 21, 0);
        NF_SpriteSetPalColor(game_screen, PALETTE_HEAD, 1, 31, 27, 0);
        break;
    case 2:
        // Red snake
        NF_BgSetPalColor(game_screen, FG_LAYER, 5, 26, 6, 6);
        NF_BgSetPalColor(game_screen, FG_LAYER, 6, 31, 7, 7);
        NF_BgSetPalColor(game_screen, FG_LAYER, 7, 31, 13, 0);
        NF_SpriteSetPalColor(game_screen, PALETTE_HEAD, 5, 26, 6, 6);
        NF_SpriteSetPalColor(game_screen, PALETTE_HEAD, 1, 31, 7, 7);
        break;
    default:
        break;
    }

    // Stars
    if (stars >= 1) {
        NF_SetTileOfMap(game_screen, TEXT_LAYER, 11, 0, 114);
        if (stars >= 2) {
            NF_SetTileOfMap(game_screen, TEXT_LAYER, 12, 0, 115);
            if (stars >= 3) {
                NF_SetTileOfMap(game_screen, TEXT_LAYER, 13, 0, 116);
            }
        }
    }
    // Food
    NF_SpriteFrame(game_screen, SPRITE_FOOD, stars);
}

void update_score_text() {
    snprintf(score_text_buffer, sizeof(score_text_buffer), "Score: %d", score);
    NF_WriteText(game_screen, TEXT_LAYER, 0, 0, score_text_buffer);
    NF_UpdateTextLayers();
}

// Init game state
void init_game(Difficulty selected_difficulty, bool selected_wrap, int initial_stars) {
    state = STATE_GAME;
    NF_ResetTiledBgBuffers();
    NF_ResetSpriteBuffers();

    NF_Set2D(game_screen, 0);
    // Init background
    NF_InitTiledBgSys(game_screen);
    NF_InitTextSys(game_screen);
    NF_LoadTilesForBg("tiles", "BG", 256, 256, TILE_VOID, TILE_BORDER_CORNER);
    NF_LoadTilesForBg("tiles", "FG", 256, 256, TILE_INVISIBLE, TILE_TAIL_VER + TILEBASE_BODYFAT);
    NF_LoadTilesForBg("tiles", "Window", 256, 256, TILE_VOID, TILE_INVISIBLE);
    NF_LoadTextFont("fnt/default", "font", 256, 256, 0);
    NF_CreateTiledBg(game_screen, BG_LAYER, "BG");
    NF_CreateTiledBg(game_screen, FG_LAYER, "FG");
    NF_CreateTiledBg(game_screen, WINDOW_LAYER, "Window");
    NF_CreateTextLayer(game_screen, TEXT_LAYER, 0, "font");
    NF_DefineTextColor(game_screen, TEXT_LAYER, 0, 31, 7, 7);
    // Init sprites
    NF_InitSpriteSys(game_screen);
    NF_LoadSpriteGfx("head", SPRITE_HEAD, 8, 8);
    NF_LoadSpritePal("head", PALETTE_HEAD);
    NF_VramSpriteGfx(game_screen, SPRITE_HEAD, SPRITE_HEAD, false);
    NF_VramSpritePal(game_screen, PALETTE_HEAD, PALETTE_HEAD);
    NF_CreateSprite(game_screen, SPRITE_HEAD, SPRITE_HEAD, PALETTE_HEAD, 0, 0);

    NF_LoadSpriteGfx("fruit", SPRITE_FOOD, 8, 8);
    NF_LoadSpritePal("fruit", PALETTE_FOOD);
    NF_VramSpriteGfx(game_screen, SPRITE_FOOD, SPRITE_FOOD, false);
    NF_VramSpritePal(game_screen, PALETTE_FOOD, PALETTE_FOOD);
    NF_CreateSprite(game_screen, SPRITE_FOOD, SPRITE_FOOD, PALETTE_FOOD, 0, 0);

    NF_LoadSpriteGfx("arrow", SPRITE_ARROW, 8, 8);
    NF_LoadSpritePal("arrow", PALETTE_ARROW);
    NF_VramSpriteGfx(game_screen, SPRITE_ARROW, SPRITE_ARROW, false);
    NF_VramSpritePal(game_screen, PALETTE_ARROW, PALETTE_ARROW);
    NF_CreateSprite(game_screen, SPRITE_ARROW, SPRITE_ARROW, PALETTE_ARROW, 0, 0);
    NF_ShowSprite(game_screen, SPRITE_ARROW, false);

    // Init variables
    difficulty = selected_difficulty;
    score = 0;
    stars = initial_stars;
    snake_size = INITIAL_SNAKE_SIZE;
    for (int i = INITIAL_SNAKE_SIZE; i < MAX_SNAKE_TILES; i++) {
        snake_tiles[i].valid = false;
    }
    snake_tile_i = 0;
    head_x = RANDOM_SNAKE_MARGIN + rand() % (grid_width-2*RANDOM_SNAKE_MARGIN);
    head_y = RANDOM_SNAKE_MARGIN + rand() % (grid_height-2*RANDOM_SNAKE_MARGIN);
    direction = rand() % NUM_DIRECTIONS;
    prev_direction = direction;
    randomize_food_pos();
    food_anim_frame = 0;
    food_anim_counter = 0;
    spd_counter = 0;
    move_lock = false;
    wrap = selected_wrap;
    paused = false;
    pause_selection = PAUSE_CONTINUE;
    game_over = false;

    level_beaten = false;
    end_game_counter = 0;
    text_shown = true;

    // Build BG border
    build_bg_border();

    init_window();

    // Set initial snake body tiles
    char x = head_x, y = head_y;
    for (int i = INITIAL_SNAKE_SIZE-1; i >= 0; i--) {
        TileDir tile_dir = TILEDIR_HORIZONTAL;
        switch (direction)
        {
        case DIR_LEFT:
            x++;
            break;
        case DIR_UP:
            tile_dir = TILEDIR_VERTICAL;
            y++;
            break;
        case DIR_RIGHT:
            x--;
            break;
        case DIR_DOWN:
            tile_dir = TILEDIR_VERTICAL;
            y--;
            break;
        default:
            break;
        }

        snake_tiles[i].valid = true;
        snake_tiles[i].x = x;
        snake_tiles[i].y = y;
        snake_tiles[i].forward_dir = direction;

        if (i == 0)
            set_snake_tail_tile(x, y, direction);
        else
            set_snake_body_tile(x, y, tile_dir);
    }
    NF_UpdateVramMap(game_screen, FG_LAYER);

    update_level_graphics();
    update_score_text();

    update_snake_head();
    update_food_spr();
    NF_SpriteOamSet(game_screen);
}

void pause_game() {
    paused = true;

    NF_ShowBg(game_screen, WINDOW_LAYER);
    NF_ShowSprite(game_screen, SPRITE_HEAD, false);
    NF_ShowSprite(game_screen, SPRITE_FOOD, false);
    NF_ShowSprite(game_screen, SPRITE_ARROW, true);
    update_pause_arrow_position();

    NF_WriteText(game_screen, TEXT_LAYER, 11, 11, "Game paused");
    NF_WriteText(game_screen, TEXT_LAYER, 13, 13, "Continue");
    NF_WriteText(game_screen, TEXT_LAYER, 13, 14, "Quit");
    NF_UpdateTextLayers();
}

void unpause_game() {
    paused = false;

    NF_HideBg(game_screen, WINDOW_LAYER);
    NF_ShowSprite(game_screen, SPRITE_HEAD, true);
    NF_ShowSprite(game_screen, SPRITE_FOOD, true);
    NF_ShowSprite(game_screen, SPRITE_ARROW, false);
    NF_SpriteOamSet(game_screen);

    NF_WriteText(game_screen, TEXT_LAYER, 11, 11, "           ");
    NF_WriteText(game_screen, TEXT_LAYER, 13, 13, "        ");
    NF_WriteText(game_screen, TEXT_LAYER, 13, 14, "    ");
    NF_UpdateTextLayers();
}

void do_game_over() {
    game_over = true;

    NF_WriteText(game_screen, TEXT_LAYER, 23, 0, "Game over");
    NF_UpdateTextLayers();
}

void tick_game() {
    int keys = keysHeld();
    int keysPressed = keysDown();

    if (level_beaten) {
        if (!(end_game_counter % END_GAME_BLINK_SPD)) {
            if (text_shown) {
                NF_HideBg(game_screen, TEXT_LAYER);
                text_shown = false;
            }
            else {
                NF_ShowBg(game_screen, TEXT_LAYER);
                text_shown = true;
            }
        }
        if (end_game_counter >= END_GAME_TIME) {
            int old_score = score;
            int old_stars = stars;
            if (++stars > MAX_STARS)
                stars = MAX_STARS;
            init_game(difficulty, wrap, stars);
            if (old_stars == MAX_STARS) {
                score = old_score;
                update_score_text();
            }
        }
        end_game_counter++;
        return;
    }

    if (game_over) {
        if (keysPressed & KEY_START)
            init_title();
        return;
    }

    if (keysPressed & KEY_START) {
        if (paused) {
            unpause_game();
        }
        else {
            pause_game();
            return;
        }
    }

    if (paused) {
        if ((keysPressed & KEY_UP) && pause_selection > 0) {
            pause_selection--;
            update_pause_arrow_position();
        }
        else if ((keysPressed & KEY_DOWN) && pause_selection < NUM_PAUSE_SELECTIONS-1) {
            pause_selection++;
            update_pause_arrow_position();
        }
        if (keys & KEY_A) {
            switch (pause_selection)
            {
            case PAUSE_CONTINUE:
                unpause_game();
                break;
            case PAUSE_QUIT:
                init_title();
                break;
            default:
                break;
            }
        }
        return;
    }

    do_food_anim();

    if (!move_lock) {
        if (direction == DIR_LEFT || direction == DIR_RIGHT) {
            if (keys & KEY_UP) {
                direction = DIR_UP;
                move_lock = true;
            }
            else if (keys & KEY_DOWN) {
                direction = DIR_DOWN;
                move_lock = true;
            }
        }
    }
    if (!move_lock) {
        if (direction == DIR_UP || direction == DIR_DOWN) {
            if (keys & KEY_LEFT) {
                direction = DIR_LEFT;
                move_lock = true;
            }
            else if (keys & KEY_RIGHT) {
                direction = DIR_RIGHT;
                move_lock = true;
            }
        }
    }

    spd_counter = (spd_counter + 1) % spd_table[stars][difficulty];
    if (!spd_counter) {
        // Move snake

        // Delete tail tile
        if (snake_tiles[snake_tile_i].valid) {
            NF_SetTileOfMap(game_screen, FG_LAYER,
                get_translated_x(snake_tiles[snake_tile_i].x),
                get_translated_y(snake_tiles[snake_tile_i].y),
                TILE_INVISIBLE - TILE_INVISIBLE);
        }
        
        // Add new tile at head position
        new_snake_tile();

        // Replace oldest tile with tail
        snake_tile_i = (snake_tile_i + 1) % get_snake_size();
        if (snake_tiles[snake_tile_i].valid) {
            set_snake_tail_tile(snake_tiles[snake_tile_i].x, snake_tiles[snake_tile_i].y,
                snake_tiles[snake_tile_i].forward_dir);
        }

        // Move head
        switch (direction)
        {
            case DIR_UP:
                head_y--;
                break;
            case DIR_DOWN:
                head_y++;
                break;
            case DIR_LEFT:
                head_x--;
                break;
            case DIR_RIGHT:
                head_x++;
                break;
            default:
                break;
        }
        move_lock = false;

        // Wrapping/border collision
        if (wrap) {
            if (head_x == (char) -1)
                head_x = grid_width-1;
            else if (head_x == grid_width)
                head_x = 0;

            if (head_y == (char) -1)
                head_y = grid_height-1;
            else if (head_y == grid_height)
                head_y = 0;
        }
        else if (head_x == (char) -1 || head_x == grid_width || head_y == (char) -1 || head_y == grid_height) {
            do_game_over();
        }

        
        if (!game_over) {
            // Collision with body
            u32 tile_at_head = NF_GetTileOfMap(game_screen, FG_LAYER,
                get_translated_x(head_x), get_translated_y(head_y));
            if (tile_at_head >= TILE_BODY_HOR - TILE_INVISIBLE
            && tile_at_head <= TILE_TAIL_VER + TILEBASE_BODYFAT - TILE_INVISIBLE) {
                do_game_over();
            }
        }
        // Check game over again in case it happened in previous step
        if (!game_over) {
            if (head_x == food_x && head_y == food_y) {
                score++;
                if (score > MAX_SCORE)
                    score = MAX_SCORE;
                snake_size++;
                if (snake_size-INITIAL_SNAKE_SIZE >= star_threshold[stars]) {
                    NF_ShowSprite(game_screen, SPRITE_FOOD, false);
                    level_beaten = true;
                }
                else {
                    randomize_food_pos();
                    update_food_spr();
                }
                update_score_text();
            }
        }

        update_snake_head();

        NF_UpdateVramMap(game_screen, FG_LAYER);
        NF_SpriteOamSet(game_screen);
    }
}
