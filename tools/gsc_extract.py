#!/usr/bin/env python3
# gsc_extract - extract one file from a Cossacks GSC archive (e.g. all.gsc).
#
# Used to pull per-nation data scripts (<NATION>.NDS, Nations.lst) for the
# AI porting workflow - see docs/AI.md. Format per src/game/arc/gscarch.{h,cpp}
# and isimasks.cpp:
#   header: 6B descriptor + WORD version + WORD key + DWORD entries (14 B)
#   FAT: entries x 81 B { DWORD hash; char name[64]; DWORD offset; DWORD size;
#         DWORD reserved; BYTE flags }   (offset is stored NEGATED and is
#         relative to the data area at 14 + 81*entries)
#   flagged files are byte-encrypted: plain = (~cipher & 0xFF) ^ ~(key >> 8)
#
# Usage: python3 gsc_extract.py all.gsc NATIONS.LST        (stdout)
#        python3 gsc_extract.py all.gsc TURKISH.NDS > turkish.nds
import struct
import sys


def extract(archive, wanted):
    data = open(archive, 'rb').read()
    ver, key, entries = struct.unpack_from('<HHI', data, 6)
    key_byte = (~(key >> 8)) & 0xFF
    data_start = 14 + 81 * entries
    for i in range(entries):
        _hash, name, off, size, _res, flags = struct.unpack_from(
            '<I64sIIIB', data, 14 + i * 81)
        name = name.split(b'\0')[0].decode('latin1')
        if name.upper() != wanted.upper():
            continue
        real_off = data_start + ((~off) & 0xFFFFFFFF)
        blob = bytearray(data[real_off:real_off + size])
        if flags:
            for j in range(len(blob)):
                blob[j] = ((~blob[j]) & 0xFF) ^ key_byte
        return bytes(blob)
    return None


def main():
    if len(sys.argv) != 3:
        sys.exit('usage: gsc_extract.py <archive.gsc> <inner file name>')
    blob = extract(sys.argv[1], sys.argv[2])
    if blob is None:
        sys.exit('not found: ' + sys.argv[2])
    sys.stdout.buffer.write(blob)


if __name__ == '__main__':
    main()
