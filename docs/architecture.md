# Design and tuning

The application uses C objects: a struct owns its state, and functions take a
pointer to that object. There is no inheritance, dynamic allocation or C++
runtime. This keeps memory use explicit on the Cortex-M0.

- `DinoGame` owns the player, obstacle pool, score, random generator and state.
  It receives one fixed 20 ms step and makes no hardware calls.
- `DinoButtons` owns four separate debounce timers and produces new press edges
  plus held levels. Unsigned timestamp subtraction handles tick wrap.
- `dino_game_render` converts the game into two arrays of 16 raw LCD bytes.
  Sprite zero is valid; these arrays must never be treated as C strings.
- `board_io` owns LCD control assignments, the last transmitted frame and the
  physical input object. It writes only changed spans and loads CGRAM once.
- `main` accumulates press edges between simulation steps, polls input every
  millisecond and renders after each update. SysTick only advances HAL time.

The game updates physics before scrolling, then checks collisions. Checking
on every tick catches a landing while an obstacle remains under the player.
Pause freezes all game timers. Inputs remain responsive because the LCD uses
bounded waits in foreground and no game work runs inside an interrupt.

A debugger delay over 100 ms resynchronizes the foreground schedule. It does
not replay a backlog of missed input or collisions. Normal LCD transfers are
well below a 20 ms game step; changed spans further reduce bus occupancy.

## Tuning

Edit `Core/Inc/dino_config.h`. Settings cover jump duration, jump buffering,
scroll pace, difficulty thresholds, score cap and obstacle spacing. Compile
time checks reject gaps too short for a full jump, insufficient obstacle
capacity and invalid speed ranges. The default minimum gap is 810 ms at top
speed, exceeding the 680 ms jump plus a 20 ms landing tick.

The LCD is fixed at 16x2. Changing its geometry requires changing the HUD,
rendering and display driver. The microsecond LCD delays assume a 48 MHz CPU;
update their calibration if changing the system clock.

Best score is session RAM. The EEPROM stays deselected, avoiding write wear
and keeping the first version independent of storage-driver behavior.
