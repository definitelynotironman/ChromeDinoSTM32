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
int main(void)
{
    test_controls_and_collision();
    puts("PASS: controls_and_collision");
    return 0;
}
