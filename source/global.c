#include "global.h"
#include <stdio.h>
#include <nds.h>

State state;

// High scores for each speed level, depending on whether screen wrap is enabled or not
HighScore high_scores[2][NUM_DIFFICULTIES];

char* save_path;

void save_highscores() {
    if (!save_path)
        return;
    
    FILE* save_file = fopen(save_path, "wb");
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < NUM_DIFFICULTIES; j++) {
            u16 score = (u16) (high_scores[i][j].score);
            u16 stars = (u16) (high_scores[i][j].stars);
            fwrite(&score, sizeof(u16), 1, save_file);
            fwrite(&stars, sizeof(u16), 1, save_file);
        }
    }
    fclose(save_file);
}
