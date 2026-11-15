#!/usr/bin/env python3
"""fog_spot.py - animate the fog-of-war diffusion (fog.cpp) for one moving spot.

Replicates, cell for cell, the logic of src/game/fog.cpp for ADDSH=1:
  - grid: fmap is LX_fmap x LX_fmap words, LX_fmap = 134 << (ADDSH-1) = 134,
    row stride FMSX = LX_fmap, plus a kFogOffset = 3-cell border;
  - FogSpot(x, y): fx = (x+64)/128 + kFogOffset (clamped to [1..maxFX]),
    fmap[fy][fx] = 8000 (kFogRevealValue);
  - FogBlurPass(): per frame, copy fmap -> fmap1, then over the active
    sub-rect (rows/cols 1..(msx/4+6)-1, msx = 240 << ADDSH):
        sum  = (up + down + left + right)  as 16-bit (wraps)
        dst  = (sum >> 2) - (sum >> 10)    as 16-bit (wraps)
    (numpy uint16 arithmetic wraps mod 2^16 exactly like the C word casts).

The animation moves ONE FogSpot (VisionType-0 style, a single cell) in a
straight line left to right at CELLS_PER_STEP cells per step (a real unit
needs ~64 frames per 128px fog cell, see GEOMETRY/MotionDist in
newmon.cpp), stamping it and running one blur pass per step - so the
frames show both the reveal at the current position and the decaying
trail behind it. FRAME_SKIP steps are skipped between displayed frames.

Output: book/figures/fog_spot_frames.png (every FRAME_SKIP-th frame, cropped
        to a small window around the path, for the book)
        book/figures/fog_spot_anim.gif   (the same skipped frames, as an animation)

Usage:  python3 tools/fog_spot.py [outdir]     (default: book/figures)
"""
import os
import sys

import numpy as np
from PIL import Image

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

# Constants from fog.cpp
K_FOG_OFFSET = 3
K_FOG_CELL = 128          # one fog cell = 128x128 map pixels
K_FOG_REVEAL = 8000       # kFogRevealValue
K_MINI_FOG_THRESHOLD = 1300  # kMiniFogThreshold (minimap blackout level)
ADDSH = 1
MSX = 240 << ADDSH        # map width in fine cells (interface.cpp: msx = 240<<ADDSH)
LX_FMAP = 134 << (ADDSH - 1)
MAX_FX = (MSX // 4) + K_FOG_OFFSET   # fog-grid clamp used by FogSpot/GetFog

CELLS_PER_STEP = 0.1  # spot speed in fog cells per step (real unit: ~1/64)
N_FRAMES = 15         # frames shown (and simulated / FRAME_SKIP)
FRAME_SKIP = 10       # show every FRAME_SKIP-th step
N_STEPS = N_FRAMES * FRAME_SKIP
PATH_CELLS = 15       # total path length in cells (left to right)
VIEW_RX = 10          # crop: +-VIEW_RX cells around path center in x
VIEW_RY = 9           # crop: +-VIEW_RY cells around path center in y
N_COLS = 5            # PNG grid columns (3 rows)


def fog_spot(fmap, x, y):
    """FogSpot() from fog.cpp: stamp the cell containing map point (x, y)."""
    fx = (x + K_FOG_CELL // 2) // K_FOG_CELL + K_FOG_OFFSET
    fy = (y + K_FOG_CELL // 2) // K_FOG_CELL + K_FOG_OFFSET
    fx = min(max(fx, 1), MAX_FX)
    fy = min(max(fy, 1), MAX_FX)
    fmap[fy, fx] = K_FOG_REVEAL


def fog_blur_pass(fmap, fmap1):
    """FogBlurPass() from fog.cpp: in-place blur+decay over the active sub-rect."""
    fmap1[:] = fmap
    S = LX_FMAP  # row stride in words
    rows = (MSX >> 2) + 2 * K_FOG_OFFSET - 1
    cols = rows
    src = fmap1
    s = (src[0:rows, 1:1 + cols] + src[2:2 + rows, 1:1 + cols] +
         src[1:1 + rows, 0:cols] + src[1:1 + rows, 2:2 + cols])  # uint16 wraps
    fmap[1:1 + rows, 1:1 + cols] = (s >> 2) - (s >> 10)          # uint16 wraps


def simulate():
    """Move one FogSpot left to right; returns (all_frames, shown_frames, center).

    all_frames:   every step - (title, fmap)
    shown_frames: every FRAME_SKIP-th step, for PNG and GIF
    center:       path center in pixel coords
    """
    fmap = np.zeros((LX_FMAP, LX_FMAP), dtype=np.uint16)
    fmap1 = np.zeros_like(fmap)
    cx = (MAX_FX // 2) * K_FOG_CELL  # path center in map pixels
    cy = cx
    x0 = cx - (PATH_CELLS // 2) * K_FOG_CELL

    all_frames = []
    for k in range(N_STEPS):
        fog_blur_pass(fmap, fmap1)
        fog_spot(fmap, int(x0 + CELLS_PER_STEP * K_FOG_CELL * k), cy)
        all_frames.append((f'step {k}', fmap.copy()))
    shown = all_frames[::FRAME_SKIP]
    return all_frames, shown, (cx, cy)


def main():
    outdir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
        os.path.dirname(os.path.abspath(__file__)), '..', 'book', 'figures')
    os.makedirs(outdir, exist_ok=True)

    all_frames, shown, (cx, cy) = simulate()
    # crop to a small window around the path center
    ccx = (cx + K_FOG_CELL // 2) // K_FOG_CELL + K_FOG_OFFSET
    ccy = (cy + K_FOG_CELL // 2) // K_FOG_CELL + K_FOG_OFFSET
    x0, x1 = ccx - VIEW_RX, ccx + VIEW_RX + 1
    y0, y1 = ccy - VIEW_RY, ccy + VIEW_RY + 1

    n = len(shown)
    ncols = N_COLS
    nrows = (n + ncols - 1) // ncols
    fig, axes = plt.subplots(nrows, ncols, figsize=(15, 3.2 * nrows),
                             constrained_layout=True)
    im = None
    for ax, (title, fmap) in zip(axes.flat, shown):
        crop = fmap[y0:y1, x0:x1]
        im = ax.imshow(crop, cmap='gray', vmin=0, vmax=K_FOG_REVEAL,
                       interpolation='nearest')
        ax.contour(crop, levels=[K_MINI_FOG_THRESHOLD], colors=['red'],
                   linewidths=0.5)  # boundary between under/above kMiniFogThreshold
        ax.set_title(title, fontsize=9)
        ax.set_xticks([]); ax.set_yticks([])
    for ax in list(axes.flat)[n:]:
        ax.axis('off')
    cbar = fig.colorbar(im, ax=axes, shrink=0.6)
    cbar.set_label('fmap cell value (0 = unexplored, 8000 = fresh spot)')
    fig.suptitle('One FogSpot moving left to right at %.2f cells/step, one '
                 'blur pass per step, every %dth of %d steps shown '
                 '(fog.cpp: FogSpot + FogBlurPass, ADDSH=1)'
                 % (CELLS_PER_STEP, FRAME_SKIP, N_STEPS))
    png = os.path.join(outdir, 'fog_spot_frames.png')
    fig.savefig(png, dpi=150)
    plt.close(fig)

    # animated GIF of the shown frames (grayscale + red threshold line)
    imgs = []
    for title, fmap in shown:
        crop = fmap[y0:y1, x0:x1]
        g = (crop.astype(np.float32) / K_FOG_REVEAL * 255).astype(np.uint8)
        rgb = np.stack([g, g, g], axis=-1)
        # thin red line between cells under and above kMiniFogThreshold
        above = crop >= K_MINI_FOG_THRESHOLD
        boundary = above & ~np.roll(above, 1, 0) | above & ~np.roll(above, -1, 0) \
            | above & ~np.roll(above, 1, 1) | above & ~np.roll(above, -1, 1)
        rgb[boundary] = (255, 0, 0)
        img = Image.fromarray(rgb).resize((g.shape[1] * 12, g.shape[0] * 12),
                                          Image.NEAREST)
        imgs.append(img)
    gif = os.path.join(outdir, 'fog_spot_anim.gif')
    imgs[0].save(gif, save_all=True, append_images=imgs[1:] + imgs[:1],
                 duration=300, loop=0)
    print('wrote', png)
    print('wrote', gif)


if __name__ == '__main__':
    main()
