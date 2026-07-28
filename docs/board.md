# Board setup and troubleshooting

The board reference is the local UCT EEE3096S Practical 3 STM32F051C6 starter.
It supplies the startup assembly, system support and HAL used in this project.

| Connection | Pin | Behavior |
| --- | --- | --- |
| SW0 | PA0 | Active-low start, jump, retry |
| SW1 | PA1 | Active-low duck and fast descent |
| SW2 | PA2 | Active-low pause/resume |
| SW3 | PA3 | Active-low title; LCD wiring swap at reset |
| LCD D4, D5 | PB8, PB9 | Four-bit data bus |
| LCD D6, D7 | PA12, PA15 | Four-bit data bus |
| LCD RS, E | PC14, PC15 | Default control assignment |
| Red / green status | PB10 / PB11 | Collision or fault / ready or running |
| EEPROM CS | PB12 | Kept high, EEPROM unused |
| SWDIO / SWCLK | PA13 / PA14 | Reserved for ST-LINK |

The original reference documents disagree about the RS/E labels. This project
uses the supplied working driver's default assignment and supports its reset
swap: hold SW3 while resetting, then release it. This swaps PC14 and PC15 for
that boot. No source changes are needed.

The clock assumes an 8 MHz external crystal and runs at 48 MHz with PLL x6.
The C6 linker reserves 32 KiB flash and 8 KiB RAM, including 2 KiB stack and
512 bytes heap reservation. Game objects allocate no heap memory.

## Load firmware

Build first, then load `build/Release/ChromeDinoSTM32.elf` through ST-LINK/SWD
using STM32CubeProgrammer. The HEX can also be loaded directly; raw BIN must
be written at `0x08000000`. The initial stack is `0x20002000`.

`python tools/build.py flash` is an explicit build/program/verify/reset command.
It discovers CubeProgrammer in the Windows CubeIDE installation or on PATH.
Use `--programmer` to specify another installation's `bin` directory. Flashing
replaces the application currently on the board.

## Checks at the board

1. Power the board and check the power indicator.
2. If the LCD is empty or shows solid blocks, adjust POT2 contrast.
3. Try the SW3 reset swap if contrast is correct and text still fails to appear.
4. Release boot buttons, press SW0, and check the animated runner and score.
5. Verify jump, held duck, pause/resume, return to title and collision/retry.
6. After a run, return to title to check the best score. Reset clears it.

A red LED without an LCD title can indicate failure to start the expected
external clock. Check the crystal and board variant. A red LED with GAME OVER
is the normal collision indication. Test both buttons and LCD on physical
hardware; host checks cannot measure wiring or LCD pulse timing.
