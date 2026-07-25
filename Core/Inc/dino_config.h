#ifndef DINO_CONFIG_H
#define DINO_CONFIG_H

/* Timing is in milliseconds, distance in LCD columns. Keep the 16x2 geometry
 * fixed unless the board display driver and HUD layout are also adapted. */
#define DINO_TICK_MS 20u
#define DINO_COLUMNS 16u
#define DINO_OBSTACLES 3u
#define DINO_PLAYER_COLUMN 1
#define DINO_JUMP_MS 680u
#define DINO_JUMP_BUFFER_MS 120u
#define DINO_SCROLL_START_MS 180u
#define DINO_SCROLL_MIN_MS 90u
#define DINO_SPEED_STEP_MS 10u
#define DINO_LEVEL_POINTS 50u
#define DINO_BIRD_POINTS 50u
#define DINO_SPAWN_GAP_MIN 9u
#define DINO_SPAWN_GAP_MAX 12u
#define DINO_MAX_SCORE 99999u

/* Reject settings that could skip columns, exhaust the obstacle pool, or
 * create a course whose gaps are shorter than the jump and landing tick. */
_Static_assert(DINO_TICK_MS <= DINO_SCROLL_MIN_MS, "Tick must not skip columns");
_Static_assert(DINO_SCROLL_START_MS >= DINO_SCROLL_MIN_MS, "Invalid speed range");
_Static_assert(DINO_SPEED_STEP_MS > 0u && DINO_LEVEL_POINTS > 0u, "Invalid difficulty step");
_Static_assert((DINO_SCROLL_START_MS-DINO_SCROLL_MIN_MS) % DINO_SPEED_STEP_MS == 0u,
               "Speed range must contain whole difficulty steps");
_Static_assert(DINO_JUMP_MS % DINO_TICK_MS == 0u, "Jump must contain whole ticks");
_Static_assert(DINO_SPAWN_GAP_MAX >= DINO_SPAWN_GAP_MIN, "Invalid spawn range");
_Static_assert(DINO_SPAWN_GAP_MIN * DINO_SCROLL_MIN_MS > DINO_JUMP_MS + DINO_TICK_MS,
               "Minimum gap must allow a complete jump and landing");
_Static_assert(DINO_OBSTACLES * DINO_SPAWN_GAP_MIN > DINO_COLUMNS,
               "Obstacle pool is too small for the visible course");
_Static_assert(DINO_MAX_SCORE <= 99999u, "LCD score has five digits");
#endif
