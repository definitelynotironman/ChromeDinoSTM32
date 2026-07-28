#include "dino_game.h"
#include "dino_buttons.h"
#include "dino_sprites.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); exit(1); \
} } while (0)

static DinoGame running_game(void)
{
    DinoGame game;
    dino_game_init(&game, 12345u);
    dino_game_tick(&game, DINO_JUMP, DINO_JUMP);
    CHECK(game.state == DINO_RUNNING);
    return game;
}
static void obstacle_at(DinoGame *game, DinoObstacleKind kind, int8_t column)
{
    game->obstacles[0] = (DinoObstacle){column, kind, true};
}
static void test_controls_and_collision(void)
{
    DinoGame game = running_game();
    obstacle_at(&game, DINO_CACTUS, DINO_PLAYER_COLUMN);
    dino_game_tick(&game, 0u, 0u);
    CHECK(game.state == DINO_GAME_OVER);
    game.best = 123u;
    dino_game_tick(&game, DINO_JUMP, DINO_JUMP);
    CHECK(game.state == DINO_RUNNING && game.best == 123u && game.score == 0u);
    CHECK(!game.obstacles[0].active);
    obstacle_at(&game, DINO_CACTUS, DINO_PLAYER_COLUMN);
    dino_game_tick(&game, DINO_JUMP, DINO_JUMP);
    CHECK(game.state == DINO_RUNNING && game.jump_ms != 0u);
    game.jump_ms = DINO_TICK_MS;
    dino_game_tick(&game, 0u, DINO_JUMP);
    CHECK(game.state == DINO_GAME_OVER); /* Held jump must not auto-repeat. */

    game = running_game();
    obstacle_at(&game, DINO_CACTUS, 2);
    game.scroll_ms = dino_game_scroll_period(&game) - DINO_TICK_MS;
    dino_game_tick(&game, 0u, 0u);
    CHECK(game.state == DINO_GAME_OVER); /* Collision on the entering column. */

    game = running_game();
    obstacle_at(&game, DINO_BIRD, DINO_PLAYER_COLUMN);
    dino_game_tick(&game, 0u, DINO_DUCK);
    CHECK(game.state == DINO_RUNNING && game.ducking);
    dino_game_tick(&game, 0u, 0u);
    CHECK(game.state == DINO_GAME_OVER);
    game = running_game();
    obstacle_at(&game, DINO_CACTUS, DINO_PLAYER_COLUMN);
    dino_game_tick(&game, 0u, DINO_DUCK);
    CHECK(game.state == DINO_GAME_OVER);
    game = running_game();
    obstacle_at(&game, DINO_BIRD, DINO_PLAYER_COLUMN);
    game.jump_ms = DINO_TICK_MS;
    dino_game_tick(&game, 0u, DINO_DUCK);
    CHECK(game.state == DINO_RUNNING && game.ducking); /* Duck upon landing. */
}
static void test_pause_and_jump_buffer(void)
{
    DinoGame game = running_game();
    dino_game_tick(&game, DINO_JUMP, DINO_JUMP);
    dino_game_tick(&game, DINO_PAUSE, 0u);
    DinoGame snapshot = game;
    for (unsigned i = 0u; i < 1000u; ++i) dino_game_tick(&game, 0u, 0u);
    CHECK(memcmp(&game, &snapshot, sizeof game) == 0);
    dino_game_tick(&game, DINO_PAUSE, 0u);
    CHECK(game.state == DINO_RUNNING && game.jump_ms == snapshot.jump_ms);
    game.jump_ms = 40u;
    dino_game_tick(&game, DINO_JUMP, DINO_JUMP);
    CHECK(game.jump_ms == 20u);
    dino_game_tick(&game, 0u, 0u);
    CHECK(game.jump_ms == 680u); /* Buffered edge launches at landing. */
    dino_game_tick(&game, DINO_HOME, 0u);
    CHECK(game.state == DINO_READY);
}
static void test_buttons(void)
{
    DinoButtons buttons;
    dino_buttons_init(&buttons, 0u, 0u);
    CHECK(dino_buttons_poll(&buttons, 1u, 1u) == 0u);
    CHECK(dino_buttons_poll(&buttons, 0u, 8u) == 0u);
    CHECK(dino_buttons_poll(&buttons, 1u, 10u) == 0u);
    CHECK(dino_buttons_poll(&buttons, 1u, 29u) == 0u);
    CHECK(dino_buttons_poll(&buttons, 1u, 30u) == 1u);
    CHECK(dino_buttons_poll(&buttons, 1u, 100u) == 0u && buttons.held == 1u);
    CHECK(dino_buttons_poll(&buttons, 3u, 101u) == 0u);
    CHECK(dino_buttons_poll(&buttons, 2u, 110u) == 0u);
    CHECK(dino_buttons_poll(&buttons, 2u, 121u) == 2u);
    CHECK(dino_buttons_poll(&buttons, 2u, 130u) == 0u && buttons.held == 2u);
    dino_buttons_init(&buttons, 8u, UINT32_MAX - 20u);
    CHECK(dino_buttons_poll(&buttons, 8u, UINT32_MAX - 10u) == 0u);
    CHECK(dino_buttons_poll(&buttons, 0u, UINT32_MAX - 5u) == 0u);
    CHECK(dino_buttons_poll(&buttons, 0u, 14u) == 0u && buttons.held == 0u);
    CHECK(dino_buttons_poll(&buttons, 1u, 15u) == 0u);
    CHECK(dino_buttons_poll(&buttons, 1u, 35u) == 1u);
}
static void test_render_and_limits(void)
{
    struct { uint8_t before; uint8_t frame[2][DINO_COLUMNS]; uint8_t after; } output;
    memset(&output, 0xa5, sizeof output);
    DinoGame game;
    dino_game_init(&game, 0u);
    CHECK(game.random != 0u);
    for (unsigned state = DINO_READY; state <= DINO_GAME_OVER; ++state) {
        game.state = (DinoState)state;
        game.score = 99999u;
        game.best = 99999u;
        dino_game_render(&game, output.frame);
        CHECK(output.before == 0xa5 && output.after == 0xa5);
        for (unsigned row = 0; row < 2u; ++row)
            for (unsigned col = 0; col < DINO_COLUMNS; ++col)
                CHECK(output.frame[row][col] < 8u || output.frame[row][col] >= ' ');
    }
    game = running_game();
    dino_game_render(&game, output.frame);
    CHECK(output.frame[1][DINO_PLAYER_COLUMN] == 0u); /* NUL is a sprite. */
    obstacle_at(&game, DINO_CACTUS, -1);
    game.obstacles[1] = (DinoObstacle){16, DINO_BIRD, true};
    dino_game_render(&game, output.frame);
    CHECK(output.before == 0xa5 && output.after == 0xa5);
    game.score = 99999u;
    game.scroll_ms = 80u;
    dino_game_tick(&game, 0u, 0u);
    CHECK(game.score == 99999u && dino_game_scroll_period(&game) == 90u);
}
static void test_long_courses(void)
{
    /* Drive a simple reaction player across many seeded courses. This checks
     * spawn capacity, score progression, bird unlock and maximum-speed play. */
    unsigned birds = 0u;
    for (uint32_t seed = 1u; seed <= 32u; ++seed) {
        DinoGame game;
        dino_game_init(&game, seed);
        dino_game_tick(&game, DINO_JUMP, 0u);
        int32_t last_spawn_score = -100;
        uint32_t old_score = 0u;
        for (unsigned step = 0u; step < 40000u; ++step) {
            uint8_t press = 0u;
            for (unsigned i = 0u; i < DINO_OBSTACLES; ++i) {
                DinoObstacle *obstacle = &game.obstacles[i];
                if (obstacle->active && obstacle->column == 2 && game.jump_ms == 0u)
                    press = DINO_JUMP;
            }
            dino_game_tick(&game, press, 0u);
            CHECK(game.state == DINO_RUNNING);
            CHECK(game.best >= game.score && game.spawn_steps >= 1u);
            CHECK(dino_game_scroll_period(&game) >= 90u);
            if (game.score != old_score) {
                for (unsigned i = 0u; i < DINO_OBSTACLES; ++i) {
                    DinoObstacle *obstacle = &game.obstacles[i];
                    if (!obstacle->active) continue;
                    CHECK(obstacle->column >= 0 && obstacle->column < 16);
                    if (obstacle->column == 15) {
                        CHECK((int32_t)game.score - last_spawn_score >= 9);
                        last_spawn_score = (int32_t)game.score;
                        if (obstacle->kind == DINO_BIRD) {
                            CHECK(game.score >= 50u);
                            ++birds;
                        }
                    }
                }
                old_score = game.score;
            }
        }
        CHECK(game.score > 5000u);
    }
    CHECK(birds > 100u);
}
static void test_fast_fall(void)
{
    DinoGame game = running_game();
    dino_game_tick(&game, DINO_JUMP, DINO_JUMP);
    uint16_t before = game.jump_ms;
    dino_game_tick(&game, 0u, DINO_DUCK);
    CHECK(game.jump_ms == before - 3u * DINO_TICK_MS && !game.ducking);
    game.jump_ms = 40u;
    obstacle_at(&game, DINO_BIRD, DINO_PLAYER_COLUMN);
    dino_game_tick(&game, 0u, DINO_DUCK);
    CHECK(game.state == DINO_RUNNING && game.jump_ms == 0u && game.ducking);
    game = running_game();
    obstacle_at(&game, DINO_BIRD, DINO_PLAYER_COLUMN);
    dino_game_tick(&game, DINO_JUMP, DINO_JUMP);
    CHECK(game.state == DINO_RUNNING && game.jump_ms != 0u);
}
static void test_configuration_and_sprites(void)
{
    CHECK(DINO_SPAWN_GAP_MIN * DINO_SCROLL_MIN_MS > DINO_JUMP_MS + DINO_TICK_MS);
    DinoGame game = running_game();
    game.score = 0u;
    CHECK(dino_game_scroll_period(&game) == DINO_SCROLL_START_MS);
    game.score = DINO_LEVEL_POINTS;
    CHECK(dino_game_scroll_period(&game) == DINO_SCROLL_START_MS-DINO_SPEED_STEP_MS);
    game.score = DINO_MAX_SCORE;
    CHECK(dino_game_scroll_period(&game) == DINO_SCROLL_MIN_MS);
    for (unsigned glyph = 0u; glyph < 8u; ++glyph) {
        unsigned lit_rows = 0u;
        for (unsigned row = 0u; row < 8u; ++row) {
            CHECK(dino_sprites[glyph][row] < 32u);
            if (dino_sprites[glyph][row] != 0u) ++lit_rows;
        }
        CHECK(lit_rows != 0u);
    }
}
static void test_object_isolation_and_repeatability(void)
{
    DinoGame first = running_game();
    DinoGame same = first;
    DinoGame idle = first;
    for (unsigned tick = 0u; tick < 1000u; ++tick) {
        uint8_t edge = tick % 31u == 0u ? DINO_JUMP : 0u;
        dino_game_tick(&first, edge, 0u);
        dino_game_tick(&same, edge, 0u);
        CHECK(memcmp(&first, &same, sizeof first) == 0);
    }
    CHECK(idle.score == 0u && idle.scroll_ms == 0u && idle.state == DINO_RUNNING);
    DinoButtons buttons;
    for (uint8_t mask = 0u; mask < 16u; ++mask) {
        dino_buttons_init(&buttons, 0u, UINT32_MAX - 10u);
        CHECK(dino_buttons_poll(&buttons, mask, UINT32_MAX - 5u) == 0u);
        CHECK(dino_buttons_poll(&buttons, mask, 14u) == mask);
        CHECK(buttons.held == mask);
        CHECK(dino_buttons_poll(&buttons, mask, 100u) == 0u);
    }
}
int main(void)
{
    test_controls_and_collision();
    test_pause_and_jump_buffer();
    test_buttons();
    test_render_and_limits();
    test_long_courses();
    test_fast_fall();
    test_configuration_and_sprites();
    test_object_isolation_and_repeatability();
    puts("PASS: collision boundaries, jump buffering, fast fall, pause, debounce and wrap,");
    puts("      rendering, score limits, sprites, object isolation and 32 long courses.");
    return 0;
}
