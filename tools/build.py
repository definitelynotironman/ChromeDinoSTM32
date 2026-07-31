#!/usr/bin/env python3
"""Build local firmware or host checks using installed compilers only."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
LOCAL_TOOLCHAIN = ROOT / '.toolchain.local.json'

def find_tool(name, explicit=None):
    if not explicit and name == 'arm-none-eabi-gcc' and LOCAL_TOOLCHAIN.is_file():
        try:
            explicit = json.loads(LOCAL_TOOLCHAIN.read_text())['gcc_path']
        except (ValueError, KeyError) as exc:
            raise RuntimeError('Invalid .toolchain.local.json; set --gcc-path to replace it.') from exc
    if explicit:
        candidate = Path(explicit) / (name + ('.exe' if os.name == 'nt' else ''))
        if candidate.is_file():
            return str(candidate.resolve())
        raise RuntimeError(f'{name} was not found in {explicit}')
    on_path = shutil.which(name)
    if on_path:
        return on_path
    # CubeIDE bundles the complete Arm toolchain on Windows. Resolve its
    # versioned directory instead of hard-coding one installation's version.
    if os.name == 'nt':
        patterns = {
            'arm-none-eabi-gcc': 'C:/ST/STM32CubeIDE*/STM32CubeIDE/plugins/*gnu-tools*/tools/bin/arm-none-eabi-gcc.exe',
            'STM32_Programmer_CLI': 'C:/ST/STM32CubeIDE*/STM32CubeIDE/plugins/*cubeprogrammer*/tools/bin/STM32_Programmer_CLI.exe',
        }
        import glob
        matches = sorted(glob.glob(patterns.get(name, '')), reverse=True)
        if matches:
            return matches[0]
    raise RuntimeError(f'Cannot find {name}. Install it or specify its directory.')

def run(command):
    subprocess.run([str(item) for item in command], cwd=ROOT, check=True)

def build(args):
    compiler = find_tool('arm-none-eabi-gcc', args.gcc_path or os.getenv('GCC_PATH'))
    suffix = '.exe' if os.name == 'nt' else ''
    tools = Path(compiler).parent
    out = ROOT / 'build' / args.config
    out.mkdir(parents=True, exist_ok=True)
    common = ['-mcpu=cortex-m0', '-mthumb', '-mfloat-abi=soft']
    includes = ['Core/Inc', 'Drivers/STM32F0xx_HAL_Driver/Inc',
                'Drivers/STM32F0xx_HAL_Driver/Inc/Legacy',
                'Drivers/CMSIS/Device/ST/STM32F0xx/Include', 'Drivers/CMSIS/Include']
    flags = common + ['-DSTM32F051x8', '-DUSE_HAL_DRIVER', '-std=c11',
                     '-Wall', '-Wextra', '-Werror', '-fdata-sections',
                     '-ffunction-sections', '-fno-common', '-fstack-usage']
    flags += ['-O0', '-g3'] if args.config == 'Debug' else ['-Os', '-g3']
    flags += ['-I' + directory for directory in includes]
    sources = sorted(ROOT.glob('Core/Src/*.c'))
    sources += sorted(ROOT.glob('Drivers/STM32F0xx_HAL_Driver/Src/*.c'))
    objects = []
    for source in sources:
        target = out / source.relative_to(ROOT).with_suffix('.o')
        target.parent.mkdir(parents=True, exist_ok=True)
        extra = ['-Wno-unused-parameter'] if 'Drivers' in source.parts else []
        run([compiler, *flags, *extra, '-c', source, '-o', target])
        objects.append(target)
    startup = out / 'startup.o'
    run([compiler, *common, '-g3', '-c', 'Core/Startup/startup_stm32f051c6tx.s', '-o', startup])
    elf = out / 'ChromeDinoSTM32.elf'
    run([compiler, *common, *objects, startup, '--specs=nano.specs', '--specs=nosys.specs',
         '-TSTM32F051C6TX_FLASH.ld', '-Wl,--gc-sections', '-Wl,--print-memory-usage',
         '-Wl,-Map=' + str(out / 'ChromeDinoSTM32.map'), '-Wl,--start-group', '-lc',
         '-lm', '-Wl,--end-group', '-o', elf])
    for extension, fmt in [('hex', 'ihex'), ('bin', 'binary')]:
        run([tools / ('arm-none-eabi-objcopy' + suffix), '-O', fmt, elf, elf.with_suffix('.' + extension)])
    run([tools / ('arm-none-eabi-size' + suffix), elf])
    run([sys.executable, ROOT / 'tools' / 'verify_firmware.py', elf])
    manifest = {
        'configuration': args.config,
        'compiler': subprocess.check_output([compiler, '--version'], text=True).splitlines()[0],
        'cpu': 'cortex-m0', 'device': 'STM32F051C6',
        'images': {image.name: hashlib.sha256(image.read_bytes()).hexdigest()
                   for image in [elf, elf.with_suffix('.hex'), elf.with_suffix('.bin')]},
    }
    (out / 'build-manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    # Save a successful local selection; this machine-specific file is ignored.
    LOCAL_TOOLCHAIN.write_text(json.dumps({'gcc_path': str(tools.resolve())}, indent=2) + '\n')
    print(f'Firmware: {out}', flush=True)
    return elf

def host_test(args):
    compiler = args.host_cc or os.getenv('HOST_CC') or shutil.which('gcc') or shutil.which('cc')
    if not compiler:
        raise RuntimeError('Host checks need gcc or cc on PATH (or --host-cc).')
    out = ROOT / 'build' / 'tests'
    out.mkdir(parents=True, exist_ok=True)
    exe = out / ('dino_tests.exe' if os.name == 'nt' else 'dino_tests')
    run([compiler, '-std=c11', '-O2', '-g', '-Wall', '-Wextra', '-Werror', '-pedantic',
         '-ICore/Inc', 'Core/Src/dino_game.c', 'Core/Src/dino_buttons.c', 'Core/Src/dino_sprites.c',
         'tests/test_dino.c', '-o', exe])
    run([exe])
    run([sys.executable, ROOT / 'tests' / 'test_cubeide_project.py'])

def simulator(args):
    compiler = args.host_cc or os.getenv('HOST_CC') or shutil.which('gcc') or shutil.which('cc')
    if not compiler:
        raise RuntimeError('Simulator needs a host gcc or cc compiler.')
    out = ROOT / 'build' / 'simulator'
    out.mkdir(parents=True, exist_ok=True)
    exe = out / ('dino_simulator.exe' if os.name == 'nt' else 'dino_simulator')
    run([compiler, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-pedantic',
         '-ICore/Inc', 'Core/Src/dino_game.c', 'tools/simulator.c', '-o', exe])
    run([exe, '--smoke'])
    print(f'Play in a terminal: {exe}')

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=['build', 'test', 'flash', 'sim'], nargs='?', default='build')
    parser.add_argument('--config', choices=['Debug', 'Release'], default='Release')
    parser.add_argument('--gcc-path', help='Directory containing arm-none-eabi-gcc')
    parser.add_argument('--host-cc', help='Host C compiler executable')
    parser.add_argument('--programmer', help='Directory containing STM32_Programmer_CLI')
    args = parser.parse_args()
    if args.action == 'sim':
        simulator(args)
    elif args.action == 'test':
        host_test(args)
    else:
        elf = build(args)
        if args.action == 'flash':
            # Flashing is explicit; an ordinary build never accesses the board.
            programmer = find_tool('STM32_Programmer_CLI', args.programmer)
            run([programmer, '-c', 'port=SWD', '-w', elf, '-v', '-rst'])

if __name__ == '__main__':
    try:
        main()
    except (RuntimeError, subprocess.CalledProcessError, OSError) as exc:
        print(f'Build failed: {exc}', file=sys.stderr)
        sys.exit(1)
