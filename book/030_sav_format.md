# The `.sav` file: how a Cossacks game is frozen and restored

This is a walk-through of the save-game format: what bytes are inside a
`.sav` file, in which order they are written, and how they are read back.
It assumes only general programming knowledge: little-endian integers,
raw struct dumps, and the idea that a game state is a graph of structs
full of pointers. All code references are to this repository; every byte
example is real, decoded from `gamedata/taras.sav` (6,991,590 bytes,
save name "taras", format version 285). The writer is `PreSaveGame` and
the reader is `SFLB_PreLoadGame`, both in `src/game/loadsave.cpp`; the
file itself is produced by `SaveGame` and consumed
by `SFLB_LoadGame`.

## 0. The 60-second version

A save is **one flat, uncompressed stream of bytes**, assembled in RAM
and written with a single `RBlockWrite`. There is no central directory
and no per-section headers beyond a few 4-byte magic tags. The stream
is a fixed sequence of sections, written and read **in exactly the same
order**:

```
SaveGame(name, desc, ID)
  └─ PreSaveGame(SB, desc, ID)
       header: "KASF", version, ID, players, save name, camera, player names
       ├─ SaveMap          msx,msy, map name, light; RLE-compressed overlay maps
       ├─ SaveNations      'NATI'  per-nation stats, units, upgrades, resources
       ├─ SaveObjects      'SJBO'  every OneObject: raw struct slice + orders
       ├─ SaveSelection    'SELI'  80 selection groups
       ├─ SaveWalls        'LLAW'  wall clusters, gates, per-player resources
       ├─ SaveSprites      'IRPS'  map sprites (trees/stones), timers
       ├─ SaveAnmObj       'OMNA'  explosions / flying projectiles
       ├─ Save3DBars       'RAB3'  3D bars (health bars etc.), 3D object map
       ├─ SaveCost                   building-cost placements
       ├─ SaveAI                     AI registers, brigades, armies, ideas
       ├─ LS_SaveTopology            pathfinding areas + link matrices
       ├─ SaveActiveObjects          scenario active zones/groups
       ├─ SaveMission                campaign/scenario state
       └─ tail: economy struct, 0xFFFF marker, clocks, CURIGAME struct, ...
  └─ SB.SaveToFile(f)  -> one RBlockWrite  (`SaveBuf::SaveToFile`)

SFLB_LoadGame(fnm, LoadNation)
  └─ SB.LoadFromFile(f)  -> whole file into RAM, parse cursor
  └─ SFLB_PreLoadGame(SB, LoadNation)        — same order
```

Two ideas carry the whole design:

1. **Raw struct dumps.** Most sections are `xBlockWrite(SB, &someStruct,
   sizeof someStruct)` — the in-memory bytes are copied verbatim. The
   format is therefore locked to the exact struct layout *and* to
   little-endian x86; that is what `sfVersion = 285` is for in this
   build: the load menu only lists saves whose version field matches the
   running build (see `InstallName`).
2. **Pointers are never stored.** Every pointer is replaced by an index
   or an offset before writing (object index into `Group[]`, animation
   offset relative to `newMons`, order function as a small table index,
   weapon as `MyIndex`, sprite type as a byte) and rebuilt after
   loading. This is why a save is portable within one build and
   meaningless outside it.

## 1. The container: `SaveBuf`, `xBlockWrite`, `xBlockRead`

`SaveBuf` is just a growing byte buffer:

```cpp
class SaveBuf {
    byte* Buf; int Pos; int Size; int RealSize;
    void SaveToFile( ResFile f1 );   // one RBlockWrite of Buf[0..Size)
    void LoadFromFile( ResFile f1 ); // whole file into Buf, Pos = 0
};

void xBlockWrite( SaveBuf* SB, void* Data, int Size )  // memcpy at Size, grow by 64 KB chunks
void xBlockRead(  SaveBuf* SB, void* Data, int Size )  // memcpy from Pos, Pos += Size
```

So both saving and loading happen entirely in RAM: the whole 7 MB of
`taras.sav` is one memory blob. There is no compression at the
container level — the only compressed parts are the map overlay arrays
(section 3, `SaveRLE1`). All integers are little-endian; all structs
are written with their natural alignment padding.

Two debug helpers write human-readable 16-byte markers into the
stream — `SAVMES(SB, " NATSAVE")` writes the string zero-padded to 16
bytes, `SAVMES1(SB, " NATION: ", tt)` writes `"NATION:  <n>"`. The
loader's `LOADMES` just skips 16 bytes. They are why strings like
`" OBJECTSINFO"` are visible in a hex dump (and in section 6).

## 2. The file header (written first by `PreSaveGame`)

```cpp
xBlockWrite( SB, &sfHeader, 4 );    // static int sfHeader = 'FSAK';
xBlockWrite( SB, &sfVersion, 4 );   // sfVersion = 285 in this build
xBlockWrite( SB, &ID, 4 );          // save serial
xBlockWrite( SB, &NNN, 2 );         // number of players (non-empty PINFO names)
xBlockWrite( SB, &LOADNATMASK, 4 ); // bitmask of nations present (multiplayer resume)
xBlockWrite( SB, &sl, 1 );          // strlen(Messtr)+1
xBlockWrite( SB, Messtr, sl );      // save description, NUL-terminated
xBlockWrite( SB, &mapx, 4 );        // camera top-left, in map cells
xBlockWrite( SB, &mapy, 4 );
for each player:  name[32], ColorID[1]
xBlockWrite( SB, NatRefTBL, 8 );    // nation -> player reference table
```

The 4-byte tags are C multi-character constants, so on disk they appear
**byte-reversed**: `'FSAK'` = 0x4B534146 is stored little-endian as the
bytes `4B 41 53 46` = the ASCII string `"KASF"`. The loader reads them
back as ints and compares against the same constants, so in code you
see `if (i != sfHeader)` while in a hex editor you see `KASF`.

Decoded header of `gamedata/taras.sav` (first 0x8C = 140 bytes):

| offset | bytes | value | meaning |
|---|---|---|---|
| 0x00 | `4B 41 53 46` | "KASF" | magic (`'FSAK'`) |
| 0x04 | `1D 01 00 00` | 285 | `sfVersion` — must equal the running build |
| 0x08 | `05 72 0B 00` | 0x0B7205 | save ID (serial) |
| 0x0C | `03 00` | 3 | players in this game |
| 0x0E | `04 00 05 00` | 0x00050004 | `LOADNATMASK` |
| 0x12 | `06` | 6 | length of description incl. NUL |
| 0x13 | `"taras\0"` | "taras" | the save's display name (`Messtr`) |
| 0x19 | `BE 00 00 00` | 190 | `mapx` (camera X) |
| 0x1D | `01 00 00 00` | 1 | `mapy` (camera Y) |
| 0x21 | `"Taras"` + zeros, `00` | name[32], color 0 | player 1 |
| 0x42 | `"Very hard"` + zeros, `01` | name[32], color 1 | player 2 |
| 0x63 | `"Hard"` + zeros, `02` | name[32], color 2 | player 3 |
| 0x84 | `00 01 02 03 04 05 06 07` | identity | `NatRefTBL[8]` |

Note the player names are fixed 32-byte fields, not length-prefixed —
only the save description has a length byte. The load/save menu does
not even read the name back for the list (the `RBlockRead` calls are
commented out in `SFLB_CreateGamesList`); it only
checks magic + version to decide whether to show the file at all, and
adds `*.sav` files whose 4-byte version word equals `sfVersion`. Legacy
saves with signature `'[CER'` / `']CER'` are treated as current.
`CreateMaskForSaveFile` similarly reads just offset
14 to learn `LOADNATMASK` for multiplayer resume.

## 3. Section: the map — `SaveMap` / `LoadMap`

```cpp
void SaveMap( SaveBuf* SB ) {
    xBlockWrite( SB, &msx, 4 );                     // map size in cells: 960, 960 in taras.sav
    xBlockWrite( SB, &msy, 4 );
    xBlockWrite( SB, CurrentMap, 64 );              // "RN1 6F99 20210 000003JF 0000000.m3d"
    xBlockWrite( SB, &LightDX, 4 );                 // light direction: 0, 142, 213
    xBlockWrite( SB, &LightDY, 4 );
    xBlockWrite( SB, &LightDZ, 4 );
    SaveRLE1( SB, UnitsField.MapV, MAPSY*BMSX );    // object-occupancy field
    SaveRLE1( SB, MFIELDS[0].MapV, MAPSY*BMSX );    // motion field 0 (passability)
    SaveRLE1( SB, MFIELDS[1].MapV, MAPSY*BMSX );    // motion field 1
    if (FishMap) { xBlockWrite(&FishLx,4); SaveRLE1(SB, FishMap, FishLx*FishLx); }
    else { int i = 0; xBlockWrite( SB, &i, 4 ); }
}
```

In `taras.sav` this section starts at 0x8C: `msx = msy = 960`,
`CurrentMap = "RN1 6F99 20210 000003JF 0000000.m3d"`, light
`(0, 142, 213)`, then the first RLE block whose raw-size field is
`00 00 08 00` = **524288** = `MAPSY*BMSX` = 2048 x 256, i.e. this save
was made on a "large" map (`ADDSH = 2`; in `SetupArrays`:
`MAPSY = 512 << ADDSH`, `BMSX = 64 << ADDSH`).

**The terrain itself is not in the .sav.** `LoadMap` calls
`Load3DMapLandOnly(CurrentMap)` — heights and textures are reloaded
from the named `.m3d` map file. The save only carries the *gameplay
overlays* on top of the terrain: which cells are occupied
(`UnitsField.MapV`) and the two motion/passability fields
(`MFIELDS[0..1].MapV`), plus optionally the fish-amount map.

**`SaveRLE1` / `LoadRLE1`** is a tiny
byte-oriented RLE. Layout of one block:

```
int32  RawSize                  // size of the UNCOMPRESSED data
then chunks until RawSize bytes are produced:
  byte n, n < 128  -> n literal bytes follow, copy them
  byte n, n >= 128 -> run: the next 1 byte is repeated (n - 128) times
```

The writer caps both literals and runs at 126 bytes per chunk, so runs
are 3..126 bytes and literal chunks 1..126 bytes. In `taras.sav` the
first overlay block starts with `FE 00 FE 00 ...`: `FE` = run of 126
copies of `00` — the map border is empty, exactly what you'd expect.

## 4. Section: nations — `SaveNations` / `LoadNations` ('NATI')

Tag: `int tt = 0x4954414E` (`'NATI'`, on disk "NATI"), then the
16-byte marker `" NATSAVE"`, then a byte `8`, then **8 fixed nation
slots** (all 8 are always written, even dead ones). In `taras.sav` the
tag sits at offset 96188. Per nation (`Nation* NT = &NATIONS[tt]`):

```
SAVMES1 " NATION: <tt>"
byte   NMask                        // which players control this nation
byte   NNUM | (VictState << 4)
SAVMES1 " MONSAMOUNT : <NMon>"
int32  NMon                         // unit types in the nation
word   Harch
int    BalloonStage .. UID_PEASANT  // raw slice: (char*)&NT->UID_PEASANT - (char*)&NT->BalloonStage bytes
word   NMON                         // how many unit types have Changed != 0
per changed type: SAVMES1 " NMONSTER:" + word index + byte flags(Enabled|CondEnabled|ManualDisable)
                  + raw AdvCharacter (sizeof(AdvCharacter) bytes: costs, HP, attack...)
SAVMES "CITYINFO"
word   UnitAmount[NMon], ReadyAmount[NMon]   // per unit type
int32  NGroups; word NGroupsInSet[NGroups]; per group: word members[NGroupsInSet[i]]
SAVMES "UPGRADEINFO"
int32  -NUpgrades                   // note: negative!
per upgrade: byte flags(Done|Enabled|PermanentEnabled|IsDoing|ManualDisable) + word CurStage
int32  NtNUnits[tt], NtMaxUnits[tt]
word   NatList[2*NtNUnits[tt]]      // indices of alive unit types
word   NKilled[NMon]                // per-type kill counts
int32  ResTotal[8], ResOnMines[8], ResOnBuildings[8], ResOnUpgrade[8],
       ResOnUnits[8], ResOnLife[8]  // each written as 32 raw bytes = 8 resources x int32
int32  NHistory; per entry: int32 len + chars (NUL included)
int32  NPopul; word Popul[2*NPopul]
int32  NUpgMade; word UpgIDS[..]; int32 UpgTime[..]
int32  ResBuy[8], ResSell[8]        // market prices, 32 bytes each
word   NProduced[NMon]
int32  NAccount; word Account[2*NAccount]
byte   ThereWasUnit
```

On load, `LoadNations` first calls `LoadAllNations(0..7)` — which
re-reads every nation's `.md`/script files from the archive — and then
overwrites the dynamic parts above. That's the general pattern of the
whole loader: **static definitions are reloaded from `all.gsc`, only
runtime state comes from the .sav**.

## 5. Section: objects — `SaveObjects` / `LoadObjects` ('SJBO')

Tag `'SJBO'` (on disk "OBJS" — offset 135327 in taras.sav), marker
`" OBJECTSINFO"`, then `int32 NObjects` (count of non-null `Group[i]`)
and `int32 MAXOBJECT` (capacity of the `Group[]` pointer array).
Then for every live object:

```cpp
SAVMES1( SB, " OBJECT:", OB->Index );
xBlockWrite( SB, &OB->Index, 2 );                                  // word Index
xBlockWrite( SB, &OB->NeedPath, int(&OB->Inside) - int(&OB->NeedPath) ); // raw POD slice
if (OB->NInside) xBlockWrite( SB, OB->Inside, OB->NInside << 1 );  // garrisoned unit indices
xBlockWrite( SB, &pp, 1 );   // selection-group index (255 = none)
SAVMES( SB, " ORDERINFO" );
xBlockWrite( SB, &pp, 2 );   // number of queued orders
per order: Order1 ORD1 = *ORDR; ORD1.DoLink = (ReportFn*)GetOrderKind(ORDR->DoLink);
           xBlockWrite( SB, &ORD1, sizeof ORD1 );                  // function pointer -> small int
xBlockWrite( SB, &pp, 4 );   // HiLayer as offset from newMons (-1 = null)
xBlockWrite( SB, &pp, 4 );   // NewAnm  as offset from newMons
xBlockWrite( SB, &pp, 4 );   // LoLayer as offset from newMons
```

The raw slice from `NeedPath` to `Inside` is the whole POD prefix of
`OneObject` (in `mapdiscr.h`): `Index`, `Serial`, `NIndex` (unit type
within the nation), `NNUM` (nation), position `RealX/RealY`, facing
`RealDir`, animation frame `NewCurSprite`, HP, resources carried, etc.
Everything after `Inside` is pointers and gets rebuilt on load:

```cpp
OneObject* OB = OBJECTS + IND;  Group[IND] = OB;
memset( OB, 0, sizeof(OneObject) );
OB->Index = IND;
xBlockRead( SB, &OB->NeedPath, ... );          // the raw slice back
OB->Nat    = &NATIONS[OB->NNUM];                // pointers re-attached
OB->Ref.General = OB->Nat->Mon[OB->NIndex];
OB->newMons = OB->Ref.General->newMons;
if (OB->NInside) { OB->Inside = new word[...]; xBlockRead(...); }
// orders: DoLink = GetOrderRef(int(OR1->DoLink)) — index back to function pointer
// animations: OB->NewAnm = (NewAnimation*)((byte*)OB->newMons + pp)
```

So a unit's identity in the file is `(word Index, raw bytes, order
list, 3 animation offsets)`; its *type* is re-resolved through
`NATIONS[NNUM].Mon[NIndex]`, which is why the nation section must be
loaded first.

## 6. The remaining sections, in file order

All tags below are the on-disk (byte-reversed) spellings; offsets are
where they appear in `taras.sav`.

| # | writer | tag (offset in taras.sav) | contents |
|---|---|---|---|
| 1 | `SaveMap` | none (starts at 0x8C) | msx/msy, map name, light, 3-4 RLE map blocks |
| 2 | `SaveNations` | "NATI" (96188) | 8 nations: stats, changed unit types, city, upgrades, resources, history |
| 3 | `SaveObjects` | "OBJS" (135327) | all `OneObject`s: raw slice + inside + orders + animation offsets |
| 4 | `SaveSelection` | "ILES" (565931) | 80 `SelGroup`s: `NMemb` + rest-of-struct, then `Member[]` and `SerialN[]` word arrays |
| 5 | `SaveWalls` | "WALL" (566831) | wall clusters (`Type`, `NIndex`, `NI`, monster `MyIndex`, `NCells`, raw `WallCell[]`), gates (`NGates`, `MaxGates`, raw `Gate[]`), marker `" RESOURCES"`, `XRESRC[8][8]` and `RESADD[8][8]` raw — per-player stockpiles and income additions |
| 6 | `SaveSprites` | "SPRI" (567391) | `MAXSPR`, count, then per enabled map sprite (trees/stones/holes): index, raw prefix up to `SG`, one type byte (0=TREES,1=STONES,2=HOLES,3=COMPLEX), raw `Index..Damage`; then `ObjTimer` (NMembers + IDS ints + Kinds bytes), then the sprite-reference map: count, and per used cell `NSpri` + `SpRefs` ints |
| 7 | `SaveAnmObj` | "ANMO" (2286743) | explosions/flying projectiles: count, then per entry slot index + raw `AnmObject` minus last 4 bytes, with `Sender` stored as the sender's object index (0xFFFFFFFF = none) and `Weap` as `Weapon::MyIndex` |
| 8 | `Save3DBars` | "3BAR" (2286751) | `NBars`, count, per bar slot index + raw `OneBar`; then 3D-object map: count, per cell `NObj3` + word array |
| 9 | `SaveCost` | none | `NCost` + raw `CostPlace[]` (build-cost markers) |
| 10 | `SaveAI` | none (checksum 654321 first) | `AI_Registers[8][32]`; per nation: `AI_Enabled`, `PINFO` fields (ColorID, GameTime, VictCond, NationID, name[32]), `Account`, `CurAI`, raw `City` if AI, then brigades (each with `CFN` as an index into the `LS_CFN[6]` table and `BOrder` links as indices into `BR_ORD[14]`, checksums 1234500/1234501 around them), armies (link indices into `AR_ORD[6]`), ideas (`IBR[3]`), etc. |
| 11 | `LS_SaveTopology` | none | `NAreas`, per area raw `Area` + mines + links, `MotionLinks` and `LinksDist` (NAreas^2 words each), `TopRef` (TopLx*TopLy words), then the dynamic-topology update state (flags, changed-area list, bounds, temp matrices) |
| 12 | `SaveActiveObjects` | none | `NAZones`, `NAGroups`, per zone/group the raw struct + length-prefixed name; groups also carry `Units[]` and `Serials[]` |
| 13 | `SaveMission` | none | `CurrentMission`, scenario DLL handle, `PlName[64]`, player data, campaign/mission indices, `SCENINF` time/trigger/luminous-spot arrays, `TextDisable[26]`, saved scenario zones, unit groups, `Victory`/`LooseGame` bytes |
| 14 | tail (in `PreSaveGame`) | `FF FF` marker | raw `ECO` economy struct (`GetEconomyData` = `sizeof(ECO)`, in economy.cpp), `word 0xFFFF`, `rpos`(2), `REALTIME`(4), `tmtmt`(4), byte `1`, `CITY[i].Difficulty[8]`, `PeaceTimeLeft/MaxPeaceTime/PeaceTimeStage`(4 each), `CURTMTMT`(4), `GameTime`(4), `sz`(4) + raw `CURIGAME` struct (`GetCurGamePtr` = `sizeof(CURIGAME)`, in ir_main.cpp), 7 state ints (`BalloonState`, `CannonState`, `NoArtilleryState`, `XVIIIState`, `DipCentreState`, `ShipyardState`, `MarketState`), `MyNation` **written as 8 bytes**, `NatRefTBL`(8), byte `2`, `COMPSTART`(16), `SaveState`(1), `GMID[64]` (always empty) |

The tail of `taras.sav` matches this exactly: `... FF FF` (marker),
`00 00` (rpos), `FF FF FF 00` (REALTIME), ... then near the end
`00 FF FF 03 00 00 00 06` — the 8-byte `MyNation` write (value 0 plus
7 bytes of stack garbage), then `00 01 02 03 04 05 06 07`
(`NatRefTBL`), `02`, the 16-byte `COMPSTART` starting `00 00 71 94
72 13 ...`, `SaveState = 00`, and 64 zero bytes of `GMID`.

The load side mirrors this order exactly in `SFLB_PreLoadGame`:
`LoadMap, LoadNations, LoadObjects,
LoadSelection, LoadWalls, LoadSprites, LoadAnmObj, Load3DBars,
LoadCost, LoadAI, LS_LoadTopology, LoadActiveObjects, LoadMission`,
then the tail. Two details worth knowing:

* After loading, the engine **recomputes derived state** rather than
  reading it: `ResearchIslands()`, `ResearchCurrentIsland(i)`,
  `SetMonstersInCells()`, `GNFO.Setup()`, `CreateMiniMap()`.
* The `LoadNation` flag (single-player load passes 1 from the menu code;
  multiplayer passes 0 in `LoadSaveFile`) only
  controls whether the final `MyNation`/`NatRefTBL` block is applied —
  in multiplayer every client keeps its own nation.

## 7. How a save is triggered

* Menu save: `SaveGame(fnm, Messtr, ID)` from the save menu
  (`ProcessGSaveGame`) ->
  `PreSaveGame` + `RRewrite` (file created/truncated); the `.sav`
  extension is appended if missing (inside `SaveGame`).
* Autosave: `SaveGame("AUTO.sav", "auto.sav", 0)` from the main loop.
* Multiplayer sync saves go through `SaveGame(str, Name, ID)`
  (`SaveNetworkGame`); `SFLB_LoadGame(SaveFileName, 0)` resumes them
  (`LoadSaveFile`).

## 8. Appendix: checklist for a naive .sav parser

Things a straightforward reader gets wrong (all verified against
`taras.sav` and the code):

1. **Multi-char constants are byte-reversed on disk**: `'FSAK'` -> "KASF",
   `'SJBO'` -> "OBJS", `'SELI'` -> "ILES", `'LLAW'` -> "WALL",
   `'OMNA'` -> "ANMO", `'IRPS'` -> "SPRI", `'RAB3'` -> "3BAR".
2. **Version gate**: bytes 4..7 must equal `sfVersion` (285 in this
   build) or the game refuses the file; magic alone is not enough.
3. **Raw struct dumps**: most sections are `memcpy` of in-memory structs,
   including padding. Parsing them requires the exact struct definitions
   of build 285 (`OneObject`, `Nation`, `City`, `Area`, `ECO`,
   `CurrentGame`, ...). The file is little-endian only.
4. **No pointers in the stream** — object references are `Group[]`
   indices, animations are offsets from `newMons`, order/AI/brigade
   callbacks are indices into lookup tables (`GetOrderKind`/`GetOrderRef`,
   `LS_CFN`, `BR_ORD`, `AR_ORD`, `IBR`), weapons are `MyIndex`.
5. **Only the map overlays are RLE-compressed** (`SaveRLE1`): 4-byte
   raw size, then chunks — `n < 128` = n literals, `n >= 128` = run of
   `n-128` copies of the next byte. Everything else is verbatim.
6. **The 16-byte `SAVMES` markers** (" OBJECTSINFO", " NATION: 3", ...)
   are pure debug text; a parser must still consume all 16 bytes.
7. **The terrain is not in the file**: only its name (`CurrentMap`,
   64 bytes in the map section); heights/textures come from the `.m3d`.
8. **Fixed-size blocks regardless of content**: 8 nations always, 80
   selection groups always, `MAPSY*BMSX`-sized map arrays always
   (524288 bytes each in taras.sav -> large map, ADDSH=2).
9. **Quirks**: `SavePackArray` writes a 4-byte count but `LoadPackArray`
   reads 2 (dead code — neither is ever called); `xBlockWrite(SB, &NN,
   8)` writes 8 bytes from a 1-byte variable (visible as garbage after
   `MyNation` in the tail); the nations section writes `NUpgrades` as a
   *negative* int.
10. **The whole file is memory-resident** during both save and load —
    `LoadFromFile` reads all bytes at once and `xBlockRead` is a cursor
    over that buffer. There is no streaming.

A minimal header parser (Python) that recovers the table in section 2:

```python
import struct
d = open('gamedata/taras.sav','rb').read()
magic, ver, sid = struct.unpack_from('<4sIi', d, 0)
nplayers, = struct.unpack_from('<H', d, 12)
loadnat,  = struct.unpack_from('<I', d, 14)
sl = d[18]; name = d[19:19+sl].rstrip(b'\0')
off = 19 + sl
mapx, mapy = struct.unpack_from('<ii', d, off); off += 8
players = []
for i in range(nplayers):
    players.append((d[off:off+32].split(b'\0')[0], d[off+32])); off += 33
natref = list(d[off:off+8]); off += 8
msx, msy = struct.unpack_from('<ii', d, off); off += 8
curmap = d[off:off+64].split(b'\0')[0]; off += 64
# -> magic=b'KASF', ver=285, name=b'taras', players=[(b'Taras',0),(b'Very hard',1),(b'Hard',2)],
#    msx=msy=960, curmap=b'RN1 6F99 20210 000003JF 0000000.m3d'
```
