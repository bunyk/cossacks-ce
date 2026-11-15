#!/usr/bin/env python3
"""fog_darkfog.py - visualize the darkfog table (0\\AGEW_1DR.GRD) itself.

.grd files are raw byte lists: LoadFog() in fog.cpp does RBlockRead() with a
fixed size and no header, so 0\\AGEW_1DR.GRD is exactly 40960 bytes =
160 darkness levels x 256 palette entries, indexed darkfog[level*256 + pixel].
Verified here: level 0 is the identity (darkfog[0][p] == p), level 158 maps
every pixel to 0 (black).

Output: book/figures/fog_darkfog_table.png - left: the whole 160x256 table
colored through the game palette (0\\AGEW_1.PAL, 768 raw bytes = 256 RGB
triplets); right: output luminance vs level for a few input colors (the
grass tile indices + mid/white).

Usage:  python3 tools/fog_darkfog.py [outdir]   (default: book/figures)
"""
import os
import sys

import numpy as np

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gsc_extract import extract  # noqa: E402

ARCHIVE = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                       '..', 'gamedata', 'all.gsc')


def main():
    outdir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        os.path.dirname(os.path.abspath(__file__)), '..', 'book', 'figures')
    os.makedirs(outdir, exist_ok=True)

    darkfog = np.frombuffer(extract(ARCHIVE, '0\\AGEW_1DR.GRD'),
                            dtype=np.uint8).reshape(160, 256)
    pal = np.frombuffer(extract(ARCHIVE, '0\\AGEW_1.PAL'),
                        dtype=np.uint8).reshape(256, 3)
    assert (darkfog[0] == np.arange(256)).all()      # level 0 = identity
    assert (darkfog[158] == 0).all()                 # level 158 = black

    lum = (pal.astype(np.float32) * [0.299, 0.587, 0.114]).sum(axis=1)

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5.2),
                                   constrained_layout=True,
                                   gridspec_kw={'width_ratios': [1.15, 1]})

    # left: the table as an image, each cell = palette color of the output
    ax1.imshow(pal[darkfog], interpolation='nearest', aspect='auto')
    ax1.set_xlabel('input palette index (pixel value) 0..255')
    ax1.set_ylabel('darkness level 0..159')
    ax1.set_title('darkfog[level*256 + pixel] colored via 0\\AGEW_1.PAL\n'
                  '(level 0 = identity row, level 158 = black row)', fontsize=10)
    ax1.set_yticks([0, 32, 64, 96, 128, 158])

    # right: luminance curves for representative input colors
    samples = [0, 45, 46, 84, 89, 91, 128, 200, 255]
    levels = np.arange(160)
    for p in samples:
        ax2.plot(levels, lum[darkfog[:, p]], lw=1.4,
                 label=f'in {p} ({pal[p].tolist()})')
    ax2.set_xlabel('darkness level')
    ax2.set_ylabel('output luminance (0..255)')
    ax2.set_title('darkening curves: output color luminance vs level\n'
                  '(grass-tile indices 45/46/84/89/91, black, mid, bright, white)',
                  fontsize=10)
    ax2.legend(fontsize=7, loc='upper right')
    ax2.grid(alpha=0.3)

    fig.suptitle('The darkfog table (0\\AGEW_1DR.GRD): 160 levels x 256 colors '
                 '= 40960 raw bytes, no header')
    png = os.path.join(outdir, 'fog_darkfog_table.png')
    fig.savefig(png, dpi=150)
    plt.close(fig)
    print('wrote', png)


if __name__ == '__main__':
    main()
