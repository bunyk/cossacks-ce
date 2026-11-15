#!/usr/bin/env python3
# pe_dump - dump strings and .data tables from a Cossacks AI DLL (PE32).
#
# Used by the nation-AI porting workflow (docs/AI.md): the original AI DLLs
# keep their unit/upgrade names and the mine-economy tables in .data, and
# Ghidra's mangled symbol names can hide them (e.g. Bavaria's 18th c.
# musketeer "Mushketer_BA(BA)" shows as s_Mushketer_BA_BA_).
#
# Usage:
#   pe_dump.py <dll> strings                 all printable strings with VAs
#   pe_dump.py <dll> strings 1000f4e8 1000fc30 strings from a VA range
#   pe_dump.py <dll> words <va> <count>       16-bit words at a VA
#   pe_dump.py <dll> dwords <va> <count>     32-bit ints at a VA
#   pe_dump.py <dll> pairs <va> <count>      count pairs {a, b} as words
#
# Known table addresses (identical across the European 94208-byte DLLs):
#   water mine balance   0x0f07c (19 pairs + {4000,x} terminator)
#   land mine balance    0x0f114 (19 pairs + {4000,x} terminator)
#   mine upgrade rules B 0x0f16c (27 ints), C 0x0f1d8 (27 ints)
#   mines build rules    0x0f244 (27 pairs {RSM, ResID})
# (Addresses are image-relative below 0x1000000; pass "1000f07c" style VAs.)
import struct
import sys

FAMILIES = {
    'land_balance': (0x0f114, 'w'),
    'water_balance': (0x0f07c, 'w'),
}


def load(path):
    data = open(path, 'rb').read()
    pe_off = struct.unpack_from('<I', data, 0x3c)[0]
    opt_size = struct.unpack_from('<H', data, pe_off + 20)[0]
    image_base = struct.unpack_from('<I', data, pe_off + 0x34)[0]
    secs = []
    for i in range(struct.unpack_from('<H', data, pe_off + 6)[0]):
        name, vsize, vaddr, rsize, roff = struct.unpack_from(
            '<8sIIII', data, pe_off + 24 + opt_size + i * 40)
        secs.append((image_base + vaddr, vsize, roff, rsize))
    return data, image_base, secs


def va2off(image_base, secs, va):
    rva = va - image_base
    for vaddr, vsize, roff, rsize in secs:
        if vaddr <= va < vaddr + max(vsize, rsize):
            return roff + (rva + image_base - vaddr)
    raise SystemExit('VA %x outside any section' % va)


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return
    path, mode = sys.argv[1], sys.argv[2]
    data, image_base, secs = load(path)
    if mode == 'strings':
        va0 = int(sys.argv[3], 16) if len(sys.argv) > 3 else None
        va1 = int(sys.argv[4], 16) if len(sys.argv) > 4 else None
        out = []
        for vaddr, vsize, roff, rsize in secs:
            # Default scan covers the low sections only (the AI DLLs).
            # An explicit VA range overrides the heuristic - mission DLLs
            # keep their strings further up (e.g. .rdata at +0x2b000).
            if va1 is None and vaddr >= image_base + 0x20000:
                continue
            blob = data[roff:roff + rsize]
            i = 0
            while i < len(blob):
                if 32 <= blob[i] < 127:
                    j = i
                    while j < len(blob) and 32 <= blob[j] < 127:
                        j += 1
                    if j - i >= 3:
                        va = vaddr + i
                        if (va0 is None or va >= va0) and (va1 is None or va <= va1):
                            out.append((va, blob[i:j].decode('latin1')))
                    i = j
                else:
                    i += 1
        for va, s in out:
            print('%08x %r' % (va, s))
    elif mode in ('words', 'dwords', 'pairs'):
        va = int(sys.argv[3], 16)
        n = int(sys.argv[4])
        off = va2off(image_base, secs, va)
        if mode == 'words':
            print(list(struct.unpack_from('<%dH' % n, data, off)))
        elif mode == 'dwords':
            print(list(struct.unpack_from('<%di' % n, data, off)))
        else:
            vals = struct.unpack_from('<%dH' % (n * 2), data, off)
            print([(vals[i], vals[i + 1]) for i in range(0, len(vals), 2)])
    else:
        print(__doc__)


if __name__ == '__main__':
    main()
