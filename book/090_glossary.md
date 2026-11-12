# Glossary

Terms as used in this book and in the code (`src/game`). Where a term is
explained in depth, the chapter is linked. All code references name functions
and structs rather than line numbers, since the code keeps moving.

## Area (topology area)

A **connected blob of passable land** on the coarse topology grid - a node of
the area graph built by `CreateAreas()` (topograf.cpp); see
[020_topology.md](020_topology.md). Stored as `struct Area` (topograf.h):
center cell `x, y`, resource census (`Importance` bits 1=gold/2=iron/4=coal,
`NMines`/`MinesIdx`), links `Link[]` to touching areas with `Norma`
center-distance weights, and the per-nation strategic census `SINF[8]`.

Areas are grown by flood fill from random seed points (8-neighborhood, rings
`Rarr[1..89]`), and every cell of an area holds the area id in `TopRef` (the id
is also stored back at the center cell). Two consequences:

* **Connected, but not convex.** Any two cells of one area are joined by a
  path of 8-neighbor cells inside the area (that is how the fill grew), but the
  shape is arbitrary - it wraps around water, ridges and the map border, so it
  can be concave and can enclose impassable holes.
* **Passable at creation, not guaranteed later.** A cell joins an area only
  while it is free, but at runtime a building or wall locks its cells
  (`DynamicalLockTopCell` sets them to `0xFFFF`) and the affected areas are
  rebuilt locally (`ProcessDynamicalTopology` / `RemakeGroupOfAreas`). So an
  area id on a cell means "passable land belonging to this area", and a unit
  standing on a locked cell gets its area through the ring-scan fallback in
  `GetTopology` instead.

`NAreas` is capped at 1016 (`AddArea`).

## TopRef

`word TopRef[TopLx*TopLy]` (topograf.cpp), indexed `x + (y << TopSH)` with
`TopLx = MAPSX>>2 = 128<<ADDSH` and `TopSH = 7+ADDSH`; one cell = 64 px. Values:

* `< 0xFFFE` - area id (0..NAreas-1): passable land assigned to that area;
* `0xFFFE` - free passable land not assigned to any area (the value
  `DynamicalUnLockTopCell` restores when a building is removed);
* `0xFFFF` - impassable or locked: water, ridges, the map border (the memset
  default, which is why the outer ring stays this value), and cells locked by
  buildings/walls at runtime (`DynamicalLockTopCell`).

`GetTopology(x, y)` (brigade.cpp) maps pixel coordinates in (`x>>6` to a cell)
and returns the area id, scanning rings up to radius 20 for a valid area if the
cell itself has none.

## Search cell

The coordinate unit of the `InfoMap`/`CantBuild` grids used for building
placement (`CreateInfoMap`, `SearchPlace`/`SearchTowerPlace` in newai.cpp):
arrays indexed `x + y*VAL_SPRNX` with `VAL_SPRNX = 64<<ADDSH`; the playable
extent is `SsMaxX/SsMaxY = msx>>2, msy>>2 = 60<<ADDSH` (`msx = 240<<ADDSH`).
One search cell = 2 TopRef cells = 128 px - that is why `SearchPlace` converts
with `GetTopology(x<<7, y<<7)` (it shifts `>>6` inside) and reads
`TopRef[xx+xx + (yy<<(TopSH+1))]`. Object positions convert as `RealX>>11`
(`RealX>>4` is the pixel coordinate, `>>7` more is the search cell - same scale
`FindCenter` produces). The `SafeMAP` danger bitmap is coarser still: one
bitmap cell = 8 search cells (`EnemyInfo::GetSafeVal` shifts `>>3`).

`InfoMap` bits (newai.cpp): 1 = wood (more than 2 trees), 2 = stone (more than
5 stones), 4 = fully empty, 8 = free of locking, 16 = flat plane zone,
32 = high plateau. `CantBuild` bits (`CB_*` in newai.h): 1 = sklad, 2 = farm,
4 = tower, 8 = building, 16 = mill, 32 = port - cells reserved by existing
construction.

## Group

`OneObject* Group[ULIMIT]` (mapdiscr.h), `ULIMIT = 65535`: the global array of
**all game objects**, used sparsely. On creation a free slot is taken with
`Group[i] = OBJECTS + i` (the entry points into the static pool `OBJECTS`, and
`MAXOBJECT` is raised to `i+1` - it is the high-water mark that object scans
loop up to), and on removal the slot is set to `nullptr`. So the usual scan is
`for (i = 0; i < MAXOBJECT; i++) { OneObject* OB = Group[i]; if (OB && ...) }`.

So no - it is **not a list of all units**. Every `OneObject` lives in there:
units, buildings, mines, walls - what distinguishes them is the type chain
`OB->Ref.General` -> `GeneralObject` -> `newMons` (`NewMonster` fields such as
`Usage`, `Peasant`, `Building`). Pure map scenery (trees, stones, resource
sprites) is *not* in `Group`; it lives in `Sprites[]` as `OneSprite`. A slot
with `OB->Sdoxlo != 0` is a dying/dead object (`Sdoxlo` is the death timer), so
"alive objects" scans test `OB && !OB->Sdoxlo` - exactly the loop
`City::FindApproximateBuildingPlace` runs to average the enemy position
(`NNUM == 0`) in its `TowerID` case, see the enemy-facing half-plane subsection
of [020_topology.md](020_topology.md).

## OneObject

The runtime object - one per unit/building/mine, stored in `Group[]` and in the
`OBJECTS` pool. Carries the position `RealX/RealY` plus integer cell
coordinates `x/y`, the nation `NNUM` and alliance mask `NMask`, the type via
`Ref.General`/`newMons`, `Index`/`Serial` (identity), and the death timer
`Sdoxlo`.

## RealX / RealY

Object position in 1/16-pixel units: `RealX>>4` is the pixel coordinate (the
unit `GetTopology` and the `Norma` distance calls use), `RealX>>11` is the
search cell. Example: a tower's search-cell position is stored as
`OB->RealX >> (4+7)` in `PORTSX[]`.
