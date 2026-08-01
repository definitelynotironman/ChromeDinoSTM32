# STM32CubeIDE setup

The native project declares STM32F051C6Tx, LQFP48, Cortex-M0, with the C6
startup and 32 KiB flash / 8 KiB RAM linker script. The shared `STM32F051x8`
CMSIS preprocessor symbol is required by ST's headers for this device family;
it does not change the selected MCU or give the C6 additional flash.

## Import and launch

1. Use **File > Import > General > Existing Projects into Workspace** and
   select the ChromeDinoSTM32 directory containing `.project` and `.cproject`.
2. Select **Project > Build Configurations > Set Active > Debug**, then build.
3. Open **Run > Debug Configurations > STM32 C/C++ Application**, select
   **ChromeDinoSTM32 Debug**, and launch through ST-LINK/SWD.

CubeIDE builds and loads `Debug/ChromeDinoSTM32.elf`. The Release launch builds
and loads `Release/ChromeDinoSTM32.elf`. Both configurations also generate HEX
and BIN images. These directories are separate from the Python/Make build's
`build/Debug` and `build/Release` outputs.

Each launch selects its own build configuration explicitly, so an active
Release build cannot accidentally supply stale Debug firmware. The previous
Default launch filename is retained as an alias of the corrected Debug launch.

## Recover an earlier generic import

The error **Not able to get mcu target / Described mcu target is not supported**
occurs when CubeIDE has a generic empty project without a selected MCU.
The corrected files provide native STM32 project natures, a concrete C6 target,
managed Debug/Release configuration types, source roots and matching launch IDs.

If the IDE still shows only Default after the files change, remove the project
from the workspace with **Delete project contents on disk unchecked**, then
import this directory again. Select the supplied Debug launch. Do not create
another generic C/C++ project around the folder.

The `.ioc` is a device/clock reference, not a source generator for this game.
CubeMX regeneration can replace the custom startup and board application code.

## Verification

`python tools/build.py test` checks the native project natures, CDT file-version
instruction, both C6 MCU selections, linker memory, firmware-only source roots,
launch IDs, binary paths, SWD mode and language-settings configurations.

CubeIDE 2.2.0 was used to import the project and generate both managed builds.
Those generated Makefiles were compiled with standalone Arm GCC 14.3.1 and
native GNU Make; both firmware images passed C6 memory/reset-vector checks.
The installed IDE's bundled executables could not start from the automation
process, so Make was invoked separately against the IDE-generated files.
Launching a physical debugger session still requires the connected board.

## Reload settings in the running workspace

If the Build Configurations menu still lists only Default, close the project
with **Project > Close Project** before restoring its saved configuration.
With the project closed, run this from the repository directory:

```powershell
git restore -- .project .cproject .settings/language.settings.xml "ChromeDinoSTM32 Debug.launch" "ChromeDinoSTM32 Default.launch" "ChromeDinoSTM32 Release.launch"
```

Double-click the closed project to reopen it. Select the project once, then
check **Project > Build Configurations > Set Active**: Debug and Release should
both be present. This sequence was verified in the running CubeIDE workspace.
It reloads the native project without creating another generic C/C++ import.

The restore command discards local changes to only these six configuration
files, so preserve any intentional custom settings before using it. Building
only compiles the firmware. To download it, use the supplied STM32/ST-LINK Debug
launch; if execution stops at main, choose Resume (F8).
