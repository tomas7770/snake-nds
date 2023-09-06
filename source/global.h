#ifndef __GLOBAL_H__
#define __GLOBAL_H__

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

extern State state;

#endif
