#include <nf_lib.h>
#include <nds.h>
#include "title.h"
#include "global.h"
#include "game.h"

#define TEXT_LAYER 1
#define TITLE_LAYER 0

#define SPRITE_ARROW 0
#define PALETTE_ARROW 0

#define WRAP_TEXT " Wrap around screen edges"
#define NO_WRAP_TEXT "Collide with screen edges"

const char title_screen = 0;

Difficulty selected_difficulty = NORMAL;
bool selected_wrap = true;

// Wave effect variables
s16 bgx[192];       // Horizontal scroll of each line
s8 i[192];          // Scroll speed of each line

// Function that runs after a scanline is drawn. By modifying the values of the
// scroll registers it's possible to add a wave effect.
void hblank_handler()
{
    u32 vline = REG_VCOUNT; // Get the current line

    if (vline >= 20 && vline <= 50)
    {
        // If this is a line inside the desired screen area, handle the effect
        bgx[vline] += i[vline];

        if ((bgx[vline] < 1) || (bgx[vline] > 63))
            i[vline] *= -1;

        NF_ScrollBg(title_screen, TITLE_LAYER, (bgx[vline] / 8) - 4, 0);
    }
}

void init_wave_effect() {
    // Initialize the wave effect
    s8 inc = 1;
    int x = 0;

    for (int y = 0; y < 192; y++)
    {
        x += inc;
        bgx[y] = x;

        if ((x < 1) || (x > 63))
            inc *= -1;

        i[y] = inc;
    }

    // Register a function to be called after a screen line has been drawn (when
    // the horizontal blanking period starts)
    irqSet(IRQ_HBLANK, hblank_handler);
    irqEnable(IRQ_HBLANK);
}

// Update position of difficulty selection arrow
void update_arrow_position() {
    NF_MoveSprite(title_screen, SPRITE_ARROW, 11*8, (10 + selected_difficulty)*8);
    NF_SpriteOamSet(title_screen);
}

void update_wrap_text() {
    if (selected_wrap) {
        NF_WriteText(title_screen, TEXT_LAYER, 3, 18, WRAP_TEXT);
    }
    else {
        NF_WriteText(title_screen, TEXT_LAYER, 3, 18, NO_WRAP_TEXT);
    }
    NF_UpdateTextLayers();
}

void init_title() {
    state = STATE_TITLE;
    NF_ResetTiledBgBuffers();
    NF_ResetSpriteBuffers();

    NF_Set2D(title_screen, 0);
    // Init background and text
    NF_InitTiledBgSys(title_screen);
    NF_InitTextSys(title_screen);
    NF_LoadTextFont("fnt/default", "font", 256, 256, 0);
    NF_CreateTextLayer(title_screen, TEXT_LAYER, 0, "font");

    NF_WriteText(title_screen, TEXT_LAYER, 12, 10, "Very slow");
    NF_WriteText(title_screen, TEXT_LAYER, 14, 11, "Slow");
    NF_WriteText(title_screen, TEXT_LAYER, 13, 12, "Normal");
    NF_WriteText(title_screen, TEXT_LAYER, 14, 13, "Fast");
    NF_WriteText(title_screen, TEXT_LAYER, 12, 14, "Very fast");

    NF_WriteText(title_screen, TEXT_LAYER, 12, 17, "<Select>");
    update_wrap_text();

    NF_LoadTiledBg("title", "title", 256, 256);
    NF_CreateTiledBg(title_screen, TITLE_LAYER, "title");

    // Init sprites
    NF_InitSpriteSys(title_screen);
    NF_LoadSpriteGfx("arrow", SPRITE_ARROW, 8, 8);
    NF_LoadSpritePal("arrow", PALETTE_ARROW);
    NF_VramSpriteGfx(title_screen, SPRITE_ARROW, SPRITE_ARROW, false);
    NF_VramSpritePal(title_screen, PALETTE_ARROW, PALETTE_ARROW);
    NF_CreateSprite(title_screen, SPRITE_ARROW, SPRITE_ARROW, PALETTE_ARROW, 0, 0);

    update_arrow_position();

    init_wave_effect();
}

void tick_title() {
    int keys = keysDown();
    if ((keys & KEY_UP) && selected_difficulty > 0) {
        selected_difficulty--;
        update_arrow_position();
    }
    else if ((keys & KEY_DOWN) && selected_difficulty < NUM_DIFFICULTIES-1) {
        selected_difficulty++;
        update_arrow_position();
    }
    if (keys & KEY_SELECT) {
        selected_wrap = !selected_wrap;
        update_wrap_text();
    }
    if (keys & (KEY_A | KEY_START)) {
        irqDisable(IRQ_HBLANK);
        irqClear(IRQ_HBLANK);
        init_game(selected_difficulty, selected_wrap, 0);
    }
}
