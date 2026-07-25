# Playing Dino

SW0 starts a run. Once running, each new SW0 press launches a 680 ms jump.
Holding the button does not repeat the jump. A press in the final 120 ms of a
jump is buffered until landing. Release SW1 to allow that buffered jump.

SW1 ducks on the ground and accelerates descent while airborne. Cacti must be
jumped; birds can be jumped or ducked. Obstacles occupy one character column
and the player occupies column 1 (the second visible cell). Collision is
checked every 20 ms, including the moment a jump lands between scroll events.

SW2 freezes the entire run; press again to continue. SW3 returns to the title
without clearing the session best. A collision shows the score and a retry
prompt. SW0 retries. Reset and power removal clear the best score.

The score counts scrolled columns, up to 99999. The initial pace is 180 ms per
column. Every 50 points removes 10 ms, down to 90 ms. Birds enter the random
obstacle selection at 50 points. Spawns stay 9-12 columns apart.

## Preview on a computer

Run `python tools/build.py sim`, then open the executable it prints in an
interactive terminal. Space starts or jumps, D toggles duck, P pauses, R
returns to the title and Q exits. The simulator uses letters for LCD sprites:
D/d for running, ^ for jumping, _ for ducking, Y for cactus and v/w for birds.

Terminal input cannot report releases consistently, so duck is a toggle in
the simulator; the physical SW1 button is held. Terminal key repeat can issue
repeated jump presses. The simulator checks game logic, not LCD wiring or
physical button timing. `--smoke` runs a short noninteractive startup check.
