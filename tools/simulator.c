/* Text preview of the same C engine used by the board. Terminal input uses
 * a duck toggle because ordinary console input does not report key releases. */
#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
#include "dino_game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#ifdef _WIN32
#include <conio.h>
#include <windows.h>
static void wait_tick(void) { Sleep(DINO_TICK_MS); }
static int read_key(void) { return _kbhit() ? _getch() : -1; }
#else
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <sys/select.h>
static struct termios original_terminal;
static void restore_terminal(void) { tcsetattr(STDIN_FILENO, TCSANOW, &original_terminal); }
static void wait_tick(void)
{
    struct timespec delay = {0, (long)DINO_TICK_MS * 1000000L};
    nanosleep(&delay, NULL);
}
static int read_key(void)
{
    fd_set input;
    FD_ZERO(&input);
    FD_SET(STDIN_FILENO, &input);
    struct timeval wait = {0, 0};
    if (select(STDIN_FILENO + 1, &input, NULL, NULL, &wait) <= 0) return -1;
    unsigned char ch;
    return read(STDIN_FILENO, &ch, 1) == 1 ? (int)ch : -1;
}
#endif
static volatile sig_atomic_t stop;
static void stop_signal(int number) { (void)number; stop = 1; }
static char printable(uint8_t code)
{
    static const char glyphs[8] = {'D','d','^','_','Y','v','w','X'};
    return code < 8u ? glyphs[code] : (char)code;
}
static void draw(const DinoGame *game)
{
    uint8_t frame[2][DINO_COLUMNS];
    dino_game_render(game, frame);
    fputs("\033[HChrome Dino - 16x2 LCD simulator\n", stdout);
    puts("Space: start/jump | D: duck toggle | P: pause | R: title | Q: quit");
    puts("+----------------+");
    for (unsigned row = 0u; row < 2u; ++row) {
        putchar('|');
        for (unsigned col = 0u; col < DINO_COLUMNS; ++col) putchar(printable(frame[row][col]));
        puts("|");
    }
    puts("+----------------+");
    printf("Duck: %-3s  Best: %05lu   \n", game->ducking ? "ON" : "OFF", (unsigned long)game->best);
    puts("D/d = runner, ^ = jumping, _ = duck, Y = cactus, v/w = bird");
    fflush(stdout);
}
int main(int argc, char **argv)
{
    DinoGame game;
    dino_game_init(&game, 0x1234u);
    if (argc == 2 && strcmp(argv[1], "--smoke") == 0) {
        dino_game_tick(&game, DINO_JUMP, 0u);
        for (unsigned tick = 0; tick < 20u; ++tick) dino_game_tick(&game, 0u, 0u);
        if (game.state != DINO_RUNNING || game.score == 0u) return 1;
        puts("PASS: simulator engine starts and advances.");
        return 0;
    }
#ifdef _WIN32
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    if (!GetConsoleMode(console, &mode) || !SetConsoleMode(console, mode | 0x0004u)) {
        fputs("Use an interactive terminal, or --smoke for a headless check.\n", stderr);
        return 1;
    }
#else
    if (tcgetattr(STDIN_FILENO, &original_terminal) != 0) {
        fputs("Use an interactive terminal, or --smoke for a headless check.\n", stderr);
        return 1;
    }
    atexit(restore_terminal);
    struct termios raw = original_terminal;
    raw.c_lflag &= (tcflag_t)~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
#endif
    signal(SIGINT, stop_signal);
    fputs("\033[2J", stdout);
    bool duck = false;
    while (!stop) {
        uint8_t pressed = 0u;
        int key;
        while ((key = read_key()) >= 0) {
            if (key == 'q' || key == 'Q') stop = 1;
            if (key == ' ') pressed |= DINO_JUMP;
            if (key == 'd' || key == 'D') duck = !duck;
            if (key == 'p' || key == 'P') pressed |= DINO_PAUSE;
            if (key == 'r' || key == 'R') pressed |= DINO_HOME;
        }
        dino_game_tick(&game, pressed, duck ? DINO_DUCK : 0u);
        draw(&game);
        wait_tick();
    }
#ifdef _WIN32
    SetConsoleMode(console, mode);
#endif
    return 0;
}
