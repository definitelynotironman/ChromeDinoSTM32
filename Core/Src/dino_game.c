#include "dino_game.h"
#include <string.h>
#define JUMP_DURATION_MS 680u
#define JUMP_BUFFER_MS 120u
#define MAX_SCORE 99999u

/* Local random state permits repeatable tests without libc's global rand. */
static uint32_t random_next(DinoGame *game)
{
    uint32_t value = game->random;
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    game->random = value;
    return value;
}
void dino_game_init(DinoGame *game, uint32_t seed)
{
    memset(game, 0, sizeof *game);
    game->state = DINO_READY;
    game->random = seed != 0u ? seed : 0x51d1a0u;
}
static void start_run(DinoGame *game)
{
    uint32_t best = game->best;
    uint32_t seed = random_next(game);
    dino_game_init(game, seed);
    game->best = best; /* Retain the record until reset or power removal. */
    game->state = DINO_RUNNING;
    game->spawn_steps = 5u;
}
uint16_t dino_game_scroll_period(const DinoGame *game)
{
    uint32_t level = game->score / 50u;
    if (level > 9u) level = 9u;
    return (uint16_t)(180u - level * 10u);
}
static void check_collisions(DinoGame *game)
{
    for (uint32_t i = 0u; i < DINO_OBSTACLES; ++i) {
        const DinoObstacle *obstacle = &game->obstacles[i];
        if (!obstacle->active || obstacle->column != DINO_PLAYER_COLUMN) continue;
        /* Jump clears either obstacle. Duck only clears a low flying bird. */
        bool safe = game->jump_ms != 0u
                 || (obstacle->kind == DINO_BIRD && game->ducking);
        if (!safe) {
            game->state = DINO_GAME_OVER;
            if (game->score > game->best) game->best = game->score;
            return;
        }
    }
}
static void scroll_world(DinoGame *game)
{
    for (uint32_t i = 0u; i < DINO_OBSTACLES; ++i) {
        DinoObstacle *obstacle = &game->obstacles[i];
        if (obstacle->active && --obstacle->column < 0) obstacle->active = false;
    }
    /* Column spacing remains playable at every speed: even at 90 ms/column,
     * nine columns leave more time than a full 680 ms jump. */
    if (--game->spawn_steps == 0u) {
        for (uint32_t i = 0u; i < DINO_OBSTACLES; ++i) {
            DinoObstacle *obstacle = &game->obstacles[i];
            if (obstacle->active) continue;
            obstacle->active = true;
            obstacle->column = (int8_t)(DINO_COLUMNS - 1u);
            obstacle->kind = game->score >= 50u && (random_next(game) & 3u) == 0u
                           ? DINO_BIRD : DINO_CACTUS;
            break;
        }
        game->spawn_steps = (uint8_t)(9u + random_next(game) % 4u);
    }
    if (game->score < MAX_SCORE) ++game->score;
    if (game->score > game->best) game->best = game->score;
}
void dino_game_tick(DinoGame *game, uint8_t pressed, uint8_t held)
{
    if ((pressed & DINO_HOME) != 0u) {
        game->state = DINO_READY;
        return;
    }
    if (game->state == DINO_READY || game->state == DINO_GAME_OVER) {
        if ((pressed & DINO_JUMP) != 0u) start_run(game);
        return;
    }
    if ((pressed & DINO_PAUSE) != 0u) {
        game->state = game->state == DINO_PAUSED ? DINO_RUNNING : DINO_PAUSED;
        return; /* Freeze jump, animation and scroll together. */
    }
    if (game->state != DINO_RUNNING) return;
    if ((pressed & DINO_JUMP) != 0u) game->jump_buffer_ms = JUMP_BUFFER_MS;
    if (game->jump_ms > DINO_TICK_MS) game->jump_ms -= DINO_TICK_MS;
    else game->jump_ms = 0u;
    game->ducking = (held & DINO_DUCK) != 0u && game->jump_ms == 0u;
    /* Buffer a press just before landing; holding jump never causes repeats. */
    if (game->jump_ms == 0u && game->jump_buffer_ms != 0u
        && (held & DINO_DUCK) == 0u) {
        game->jump_ms = JUMP_DURATION_MS;
        game->jump_buffer_ms = 0u;
        game->ducking = false;
    }
    if (game->jump_buffer_ms > DINO_TICK_MS) game->jump_buffer_ms -= DINO_TICK_MS;
    else game->jump_buffer_ms = 0u;
    game->animation_ms = (uint16_t)((game->animation_ms + DINO_TICK_MS) % 400u);
    game->scroll_ms += DINO_TICK_MS;
    uint16_t period = dino_game_scroll_period(game);
    if (game->scroll_ms >= period) {
        game->scroll_ms -= period;
        scroll_world(game);
    }
    /* Landing on a cactus still collides between column movements. */
    check_collisions(game);
}
static void write_score(uint8_t *out, uint32_t value)
{
    for (uint32_t digits = 5u; digits != 0u; --digits) {
        out[digits - 1u] = (uint8_t)('0' + value % 10u);
        value /= 10u;
    }
}
void dino_game_render(const DinoGame *game, uint8_t frame[2][DINO_COLUMNS])
{
    memset(frame, ' ', 2u * DINO_COLUMNS);
    if (game->state == DINO_READY) {
        memcpy(frame[0], "DINO  BEST ", 10u);
        write_score(&frame[0][11], game->best);
        memcpy(frame[1], "SW0: START/JUMP ", 15u);
        return;
    }
    if (game->state == DINO_PAUSED) {
        memcpy(frame[0], "PAUSED  SW2:GO  ", 15u);
        memcpy(frame[1], "SCORE ", 6u);
        write_score(&frame[1][6], game->score);
        return;
    }
    if (game->state == DINO_GAME_OVER) {
        memcpy(frame[0], "GAME OVER ", 10u);
        write_score(&frame[0][10], game->score);
        memcpy(frame[1], "SW0: RETRY ", 11u);
        write_score(&frame[1][11], game->best);
        return;
    }
    frame[0][10] = 'S';
    write_score(&frame[0][11], game->score);
    for (uint32_t i = 0u; i < DINO_OBSTACLES; ++i) {
        const DinoObstacle *obstacle = &game->obstacles[i];
        if (!obstacle->active || obstacle->column < 0
            || obstacle->column >= (int8_t)DINO_COLUMNS) continue;
        frame[1][(uint8_t)obstacle->column] = obstacle->kind == DINO_CACTUS
            ? DINO_SPRITE_CACTUS
            : (game->animation_ms < 200u ? DINO_SPRITE_BIRD_A : DINO_SPRITE_BIRD_B);
    }
    if (game->jump_ms != 0u) frame[0][DINO_PLAYER_COLUMN] = DINO_SPRITE_JUMP;
    else frame[1][DINO_PLAYER_COLUMN] = game->ducking ? DINO_SPRITE_DUCK
        : (game->animation_ms < 200u ? DINO_SPRITE_RUN_A : DINO_SPRITE_RUN_B);
}
