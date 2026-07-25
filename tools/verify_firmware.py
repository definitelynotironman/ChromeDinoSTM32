#!/usr/bin/env python3
"""Check linked ELF placement and matching raw firmware for STM32F051C6."""
import argparse
import json
from pathlib import Path
import struct

FLASH_START = 0x08000000
FLASH_BYTES = 32 * 1024
RAM_START = 0x20000000
RAM_BYTES = 8 * 1024

def require(condition, message):
    if not condition:
        raise ValueError(message)

def verify(elf_path):
    elf_path = Path(elf_path)
    data = elf_path.read_bytes()
    require(data[:7] == b'\x7fELF\x01\x01\x01', 'Expected a 32-bit little-endian ELF')
    header = struct.unpack_from('<16sHHIIIIIHHHHHH', data)
    require(header[1] == 2 and header[2] == 40, 'Expected an ARM executable')
    entry, phoff, shoff = header[4:7]
    phsize, phcount, shsize, shcount, shstr = header[9:14]
    require(phsize == 32 and shsize == 40, 'Unsupported ELF table layout')
    sections = [struct.unpack_from('<IIIIIIIIII', data, shoff + i * shsize) for i in range(shcount)]
    names_section = sections[shstr]
    names = data[names_section[4]:names_section[4] + names_section[5]]
    flash_end, ram_end = FLASH_START, RAM_START
    vector = None
    stack_reserve = 0
    for section in sections:
        name = names[section[0]:].split(b'\0', 1)[0].decode('ascii')
        flags, address, offset, size = section[2:6]
        if name == '.isr_vector':
            require(address == FLASH_START, 'Interrupt vectors are not at flash base')
            vector = data[offset:offset + size]
        if name == '._user_heap_stack':
            stack_reserve = size
        if flags & 2 and size:
            if RAM_START <= address < RAM_START + RAM_BYTES:
                require(address + size <= RAM_START + RAM_BYTES, f'{name} exceeds SRAM')
                ram_end = max(ram_end, address + size)
            elif FLASH_START <= address < FLASH_START + FLASH_BYTES:
                require(address + size <= FLASH_START + FLASH_BYTES, f'{name} exceeds flash')
            else:
                raise ValueError(f'Unexpected allocated section {name} at {address:#x}')
    for i in range(phcount):
        kind, offset, virtual, physical, file_size, mem_size, flags, align = struct.unpack_from(
            '<IIIIIIII', data, phoff + i * phsize)
        if kind == 1 and file_size:
            require(FLASH_START <= physical < FLASH_START + FLASH_BYTES, 'Load image outside flash')
            require(physical + file_size <= FLASH_START + FLASH_BYTES, 'Load image exceeds flash')
            flash_end = max(flash_end, physical + file_size)
    require(vector is not None and len(vector) >= 8, 'Missing reset vectors')
    stack, reset = struct.unpack_from('<II', vector)
    require(stack == RAM_START + RAM_BYTES, 'Initial stack does not match C6 SRAM end')
    require(reset & 1 and FLASH_START <= (reset & ~1) < flash_end, 'Invalid Thumb reset handler')
    require((entry & ~1) == (reset & ~1), 'Entry point and reset vector differ')
    require(stack_reserve >= 2048, 'Missing reserved stack space')
    raw = elf_path.with_suffix('.bin').read_bytes()
    require(len(raw) == flash_end - FLASH_START, 'Raw image size differs from ELF load image')
    require(raw[:len(vector)] == vector, 'Raw image vectors differ from ELF')
    result = {'flash_bytes': flash_end-FLASH_START, 'flash_limit': FLASH_BYTES,
              'ram_bytes_including_reserve': ram_end-RAM_START, 'ram_limit': RAM_BYTES,
              'heap_stack_reserve_bytes': stack_reserve, 'initial_stack': hex(stack),
              'reset_handler': hex(reset), 'vector_and_binary_match': True}
    elf_path.with_suffix('.verification.json').write_text(json.dumps(result, indent=2) + '\n')
    print(f"PASS: C6 flash {result['flash_bytes']}/{FLASH_BYTES} bytes; "
          f"RAM including reserve {result['ram_bytes_including_reserve']}/{RAM_BYTES} bytes; "
          'reset vector and raw binary match.')
    return result

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('elf', type=Path)
    args = parser.parse_args()
    try:
        verify(args.elf)
    except (ValueError, IndexError, struct.error, OSError) as exc:
        parser.exit(1, f'Firmware verification failed: {exc}\n')
