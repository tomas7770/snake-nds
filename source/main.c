#include <nf_lib.h>
#include <filesystem.h>
#include <stdlib.h>
#include <time.h>
#include <fat.h>
#include <stdio.h>
#include <string.h>

#include "global.h"
#include "title.h"
#include "game.h"

void load_highscores(char* game_path) {
    // Initialize to 0
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < NUM_DIFFICULTIES; j++) {
            high_scores[i][j].score = 0;
            high_scores[i][j].stars = 0;
        }
    }

    // Attempt to load from save
    // But first, find save path
    if (game_path) {
        char* extension = strrchr(game_path, '.');
        if (extension && !strcmp(extension, ".nds")) {
            save_path = (char*) malloc(sizeof(char)*(strlen(game_path) + 1));
            strcpy(save_path, game_path);
            char* save_extension = strrchr(save_path, '.');
            strcpy(save_extension, ".sav");
        }
        else {
            int path_len = strlen(game_path);
            save_path = (char*) malloc(sizeof(char)*(path_len+4 + 1));
            strcpy(save_path, game_path);
            strcpy(save_path + path_len, ".sav");
        }
    }
    else {
        save_path = "fat:/snake-nds.sav";
    }

    FILE* save_file = fopen(save_path, "rb");
    if (!save_file)
        return;

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < NUM_DIFFICULTIES; j++) {
            u16 score, stars;
            fread(&score, sizeof(u16), 1, save_file);
            fread(&stars, sizeof(u16), 1, save_file);
            high_scores[i][j].score = (int) score;
            high_scores[i][j].stars = (int) stars;
        }
    }
    fclose(save_file);
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

int main(int argc, char* argv[]) {
    srand(time(NULL));
    NF_InitTiledBgBuffers();
    NF_InitSpriteBuffers();

    nitroFSInit(NULL);
    NF_SetRootFolder("NITROFS");

    if (argc)
        load_highscores(argv[0]);
    else
        load_highscores(NULL);

    init_title();

    while (1) {
        tick();
        swiWaitForVBlank();
        oamUpdate(&oamMain);
    }

    return 0;
}
