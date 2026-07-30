# Local validation results

Validated on 28 October 2026.

## Firmware

Both configurations compile with Arm GNU Toolchain 14.3.Rel1 (GCC 14.3.1).

| Configuration | Flash used / limit | RAM including reservations / limit |
| --- | --- | --- |
| Release | 4860 / 32768 bytes | 2672 / 8192 bytes |
| Debug | 7800 / 32768 bytes | 2672 / 8192 bytes |

The RAM total includes the linker reservation for 2048 bytes stack, 512 bytes
heap and alignment. Game state uses no dynamic allocation. The ELF entry and
Thumb reset vector match; the initial stack is 0x20002000 and the vector table
is at 0x08000000. Both BIN images match their ELF vectors. A deliberately
corrupted BIN was rejected by the verifier.

Both configurations produce ELF, HEX, BIN, map, verification JSON, compiler
stack-use reports and a manifest with image SHA-256 hashes. Outputs are under
`build/Release/` and `build/Debug/`. The default compiler selection now points
to a verified standalone compiler through ignored local settings.

## Game and tools

The strict C11 host suite passes with MSYS2 GCC 14.2.0. It checks controls,
collision entry/landing, jumping, ducking, fast fall, buffered input, pause,
restart, score limits, simultaneous buttons, debounce and clock rollover,
frame boundaries, sprite pixels, repeatability and independent game objects.
The long-course tests run 32 seeds for 40000 ticks each (1.28 million steps).

The desktop simulator compiles and passes its noninteractive startup check.
The PowerShell test entry point passes; the Make build entry point resolves
to the same Python build tool. Application and board-support whitespace checks
pass. Source comments contain no assistant branding or unfinished TODOs.

## Remaining physical check

The board was not flashed during this work. LCD wiring, contrast, electrical
timing and physical buttons still need the checks in [board.md](board.md).
No commits were pushed and hosted CI was not run, per the final instruction.
