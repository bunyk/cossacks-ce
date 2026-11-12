# The `.m3d` map file: container, chunks, and the stored topology

A walk-through of the binary format of `gamedata/*.m3d` - the saved 3D maps
that skirmish games and editor maps live in. Everything below was verified
byte-exact against the shipped GOG maps (`scene1.m3d`, `MyFirst.m3d`,
`test.m3d`) by the parser in `tools/m3d_areas.py`; the example numbers are
real offsets and sizes from `gamedata/test.m3d` (3,163,177 bytes).

All integers are **little-endian**. All structs are stored in the raw
**32-bit Windows `pack(1)` layout** - 4-byte pointer placeholders included.
See section 4 for why reading them with a 64-bit `sizeof(struct)` corrupts the
stream.

## 0. The 60-second version

```
.m3d file:
  header (12 bytes): u32 magic, u32 VertInLine, u32 MaxTH
  then chunks until EOF:
      tag   : 4 bytes, a fourcc stored REVERSED ("WEIV" in the file = 'WEIV'
              as a C multichar constant = "VIEW" read backwards)
      size  : u32, includes its own 4 bytes
      data  : size-4 bytes

header magic encodes the map size class:
  magic = 'PMD3' + ADDSH - 1        (C multichar constant 'PMD3' = 0x504D4433)
  ADDSH = 1 normal, 2 large (2x), 3 huge (4x)
  derived sizes (loadsave.cpp SetupArrays):
      MAPSX = MAPSY = 512 << ADDSH     TopLx = TopLy = MAPSX >> 2
      msx   = msy   = 240 << ADDSH     TopSH = 7 + ADDSH
      MaxSector = 128 << ADDSH         MaxTH = 2*MaxSector
      VertInLine = MaxTH + 1

test.m3d header bytes:  33 44 4d 50 | 01 02 00 00 | 00 02 00 00
                         magic ADDSH=1   VertInLine=513   MaxTH=512
```

## 1. The chunk walk

After the 12-byte header, chunks are back to back. The loader
(`Load3DMap`, savenewmap.cpp:2142) does exactly:

```c
int posit = 12;
do {
    RBlockRead( f1, &sign, 4 );      // tag, reversed fourcc
    RBlockRead( f1, &size, 4 );      // includes its own 4 bytes!
    posit += 4 + size;               // next chunk starts here
    switch ( sign ) { case 'FRUS': LoadSurface(f1); break; ... }
} while ( ... );
```

So in Python: `pos = 12; tag = data[pos:pos+4][::-1]; size = u32(pos+4);
payload = data[pos+8 : pos+4+size]; pos += 4 + size`. Note the off-by-four
trap: the stride is `4 + size`, not `8 + size`, because `size` counts the
size field itself.

## 2. Chunk inventory of a real map

`test.m3d` (ADDSH=1) walks to exactly EOF with this layout:

| offset | tag (file bytes, reversed) | C constant | payload bytes | writer | content |
|---|---|---|---|---|---|
| 12 | `WEIV` | `'WEIV'` | 55,480 | `SavePreview` | 292x190-byte minimap preview image |
| 55,500 | `FRUS` | `'FRUS'` | 525,312 | `SaveSurface` | `THMap`: (MaxTH+1)*MaxTH words = 513x512 height grid |
| 580,820 | `ELIT` | `'ELIT'` | 262,656 | `SaveTiles` | `TexMap`: (MaxTH+1)*MaxTH bytes = tile index per height cell |
| 843,484 | `TCES` | `'TCES'` | 786,432 | `SaveSect` | `SectMap`: MaxSector*MaxTH*6 bytes of sector data |
| 1,629,924 | `EERT` | `'EERT'` | 75,988 | `SaveSprites` | u32 count, then count x 12-byte sprite records (trees/stones/holes/mines...) |
| 1,705,920 | `3INU` | `'3INU'` | 9,346 | `SaveUnits3` | placed units, v3 record format (older: `2INU`, `TINU`) |
| 1,715,274 | `2LAW` | `'2LAW'` | 4 | `SaveNewWallsV2` | walls v2 (older: `1LAW`, `LLAW`) |
| 1,715,286 | `1COL` | `'1COL'` | 80 | `SaveLockNew` | u32 NBars, u32 NUnLockBars, then the 4-byte BlockCell arrays |
| 1,715,374 | `2AES` | `'2AES'` | 464,656 | `SaveWaterCost` | water/sea cost map |
| 2,180,038 | `USER` | `'USER'` | 256 | `SaveRES` | per-nation resource setup |
| 2,180,302 | `1POT` | `'1POT'` | 770,735 | `SaveTopology` | **the area graph - section 3** |
| 2,951,045 | `WPOT` | `'WPOT'` | 177,027 | `SaveWTopology` | water area graph (`GTOP[1]`) |
| 3,128,080 | `1TAG` | `'1TAG'` | 98 | `SaveGates` | gates |
| 3,128,186 | `1NOZ` | `'1NOZ'` | 1,430 | `SaveZonesAndGroups` | u32 NAZones, u32 NAGroups, then 17-byte ActiveZone + 49-byte ActiveGroup records |
| 3,129,624 | `1ROF` | `'1ROF'` | 525 | `SaveFormationsNew` | formations |
| 3,130,157 | `LLD.` | `'LLD.'` | 200 | `SaveDLL` | 200-byte mission DLL name |
| 3,130,365 | `AIIA` | `'AIIA'` | 12 | `SaveAIParam` | AI parameters (land type etc.) |
| 3,130,385 | `SMSP` | `'SMSP'` | 4 | `SaveSMSInMap` | sound-map marker |
| 3,130,397 | `1VIR` | `'1VIR'` | 32,768 | `SaveRivers` | river direction/volume grids (RivNX^2 each) |

Tags are the C multichar constants written raw, so they appear byte-reversed
in the file: `'FRUS'` is stored as `46 52 55 53` ("FRUS"), which is why the
table shows both spellings.

The height grid is a diamond (isometric) projection: `GetHeight(x, y)`
(mapsprites.cpp:400) takes length units (32 per fine cell), computes
`nx = x / 32` and indexes `THMap[nx + y1*VertInLine]` - that is why the grid
is `MaxTH+1` wide per row (`VertInLine = MaxTH+1`).

## 3. The `1POT` (TOP1) topology chunk, byte-exact

This is the chunk that stores the area graph from
[topology.md](topology.md). Layout of the payload (after tag+size):

```
u32 NAreas
NAreas times:
    85-byte Area record (table below)
    u16 MinesIdx[NMines]          (sprite indices of mines/resources)
    NLinks times (u16 targetArea, u16 weight)   // Area::Link pairs
u16 MotionLinks[NAreas*NAreas]
u16 LinksDist[NAreas*NAreas]
u16 TopRef[TopLx*TopLy]
```

The 85-byte on-disk `Area` record (pack(1), 32-bit - `ReadAreaFromMapFile`,
savenewmap.cpp:1124):

| offset | size | field | notes |
|---|---|---|---|
| 0 | 2 | `x` (i16) | area center, topology-cell units |
| 2 | 2 | `y` (i16) | |
| 4 | 1 | `Importance` | mine bits: 1=gold, 2=iron, 4=coal |
| 5 | 1 | `NTrees` | |
| 6 | 1 | `NStones` | |
| 7 | 2 | `NMines` | count of MinesIdx entries |
| 9 | 4 | `MinesIdx` pointer | **placeholder, meaningless on disk** |
| 13 | 2 | `NLinks` | count of Link pairs |
| 15 | 4 | `Link` pointer | **placeholder** |
| 19 | 2 | `MaxLink` | allocated link capacity |
| 21 | 64 | `SINF[8]` | 8 nations x 8 bytes: u16 BuildInfo (SI_* bits) + 6 x u8 unit counts |

Matrices are indexed `[from*NAreas + to]`; `MotionLinks` holds the next hop,
`LinksDist` the shortest distance. `0xFFFF` = unreachable - and the diagonal
`[a][a]` is *always* `0xFFFF`, because the diagonal is never written (see the
comment at `ClearLinkInfo` in topograf.cpp). `TopRef` uses `0xFFFF` =
unassigned, `0xFFFE` = impassable, otherwise the area id.

Parsed with `tools/m3d_areas.py`, the `1POT` payload of `test.m3d` consumes
exactly 770,735 of 770,735 bytes (zero leftover), giving:

```
NAreas = 387
TopRef (256x256): 4 impassable (0xFFFE), 24,386 unassigned border (0xFFFF),
                  41,146 area cells
links  = 1,922 directed, weights min 8 / max 25 / mean 12.3
LinksDist: 85,311 of 149,769 entries are 0xFFFF; diagonal[0][0] = 0xFFFF
```

## 4. The 32-bit trap (why the port serializes explicitly)

The on-disk structs are the original 32-bit `pack(1)` layouts, where a pointer
is 4 bytes. On 64-bit, `sizeof(Area)` is 93, not 85 (8-byte pointers), so a raw
`fread(&area, sizeof(Area), ...)` desyncs the whole block stream - the code
comment at savenewmap.cpp:1112 notes this produced garbage `NLinks`/`MaxLink`
and heap corruption before `ReadAreaFromMapFile`/`WriteAreaToMapFile`
made the 85-byte layout explicit. Same story for `ActiveZone` (17 bytes on
disk) and `ActiveGroup` (49 bytes). Pointer fields on disk are zero/garbage
placeholders; the loader re-creates the arrays and never reads them.

## 5. Which files carry `1POT`

There are two writers, and you can tell them apart by the chunks present:

* `Save3DMap` (savenewmap.cpp:2040) writes **everything**, including the
  preview, sectors, gates and `1POT`/`WPOT`. This is what the map editor
  saves (`test.m3d`, `scene1.m3d`, `MyFirst.m3d`) **and** what random-map
  generation saves: `GenerateMap` ends with `Save3DMap(Name)`
  (3drandmap.cpp:1702), so the cached `RN*.m3d` files contain a ready-made
  topology (verified: `RN1 ....m3d` chunks include `1POT`, `WPOT`, `PEAC`).
  Example: the game saves `gamedata/taras.sav` and `gamedata/test.sav` both
  name `CurrentMap = "RN1 6F99 20210 000003JF 0000000.m3d"` in their `SaveMap`
  block (see [sav_format.md](sav_format.md)), i.e. both were played on that
  one large random map.
* `FastSave3DMap` (savenewmap.cpp:2100) is the mid-game autosave
  (`MapAutosave.m3d`). It has `//SaveTopology(f1); //SaveWTopology(f1);`
  **commented out** and also skips preview/sectors/gates - verified:
  `MapAutosave.m3d` chunks are `FRUS ELIT EERT 3INU 2LAW 1COL 2AES USER
  1NOZ 1ROF LLD. AIIA SMSP PEAC 1VIR`, no `1POT`.

On load, `Load3DMap` simply never hits the `'1POT'` case for such files, and
the graph is rebuilt from the terrain instead: `CreateTotalLocking()` ->
`if (!NAreas) CreateAreas()` (3dmaped.cpp:1148, called from the load/start
paths in interface.cpp and savenewmap.cpp:2302). So `1POT` present = a map
file (editor or random-map cache); absent = a runtime autosave.

## 6. Minimal parser

`tools/m3d_areas.py` implements the walk above and the `1POT` layout, and
renders the stored areas/graph/distances as PNGs (the figures in
[topology.md](topology.md)):

```
python3 tools/m3d_areas.py "gamedata/RN1 6F99 20210 000003JF 0000000.m3d" book/figures --at 187,9
```

## 7. Why it looks like this

`.m3d` is a straight struct-dump of the engine's in-memory map state, chunked
so each subsystem can be versioned independently (walls even have three
generations: `LLAW`/`1LAW`/`2LAW`). That is typical of late-90s/2001 game
formats: no compression, no alignment, little-endian x86 layout, magic fourccs
stored as reversed multichar constants. Today you would use a real container
(ZIP/ pak with per-chunk versioning) or a data-oriented format (FlatBuffers /
Cap'n Proto) where the 32-bit-vs-64-bit pointer problem cannot exist - the
explicit 85-byte record layout in section 3 is exactly the hand-rolled version
of that fix.
