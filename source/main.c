#include <nf_lib.h>
#include <filesystem.h>
#include <stdlib.h>
#include <time.h>

#include "global.h"
#include "title.h"
#include "game.h"

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

    init_title();

    while (1) {
        tick();
        swiWaitForVBlank();
        oamUpdate(&oamMain);
    }

    return 0;
}
