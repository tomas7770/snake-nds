#include <nf_lib.h>
#include <filesystem.h>
#include <stdlib.h>
#include <time.h>

#include "global.h"
#include "title.h"
#include "game.h"

void load_highscores() {
    // This should load high scores from saved data, but currently it's WIP, always inits to 0
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < NUM_DIFFICULTIES; j++) {
            high_scores[i][j].score = 0;
            high_scores[i][j].stars = 0;
        }
    }
}

void tick() {
    scanKeys();
    switch (state)
    {
    case STATE_TITLE:
        tick_title();
        break;
    case STATE_GAME:
        tick_game();
        break;
    default:
        break;
    }
}

int main() {
    srand(time(NULL));
    NF_InitTiledBgBuffers();
    NF_InitSpriteBuffers();

    nitroFSInit(NULL);
    NF_SetRootFolder("NITROFS");

    load_highscores();
    init_title();

    while (1) {
        tick();
        swiWaitForVBlank();
        oamUpdate(&oamMain);
    }

    return 0;
}
