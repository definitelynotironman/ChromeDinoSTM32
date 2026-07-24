/* UCT board LCD signalling adapted from the Practical 3 reference.
 * D4 PB8, D5 PB9, D6 PA12, D7 PA15, RS PC14, E PC15.
 * The LCD R/W line is grounded, so bounded delays replace busy-flag reads.
 * PA13/PA14 retain their reset configuration for SWD debugging.
 */
#include "board_io.h"
#include "dino_buttons.h"
#include "dino_sprites.h"
#include "main.h"
#include <string.h>

static uint32_t lcd_rs_pin;
static uint32_t lcd_e_pin;
static uint8_t lcd_previous[2][DINO_COLUMNS];
static bool lcd_previous_valid;
static DinoButtons buttons;


static void gpio_output(GPIO_TypeDef *port, uint32_t pin)
{
    uint32_t shift = pin * 2u;
    port->MODER = (port->MODER & ~(3u << shift)) | (1u << shift);
    port->OTYPER &= ~(1u << pin);
    port->OSPEEDR = (port->OSPEEDR & ~(3u << shift)) | (3u << shift);
    port->PUPDR &= ~(3u << shift);
}

static void lcd_delay_us(uint32_t us)
{
    /* At 48 MHz each taken loop takes four cycles (SUBS 1, BNE 3).
     * Interrupts may lengthen an LCD pulse, which is harmless. */
    uint32_t loops = us * 12u;
    if (loops == 0u) return;
    __asm volatile (
        ".syntax unified\n"
        "1: subs %0, %0, #1\n"
        "bne 1b\n"
        : "+l" (loops) : : "cc");
}

static void lcd_write_nibble(uint8_t nibble)
{
    uint32_t a_set = 0u;
    uint32_t b_set = 0u;
    if ((nibble & 1u) != 0u) b_set |= 1u << 8;
    if ((nibble & 2u) != 0u) b_set |= 1u << 9;
    if ((nibble & 4u) != 0u) a_set |= 1u << 12;
    if ((nibble & 8u) != 0u) a_set |= 1u << 15;
    GPIOA->BSRR = a_set | ((((1u << 12) | (1u << 15)) & ~a_set) << 16);
    GPIOB->BSRR = b_set | ((((1u << 8) | (1u << 9)) & ~b_set) << 16);
    lcd_delay_us(2u);
    GPIOC->BSRR = lcd_e_pin;
    lcd_delay_us(10u); /* Allow the NMOS level-shifter pull-up to settle. */
    GPIOC->BSRR = lcd_e_pin << 16;
    lcd_delay_us(2u);
}

static void lcd_write(uint8_t byte, bool data)
{
    GPIOC->BSRR = data ? lcd_rs_pin : (lcd_rs_pin << 16);
    lcd_write_nibble((uint8_t)(byte >> 4));
    lcd_write_nibble((uint8_t)(byte & 0x0fu));
    lcd_delay_us(45u);
}

static void lcd_initialize(void)
{
    GPIOC->BSRR = (lcd_rs_pin | lcd_e_pin) << 16;
    HAL_Delay(50u);
    lcd_write_nibble(3u);
    HAL_Delay(5u);
    lcd_write_nibble(3u);
    lcd_delay_us(150u);
    lcd_write_nibble(3u);
    lcd_delay_us(150u);
    lcd_write_nibble(2u);
    lcd_delay_us(150u);
    lcd_write(0x28u, false); /* Four-bit interface, two lines. */
    lcd_write(0x08u, false);
    lcd_write(0x01u, false);
    HAL_Delay(3u);
    lcd_write(0x06u, false);
    lcd_write(0x0cu, false);
    lcd_previous_valid = false;
}

void board_io_init(void)
{
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOCEN;
    (void)RCC->AHBENR; /* Complete clock enable before accessing the ports. */
    GPIOA->MODER &= ~0xffu;
    GPIOA->PUPDR = (GPIOA->PUPDR & ~0xffu) | 0x55u;
    bool swap_controls = (GPIOA->IDR & (1u << 3)) == 0u;
    lcd_rs_pin = 1u << (swap_controls ? 15u : 14u);
    lcd_e_pin = 1u << (swap_controls ? 14u : 15u);

    /* Preload output levels before enabling their drivers. Keep EEPROM CS
     * inactive and LCD enable low while the interface is being configured. */
    GPIOA->BSRR = ((1u << 12) | (1u << 15)) << 16;
    GPIOB->BSRR = (1u << 12) | (((1u << 8) | (1u << 9)
                              | (1u << 10) | (1u << 11)) << 16);
    GPIOC->BSRR = ((1u << 14) | (1u << 15)) << 16;
    gpio_output(GPIOA, 12u);
    gpio_output(GPIOA, 15u);
    gpio_output(GPIOB, 8u);
    gpio_output(GPIOB, 9u);
    gpio_output(GPIOB, 10u); /* Red status LED. */
    gpio_output(GPIOB, 11u); /* Green status LED. */
    gpio_output(GPIOB, 12u); /* EEPROM chip select stays inactive. */
    gpio_output(GPIOC, 14u);
    gpio_output(GPIOC, 15u);
    lcd_initialize();

    lcd_write(0x40u, false); /* Select CGRAM; stream eight sprite definitions. */
    for (uint32_t glyph = 0u; glyph < 8u; ++glyph)
        for (uint32_t row = 0u; row < 8u; ++row) lcd_write(dino_sprites[glyph][row], true);
    lcd_write(0x80u, false); /* Return to visible display RAM. */
    dino_buttons_init(&buttons, (uint8_t)(~GPIOA->IDR) & 0x0fu, HAL_GetTick());
    board_status(false);
}
uint8_t board_buttons_poll(uint32_t now_ms)
{
    return dino_buttons_poll(&buttons, (uint8_t)(~GPIOA->IDR) & 0x0fu, now_ms);
}
uint8_t board_buttons_held(void)
{
    return buttons.held;
}
void board_lcd_present(const uint8_t frame[2][DINO_COLUMNS])
{
    /* Only rewrite changed spans. Avoid CLEAR during play because it causes
     * flicker and takes much longer than ordinary character writes. */
    for (uint32_t row = 0u; row < 2u; ++row) {
        uint32_t column = 0u;
        while (column < DINO_COLUMNS) {
            if (lcd_previous_valid && frame[row][column] == lcd_previous[row][column]) {
                ++column;
                continue;
            }
            lcd_write((uint8_t)((row == 0u ? 0x80u : 0xc0u) + column), false);
            do {
                lcd_write(frame[row][column], true);
                lcd_previous[row][column] = frame[row][column];
                ++column;
            } while (column < DINO_COLUMNS && (!lcd_previous_valid
                     || frame[row][column] != lcd_previous[row][column]));
        }
    }
    lcd_previous_valid = true;
}
void board_status(bool fault)
{
    GPIOB->BSRR = fault ? ((1u << 10) | (1u << (11u + 16u)))
                       : ((1u << 11) | (1u << (10u + 16u)));
}
