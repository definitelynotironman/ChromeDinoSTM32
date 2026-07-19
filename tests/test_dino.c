#include "dino_game.h"
#include "dino_buttons.h"
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
int main(void)
{
    test_controls_and_collision();
    test_pause_and_jump_buffer();
    test_buttons();
    test_render_and_limits();
    puts("PASS: controls_and_collision, pause_and_jump_buffer, buttons, render_and_limits");
    return 0;
}
