#include "main.h"
#include "board_io.h"

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    board_io_init();
    DinoGame game;
    uint8_t frame[2][DINO_COLUMNS];
    uint8_t pending_presses = 0u;
    dino_game_init(&game, HAL_GetTick() ^ 0x051c6001u);
    dino_game_render(&game, frame);
    board_lcd_present(frame);
    uint32_t previous_ms = HAL_GetTick();
    bool seeded = false;

    for (;;) {
        uint32_t now_ms = HAL_GetTick();
        pending_presses |= board_buttons_poll(now_ms);
        /* Human start timing varies the course between power cycles. */
        if (!seeded && (pending_presses & DINO_JUMP) != 0u) {
            game.random ^= now_ms;
            if (game.random == 0u) game.random = 1u;
            seeded = true;
        }
        if ((uint32_t)(now_ms - previous_ms) >= DINO_TICK_MS) {
            /* Resynchronise after a debugger stop: never replay seconds of
             * missed collisions before the player gets a chance to react. */
            if ((uint32_t)(now_ms - previous_ms) > 5u * DINO_TICK_MS)
                previous_ms = now_ms;
            else previous_ms += DINO_TICK_MS;
            dino_game_tick(&game, pending_presses, board_buttons_held());
            pending_presses = 0u;
            dino_game_render(&game, frame);
            board_lcd_present(frame);
            board_status(game.state == DINO_GAME_OVER);
        }
        __WFI(); /* SysTick wakes the foreground approximately every 1 ms. */
    }
}

/* Match the reference board: 8 MHz external crystal, PLL x6, 48 MHz CPU.
 * LCD microsecond delays depend on this clock, so do not change it alone. */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef oscillator = {0};
    RCC_ClkInitTypeDef clocks = {0};
    oscillator.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    oscillator.HSEState = RCC_HSE_ON;
    oscillator.PLL.PLLState = RCC_PLL_ON;
    oscillator.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    oscillator.PLL.PREDIV = RCC_PREDIV_DIV1;
    oscillator.PLL.PLLMUL = RCC_PLL_MUL6;
    if (HAL_RCC_OscConfig(&oscillator) != HAL_OK) Error_Handler();
    clocks.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1;
    clocks.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clocks.AHBCLKDivider = RCC_HCLK_DIV1;
    clocks.APB1CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clocks, FLASH_LATENCY_1) != HAL_OK) Error_Handler();
}
void Error_Handler(void)
{
    /* Indicate startup failure even when the LCD has not been initialized. */
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    (void)RCC->AHBENR;
    GPIOB->BSRR = (1u << 10) | (1u << (11u + 16u));
    GPIOB->MODER = (GPIOB->MODER & ~((3u << 20) | (3u << 22)))
                  | (1u << 20) | (1u << 22);
    __disable_irq();
    for (;;) __NOP();
}
#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file;
    (void)line;
    Error_Handler();
}
#endif
