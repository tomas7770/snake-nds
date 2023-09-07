#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#define MAX_STARS 3

typedef enum {
    STATE_GAME,
    STATE_TITLE,
} State;

typedef enum {
    VERY_SLOW,
    SLOW,
    NORMAL,
    FAST,
    VERY_FAST,
    // Not an actual difficulty level
    NUM_DIFFICULTIES,
} Difficulty;

typedef struct {
    int score;
    int stars;
} HighScore;

extern State state;
extern HighScore high_scores[2][NUM_DIFFICULTIES];

#endif
