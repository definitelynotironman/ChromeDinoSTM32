#include "dino_buttons.h"
void dino_buttons_init(DinoButtons *buttons, uint8_t raw, uint32_t now_ms)
{
    /* A held boot button is not a new edge. Release it before playing. */
    buttons->held = buttons->candidate = raw & 0x0fu;
    for (uint32_t i = 0u; i < 4u; ++i) buttons->changed_ms[i] = now_ms;
}
uint8_t dino_buttons_poll(DinoButtons *buttons, uint8_t raw, uint32_t now_ms)
{
    uint8_t pressed = 0u;
    for (uint32_t i = 0u; i < 4u; ++i) {
        uint8_t bit = (uint8_t)(1u << i);
        if ((raw & bit) != (buttons->candidate & bit)) {
            buttons->candidate ^= bit;
            buttons->changed_ms[i] = now_ms;
        }
        if ((uint32_t)(now_ms - buttons->changed_ms[i]) >= 20u
            && (buttons->held & bit) != (buttons->candidate & bit)) {
            buttons->held ^= bit;
            if ((buttons->held & bit) != 0u) pressed |= bit;
        }
    }
    return pressed;
}
