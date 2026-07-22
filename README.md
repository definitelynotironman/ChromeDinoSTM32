# ChromeDinoSTM32

A Dino runner in C for the UCT STM32F051 development board, adapted to its
onboard WC1602A 16x2 character LCD. No additional display is required.

## Build

Install Python 3 and GNU Arm GCC (STM32CubeIDE's bundled compiler is detected
on Windows). From this directory:

```powershell
.\build.ps1
.\build.ps1 -Configuration Debug
.\build.ps1 -Action test
```

Or run `python tools/build.py build`, `python tools/build.py test`, or `make`.
Tests require a host GCC compiler. The build creates `.elf`, `.hex`, `.bin` and
`.map` files under `build/Release/` or `build/Debug/`.

## Controls

- SW0: start, jump, or retry after a collision.
- SW1: hold to duck under birds. Jumping also clears birds.
- SW2: pause or resume.
- SW3: return to the title. Hold during reset to exchange LCD RS/E wiring.

The score increases with distance. Speed increases every 50 points, birds
appear after 50 points, and the best score lasts until reset or power removal.

## Board

The target is STM32F051C6: **32 KiB flash, 8 KiB RAM**, with an 8 MHz external
crystal and 48 MHz CPU. The STM32F051x8 compiler define is the shared device
header selection; the linker still enforces C6 memory sizes.

LCD: PB8/PB9/PA12/PA15 = D4/D5/D6/D7, PC14 = RS, PC15 = E. Buttons PA0-PA3
are active low. SWD remains on PA13/PA14 and EEPROM select PB12 stays high.
Adjust POT2 if the LCD is blank.

To program a connected board explicitly, use `python tools/build.py flash`
with STM32CubeProgrammer installed, or load the built ELF through ST-LINK/SWD.
Flashing replaces the firmware already on the board. Building never flashes.

See [THIRD_PARTY.md](THIRD_PARTY.md) for the reused board support and licenses.
