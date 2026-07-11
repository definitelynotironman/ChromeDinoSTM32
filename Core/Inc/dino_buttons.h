#ifndef DINO_BUTTONS_H
#define DINO_BUTTONS_H
#include <stdint.h>
/* Each button has its own candidate timestamp; simultaneous presses do not
 * reset one another. Unsigned time differences tolerate millisecond wrap. */
typedef struct {
    uint32_t changed_ms[4];
    uint8_t candidate;
    uint8_t held;
} DinoButtons;
void dino_buttons_init(DinoButtons *buttons, uint8_t raw, uint32_t now_ms);
uint8_t dino_buttons_poll(DinoButtons *buttons, uint8_t raw, uint32_t now_ms);
#endif
