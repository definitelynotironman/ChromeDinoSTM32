# Third-party components

The STM32 startup file, system initialization, CMSIS headers and STM32F0 HAL
were copied from the local UCT EEE3096S-2026 Practical 3 starter supplied as the
board reference. Their original copyright notices and component license files
are retained under `Drivers/` and `Core/Startup/`.

- CMSIS core: Arm, Apache-2.0; see `Drivers/CMSIS/LICENSE.txt`.
- STM32 device support: STMicroelectronics; see
  `Drivers/CMSIS/Device/ST/STM32F0xx/LICENSE.txt`.
- STM32F0 HAL: STMicroelectronics; see
  `Drivers/STM32F0xx_HAL_Driver/LICENSE.txt`.
- Startup assembly: STMicroelectronics; accompanying component terms are
  retained in `Core/Startup/LICENSE.txt`.

LCD signalling, pin assignments, delay timing and clock configuration follow
the supplied Practical 3 board driver. The game engine, input object, LCD frame
interface, application loop, tests and build tooling implement this project.
The project is a character-LCD adaptation inspired by the offline Dino game;
its sprites are defined locally and it does not bundle browser game assets.

## Interactive README

The optional local README viewer bundles Marked 17.0.5, by Christopher Jeffrey
and the Marked contributors, under the MIT licence. Its licence is retained in
`docs/interactive/vendor/MARKED-LICENSE.md`. The firmware does not use Marked.

The README's pixel artwork is drawn locally for this project. The demonstration
preview is derived from the supplied `demonstration/final_build.mp4` recording;
the original video is retained unchanged.
