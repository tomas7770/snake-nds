#include <nf_lib.h>
#include <filesystem.h>
#include <stdlib.h>
#include <time.h>

#include "game.h"

void tick() {
    tick_game();
}

int main() {
    srand(time(NULL));
    NF_InitTiledBgBuffers();
    NF_InitSpriteBuffers();

    nitroFSInit(NULL);
    NF_SetRootFolder("NITROFS");

    init_game(NORMAL);

    while (1) {
        tick();
        swiWaitForVBlank();
        oamUpdate(&oamMain);
    }

    return 0;
}
