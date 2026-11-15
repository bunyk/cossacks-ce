#!/usr/bin/env python3
"""terr_shape.py - render the 256x256 "shape bitmap" a random map name selects,
extracted from the GSC resource archive (gamedata/all.gsc).

Pipeline replicated (CreateRandomTerrain, 3drandmap.cpp:3375, and
CreateMapByName, 3drandmap.cpp:1557):
  1. map name "RN<size> <seed> <params> ..." -> ADDSH, mrpos = seed,
     Style = params&15 (land type), Play = (params>>4)&15;
  2. Terrain\\Res.dat STYLES table -> STY[Style].Players[Play].name
     = the TERRAIN\\*.MSP style file (e.g. land type 3 Islands, Play 1
     -> ostrova3.msp);
  3. the .MSP starts with a 'TSOC' chunk (on disk the bytes read "COST" -
     tags are reversed multichar constants, see book/010_m3d.md sec. 1):
     u32 N variants + offset table; the seed picks variant
     (mrpos*N)>>15; the blob there is a TerrainData block:
     u32 CompSize, then CompSize bytes of RLE (alternating 0x00/0xFF fill
     runs) that decode to the 256x256 shape bitmap REND (white=land,
     black=sea; sea = byte <~120 => WaterDeep>128, see
     book/015_map_generation.md sec. 3);
  4. the bitmap is blurred NN = 30>>(ADDSH-1) passes (5-tap filter, borders
     clamped).

Usage:
    python3 tools/terr_shape.py "RN0 3909 20213 00000EC6 0000000.m3d" out.png [--m3d map.m3d]
--m3d additionally cross-checks the bitmap's sea mask (byte<120) against the
saved map's SEA2/WaterDeep>128 mask.
"""
import os
import struct
import sys

import numpy as np
from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gsc_extract import extract  # noqa: E402

ARCHIVE = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                       '..', 'gamedata', 'all.gsc')


def parse_map_name(name):
    """sscanf(Name, "%s%x%x%s%d") + CreateMapByName's mrpos = x1 & 8191
    (3drandmap.cpp:1628) -> (addsh, mrpos, style, play)."""
    parts = name.split()
    cc1, x1, x2 = parts[0], int(parts[1], 16), int(parts[2], 16)
    addsh = ord(cc1[2]) - ord('0') + 1
    return addsh, x1 & 8191, x2 & 15, (x2 >> 4) & 15


def res_dat_styles(blob):
    """Parse Terrain\\Res.dat (RandomMapDesc::Load, 3drandmap.cpp) and return
    the STYLES section as a list of (name, style, [(nplayers, file), ...])."""
    tok = blob.decode('latin1').replace('\r', ' ').split()
    i = 0
    nmines, _ = int(tok[i]), int(tok[i + 1]); i += 2
    i += nmines * 4                                   # MINES: name + 3 ints
    nres, _ = int(tok[i]), int(tok[i + 1]); i += 2
    for _ in range(nres):                             # STARTRES: name, Nr, Nr ints
        nr = int(tok[i + 1]); i += 2 + nr
    nrelief, _ = int(tok[i]), int(tok[i + 1]); i += 2
    i += nrelief                                      # RELIEF: names
    nsty, _ = int(tok[i]), int(tok[i + 1]); i += 2
    styles = []
    for _ in range(nsty):
        t = tok[i]; i += 1
        if t.startswith('#'):                         # "#N" AI-style prefix
            t = tok[i]; i += 1
        name = t
        style = tok[i]; i += 1
        npl = int(tok[i]); i += 1
        players = []
        for _ in range(npl):
            players.append((int(tok[i]), tok[i + 1])); i += 2
        styles.append((name, style, players))
    return styles


def rle_decode(data, comp_size):
    """RLE-decode TerrainData.Data into the 65537-byte REND buffer
    (3drandmap.cpp:3434+): run lengths alternate 0x00-fill / 0xFF-fill."""
    rend = bytearray(65537)
    rend[65535] = rend[65536] = 0xE3                   # integrity sentinels
    dat, rpos = 0, 0
    for i in range(comp_size):
        run = data[i]
        if run:
            rend[rpos:rpos + run] = bytes([dat]) * run
        dat = (~dat) & 0xFF
        rpos += run
    return np.frombuffer(bytes(rend), np.uint8).copy(), rpos


def blur(rend, nn):
    """NN passes of the 5-tap blur with clamped borders (3drandmap.cpp:3446+)."""
    r = rend.reshape(256, 256).astype(np.int32)   # C int math, no uint8 wrap
    for _ in range(nn):
        r1 = r.copy()
        c = r[1:255, 1:255]
        r1[1:255, 1:255] = ((c << 3) + (c << 2) + r[1:255, 0:254] + r[1:255, 2:256]
                            + r[0:254, 1:255] + r[2:256, 1:255]) >> 4
        r = r1
        r[0, :] = r[1, :]
        r[255, :] = r[254, :]
        r[:, 0] = r[:, 1]
        r[:, 255] = r[:, 254]
    return r.astype(np.uint8)


def shape_bitmap(msp, mrpos):
    """Pick the variant for seed mrpos and RLE-decode its shape bitmap.
    TerrainData (3drandmap.cpp:3281, pack(1), 66 bytes): i16 Lx, Ly,
    u8 NPlaces, u8 NIDS, u16 PLACES[8], u8 PlayerID[8], u32 CompSize @30,
    u8 Data[] @34."""
    chunk, n = struct.unpack_from('<II', msp, 0)
    assert chunk == 0x54534F43, "expected 'TSOC' (file bytes 'COST'), got %08X" % chunk
    variant = (mrpos * n) >> 15
    off1, off2 = struct.unpack_from('<II', msp, 8 + variant * 4)
    blob = msp[off1:off2]
    lx, ly = struct.unpack_from('<hh', blob, 0)
    nplaces, nids = blob[4], blob[5]
    places = struct.unpack_from('<8H', blob, 6)
    player_id = list(blob[22:30])
    comp_size = struct.unpack_from('<I', blob, 30)[0]
    rend, rpos = rle_decode(blob[34:], comp_size)
    info = dict(lx=lx, ly=ly, nplaces=nplaces, nids=nids, places=places,
                player_id=player_id)
    return rend[:256 * 256].reshape(256, 256), n, variant, rpos, info


def render(rend, out, scale=3):
    """Grayscale bitmap; sea (byte < 120) tinted blue, darker = deeper."""
    rgb = np.stack([rend] * 3, axis=-1).astype(np.float64)
    sea = rend < 120
    t = np.clip((120 - rend) / 120.0, 0, 1)            # shore -> deep
    rgb[sea] = np.stack([(40 + 60 * (1 - t[sea])),
                         (120 + 90 * (1 - t[sea])),
                         np.full(sea.sum(), 255.0)], axis=-1)
    img = np.repeat(np.repeat(rgb.astype(np.uint8), scale, axis=0), scale, axis=1)
    Image.fromarray(img, 'RGB').save(out)


def sea2_mask(m3d_path):
    """Water mask (WaterDeep>128) of a saved .m3d, from its SEA2 chunk,
    sampled to 256x256 for comparison."""
    import importlib.util
    here = os.path.dirname(os.path.abspath(__file__))
    spec = importlib.util.spec_from_file_location('m3d_areas', os.path.join(here, 'm3d_areas.py'))
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    data = open(m3d_path, 'rb').read()
    addsh, _vil, _mth, chunks = m.read_chunks(data)
    wd = m.parse_2aes(chunks['2AES'])
    yy = (np.arange(256) * wd.shape[0]) // 256
    xx = (np.arange(256) * wd.shape[1]) // 256
    return wd[np.ix_(yy, xx)] > 128


def main():
    name = sys.argv[1]
    out = sys.argv[2]
    addsh, mrpos, style, play = parse_map_name(name)
    styles = res_dat_styles(extract(ARCHIVE, 'TERRAIN\\RES.DAT'))
    sname, _sstyle, players = styles[style]
    npl, msp_name = players[play]
    nn = 30 >> (addsh - 1)
    print('map "%s": ADDSH=%d seed(mrpos)=0x%X style=%d(%s) play=%d -> Terrain\\%s (NPlayers=%d), blur NN=%d'
          % (name, addsh, mrpos, style, sname, play, msp_name, npl, nn))
    msp = extract(ARCHIVE, 'TERRAIN\\' + msp_name)
    rend, n, variant, rpos, info = shape_bitmap(msp, mrpos)
    print('%s: %d bytes, TSOC variants N=%d, seed picks variant %d, RLE decoded %d/65537 bytes'
          % (msp_name, len(msp), n, variant, rpos))
    print('  TerrainData: Lx=%d Ly=%d NPlaces=%d NIDS=%d PLACES=%s PlayerID=%s'
          % (info['lx'], info['ly'], info['nplaces'], info['nids'],
             [hex(p) for p in info['places'][:info['nplaces']]], info['player_id'][:info['nids']]))
    print('  shape bitmap byte range %d..%d, sea(<120)=%.1f%%'
          % (int(rend.min()), int(rend.max()), 100.0 * (rend < 120).mean()))
    rend_b = blur(rend, nn)
    print('after %d blur passes: sea(<120)=%.1f%%' % (nn, 100.0 * (rend_b < 120).mean()))
    render(rend_b, out)
    print('wrote', out)
    if '--m3d' in sys.argv:
        m3d = sys.argv[sys.argv.index('--m3d') + 1]
        mask = sea2_mask(m3d)
        agree = (mask == (rend_b < 120)).mean()
        print('agreement with %s SEA2 WaterDeep>128 mask: %.1f%%' % (m3d, 100.0 * agree))


if __name__ == '__main__':
    main()
