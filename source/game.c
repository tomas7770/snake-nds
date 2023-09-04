#include <nf_lib.h>
#include <stdlib.h>
#include "nf_extra.h"
#include "game.h"

// Snake size doesn't include head
#define INITIAL_SNAKE_SIZE 2
#define MAX_SCORE 255
#define MAX_SNAKE_TILES (INITIAL_SNAKE_SIZE + MAX_SCORE)
// Snake head must start with some margin from the borders
// Ideally > INITIAL_SNAKE_SIZE, or else some code changes will be necessary
#define RANDOM_SNAKE_MARGIN 5

#define BG_LAYER 1
#define FG_LAYER 0

#define SPRITE_HEAD 0
#define PALETTE_HEAD 0

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
    TILE_INVISIBLE,
    TILE_BODY_HOR,
    TILE_BODY_VER,
    TILE_BODY_CORNER,
    TILE_TAIL_HOR,
    TILE_TAIL_VER,
} TileId;

typedef enum {
    TILEDIR_HORIZONTAL,
    TILEDIR_VERTICAL,
    TILEDIR_LEFTUP,
    TILEDIR_RIGHTUP,
    TILEDIR_LEFTDOWN,
    TILEDIR_RIGHTDOWN,
} TileDir;

typedef struct {
    bool valid;
    char x;
    char y;
    SnakeDir forward_dir;
} SnakeTile;

const char game_screen = 0;

// Grid size in 8x8 pixel cells
const char grid_width = 20;
const char grid_height = 17;

// For each difficulty level, snake moves every X frames
const char spd_table[NUM_DIFFICULTIES] = {15, 12, 10, 8, 6};

Difficulty difficulty;
int score;
// Snake tiles don't include head
SnakeTile snake_tiles[MAX_SNAKE_TILES];
int snake_tile_i;
// Snake head position
char head_x, head_y;
SnakeDir direction, prev_direction;
char spd_counter;
// If move_lock is true, cannot change snake direction
bool move_lock;
// Whether screen wrap is enabled or not
bool wrap;

int get_snake_size() {
    return INITIAL_SNAKE_SIZE + score;
}

void set_snake_body_tile(char x, char y, TileDir tile_dir) {
    TileId tile_id = TILE_BODY_HOR;
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
    NF_SetTileOfMap(game_screen, FG_LAYER, x, y, tile_id - TILE_INVISIBLE);
    NF_ForceTileHflip(game_screen, FG_LAYER, x, y, h_flip);
    NF_ForceTileVflip(game_screen, FG_LAYER, x, y, v_flip);
}

void set_snake_tail_tile(char x, char y, SnakeDir forward_dir) {
    TileId tile_id = TILE_TAIL_HOR;
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
    NF_SetTileOfMap(game_screen, FG_LAYER, x, y, tile_id - TILE_INVISIBLE);
    NF_ForceTileHflip(game_screen, FG_LAYER, x, y, h_flip);
    NF_ForceTileVflip(game_screen, FG_LAYER, x, y, v_flip);
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

void rotate_snake_head() {
    bool h_flip = false, v_flip = false;
    switch (direction)
    {
    case DIR_LEFT:
        h_flip = true;
    case DIR_RIGHT:
        NF_SpriteFrame(game_screen, SPRITE_HEAD, 0);
        break;
    case DIR_DOWN:
        v_flip = true;
    case DIR_UP:
        NF_SpriteFrame(game_screen, SPRITE_HEAD, 1);
        break;
    default:
        break;
    }
    NF_HflipSprite(game_screen, SPRITE_HEAD, h_flip);
    NF_VflipSprite(game_screen, SPRITE_HEAD, v_flip);
}

// Init game state
void init_game(Difficulty selected_difficulty, bool selected_wrap) {
    NF_Set2D(game_screen, 0);
    // Init background
    NF_InitTiledBgSys(game_screen);
    NF_LoadTilesForBg("tiles", "BG", 256, 256, TILE_VOID, TILE_VOID);
    NF_LoadTilesForBg("tiles", "FG", 256, 256, TILE_INVISIBLE, TILE_TAIL_VER);
    NF_CreateTiledBg(game_screen, BG_LAYER, "BG");
    NF_CreateTiledBg(game_screen, FG_LAYER, "FG");
    // Init sprites
    NF_InitSpriteSys(game_screen);
    NF_LoadSpriteGfx("head", SPRITE_HEAD, 8, 8);
    NF_LoadSpritePal("head", PALETTE_HEAD);
    NF_VramSpriteGfx(game_screen, SPRITE_HEAD, SPRITE_HEAD, false);
    NF_VramSpritePal(game_screen, PALETTE_HEAD, PALETTE_HEAD);
    NF_CreateSprite(game_screen, SPRITE_HEAD, SPRITE_HEAD, PALETTE_HEAD, 0, 0);

    // Init variables
    difficulty = selected_difficulty;
    score = 0;
    for (int i = INITIAL_SNAKE_SIZE; i < MAX_SNAKE_TILES; i++) {
        snake_tiles[i].valid = false;
    }
    snake_tile_i = 0;
    head_x = RANDOM_SNAKE_MARGIN + rand() % (grid_width-2*RANDOM_SNAKE_MARGIN);
    head_y = RANDOM_SNAKE_MARGIN + rand() % (grid_height-2*RANDOM_SNAKE_MARGIN);
    direction = rand() % NUM_DIRECTIONS;
    prev_direction = direction;
    spd_counter = 0;
    move_lock = false;
    wrap = selected_wrap;

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

    NF_MoveSprite(game_screen, SPRITE_HEAD, head_x*8, head_y*8);
    rotate_snake_head();
    NF_SpriteOamSet(game_screen);
}

void tick_game() {
    int keys = keysHeld();

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

    spd_counter = (spd_counter + 1) % spd_table[difficulty];
    if (!spd_counter) {
        // Move snake

        // Delete tail tile
        if (snake_tiles[snake_tile_i].valid) {
            NF_SetTileOfMap(game_screen, FG_LAYER,
                snake_tiles[snake_tile_i].x, snake_tiles[snake_tile_i].y, TILE_INVISIBLE - TILE_INVISIBLE);
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
            // WIP
        }

        NF_MoveSprite(game_screen, SPRITE_HEAD, head_x*8, head_y*8);
        rotate_snake_head();

        NF_UpdateVramMap(game_screen, FG_LAYER);
        NF_SpriteOamSet(game_screen);
    }
}
