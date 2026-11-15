#!/usr/bin/env python3
"""m3d_areas.py - parse a Cossacks .m3d map save and render its topology
areas as PNG figures for book/020_topology.md, as plain square grids - the
same view the in-game minimap shows (only units/buildings are isometric, the
map arrays themselves are rendered square).

Format reference: book/010_m3d.md (chunk container + TOP1 chunk layout).
Verified against the case-study map "RN1 6F99 20210 000003JF 0000000.m3d"
(referenced by taras.sav/test.sav): TOP1 payload is consumed byte-exactly and
TopRef[area center] == area id for all NAreas.

Usage (case-study replay: Austria's base area 83, search origin = logged
(223,14) in search units = TopRef (446,28), enemy = logged (59,61) = TopRef
(118,122)):
    python3 tools/m3d_areas.py "gamedata/RN1 6F99 20210 000003JF 0000000.m3d" book/figures --at 446,28 --enemy 118,122
Islands figures for book sec. 4 (map referenced by gamedata/test2.sav.sav):
    python3 tools/m3d_areas.py "gamedata/RN0 3909 20213 00000EC6 0000000.m3d" book/figures --islands --at 201,211 --enemy 139,114
"""
import struct
import sys
import math
import os

import numpy as np
from PIL import Image, ImageDraw
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

# ---------------------------------------------------------------- container

def read_chunks(data):
    """12-byte header, then chunks: tag(4, reversed fourcc) + size(4, includes
    its own 4 bytes) + payload(size-4)."""
    magic, vert_in_line, max_th = struct.unpack_from('<III', data, 0)
    addsh = magic - 0x504D4433 + 1          # 'PMD3' multichar constant + ADDSH-1
    chunks = {}
    pos = 12
    while pos + 8 <= len(data):
        tag = data[pos:pos + 4][::-1].decode('latin1')
        size = struct.unpack_from('<I', data, pos + 4)[0]
        if size < 4 or pos + 4 + size > len(data):
            break
        chunks[tag] = data[pos + 8: pos + 4 + size]
        pos += 4 + size
    return addsh, vert_in_line, max_th, chunks

# ---------------------------------------------------------------- TOP1 chunk

def parse_top1(payload, top_lx, top_ly):
    """TOP1 payload: NAreas(4), then per area: 85-byte record + NMines words
    + NLinks (target,weight) word pairs; then MotionLinks, LinksDist
    (NAreas^2 words each), then TopRef (TopLx*TopLy words, row stride
    1<<TopSH == TopLx)."""
    pos = 0
    n_areas = struct.unpack_from('<I', payload, pos)[0]
    pos += 4
    areas = []
    for _ in range(n_areas):
        rec = payload[pos:pos + 85]
        pos += 85
        x, y = struct.unpack_from('<hh', rec, 0)
        importance, n_trees, n_stones = rec[4], rec[5], rec[6]
        n_mines = struct.unpack_from('<H', rec, 7)[0]
        n_links = struct.unpack_from('<H', rec, 13)[0]
        sinfs = [rec[21 + 8 * k: 21 + 8 * k + 8] for k in range(8)]
        mines = list(struct.unpack_from('<%dH' % n_mines, payload, pos)) if n_mines else []
        pos += 2 * n_mines
        links = []
        for _ in range(n_links):
            tgt, w = struct.unpack_from('<HH', payload, pos)
            pos += 4
            links.append((tgt, w))
        areas.append(dict(x=x, y=y, importance=importance, n_trees=n_trees,
                          n_stones=n_stones, mines=mines, links=links, sinfs=sinfs))
    motion_links = struct.unpack_from('<%dH' % (n_areas * n_areas), payload, pos)
    pos += 2 * n_areas * n_areas
    links_dist = struct.unpack_from('<%dH' % (n_areas * n_areas), payload, pos)
    pos += 2 * n_areas * n_areas
    top_ref = struct.unpack_from('<%dH' % (top_lx * top_ly), payload, pos)
    pos += 2 * top_lx * top_ly
    return areas, motion_links, links_dist, top_ref, pos, len(payload)

# ---------------------------------------------------------------- helpers

def area_color(aid):
    """Deterministic pleasant color per area id (splitmix64 finalizer - area
    ids correlate with position, so the hash must avalanche fully)."""
    h = (aid * 0x9E3779B97F4A7C15) & 0xFFFFFFFFFFFFFFFF
    h ^= h >> 30
    h = (h * 0xBF58476D1CE4E5B9) & 0xFFFFFFFFFFFFFFFF
    h ^= h >> 27
    h = (h * 0x94D049BB133111EB) & 0xFFFFFFFFFFFFFFFF
    h ^= h >> 31
    return (90 + (h & 0x7F), 90 + ((h >> 8) & 0x7F), 90 + ((h >> 16) & 0x7F))


def upscale(arr, scale):
    if scale > 1:
        arr = np.repeat(np.repeat(arr, scale, axis=0), scale, axis=1)
    return arr


def lut_image(lut, tr, scale):
    """Map the TopRef grid through a (65536,3) color LUT and upscale."""
    return Image.fromarray(upscale(lut[tr], scale), 'RGB')


def base_luts(n_areas):
    """Shared LUTs: dark=0xFFFF impassable/locked (water, border, locked
    cells), white=0xFFFE free land not assigned to an area, else per-area
    color."""
    lut = np.full((65536, 3), 255, np.uint8)
    lut[0xFFFF] = (30, 30, 38)
    for aid in range(n_areas):
        lut[aid] = area_color(aid)
    return lut


# ---------------------------------------------------------------- figures
# All figures are square renders of the stored arrays (minimap view), with
# no text drawn on the image - captions live in book/topology.md.

def water_at_tr(rows, cols, wd):
    """WaterDeep (fine-cell grid from the 2AES chunk) sampled at the given
    TopRef-level row/col indices: one TopRef cell = 2x2 fine cells; take the
    deepest corner sample. Water = WaterDeep > 128 (CheckDeep,
    3drandmap.cpp:210)."""
    wy = np.minimum(rows * 2 + 1, wd.shape[0] - 1)
    wx = np.minimum(cols * 2 + 1, wd.shape[1] - 1)
    return wd[np.ix_(wy, wx)]


def sea_level(th, tr, wd=None):
    """The sea level in THMap units: the modal height of the water cells.
    THMap does not encode the sea itself - the water mask is the separate
    WaterDeep grid (2AES chunk, WaterDeep > 128) - so under water the height
    stays at the flat ground base left by the generator (60 on the large
    RN1 map). Falls back to the modal impassable-cell height without 2AES."""
    cy = np.minimum(np.arange(th.shape[0]) // 2, tr.shape[0] - 1)
    cx = np.minimum(np.arange(th.shape[1]) // 2, tr.shape[1] - 1)
    th_tr = th[np.ix_(cy, cx)]
    if wd is not None:
        mask = water_at_tr(cy, cx, wd) > 128
    else:
        mask = tr[np.ix_(cy, cx)] == 0xFFFF
    return int(np.bincount(th_tr[mask].ravel()).argmax())


def fig_height(th, tr, out, wd=None):
    """Sec 1: SURF height chunk as a flat terrain-colormap relief (matplotlib
    'terrain'), with the TopRef 0xFFFF impassable/locked cells darkened (one
    TopRef cell = a 2x2 block of THMap vertices). The normalization is
    anchored at the sea level: sea level maps to t=0.25 - the blue->green
    boundary of 'terrain' - so lowland renders green and peaks white, instead
    of the whole lowland falling into the blue band. Returns the sea level."""
    cmap = plt.get_cmap('terrain')
    hi = int(th.max())
    sea = sea_level(th, tr, wd)
    vmin = (sea - 0.25 * hi) / 0.75          # t(sea)=0.25, t(hi)=1.0
    rgb = (cmap(plt.Normalize(vmin, hi)(th.astype(np.float64)))[..., :3] * 255).astype(np.uint8)
    cy = np.minimum(np.arange(th.shape[0]) // 2, tr.shape[0] - 1)
    cx = np.minimum(np.arange(th.shape[1]) // 2, tr.shape[1] - 1)
    impassable = tr[np.ix_(cy, cx)] == 0xFFFF
    rgb[impassable] = (30, 30, 38)
    Image.fromarray(rgb, 'RGB').save(out)
    return sea


def parse_2aes(payload):
    """2AES ('SEA2' on disk) payload: u32 Lx (=msx+2), u32 Ly (=msy+2), then Ly
    rows of (WaterDeep[Lx] bytes, WaterBright[Lx] bytes) - the water
    depth/brightness grids at fine-cell resolution (SaveWaterCost,
    savenewmap.cpp:1040). Water = WaterDeep > 128."""
    lx, ly = struct.unpack_from('<II', payload, 0)
    wd = np.zeros((ly, lx), np.uint8)
    for i in range(ly):
        wd[i] = np.frombuffer(payload, np.uint8, lx, 8 + i * 2 * lx)
    return wd


def fig_water(th, tr, wd, out):
    """Map-generation figure: sea vs land from the 2AES WaterDeep grid over
    the height relief. Water (WaterDeep > 128) is blue, darker = deeper
    (depth = WaterDeep-128); land keeps the terrain colors; TopRef 0xFFFF
    cells that are not water (ridges, map border) are darkened."""
    cmap = plt.get_cmap('terrain')
    hi = int(th.max())
    sea = sea_level(th, tr, wd)
    vmin = (sea - 0.25 * hi) / 0.75
    rgb = (cmap(plt.Normalize(vmin, hi)(th.astype(np.float64)))[..., :3] * 255).astype(np.uint8)
    cy = np.minimum(np.arange(th.shape[0]) // 2, tr.shape[0] - 1)
    cx = np.minimum(np.arange(th.shape[1]) // 2, tr.shape[1] - 1)
    tr2 = tr[np.ix_(cy, cx)]
    wdep = water_at_tr(cy, cx, wd)
    water = wdep > 128
    t = np.clip((wdep.astype(np.float64) - 128) / 120.0, 0, 1)   # shore -> deep
    rgb[water] = np.stack([(90 - 80 * t[water]).astype(np.uint8),
                           (160 - 120 * t[water]).astype(np.uint8),
                           (255 - 60 * t[water]).astype(np.uint8)], axis=1)
    rgb[(tr2 == 0xFFFF) & ~water] = (30, 30, 38)
    Image.fromarray(rgb, 'RGB').save(out)
    return int(water.sum()), int(water.size), sea


def fig_topref(lut_pass, tr, scale, out):
    """Sec 1: TopRef passability grid - dark = 0xFFFF impassable/locked
    (water, border), white = 0xFFFE free land with no area, light gray =
    area ids (passable land)."""
    lut_pass[0xFFFF] = (30, 30, 38)
    lut_pass[0xFFFE] = (255, 255, 255)
    for aid in range(lut_pass.shape[0]):
        if aid < 0xFFFE:
            lut_pass[aid] = (205, 205, 215)
    lut_image(lut_pass, tr, scale).save(out)


def fig_areas(lut, tr, scale, areas, out):
    """Sec 2: flood-filled areas colored by id, centers dotted, mine-bearing
    areas ringed."""
    img = lut_image(lut, tr, scale)
    d = ImageDraw.Draw(img)
    for aid, a in enumerate(areas):
        cx, cy = a['x'] * scale + scale // 2, a['y'] * scale + scale // 2
        r = max(2, scale)
        d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=(0, 0, 0))
        if a['mines']:
            r2 = 3 * scale
            d.ellipse([cx - r2, cy - r2, cx + r2, cy + r2], outline=(255, 0, 0), width=max(1, scale))
    img.save(out)


def fig_graph(lut, tr, scale, areas, out):
    """Sec 3: area graph - stored links as edges between area centers over a
    dimmed area map."""
    base = lut[tr].astype(np.float64)
    base = (base * 0.35 + 255 * 0.65).astype(np.uint8)   # dim
    img = Image.fromarray(upscale(base, scale), 'RGB')
    d = ImageDraw.Draw(img)
    for aid, a in enumerate(areas):
        ax, ay = a['x'] * scale + scale // 2, a['y'] * scale + scale // 2
        for tgt, w in a['links']:
            if tgt <= aid:
                continue
            b = areas[tgt]
            bx, by = b['x'] * scale + scale // 2, b['y'] * scale + scale // 2
            d.line([ax, ay, bx, by], fill=(30, 30, 200), width=1)
    for a in areas:
        cx, cy = a['x'] * scale + scale // 2, a['y'] * scale + scale // 2
        r = max(1, scale - 1)
        d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=(0, 0, 180))
    img.save(out)


def fig_dist(tr, scale, links_dist, n_areas, aid, out):
    """Sec 3: LinksDist heatmap from one area, drawn over the passability
    grid (viridis: dark = near, bright = far; gray = 0xFFFF unreachable;
    black = impassable 0xFFFF cells, white = free-unassigned 0xFFFE cells)."""
    ld = np.array(links_dist, dtype=np.uint16).reshape(n_areas, n_areas)
    row = ld[aid]
    trc = np.where(tr < n_areas, tr, 0)
    dv = row[trc].astype(np.float64)
    unreachable = (tr >= n_areas) | (dv == 0xFFFF)
    vmax = float(row[row != 0xFFFF].max())
    cmap = plt.get_cmap('viridis')
    t = np.clip(dv / vmax, 0, 1)
    t[unreachable] = 0
    rgb = (cmap(t)[..., :3] * 255).astype(np.uint8)
    rgb[unreachable] = (120, 120, 120)
    rgb[tr == 0xFFFF] = (30, 30, 38)    # impassable/locked: water, border
    rgb[tr == 0xFFFE] = (255, 255, 255)  # free land, no area assigned
    img = Image.fromarray(upscale(rgb, scale), 'RGB')
    img.save(out)


def fig_search(tr, scale, areas, links_dist, n_areas, aid, cx, cy, ex, ey, mmx, out):
    """Sec 8: replay SearchTowerPlace's strict first-tower search
    (newai.cpp:382, called from megapolis.cpp:1604 with r=40). The game works
    in search units (1 unit = 2 TopRef cells, SsMaxX = msx>>2 = 240), so ring
    i is at radius 2*i in TopRef cells. Cells on the enemy-facing half-plane
    are colored by the furthest stage they pass; PrevTopDst is tracked
    sequentially per ring, exactly as the code does."""
    n = tr.shape[0]
    ld = np.array(links_dist, dtype=np.uint16).reshape(n_areas, n_areas)
    row = ld[aid]
    stage_color = {1: (255, 200, 40), 2: (255, 120, 20),
                   3: (200, 60, 200), 4: (220, 60, 120), 5: (0, 200, 0)}
    # base: passability grid - dark = impassable/locked, white = free, no area
    lut = np.full((65536, 3), 235, np.uint8)
    lut[0xFFFF] = (30, 30, 38)
    lut[0xFFFE] = (255, 255, 255)
    rgb = lut[tr].copy()
    rarr = {}
    for ix in range(-160, 161):
        for iy in range(-160, 161):
            r = int(math.sqrt(ix * ix + iy * iy))
            if r < 160:
                rarr.setdefault(r, []).append((ix, iy))
    prev_top_dst = 0
    for i in range(6, 40):                      # search rings 6..r-1, r=40
        for xi, yi in rarr.get(2 * i, []):      # 1 search unit = 2 TopRef cells
            xx, yy = cx + xi, cy + yi
            if not (0 < xx < mmx and 0 < yy < mmx):
                continue
            if (xx - cx) * (ex - cx) + (yy - cy) * (ey - cy) <= 0:
                continue  # not enemy-facing
            top = int(tr[yy, xx])
            if top >= 0xFFFE:
                stage = 1                       # impassable/unassigned cell
            else:
                tdst = 0 if top == aid else int(row[top])
                if tdst == 0xFFFF or tdst - prev_top_dst >= 24:
                    stage = 2                   # topology distance rejects
                else:
                    if tdst > prev_top_dst:
                        prev_top_dst = tdst
                    stage = 4                   # distance ok
                    ok = all(int(tr[yy + dy, xx + dx]) < 0xFFFE
                             for dx in (0, 1) for dy in (0, 1))
                    if not (ok and 1 < xx < mmx - 1 and 1 < yy < mmx - 1):
                        stage = 3               # CheckTower 2x2 walkable fails
                    else:
                        stage = 5               # would be accepted
            rgb[yy, xx] = stage_color[stage]
    img = Image.fromarray(upscale(rgb, scale), 'RGB')
    d = ImageDraw.Draw(img)
    # ring markers and the direction to the enemy
    for i in (6, 20, 39):
        for xi, yi in rarr.get(2 * i, []):
            xx, yy = cx + xi, cy + yi
            if 0 <= xx < n and 0 <= yy < n:
                d.point((xx * scale, yy * scale), fill=(0, 0, 255))
    ax, ay = cx * scale, cy * scale
    bx, by = ex * scale, ey * scale
    d.line([ax, ay, bx, by], fill=(255, 0, 255), width=2)
    r = 4 * scale
    d.ellipse([ax - r, ay - r, ax + r, ay + r], outline=(0, 0, 255), width=max(1, scale))
    img.save(out)


# ---------------------------------------------------------------- islands
# Sec 4: faithful ports of ResearchIslands/ArrangeAreas (brigade.cpp:3650-3720)

def norma(x, y):
    """Octagonal distance metric (newmon.h:10): (2*max + min)/2 on |x|,|y|."""
    x, y = abs(x), abs(y)
    return (2 * x + y) // 2 if x > y else (2 * y + x) // 2


def compute_islands(areas):
    """Port of ResearchIslands (brigade.cpp:3663): TopIslands[area] = island
    id, found by a DFS over Area::Link with the game's Deep < 100 cap
    (ArrangeAreas), then the game's second pass that re-runs the DFS from
    every labeled area with a fresh depth budget. Iterative stack instead of
    recursion. Also computes island centers (IslandX/IslandY = average of
    the island's area centers). Returns (top_islands, n_islands, centers,
    max_depth, second_pass_labeled)."""
    n = len(areas)
    top_islands = [0xFFFF] * n
    n_islands = 0
    max_depth = 0
    second_pass_labeled = 0
    for j in range(n):
        if top_islands[j] != 0xFFFF:
            continue
        stack = [(j, 0)]                        # ArrangeAreas(j, n_islands, 0)
        while stack:
            aid, deep = stack.pop()
            if top_islands[aid] != 0xFFFF:
                continue
            top_islands[aid] = n_islands
            max_depth = max(max_depth, deep)
            if deep < 100:
                for tgt, _w in areas[aid]['links']:
                    if top_islands[tgt] == 0xFFFF:
                        stack.append((tgt, deep + 1))
        for p in range(n):                      # the game's second pass
            if top_islands[p] == n_islands:
                stack = [(p, 0)]                # ArrangeAreas(p, n_islands, 0)
                while stack:
                    aid, deep = stack.pop()
                    if top_islands[aid] != 0xFFFF:
                        continue
                    top_islands[aid] = n_islands
                    second_pass_labeled += 1
                    if deep < 100:
                        for tgt, _w in areas[aid]['links']:
                            if top_islands[tgt] == 0xFFFF:
                                stack.append((tgt, deep + 1))
        n_islands += 1
    sx = [0] * n_islands
    sy = [0] * n_islands
    cnt = [0] * n_islands
    for i, a in enumerate(areas):
        isl = top_islands[i]
        sx[isl] += a['x']
        sy[isl] += a['y']
        cnt[isl] += 1
    centers = [(sx[i] // cnt[i], sy[i] // cnt[i]) for i in range(n_islands)]
    return top_islands, n_islands, centers, max_depth, second_pass_labeled


def compute_nearwater(tr, wtr, areas, wareas, msx):
    """Port of the NearWater pass of ResearchIslands (brigade.cpp:3685+):
    for every land TopRef cell, scan the 7x7 neighborhood in the water grid
    WTopRef; NearWater[land area] = water area whose center is nearest
    (Norma) to the land area's center. 0xFFFF = no water area within 7
    cells. Used for port/landing logic."""
    near_water = [0xFFFF] * len(areas)
    tpmax = msx >> 1
    for iy in range(3, tpmax - 3):
        for ix in range(3, tpmax - 3):
            ctp = int(tr[iy, ix])
            if ctp >= 0xFFFE:
                continue
            car = areas[ctp]
            for dy in range(-3, 4):
                for dx in range(-3, 4):
                    wtp = int(wtr[iy + dy, ix + dx])
                    if wtp >= 0xFFFE:
                        continue
                    old = near_water[ctp]
                    if old == 0xFFFF:
                        near_water[ctp] = wtp
                    else:
                        o, nw = wareas[old], wareas[wtp]
                        if norma(car['x'] - o['x'], car['y'] - o['y']) > \
                           norma(car['x'] - nw['x'], car['y'] - nw['y']):
                            near_water[ctp] = wtp
    return near_water


def fig_islands(tr, scale, top_islands, centers, out):
    """Sec 4: connected components of the area link graph (ResearchIslands) -
    TopRef cells colored by island id, island centers (avg of area centers)
    dotted black."""
    lut = np.full((65536, 3), 255, np.uint8)
    lut[0xFFFF] = (30, 30, 38)
    for aid, isl in enumerate(top_islands):
        lut[aid] = area_color(isl)
    img = lut_image(lut, tr, scale)
    d = ImageDraw.Draw(img)
    for ix, iy in centers:
        cx, cy = ix * scale + scale // 2, iy * scale + scale // 2
        r = 2 * scale
        d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=(0, 0, 0))
    img.save(out)


def fig_nearwater(lut, tr, scale, areas, near_water, out):
    """Sec 4: areas themselves (colored by area id, as in fig_areas) with a
    dot at each area center - blue if NearWater != 0xFFFF (a water area
    within 7 TopRef cells), black otherwise."""
    img = lut_image(lut, tr, scale)
    d = ImageDraw.Draw(img)
    for aid, nw in enumerate(near_water):
        a = areas[aid]
        cx, cy = a['x'] * scale + scale // 2, a['y'] * scale + scale // 2
        r = max(2, scale)
        fill = (0, 60, 255) if nw != 0xFFFF else (0, 0, 0)
        d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=fill)
    img.save(out)


# ---------------------------------------------------------------- main

def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    path, outdir = args[0], args[1]
    os.makedirs(outdir, exist_ok=True)
    at_xy = None
    if '--at' in sys.argv:
        i = sys.argv.index('--at')
        at_xy = tuple(int(v) for v in sys.argv[i + 1].split(','))
    enemy_xy = None
    if '--enemy' in sys.argv:
        i = sys.argv.index('--enemy')
        enemy_xy = tuple(int(v) for v in sys.argv[i + 1].split(','))
    data = open(path, 'rb').read()
    addsh, vert_in_line, max_th, chunks = read_chunks(data)
    map_sx = 512 << addsh
    top_lx = top_ly = map_sx >> 2
    top_sh = 7 + addsh
    assert (1 << top_sh) == top_lx, 'TopRef row stride must be 1<<TopSH == TopLx'
    msx = 240 << addsh
    mmx = mmy = msx >> 1
    print('file=%s size=%d ADDSH=%d VertInLine=%d MaxTH=%d MAPSX=%d TopLx=%d msx=%d mmx=%d'
          % (path, len(data), addsh, vert_in_line, max_th, map_sx, top_lx, msx, mmx))
    print('chunks:', {k: len(v) for k, v in chunks.items()})
    if 'AIIA' in chunks:
        lt, rs, rt = struct.unpack('<III', chunks['AIIA'])
        print('AIIA: landtype=%d (0=Land,1=Mediterranean,2=Peninsulas,3=Islands,'
              '4=Continents,5=Continent, ai_common.h:51) resstart=%d restot=%d' % (lt, rs, rt))

    areas, motion_links, links_dist, top_ref, consumed, total = parse_top1(
        chunks['1POT'], top_lx, top_ly)
    n = len(areas)
    print('TOP1: NAreas=%d  consumed=%d payload=%d (leftover=%d)' % (n, consumed, total, total - consumed))
    tr = np.array(top_ref, dtype=np.uint16).reshape(top_lx, top_ly)
    th = np.array(struct.unpack('<%dH' % (len(chunks['FRUS']) // 2), chunks['FRUS']),
                  dtype=np.uint16).reshape(len(chunks['FRUS']) // 2 // vert_in_line, vert_in_line)
    n_imp = int((tr == 0xFFFF).sum())    # impassable/locked: water, border, locked cells
    n_unas = int((tr == 0xFFFE).sum())   # free land, no area assigned
    n_link_cells = int((tr < n).sum())
    n_links = sum(len(a['links']) for a in areas)
    w = [w for a in areas for _, w in a['links']]
    n_unreach = sum(1 for v in links_dist if v == 0xFFFF)
    print('TopRef: impassable=%d free-unassigned=%d area-cells=%d (of %d)' % (n_imp, n_unas, n_link_cells, top_lx * top_ly))
    print('links=%d weights min=%d max=%d mean=%.1f' % (n_links, min(w), max(w), sum(w) / len(w)))
    print('LinksDist: 0xFFFF entries=%d/%d  diagonal[0][0]=0x%04X' % (n_unreach, n * n, links_dist[0]))
    print('areas with mines: %d, total mine refs=%d' % (sum(1 for a in areas if a['mines']), sum(len(a['mines']) for a in areas)))
    # sanity: TopRef[area center] == area id
    ok = sum(1 for i, a in enumerate(areas) if top_ref[a['x'] + (a['y'] << top_sh)] == i)
    print('TopRef[center]==id: %d/%d' % (ok, n))

    wd = parse_2aes(chunks['2AES']) if '2AES' in chunks else None
    scale = 2
    lut = base_luts(n)
    lut_pass = np.full((65536, 3), 235, np.uint8)
    sea = fig_height(th, tr, os.path.join(outdir, 'topo_map_height.png'), wd)
    print('height: min=%d max=%d sea level (modal WaterDeep>128 cell height)=%d' % (int(th.min()), int(th.max()), sea))
    if '--water' in sys.argv and wd is not None:
        n_water, n_cells, _ = fig_water(th, tr, wd, os.path.join(outdir, 'topo_map_water.png'))
        print('2AES: WaterDeep grid %dx%d, water cells (WaterDeep>128) at TopRef res=%d/%d, range %d..%d'
              % (wd.shape[1], wd.shape[0], n_water, n_cells, int(wd.min()), int(wd.max())))
    fig_topref(lut_pass.copy(), tr, scale, os.path.join(outdir, 'topo_map_topref.png'))
    fig_areas(lut.copy(), tr, scale, areas, os.path.join(outdir, 'topo_map_areas.png'))
    fig_graph(lut.copy(), tr, scale, areas, os.path.join(outdir, 'topo_map_graph.png'))

    if '--islands' in sys.argv:
        top_islands, n_islands, centers, max_depth, sp_labeled = compute_islands(areas)
        sizes = [0] * n_islands
        for isl in top_islands:
            sizes[isl] += 1
        order = sorted(range(n_islands), key=lambda i: -sizes[i])
        print('Islands: NIslands=%d (MaxIsl cap is 64)  sizes desc=%s  maxDFSdepth=%d  secondPassLabeled=%d'
              % (n_islands, [sizes[i] for i in order], max_depth, sp_labeled))
        print('island centers (avg of area centers), first 8: %s' % (centers[:8],))
        fig_islands(tr, scale, top_islands, centers,
                    os.path.join(outdir, 'topo_map_islands.png'))
        if 'WPOT' in chunks:
            wareas, _wml, _wld, wtop_ref, wc, wt = parse_top1(chunks['WPOT'], top_lx, top_ly)
            wtr = np.array(wtop_ref, dtype=np.uint16).reshape(top_lx, top_ly)
            print('WPOT: WNAreas=%d  consumed=%d payload=%d (leftover=%d)'
                  % (len(wareas), wc, wt, wt - wc))
            near_water = compute_nearwater(tr, wtr, areas, wareas, msx)
            n_nw = sum(1 for v in near_water if v != 0xFFFF)
            print('NearWater: areas with a water area within 7 cells=%d/%d' % (n_nw, n))
            fig_nearwater(lut.copy(), tr, scale, areas, near_water,
                          os.path.join(outdir, 'topo_map_nearwater.png'))

    if at_xy:
        aid = int(tr[at_xy[1], at_xy[0]])
    else:
        aid = 0
        counts = {}
        for y in range(mmy):
            for x in range(mmx):
                v = top_ref[x + (y << top_sh)]
                if v < n:
                    counts[v] = counts.get(v, 0) + 1
        best = None
        for i, a in enumerate(areas):
            ws = [ww for _, ww in a['links']]
            wmin = min(ws) if ws else 0xFFFF
            c = counts.get(i, 0)
            if c > 0 and wmin != 0xFFFF:
                edge = min(a['x'], mmx - a['x'], a['y'], mmy - a['y'])
                score = (-wmin, edge, -c)
                if best is None or score < best[0]:
                    best = (score, i, wmin, c)
        aid, wmin, cells = best[1], best[2], best[3]
    a = areas[aid]
    ws = [ww for _, ww in a['links']]
    print('source area: id=%d center=(%d,%d) minlinkw=%s' % (aid, a['x'], a['y'], min(ws) if ws else None))
    if enemy_xy:
        ex, ey = enemy_xy
    else:
        ex = sum(b['x'] for b in areas) // len(areas)
        ey = sum(b['y'] for b in areas) // len(areas)
    print('enemy point: (%d,%d) -> area %d center=(%d,%d)'
          % (ex, ey, tr[ey, ex], areas[int(tr[ey, ex])]['x'], areas[int(tr[ey, ex])]['y']))
    cx, cy = at_xy if at_xy else (a['x'], a['y'])
    fig_dist(tr, scale, links_dist, n, aid, os.path.join(outdir, 'topo_map_dist.png'))
    fig_search(tr, scale, areas, links_dist, n, aid, cx, cy, ex, ey, mmx,
               os.path.join(outdir, 'topo_map_search.png'))
    print('figures written to', outdir)


if __name__ == '__main__':
    main()
