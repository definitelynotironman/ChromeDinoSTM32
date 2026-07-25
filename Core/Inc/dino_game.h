#ifndef DINO_GAME_H
#define DINO_GAME_H
#include "dino_config.h"
#include <stdbool.h>
#include <stdint.h>

/* Each object owns its state. The engine uses no GPIO, heap or global clock;
 * the application supplies debounced input and a fixed 20 ms time step. */

enum { DINO_JUMP = 1u, DINO_DUCK = 2u, DINO_PAUSE = 4u, DINO_HOME = 8u };
typedef enum { DINO_READY, DINO_RUNNING, DINO_PAUSED, DINO_GAME_OVER } DinoState;
typedef enum { DINO_CACTUS, DINO_BIRD } DinoObstacleKind;
/* CGRAM indices are raw bytes, including zero. LCD frames are not strings. */
typedef enum {
    DINO_SPRITE_RUN_A, DINO_SPRITE_RUN_B, DINO_SPRITE_JUMP,
    DINO_SPRITE_DUCK, DINO_SPRITE_CACTUS, DINO_SPRITE_BIRD_A,
    DINO_SPRITE_BIRD_B, DINO_SPRITE_CRASH
} DinoSprite;
typedef struct {
    int8_t column;
    DinoObstacleKind kind;
    bool active;
} DinoObstacle;
typedef struct {
    DinoState state;
    DinoObstacle obstacles[DINO_OBSTACLES];
    uint32_t random;
    uint32_t score;
    uint32_t best;
    uint16_t scroll_ms;
    uint16_t jump_ms;
    uint16_t jump_buffer_ms;
    uint16_t animation_ms;
    uint8_t spawn_steps;
    bool ducking;
} DinoGame;
void dino_game_init(DinoGame *game, uint32_t seed);
/* pressed contains new edges; held contains the current debounced levels. */
void dino_game_tick(DinoGame *game, uint8_t pressed, uint8_t held);
void dino_game_render(const DinoGame *game, uint8_t frame[2][DINO_COLUMNS]);
uint16_t dino_game_scroll_period(const DinoGame *game);
#endif
