# Build and verification

Use Python 3.9+ and GNU Arm GCC 11+ for firmware. STM32CubeIDE's Windows
compiler is discovered automatically, or select a standalone compiler:

```powershell
.\build.ps1 -GccPath 'C:\path\to\arm-toolchain\bin'
```

The first successful firmware build saves its compiler directory in the ignored
`.toolchain.local.json`. Later builds reuse it. `--gcc-path` or `GCC_PATH`
overrides that selection. The build does not download or install compilers.
If a bundled compiler fails to start, select a working standalone Arm compiler.

```text
python tools/build.py build --config Release
python tools/build.py build --config Debug
python tools/build.py test
python tools/build.py sim
```

Make exposes `all`, `test`, `sim` and `flash`; set `CONFIG=Debug` to select that
configuration. PowerShell exposes the same actions through `build.ps1`.

## Firmware checks

Every firmware build links with the C6 memory sizes, then checks the ELF and
raw BIN. The checker verifies ARM executable format, allocation inside flash
and RAM, vector placement at `0x08000000`, the initial stack at `0x20002000`,
a valid Thumb reset address matching the entry point, stack reservation and
identical vectors in the binary. It writes `.verification.json` next to the
ELF. `build-manifest.json` records compiler version and image SHA-256 hashes.

Run `python tools/verify_firmware.py build/Release/ChromeDinoSTM32.elf` to repeat
these checks without recompiling. The matching BIN must be present.

C sources compile with warnings treated as errors. Vendor modules suppress
unused-parameter warnings; game code retains the full warning set. The
compiler also emits `.su` stack-use reports alongside object files.

## Host checks

A host GCC or cc compiler is needed; `--host-cc` and `HOST_CC` select it.
The suite covers collision entry and landing, cactus versus bird clearance,
restart/best score, held versus new jump input, jump buffering, fast descent,
pause timer freeze, button bounce, simultaneous buttons, tick rollover, frame
bounds, raw sprite zero, five-digit limits, difficulty and independent objects.

The course simulation runs 32 seeds for 40000 ticks each, reaching maximum
speed with a reaction player that jumps obstacles. This is 1.28 million game
steps, and tests spacing, score progression, obstacle capacity and bird unlock.
The simulator also provides a short `--smoke` startup check.

## Hardware verification

Host simulation and binary checks cannot prove physical LCD timing, wiring,
button behavior or operation on the actual board. Use the board checks in
[board.md](board.md) after programming. The GitHub workflow is supplied for
future pushes; local completion does not imply that hosted CI has run.
