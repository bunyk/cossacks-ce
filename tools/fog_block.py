#!/usr/bin/env python3
"""fog_block.py - explain ShowSuperFluentFog32_160_16 (fog.cpp) visually.

Scenario: ONE FogSpot stamped at the top-left corner of a single 32x16
screen block. The block's four corner fog words are (8000, 0, 0, 0); GetF
maps them to darkness levels (5, 158, 158, 158) via the makeFden curve.

Replicates, exactly, from fog.cpp:
  - makeFden()/GetF(): k >= 1500 -> 5; else fden[1500-k] with
    fden[i] = 158 if i > 300 else 5 + (i*153)//300;
  - ShowSuperFluentFog32_160_16(): z = level << 16 (16.16); per row
    p starts at z1 and gains c = (z3-z1)>>4; per pixel b starts at
    (z2-z1)>>5 and the row gains d = (z1+z4-z3-z2)>>9; each pixel is
    remapped dst = darkfog[((fog>>8)&0xFF00) + dst];
  - darkfog is the REAL table from gamedata/all.gsc (0\\AGEW_1DR.GRD,
    40960 bytes = 160 levels x 256 palette entries).

The input block is a real 32x16 crop of the grass ground tile (tile 34 of
tiles3.bmp, the 256x2368 8-bit BMP whose 64x64 tiles the terrain renderer
uses - see Loadtextures in 3dgraph.cpp), shown through the BMP's own
256-color palette.

Output: book/figures/fog_block_render.png - four panels: corner levels,
interpolated 16.16 fog level per pixel, the grass input block, and the
rendered 32x16 block after the darkfog remap.

Usage:  python3 tools/fog_block.py [outdir]     (default: book/figures)
"""
import io
import os
import sys

import numpy as np
from PIL import Image

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gsc_extract import extract  # noqa: E402

ARCHIVE = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                       '..', 'gamedata', 'all.gsc')

BLOCK_W, BLOCK_H = 32, 16   # ShowSuperFluentFog32_160_16 block size in pixels


def make_fden():
    """fden[] from makeFden() in fog.cpp."""
    fden = np.zeros(8192, dtype=np.int64)
    for i in range(8192):
        fden[i] = 158 if i > 300 else 5 + (i * 153) // 300
    return fden


FDEN = make_fden()


def get_f(k):
    """GetF() from fog.cpp: fog word -> darkness level 5..158."""
    if k >= 1500:
        return 5
    return int(FDEN[1500 - k])


def load_darkfog():
    """darkfog table from 0\\AGEW_1DR.GRD in all.gsc: flat 40960 bytes =
    160 levels x 256 palette entries, indexed darkfog[level*256 + pixel]."""
    blob = extract(ARCHIVE, '0\\AGEW_1DR.GRD')
    assert len(blob) == 40960, len(blob)
    return np.frombuffer(blob, dtype=np.uint8)


def render_block(z1x, z2x, z3x, z4x, dst0, darkfog):
    """ShowSuperFluentFog32_160_16() from fog.cpp.

    z1..z4 = top-left, top-right, bottom-left, bottom-right corner levels;
    dst0 = the 32x16 block of underlying screen pixels. Returns the rendered
    block (fast paths included: all <=63 -> unchanged, all >=96 -> black).
    """
    if z1x <= 63 and z2x <= 63 and z3x <= 63 and z4x <= 63:
        return dst0.copy()
    if z1x >= 96 and z2x >= 96 and z3x >= 96 and z4x >= 96:
        return np.zeros_like(dst0)
    z1, z2, z3, z4 = (v << 16 for v in (z1x, z2x, z3x, z4x))
    c = (z3 - z1) >> 4
    d = (z1 + z4 - z3 - z2) >> 9
    b = (z2 - z1) >> 5
    p = z1
    out = dst0.copy()
    levels = np.zeros((BLOCK_H, BLOCK_W), dtype=np.int64)
    for row in range(BLOCK_H):
        fog = p
        for i in range(BLOCK_W):
            levels[row, i] = fog >> 16
            out[row, i] = darkfog[((fog >> 8) & 0xFF00) + int(dst0[row, i])]
            fog += b
        b += d
        p += c
    return out, levels


def load_grass_block():
    """32x16 crop of the grass tile (tile 34) from tiles3.bmp, plus the BMP
    palette for display. Returns (block of palette indices, 256x3 palette)."""
    blob = extract(ARCHIVE, 'tiles3.bmp')
    img = Image.open(io.BytesIO(blob))
    pal = np.array(img.getpalette(), dtype=np.uint8).reshape(256, 3)
    px = np.array(img)  # (2368, 256) palette indices; tile j occupies rows j*64..j*64+64
    tile = px[34 * 64:35 * 64, :]           # 64x64 grass tile
    block = tile[16:16 + BLOCK_H, 32:32 + BLOCK_W]  # 32x16 crop
    return block.astype(np.uint8), pal


def main():
    outdir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        os.path.dirname(os.path.abspath(__file__)), '..', 'book', 'figures')
    os.makedirs(outdir, exist_ok=True)

    darkfog = load_darkfog()
    dst0, pal = load_grass_block()

    # one FogSpot at the block's top-left corner: corner words (8000,0,0,0)
    corners_word = np.array([[8000, 0], [0, 0]])  # [row][col]: z1 z2 / z3 z4
    z = np.vectorize(get_f)(corners_word)          # -> [[5,158],[158,158]]

    out, levels = render_block(int(z[0, 0]), int(z[0, 1]), int(z[1, 0]),
                               int(z[1, 1]), dst0, darkfog)

    fig, axes = plt.subplots(1, 4, figsize=(15, 3.8), constrained_layout=True)

    ax = axes[0]
    ax.imshow(z, cmap='gray_r', vmin=0, vmax=160)
    for r in range(2):
        for cc in range(2):
            ax.text(cc, r, f'z={z[r, cc]}', ha='center', va='center',
                    color='red', fontsize=11, weight='bold')
    ax.set_title('corner words (8000,0,0,0)\n-> GetF levels z1..z4', fontsize=9)
    ax.set_xticks([]); ax.set_yticks([])

    ax = axes[1]
    ax.imshow(levels, cmap='gray_r', vmin=0, vmax=160, interpolation='nearest')
    ax.set_title('bilinear 16.16 interpolation\nper pixel (fog>>16), 32x16', fontsize=9)
    ax.set_xticks([]); ax.set_yticks([])

    ax = axes[2]
    ax.imshow(pal[dst0], interpolation='nearest')  # grass in real palette colors
    ax.set_title('input block: grass tile crop\n(32x16 of tile 34, tiles3.bmp)', fontsize=9)
    ax.set_xticks([]); ax.set_yticks([])

    ax = axes[3]
    ax.imshow(pal[out], interpolation='nearest')   # darkened grass
    ax.set_title('rendered: darkfog[level*256+pixel]\n(top-left revealed, rest black)',
                 fontsize=9)
    ax.set_xticks([]); ax.set_yticks([])

    fig.suptitle('ShowSuperFluentFog32_160_16 (fog.cpp): one 32x16 block, one '
                 'FogSpot at its top-left corner; darkfog = real 0\\AGEW_1DR.GRD')
    png = os.path.join(outdir, 'fog_block_render.png')
    fig.savefig(png, dpi=150)
    plt.close(fig)
    print('wrote', png)


if __name__ == '__main__':
    main()
