#!/usr/bin/env python3
# gp_render - render a sprite from a Cossacks .gp file exactly the way the
# game renders it (ShowGP / ShowGPLayers path, see docs/unit_rendering.md).
#
# Game-faithful rules implemented here (mirror src/game/gp_draw.cpp):
#   * a "sprite" is a CHAIN of layers (NextPict), drawn in order, each with
#     its own Options compression type, dx/dy offset and per-line mask
#   * layer types (Options & 63):
#       0  standard packing (StdUnpack dictionary LZ) -> opaque overwrite
#       1  national mask (2-bit NatUnpackT)           -> NatPal[nation*4 + v]
#       2  transparent 1/4 (raw)                      -> trans4[(dst<<8)|src]
#       3  transparent 1/2 (raw)                      -> trans8[(dst<<8)|src]
#       4  transparent 3/4 (raw)                      -> trans4[(dst<<8)|src]
#          (the game uses GP_ShowMaskedMultiPalTPict for 2/3 and
#           GP_ShowMaskedMultiPalPict for 4 - same table, different pixel op)
#       5  shadow (raw mask only)                     -> fog[4096 + dst]
#   * ItDX from the .gpi file shifts the draw x:  x -= ItDX[sprite]
#   * sprite index >= 4096 means horizontally mirrored (the "Inv" variants)
#
# Data needed from the game (all inside all.gsc, auto-extracted):
#   <NAME>.GP          the sprite page
#   <NAME>.GPI         ItDX/ItLX per-sprite adjustments (may be absent)
#   0\AGEW_1.PAL       256*3 RGB palette
#   0\AGEW_TR.GRD      trans8 blend table (65536)
#   0\AGEW_TR4.GRD     trans4 blend table (65536)
#   0\AGEW_1D.GRD      dark table; the shadow op uses byte 4096..4351 of fog[]
#                      which is fog[1024+3072 .. ] of this file
#
# Usage:
#   python3 tools/gp_render.py --gamedata gamedata KRES.GP 0 -o stand.png
#   python3 tools/gp_render.py --gamedata gamedata KREG.GP 0 --nation 5 \
#           --strip 0,9,18,27,36,45,54,63,72,81 -o walk.png
#   python3 tools/gp_render.py --gamedata gamedata KRES.GP 0 --layers \
#           -o layers.png
import argparse
import os
import struct
import sys
import zlib

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gsc_extract import extract  # noqa: E402

NATPAL = [0xD0, 0xD1, 0xD2, 0xD3,  # red
          0xD4, 0xD5, 0xD6, 0xD7,  # blue
          0xD8, 0xD9, 0xDA, 0xDB,  # cyan
          0xDC, 0xDD, 0xDE, 0xDF,  # purple
          0xE0, 0xE1, 0xE2, 0xE3,  # orange
          0xE4, 0xE5, 0xE6, 0xE7,  # black
          0xE8, 0xE9, 0xEA, 0xEB,  # white
          0xEC, 0xED, 0xEE, 0xEF]  # mercs


def stdunpack(src, out_len, voc):
    dst = bytearray()
    si = 0
    rem = out_len
    while rem > 0:
        bits = src[si]; si += 1
        for _ in range(8):
            if rem <= 0:
                break
            if bits & 0x80:
                cx = src[si] | (src[si + 1] << 8); si += 2
                count = (cx >> 12) + 3
                soff = cx & 0xFFF
                for k in range(count):
                    dst.append(voc[soff + k])
                rem -= count
            else:
                dst.append(src[si]); si += 1
                rem -= 1
            bits = (bits << 1) & 0xFF
    return bytes(dst)


def natunpackt(src, length):
    out = bytearray()
    for b in src[:length // 4]:
        out.append(b & 3)
        out.append((b >> 2) & 3)
        out.append((b >> 4) & 3)
        out.append((b >> 6) & 3)
    return bytes(out)


def decode_layer_mask(data, hdr, pix):
    """Per-line RLE mask -> list of (row, col, pixel bytes drawn)."""
    (next_pict, dx, dy, Lx, Ly, Pack, Options, CData, NLines) = \
        struct.unpack_from('<IhhhhIBIH', data, hdr)
    runs = []
    mask = hdr + 23
    p = 0
    for row in range(NLines):
        m = data[mask]; mask += 1
        if m == 0:
            continue
        col = 0
        if not (m & 0x80):
            for _ in range(m & 0x7F):
                skip, count = data[mask], data[mask + 1]; mask += 2
                col += skip
                runs.append((row, col, pix[p:p + count]))
                p += count
                col += count
        else:
            space_mask = 16 if m & 0x40 else 0
            pix_mask = 16 if m & 0x20 else 0
            for _ in range(m & 0x1F):
                packed = data[mask]; mask += 1
                col += (packed & 0x0F) | space_mask
                count = (packed >> 4) | pix_mask
                runs.append((row, col, pix[p:p + count]))
                p += count
                col += count
    return runs


def layer_pix(data, hdr):
    (next_pict, dx, dy, Lx, Ly, Pack, Options, CData, NLines) = \
        struct.unpack_from('<IhhhhIBIH', data, hdr)
    typ = Options & 63
    unpack_len = CData >> 14
    cd_offs = (CData & 0x3FFF) + (16384 if Options & 64 else 0) + \
              (32768 if Options & 128 else 0)
    if typ == 43:
        unpack_len += 262144
    if typ == 44:
        unpack_len += 524288
    src = data[hdr + cd_offs:]
    voc_off, voc_len = struct.unpack_from('<II', data, 8)
    voc = data[voc_off:voc_off + voc_len]
    if typ == 0:
        return typ, dx, dy, Lx, NLines, stdunpack(src, unpack_len, voc)
    if typ == 1:
        return typ, dx, dy, Lx, NLines, natunpackt(src, unpack_len)
    if typ in (42, 43, 44):
        out = bytearray()
        si = 0
        rem = unpack_len
        while rem > 0:
            bits = src[si]; si += 1
            for _ in range(8):
                if rem <= 0:
                    break
                if bits & 1:
                    cx = src[si] | (src[si + 1] << 8); si += 2
                    count = (cx >> 13) + 3
                    negofs = cx & 0x1FFF
                    frm = len(out) - negofs - 1
                    for k in range(count):
                        out.append(out[frm + k])
                    rem -= count
                else:
                    out.append(src[si]); si += 1
                    rem -= 1
                bits >>= 1
        return typ, dx, dy, Lx, NLines, bytes(out)
    # raw types 2..7, 39, 41: the mask consumes as much as it needs
    return typ, dx, dy, Lx, NLines, src


def layers_of(data, sprite):
    npic = struct.unpack_from('<H', data, 4)[0]
    hdr0 = struct.unpack_from('<I', data, 14 + 4 * sprite)[0]
    out = []
    cur = hdr0
    while True:
        (next_pict, dx, dy, Lx, Ly, Pack, Options, CData, NLines) = \
            struct.unpack_from('<IhhhhIBIH', data, cur)
        typ, ldx, ldy, llx, lnl, pix = layer_pix(data, cur)
        runs = decode_layer_mask(data, cur, pix)
        out.append(dict(hdr=cur, type=typ, dx=ldx, dy=ldy, Lx=llx,
                        NLines=lnl, runs=runs))
        if next_pict == 0xFFFFFFFF:
            break
        cur = hdr0 + next_pict
    return out


def compose(data, sprite, nation, bg_index, mirror, tables, itdx):
    """Compose one sprite (all layers) onto bg_index; returns index buffer + origin."""
    layers = layers_of(data, sprite)
    x0 = -itdx if itdx else 0
    # bounds of the composed stack
    minx = miny = 10 ** 9
    maxx = maxy = -10 ** 9
    for L in layers:
        minx = min(minx, L['dx'])
        miny = min(miny, L['dy'])
        maxx = max(maxx, L['dx'] + L['Lx'])
        maxy = max(maxy, L['dy'] + L['NLines'])
    W, H = maxx - minx, maxy - miny
    buf = [[bg_index] * W for _ in range(H)]

    def put(px, py, v):
        xx = px - minx
        yy = py - miny
        if 0 <= xx < W and 0 <= yy < H:
            buf[yy][xx] = v

    for L in layers:
        if L['type'] == 5:
            for row, col, seg in L['runs']:
                for i in range(len(seg)):
                    px, py = L['dx'] + col + i, L['dy'] + row
                    xx, yy = px - minx, py - miny
                    if 0 <= xx < W and 0 <= yy < H:
                        buf[yy][xx] = tables['shadow'][buf[yy][xx]]
            continue
        for row, col, seg in L['runs']:
            for i, v in enumerate(seg):
                xx = L['dx'] + col + i - minx
                yy = L['dy'] + row - miny
                if mirror:
                    xx = W - 1 - xx
                if not (0 <= xx < W and 0 <= yy < H):
                    continue
                dst = buf[yy][xx]
                t = L['type']
                if t == 0 or t in (42, 43, 44):
                    buf[yy][xx] = v
                elif t == 1:
                    buf[yy][xx] = NATPAL[(nation << 2) + v]
                elif t == 2 or t == 4:
                    buf[yy][xx] = tables['trans4'][(dst << 8) | v]
                elif t == 3:
                    buf[yy][xx] = tables['trans8'][(dst << 8) | v]
                # other types (6/7 alpha, 39/41 raw variants) not used by units
    return buf, (minx, miny)


def write_png(path, rgb_rows):
    h = len(rgb_rows)
    w = len(rgb_rows[0]) // 3
    raw = b''
    for row in rgb_rows:
        raw += b'\x00' + bytes(row)
    def chunk(tag, payload):
        c = struct.pack('>I', len(payload)) + tag + payload
        return c + struct.pack('>I', zlib.crc32(tag + payload) & 0xFFFFFFFF)
    png = b'\x89PNG\r\n\x1a\n'
    png += chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0))
    png += chunk(b'IDAT', zlib.compress(raw, 9))
    png += chunk(b'IEND', b'')
    open(path, 'wb').write(png)


def index_rows_to_rgb(rows, pal):
    out = []
    for row in rows:
        r = []
        for v in row:
            r += [pal[v][0], pal[v][1], pal[v][2]]
        out.append(r)
    return out


def load_assets(gamedata, gpname):
    arc = os.path.join(gamedata, 'all.gsc')
    data = extract(arc, gpname)
    if data is None:
        sys.exit('not found in archive: ' + gpname)
    gpi = extract(arc, gpname[:-3] + '.GPI')
    palblob = extract(arc, '0\\AGEW_1.PAL')
    pal = [tuple(palblob[i * 3:i * 3 + 3]) for i in range(256)]
    tr8 = extract(arc, '0\\AGEW_TR.GRD')
    tr4 = extract(arc, '0\\AGEW_TR4.GRD')
    dark = extract(arc, '0\\AGEW_1D.GRD')  # fog[1024..1024+8191]
    fog = bytearray(dark[1024:1024 + 8192])
    tables = {
        'trans8': tr8,
        'trans4': tr4,
        # game passes fog+4096 = fog[1024+3072] as the shadow encoder
        'shadow': bytes(fog[3072:3072 + 256]),
    }
    return data, gpi, pal, tables


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--gamedata', required=True)
    ap.add_argument('--nation', type=int, default=0)
    ap.add_argument('--bg', type=int, default=65,
                    help='palette index used as backdrop (terrain color)')
    ap.add_argument('--layers', action='store_true',
                    help='also dump every layer as a separate tile')
    ap.add_argument('--strip', default='',
                    help='comma list of sprites; renders them side by side')
    ap.add_argument('-o', '--out', required=True)
    ap.add_argument('gpfile')
    ap.add_argument('sprite', nargs='?', type=int, default=0)
    args = ap.parse_args()

    data, gpi, pal, tables = load_assets(args.gamedata, args.gpfile)

    def itdx_of(spr):
        npic = struct.unpack_from('<H', data, 4)[0]
        if gpi is None or spr >= npic:
            return 0
        return struct.unpack_from('<b', gpi, spr)[0]

    tiles = []
    if args.strip:
        for s in [int(x) for x in args.strip.split(',') if x != '']:
            rows, org = compose(data, s, args.nation, args.bg, False,
                                tables, itdx_of(s))
            tiles.append(('sprite %d' % s, rows))
    else:
        layer_tiles = []
        if args.layers:
            # one tile per layer (each alone on the backdrop), in draw order
            layers = layers_of(data, args.sprite)
            for idx, L in enumerate(layers):
                rows_l, _ = compose_layers([L], args.nation, args.bg,
                                           tables)
                layer_tiles.append(('layer %d (type %d)' % (idx, L['type']),
                                    rows_l))
        rows, org = compose(data, args.sprite, args.nation, args.bg, False,
                            tables, itdx_of(args.sprite))
        tiles = layer_tiles + [('composed sprite %d' % args.sprite, rows)]

    # pack tiles onto one canvas with 4px gutters and captions handled by doc
    pad = 4
    maxh = max(t[1].__len__() for t in tiles)
    widths = [len(t[1][0]) for t in tiles]
    W = sum(widths) + pad * (len(tiles) + 1)
    H = maxh + pad * 2
    canvas = [[pal[args.bg]] * W for _ in range(H)]
    x = pad
    for name, rows in tiles:
        for y, row in enumerate(rows):
            for xx, v in enumerate(row):
                canvas[pad + y][x + xx] = pal[v]
        x += len(rows[0]) + pad
    flat = []
    for row in canvas:
        r = []
        for c in row:
            r += [c[0], c[1], c[2]]
        flat.append(r)
    write_png(args.out, flat)


def compose_layers(layers, nation, bg_index, tables):
    minx = min(L['dx'] for L in layers)
    miny = min(L['dy'] for L in layers)
    maxx = max(L['dx'] + L['Lx'] for L in layers)
    maxy = max(L['dy'] + L['NLines'] for L in layers)
    W, H = maxx - minx, maxy - miny
    buf = [[bg_index] * W for _ in range(H)]
    for L in layers:
        if L['type'] == 5:
            for row, col, seg in L['runs']:
                for i in range(len(seg)):
                    xx = L['dx'] + col + i - minx
                    yy = L['dy'] + row - miny
                    if 0 <= xx < W and 0 <= yy < H:
                        buf[yy][xx] = tables['shadow'][buf[yy][xx]]
            continue
        for row, col, seg in L['runs']:
            for i, v in enumerate(seg):
                xx = L['dx'] + col + i - minx
                yy = L['dy'] + row - miny
                if not (0 <= xx < W and 0 <= yy < H):
                    continue
                dst = buf[yy][xx]
                t = L['type']
                if t == 0 or t in (42, 43, 44):
                    buf[yy][xx] = v
                elif t == 1:
                    buf[yy][xx] = NATPAL[(nation << 2) + v]
                elif t == 2 or t == 4:
                    buf[yy][xx] = tables['trans4'][(dst << 8) | v]
                elif t == 3:
                    buf[yy][xx] = tables['trans8'][(dst << 8) | v]
    return buf, (minx, miny)


if __name__ == '__main__':
    main()
