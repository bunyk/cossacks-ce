# Cossacks CE Linux Port - Working Notes

State and findings as of 2026-10-09. Companion to `docs/Ubuntu.md` (build instructions).

## Current state

- **Build succeeds**: `cmake --build build` produces `build/Debug/cossacks` (64-bit ELF,
  links clean) and the `gscbrowser` resource-browser tool.
- All work is **uncommitted** on branch `linux`. Consider committing before continuing.
- Runtime progress: main menu works; single-player options screen works (nation
  dropdown populated, random-map settings visible, shared cross-platform event loop);
  **campaign flow works** (campaign list + campaign mission list screens; picking a
  mission and pressing OK stays in the list because starting a match is stubbed -
  ProcessSingleMission in linux_stubs.cpp). The "Campaign"/"Missions" buttons used to
  silently bounce back to the main menu because ProcessCampaigns was entirely
  `_WIN32`-guarded (empty function = instant return) - the same empty-function trap
  as the empty-loop one. Un-guarded for this: ProcessCampaigns, ProcessSingleCampagin
  (interface.cpp ~12172-12418), the whole TextViewer implementation block
  (dialogs.cpp ~2941-3380: AssignScroll/GetNextLine/CreateLinesList/OnDraw/OnKeyDown,
  ColoredBar_OnDraw, real addColoredBar/addViewPort2/addTextViewer), and the
  UNIFONTS/UNI_LINE*/UNI_HINT* font-adjustment globals + LoadFonts
  (gp_draw.cpp ~10593-10710, reads Unicode.dat). Deleted superseded stubs from
  linux_stubs.cpp (addColoredBar, addTextViewer, AssignScroll, UNI_LINED*, LoadFonts).
  Note: SelectSingleMission (the "Missions/Load" button screen) is STILL guarded
  (interface.cpp ~12420+, same bounce bug there - fix the same way when needed).
  **fullscreen mode works** - after fixing three fullscreen-only bugs (see pattern #3:
  LockSurface ScreenPtr truncation, RestoreMData origin double-count, CopyToScreen
  row-advance). The "image half-size, centered" look in fullscreen is intended
  (fixed 1024x768 menu canvas, centered unstretched - see the note in conventions).
- Not yet reachable: in-game UI, pathfinding, fog rendering - all stubbed in
  linux_stubs.cpp. **Starting a match IS now wired up** (2026-10-10): pressing OK in the
  single-player random map dialog runs the real start sequence instead of bouncing
  back to the main menu. See "Random map start" below for what was un-guarded and
  what is still missing in-game.
- **In-game rendering WORKS** (2026-10-11): terrain with correct textures and blended
  texture transitions (masks.cpp port), correct per-pixel fog shading (darkfog index fix),
  VS->screen copy, sprites, minimap, resources panel, and a correct translucent cursor
  shadow (RestoreMData condition fix). All temporary DBG probes in virtscr.cpp/interface.cpp
  have been stripped. Cursor GP files are stacked demo layers per frame (grey shadow +
  opaque body + trans 1/4, 1/2, 3/4 swatches) - that is the data, not a rendering bug.
  Verified against the real renderer offline: gscbrowser's variant strip + TAB animation
  run the exact Linux blit code paths.

## Files enabled / added this round (were behind `#ifdef _WIN32` or missing from CMakeLists)

- Enabled (guards removed): `mapa.cpp`, `newmon.cpp`, `activescenary.cpp`, `selprop.cpp`,
  `gamesound.cpp`, `multi.cpp`, `3drandmap.cpp`, `weapon.cpp`, `strecog.cpp`, `select.cpp`,
  `mouszone.cpp`, `3dbars.cpp`, `nature.cpp`, `loadsave.cpp`
- Added to CMakeLists (were never built): `cdirsnd.cpp`, `cwave.cpp`, `unsyncro.cpp` (RandNew!),
  `recorder.cpp`, `river.cpp`, `savenewmap.cpp`, `gvclass.cpp`, `report.cpp`, `linux_stubs.cpp`,
  `masks.cpp` (2026-10-11, asm ported - see "Terrain texture masks" below)
- Still NOT in build / not ported: `path.cpp` (heavy asm), `realwater.cpp` (15 asm),
  `shiptrace.cpp` (4 asm), `fastdraw.cpp` (12 guards, 33 asm, partially done),
  `fog.cpp` (asm regions), `interface.cpp` (~82 scattered guards,
  partially enabled), `mplayer.cpp` (defines MyDPID - conflicts with interface.cpp), `ir_main.cpp`,
  `bmptool.cpp` (pure Win GDI/DIB - left guarded on purpose).

## src/game/linux_stubs.cpp - intentionally missing functionality

No-op stubs with exact header signatures so it links. Each stub is a TODO:

- **Pathfinding**: `OneObject::CreatePath/CreatePrePath`, `CreateFullPath`, `FindBestPosition`,
  `GetNext*Cell` -> units cannot move until `path.cpp` is ported. The `MotionField`
  bit-grid methods (ClearMaps/BSet*/BClr*/Check*) are NO LONGER STUBS: they are C
  translations of the path.cpp asm, living in linux_stubs.cpp (MapV = bit matrix,
  8 y-cells per byte, row stride 1<<(6+ADDSH); CheckVLine does one little-endian
  32-bit load; BSetBar/BClrBar use Lx for BOTH extents - original quirk, preserved).
  They were the "infinite loop in CreateCostPlaces" bug: `while(!CheckBar(...))R0++`
  never terminated with CheckBar always-false. Delete the translations when
  path.cpp is ported for real.
- **Fog of war**: `DrawFog`, `FogSpot`, `GetFog` etc. (fog.cpp asm regions).
- **VirtualScreen**: `AddTHMap` stub (minimap terrain); everything else in virtscr.cpp is ported.
- **Terrain texture masks (fixed 2026-10-11)**: `masks.cpp` ported - `PrepareIntersection1/2`
  were no-op stubs, so every mixed-texture cell rendered from stale `ResultMask` (wrong
  terrain textures at all texture transitions). Ported behind `#ifdef _WIN32`:
  `CopyMaskedRLE64Lin` (shared translation of `CopyMaskedBitmap64` +
  `CopyMaskedTransparentBitmap_8/_4/_12`; RLE mask format: byte 2 = string count, then
  per string a segment count + (skip,count) pairs; source tiles are 64x64 in 256-byte-row
  pages, coordinates wrap at 64; transparency lookup order: _8/_4 = `trans[(src<<8)|dst]`,
  _12 = `trans[(dst<<8)|src]`), `FastCreateMaskedBitmap64_1/_2` (trapezoid fills of 32
  ResultMask rows, widths 2..32..2; _1 fills left-to-right from column x with wrap into
  the tile's column 0, _2 fills right-to-left from column 32 with overrun into the
  neighbouring tile at columns 64..127), `ShowIntersectionBuffer` (debug blit; the original
  `int(ScreenPtr)` truncation is avoided). `ResultMask` has 256-byte rows despite MaskLx=64.
  Also fixed here: masks.cpp read `SimpleMaskA..D->OfsTable[n]` as pointers - on 64-bit
  those are unrebased file offsets; the `MaskOfs()` helper now applies the same
  `(char*)table + 4 + OfsTable[n]` rebase that `GetRLCWidth/Height` use.
  Stub deletions in linux_stubs.cpp: ResultMask, GetBmOfst, PrepareIntersection1/2.
- **VS->screen copy (fixed 2026-10-11)**: `VirtualScreen::CopyVSPart`/`CopyVSPartMMX` had
  Windows-asm-only bodies - on Linux they compiled as EMPTY no-ops, so the fully rendered
  VS buffer was never copied to the screen (terrain invisible, menu showing through, all
  cursor/sprite traces - nothing ever overwrote stale pixels). Both now have `#else`
  row-wise memcpy branches (VS stride `RealVLx`, screen stride `SCRSizeX`).
- **Terrain fog/darkening pass (fixed 2026-10-11)**: all 5 translated rasterizers in
  3dgraph.cpp built the `darkfog` lookup as `darkfog[16384 + (fog>>8)]` - dropping the
  PIXEL byte the asm folds into the index via `mov al,[edi]` (index =
  16384 + ((fog>>8)&~0xFF) + pixel; darkfog = 160 levels x 256 pixel entries). Result:
  every terrain pixel became one constant palette entry per scanline = the
  "brown-black polygons" bug. Same bug family as pattern #3's index-word truncations -
  ALWAYS check two-value table indices in asm translations.
- **Order dispatch init (fixed 2026-10-11)**: `CDGINIT_INIT1/2` (called from
  SFLB_ReadFirstPlayerName) were inside `#ifdef _WIN32`, so on Linux `SmartLink`
  and `AttackLink` kept their dummy initializer values. First smart-send order
  (ANY move command, incl. building placement) called DoLink = 0xf8aff498
  (= 0x153829 - 0x7654391 sign-extended) -> SIGSEGV; attacks would die the same
  way via AttackLink. Fixes: un-guarded the two init calls (both functions are
  plain C++), and widened `SmartLink` from int to intptr_t (the int() obfuscation
  truncated the function pointer on x64). CHECK FOR MORE: grep for the same
  `int(FuncPtr)` + magic-constant obfuscation pattern elsewhere.
- **Font layer Opt=39 (fixed 2026-10-11)**: font GP files come in two flavours -
  MAINFONT.GP glyphs are Opt=38 (grey, StdUnpack'd), ROM10.GP / ROMBIG.GP glyphs
  are Opt=39 (grey, RAW pixel data, no unpack). The Linux ShowGP dispatcher had
  38 but not 39, so every rom10/romBig glyph (resource numbers, big in-game
  text) was silently skipped - "labels render but no numbers". Added case 39 to
  Linux ShowGP (per-imt encoders, raw data at CDOffs), and the missing 38/39
  cases to Linux ShowGPPal + ShowGPPalLayers (building placement preview /
  selected buildings; gated by layer bit 1024 in the Layers variant).
  ShowGPLayers already had both. Opt=39 appears in ~2600 frames across many
  files (BORDER.GP etc.), so this also fixes assorted interface bits.
  RESOURCE IDS: ddex1.cpp had `FoodID = GetResID("STONE")` with StoneID never
  assigned (mod-lineage bug vs upstream: FoodID=FOOD, StoneID=STONE) - stones
  matched the FoodID branch of mapa.cpp's cursor mapping and showed the
  food/scythe cursor. Restored the upstream pair (mapa checks are sequential,
  FoodID is checked last, so both must be distinct).
- **Pathfinding ported (2026-10-11)**: `path.cpp` is in the build. All 16 asm
  regions guarded with `#ifdef _WIN32` + `#else` translations:
  - MotionField bit-grid (BSetPt/BClrPt/CheckPt/CheckVLine): the ADDSH
    switch-cases all reduce to `MapV[(x << (6+ADDSH)) + (y>>3)]` with bit
    `(y&7)`; CheckVLine = one little-endian 32-bit load masked with
    `((1<<Lx)-1) << (y&7)`. These translations are the ones formerly living in
    linux_stubs.cpp (verified: fixed the CreateCostPlaces infinite loop) -
    moved into path.cpp proper.
  - The 4 identical Bresenham walkers in CreatePrePath/2/4/Bordered: 8-bit
    asm direction cases reduce to sx/sy multipliers; fills pxx/pyy word
    arrays, Pps = point count. The 16-bit cum counter is safe for map-sized
    coordinates.
  - Deleted from linux_stubs.cpp: MotionField translations, MFIELDS[2],
    AllowPathDelay, CreatePath/CreatePrePath/CreateFullPath/FindBestPosition/
    CorrectLockPosition stubs. STILL STUBBED: GetNextDiv/Gre/SafeCell
  (they live in safenet.cpp, not yet ported - AI long-range pathing across
  areas).
- **Cursor save/restore (fixed 2026-10-11)**: `RestoreMData` (mouse_x.cpp) had its
  comparison INVERTED vs the asm: asm restores the saved background only where the screen
  still equals the post-cursor save; the port wrote the STALE background wherever the
  cursor had drawn pixels, clobbering fresh content. Symptom: shadow-shaped smears of old
  texture around the mouse cursor (visible while scrolling; the opaque cursor body hides
  its own smear, blended layers don't). gscbrowser can't reproduce this - the mouse
  machinery never runs there.
- **Unit shadows (fixed 2026-10-11)**: in-game sprites draw through the Z-buffer
  (`ShowZElement` -> `ShowGPLayers`), whose Opt-5 case selects by mask bits:
  bit 32 = shadow layer on, bit 64 = SOFT table shadow (GP_ShowMaskedPictShadow,
  fog+4096), no bit 64 = black Overpoint column dither. The Linux ports of
  ShowGPLayers AND ShowGPTransparentLayers had the two branches SWAPPED, so every
  normal unit (mask=0xFFFF) rendered the Overpoint dither - "vertical black lines
  interleaved with terrain". ShowGP's case 5 was always correct, which is why the
  gscbrowser variant strip (real renderer, but via ShowGP) showed a clean soft shadow.
  gscbrowser's scratch background is now a green checker instead of black - blends
  are invisible against black. Repro tools note: gscbrowser cannot reach the
  Layers-dispatch paths; use TREES.GP frame 0 (shadow ellipse at dy=80) as the
  shadow reference. KNOWN LATENT BUG (not yet fixed): AddToVisual/AddOptPoint
  (newmon.cpp ~4460+, zbuffer.cpp ~230+) store blend-table pointers via `int()`
  truncation (`Param2 = int(yfog)` etc.) - corrupts/crashes pulsing/colored unit
  states; fix alongside the next audit of live int(ptr) sites.
- **Water**: `SetWaterSpot`, `InitBlobs` etc.; `WaterDeep`/`WaterBright` point at a static
  1MB zero buffer instead of real map data.
- **In-game UI**: `LoadMapInterface`, `ProcessMapInterface`, `DrawZones`, chat, mission
  questions (interface.cpp guarded regions).
- **Network**: `mplayer.cpp`/`ir_main.cpp` (GetPing, PSUMM/PBACK/RETRANS ctors, WINIGAME...).
  Note `SetupEBufs` and `PLNAT[8]` are REAL implementations here (not stubs); when
  mplayer.cpp gets ported, delete them along with the rest.
- If you enable the real file later, delete the matching stubs or you get multiple-definition
  link errors (this already happened with `DialogsSystem::addViewPort` in gvclass.cpp).

## Recurring bug patterns (checklist for any newly enabled file)

1. **`sizeof TypeName` without parens** - legal on MSVC, error on GCC. -> `sizeof(TypeName)`.
   Happens ~everywhere; the errors are all "expected primary-expression before ')' token".
2. **4-bytes-per-pointer allocations** - 32-bit assumption. All `**` casts with `* 4`, `<< 2`,
   `4 *` sizes were broken on 64-bit (caused heap corruption -> the ComboBox::AddLine crash
   in dialogs.cpp:4472). Fixed 33 sites; grep recipe to find more:
   `grep -rn "\*\*) *realloc\|\*\*) *malloc" src/game/*.cpp | grep -E "\* 4|<< 2|4 \*"`
   UPDATE (2026-10-10): the recipe MISSES `calloc` - extend it to
   `grep -rnE "\*\*) *(calloc|malloc|realloc)" src/game/*.cpp | grep -vE "sizeof"`.
   Second round of fixes (was the "random map start aborts in _int_malloc" crash -
   SetupArrays' OTRI.TRIANG NULL-write loop overflowed its half-size buffer and zeroed
   the adjacent free-chunk header): loadsave.cpp SpRefs (+its memset), Obj3Map,
   OTRI.TRIANG, SCINF (EnemyInfo::ALLOCATE), WRefs + its ClearMaps memset
   (WRefs is indexed by GetLI up to MaxLI = 4*VAL_MAXCIOFS entries - `*4*4` meant
   4 entries/cell on 32-bit, NOT oversized); stringshash.cpp StrHash::AddString
   (`MaxStr<<2` for a char** array). Verified NOT bugs: 3drandmap AR->Link `<<2`
   (pairs of word), Vertices `*4` (int array), savenewmap WSys.WCL `<<4` (oversized 2x).
3. **`int(ptr)` casts** - pointer truncation. Fixed so far: GetMData path (mouse_x.cpp,
   commit 8bc284f), all translated asm paths, ddini.cpp LockSurface WINDOW path,
   ddini.cpp LockSurface FULLSCREEN path (was `int(offScreenPtr)+MaxSizeX*32` - crashed
   fullscreen because the truncated low-32 went negative and sign-extended to
   0xffffffff........ - the "crashes in fullscreen" bug), and dialogs.cpp
   CopyToScreen's Linux branch (casted the truncated ints to int* AND advanced rows
   with `src += sadd` in int units = 4x overshoot; now byte pointers).
   AUDIT of all remaining `int(ScreenPtr)`/`int(RealScreenPtr)`/`int(offScreenPtr)`
   sites: the rest are inside `#ifdef _WIN32` asm regions (fastdraw RLC functions are
   empty stubs on Linux) or unused dead variables on Linux (SQPicture::Draw computes
   its own byte* in the #else). If rendering artifacts appear in newly enabled code,
   grep for this pattern first.
   RELATED: asm-translation bugs of the same family - RestoreMData's #else
   double-counted the origin (`scrof` already contained x1+y1*SSizeX and the row
   pointer added them again -> writes at 2*x + 2*y*SSizeX, off-screen at y>h/2) and
   its trailing `+= addscr/add32` mutations corrupted clipped-edge rows. Only fires
   in FULLSCREEN because the RealScreenPtr mouse paths early-return in window mode.
   GetMData/CmpMData translations were verified correct.
4. **`bool b++;`** - illegal in C++17 -> `b = true;`.
5. **MSVC token pasting in macros**: `#define PRC(x) ((32768*##x##)/100)` -> GCC rejects,
   fix by removing `##` (see activescenary.cpp).
6. **`DLLEXPORTint`** - botched search/replace; needs space after DLLEXPORT.
7. **`#include "safety.h "`** - trailing space in include name (activescenary.cpp).

## Structural conventions of the port

- **Asm**: original `__asm { ... }` stays, wrapped in `#ifdef _WIN32 ... #else <C++ translation> #endif`.
  Translated so far: all 21 blocks in `3dgraph.cpp` (software 3D rasterizer: PrepareToRender,
  addLine, Render*Triangle*, Show*Triangle, fog loops), `lines.cpp` DrawLine (was a Bresenham TODO),
  `mapa.cpp` Red/White squares, `3drandmap.cpp` grass effects, `megapolis.cpp` HandleGeology,
  `newmon.cpp` ZMem, `multi.cpp` InitSelection, `zbuffer.cpp` SortWords.
- **os.h shims** (src/game/newcode/os.h): `LoadLibrary` returns NULL (DLL scenario/AI loading
  fails gracefully to error paths), `GlobalAlloc/GlobalFree` -> malloc/free, `HRESULT` = int,
  `_strupr`, `min` do NOT go here (breaks <algorithm> - use ternaries locally).
- **pack(1) leaks (FIXED, was the Ai[] corruption crash)**: `gp_draw.h`, `topograf.h`,
  `playerinfo.h`, `bmptool.h`, `arc/gsctypes.h` used to set `#pragma pack(1)` without popping,
  so any header included after them got packed layouts. If two TUs included e.g. `nature.h`
  with different pack state, `sizeof(SingleAI_Desc)` differed (52 vs 56) and array iteration
  desynced -> garbage pointers (crash in MPL_WaitingGame / ComboBox::AddLine).
  All five now use `pack(push, 1)` ... `pack(pop)`: their own file-format structs stay packed,
  the state no longer leaks. `mapdiscr.h` already did this correctly.
  Layout canaries to keep this from regressing: `static_assert(offsetof(SimpleDialog,
  OnUserClick) == 136)` (dialogs.h) and `static_assert(sizeof(SingleAI_Desc) == 56)` (nature.h).
  If either fires in some TU, a header is leaking pack state again.
  Note: `common.h`'s `pack(1)` is inside `#ifdef _WIN32`, so it is not a leaker on Linux.
  SAME FAMILY, ROUND 2 (2026-10-10): the .lnk/.msp file-format structs in 3drandmap.cpp
  were ALSO compiled packed by the original MSVC build, but we had them unpacked:
  - `SampleRoot` must be 14 bytes PACKED (was 16) - *.smp.lnk roots are written at
    stride 14 (verified: Pieces\HIL2_015.smp.lnk sz=102 = 12 + Nsq*2 + Nrt*14). Reading
    at 16 misparsed RootType/Direction -> garbage `SMP[rt][k]` index -> realloc() on a
    stack-string pointer ("Pieces...") -> SIGBUS in LinkSys::LoadOneLink.
  - `TerrainData` must be 66 bytes PACKED (CompSize@+30, Data@+34) - *.msp chunks
    (verified Terrain\srediz4.msp chunk0: packed CompSize=594, 594+34=628=chunk size).
    With natural alignment CompSize read as -16777216 -> the RLE heightmap loop never
    ran -> the terrain buffer stayed UNINITIALIZED malloc garbage (silent: the random
    map generated on top of random-height terrain).
  Both now have pack(push,1)/pack(pop) + static_assert size canaries.
  LESSON: for any struct that is raw-read/raw-written to data files, verify the
  layout against actual archive bytes (FAT entry -> ~m_Offset + dataStart(14+81*N) ->
  isiDecrypt key ~HIBYTE(0x78CD)=0x87) before trusting natural alignment.
- **`div(x,y)` is redefined** to `SecureDivision(x,y)` in mapdiscr.h:14 (securedivision.cpp).
  Returns `div_t` with `.quot` - do not "fix" call sites to use stdlib div semantics.
- **VertBuf layout** (3dgraph.cpp rasterizer, 16 bytes/line):
  `+0 word Dx, +2 word Lx, +4 int BMX, +8 int BMY|BMxy, +12 int fog`.
- **Empty-loop trap in guarded menu loops (was the "random map freezes" bug)**: some event
  loops have their ENTIRE body inside `#ifdef _WIN32`, so on Linux they compile to
  `do { } while (...)` - a busy loop with no SDL event pump and no rendering: the window
  freezes (100% CPU, unresponsive). The MPL_WaitingGame main loop is now ONE cross-platform
  loop: the full body is shared; only the GameSpy/GSC chat block at the top stays
  `#ifdef _WIN32`. Pure helpers were un-guarded so the real implementations run on Linux:
  Decode/GetPlNameByID/ControlLingvo, SendPings area, LimitString, WaitWithMessage,
  ShowCentralMessage, AddPrimitiveChat/AddChatString, CheckForPersonalChat, DarkScreen,
  ShowLoading (interface.cpp); GP_ScrollBar class, addCustomBox, ListBox::ClearItems,
  DialogsSystem::MarkToDraw (dialogs.cpp).
  NOTE: MSVC allows `goto` jumping over variable initialization, GCC does not - when
  un-guarding loops with `goto FinCLC`, hoist declarations like `settings_were_changed`
  to the top of the loop body.
  SAME TRAP, FUNCTION FORM (was the "nation dropdown items not clickable" bug):
  DialogsSystem::ProcessDialogs had its ActiveParent dropdown-routing block
  inside `#ifdef _WIN32` - on Linux, clicks on an open combo's dropped list
  fell through to the dialog UNDER it (opened the next row's combo instead of
  selecting the item). Un-guarded; the block routes clicks inside the dropped
  zone to MouseOverActiveZone (item select) and closes the dropdown on
  outside clicks. ComboBox_OnMouseOver/ComboBox_MouseOverActiveZone were
  already cross-platform.
  The SECOND loop / phase-2 lobby of MPL_WaitingGame (interface.cpp ~5300-6062) is still
  Windows-only. The `MPL_NatRefTBL` color/nation resolution block after the loop is now
  UN-guarded (it fills NatRefTBL from PINFO/COMPINFO for SINGLE).
  Network symbols referenced by the shared loop are stubbed in linux_stubs.cpp
  (SendChat, GetMaxRealPing, SendToAllPlayers, AnalyseMessages, SendPings, SETPLAYERDATA,
  SETPLAYERNAME, ProcessInternetConnection, EncodeOptionsInNumber, DeepDeletePeer,
  CHATSTRING/CHATDPID/CurrentMaxPing/MinPing0/GMGUID/DoNewInet/PPTIME/ServerDPID/
  CurrentStartTime/NextStartTime, plus declarations in the `#ifndef _WIN32` block
  right before MPL_WaitingGame). When mplayer.cpp/ir_main.cpp get ported, delete those
  stubs. Also remember `CreateMapsList`/`gFindFirst` returns NULL on Linux (the body is
  still `_WIN32`-guarded in arc/gscset.cpp), so the designed-maps list is empty.
- **Random map start works (was the "OK bounces back to main menu" bug)**: on Windows the
  chain is SingleOptions -> StartIGame(SINGLE) -> ComeInGame -> PrepareGameMedia, and
  PrepareGameMedia sets `ItemChoose = mcmSingle`, which is what AllGame sees to call
  PlayGame(). On Linux `StartIGame`/`ComeInGame` live in the not-yet-ported mplayer.cpp,
  so SingleOptions' `#else` branch now runs the same steps inline (minus DirectPlay
  sends): NatRefTBL <- MPL_NatRefTBL, NeedCurrentTime, PrepareToGame, HideFlags,
  GameInProgress, ShowLoading, CreateNationalMaskForRandomMap/CreateMaskForSaveFile/
  CreateNationalMaskForMap, PrepareGameMedia(0,1), CenterScreen, SetupEBufs, time
  resets. Un-guarded from the Windows-only game-start block of interface.cpp
  (~9563-12174): PrepareGameMedia, CenterScreen, GetOptionsFromMap, setCoor +
  coorX/coorY, the `bool MultiplayerStart` definition, and PlayGame's LoadFog(0) +
  InitGame(). Done by MOVING THE GUARD BOUNDARIES (#endif/#ifdef splits around
  the functions in place), not by moving code - keep doing it that way
  (see AGENTS.md). linux_stubs.cpp: PrepareGameMedia stub DELETED (real one
  compiles now), added `int PLNAT[8]` and a real (non-stub) `SetupEBufs`. To make interface.cpp see
  the types, the Linux `#else` of its include block now includes megapolis.h (City),
  mapsprites.h (HideFlags), safety.h + strategyresearch.h + einfoclass.h (GNFO).
  KNOWN RUNTIME GAPS once the match starts (all pre-existing, now reachable):
  PreDrawGameProcess (main.cpp ~1185+) is entirely `_WIN32`-guarded -> no simulation
  ticks on Linux (no ProcessNature/ProcessCostPoints/ProcessGuard/GameKeyCheck...);
  PostDrawGameProcess's ExecuteBuffer/HandleMultiplayer block is guarded -> buffered
  commands never execute; DecodeOptionsFromNumber is still a stub -> GetOptionsFromMap
  leaves the random-map option states (balloons, XVIII etc.) at 0; ProcessMapInterface/
  LoadMapInterface still stubbed -> no in-game UI; ESC in-game exit sets GameExit only
  in a guarded block (ProcessGMainMenu case 1). Chase these next.
  ROUND 3 (2026-10-10): the "Creating map: Unknown unit" dialog was because
  **Loading() (ddex1.cpp:292) never ran on Linux** - its call in main.cpp doInit()
  was `_WIN32`-guarded, so LoadNations/NNations (Nations.lst), LoadWeapon,
  LoadAllNewMonsters (global monster table), LoadEconomy, LoadAllWalls etc. never
  happened; the nation dropdown still worked because GlobalAI (Ai\Ai.dat) loads via
  its global constructor. Un-guarded the Loading() call (all its callees are
  cross-platform and in the build; ReadClanData/Init_GP_IMG/LoadPalettes stay
  stubbed) and REMOVED the Linux GP-preload emulation block in SDL_AppInit
  (InitNewMonstersSystem inside Loading() claims GP slots 0-7 in the Windows
  order - verify cursor icons still look right). Also fixed pattern-#2 calloc
  sites in loadsave.cpp SetupArrays (SpRefs/Obj3Map/OTRI.TRIANG/SCINF/WRefs+memset)
  and stringshash.cpp StrHash::AddString - OTRI.TRIANG's NULL-init loop overflowed
  its half-size buffer and zeroed the adjacent free-chunk header (was the
  "_int_malloc assertion" crash in SetupNatList). Known follow-up: in
  CreateMapByName's error branch `NatUnit[k]=0` indexed with k==nu (unit count,
  can exceed 8) - FIXED to NatUnit[i].
  ROUND 4 (2026-10-10, end state): **the full random-map start now works end-to-end** -
  Start generates the map, Save3DMap writes the RN*.m3d to the game dir, CheckMapNameForStart
  finds the saved file and loads it, and PlayGame() runs the game loop (it waits in
  PostDrawGameProcess's frame pacing - that is the loop looking "hung", the window still
  responds). Last blockers fixed this round:
  - **gWriteOpen/gWriteFile had NO Linux branch** (arc/gscset.cpp): gWriteOpen returned an
    UNINITIALIZED LPGSCfile (stack garbage - the "free(): invalid pointer" on a code
    address in CloseMap), and gWriteFile was a silent no-op (the map was never written to
    disk at all!). Implemented stdio branches (fopen "wb" via toLinuxPath, fwrite on m_fp)
    matching gOpenFile's loose-file fallback style.
  - MotionField bit-grid translated from path.cpp asm (see stubs section).
  - SampleRoot (14 bytes) and TerrainData (66 bytes) packing fixed (see pack section).
  ROUND 5 (2026-10-10): **game rendering un-guarded** - the game screen now renders.
  Un-guarded (verified cross-platform, all build): PreDrawGameProcess FULL body
  (main.cpp; only AddRandomBlobs stays guarded - shiptrace.cpp not ported),
  PostDrawGameProcess incl. ExecuteBuffer (only HandleMultiplayer call stays guarded -
  mplayer.cpp), DrawAllScreen/FastScreenProcess bodies, GSSetup800 (the in-game UI setup:
  GSYS/GVPort/panels/zones), DrawZones+CopyZonesToScreen (real; stub deleted),
  ProcessGMainMenu/ProcessE_GMainMenu (ESC menu), ShowOptMessage, the GSYS/GVPort
  globals block, ContinueGame/nsmaplx/nsmaply, LoPanel. ESC-exit now works on Linux
  (GameExit=true un-guarded in PlayGame case 1; multiplayer cleanup stays guarded).
  linux_stubs: GSYSDRAW/DrawZones stubs deleted + BrigNx/Ny/BrigPnX/PnY/PanelY/Zone1*
  globals removed (now defined by the un-guarded interface.cpp code).
  STILL WINDOWS-ONLY (re-guarded after probing): in-game Load/Save-game + SaveMap/LoadMap
  + Options dialogs (interface.cpp ~6300-8020; use WIN32_FIND_DATA + CD-audio API; menu
  buttons that call them are individually #ifdef'd) and the map-editor ProcessGMainMenu
  variants. main.cpp autosave block still guarded (ProcessUpdate is ir_main.cpp).
  STUBS REMAINING IN-GAME: pathfinding (units cannot move), fog draw, VirtualScreen
  (virtscr.cpp - GFieldShow works without it), water, chat. Watch for first-frame crashes:
  this is the first time the render+simulation path runs on Linux.
  ROUND 6 (2026-10-10): the game RUNS - full draw chain executes, in-game screen renders.
  Fixes this round: GMiniShow's minimap blit used `int scr = (int)ScreenPtr + ...` /
  `int til = (int)&minimap + ...` (pattern #3, dead until rendering was un-guarded) ->
  byte* arithmetic (both the row-copy loop and the 2x2 unit-tag memset).
  **ShowGPLayers now IMPLEMENTED on Linux** (was a printf stub): same structure as the
  Linux ShowGPPalLayers (temp-buffer decompression), with the Layers-specific case-0/41/42-44
  draws (plain GP_ShowMaskedPict or Mirror variants depending on mask bit 512) and NatPal
  for the national-mask layer. Added three missing static wrappers around
  GP_ShowMaskedGeneric: GP_ShowMaskedPictInv (op 0, mirrored) and GP_ShowMaskedMirrorPict/
  Inv (APPROXIMATION: the Windows versions wobble each column vertically via the XShift
  table for water reflections - Linux draws them flat; extend Generic with a wobble param
  if water reflections ever look wrong).
  **STATE after ROUND 6 (screenshot-verified)**: the in-game screen renders coherently -
  terrain (soil/gold walls/bridge/grass/rocks), resource bar (WOOD/FOOD/STONE/GOLD/IRON/COAL),
  minimap with viewport rectangle, units/creatures, panels. The earlier "crazy colors"
  report is resolved. NOTE: the user's game data is a Dungeon Keeper-style MOD on the
  Cossacks engine (DUNGEON KEEPER title bar, rooms/creatures) - explains the unusual
  unit/nation names in the data files. gOpenFile's per-open printf is now opt-in
  (GSC_SHOW_OPEN=1 env var). KNOWN ISSUES remaining: (1) mouse cursor traces
  (VirtualScreen damage tracking - virtscr.cpp asm, next big porting task),
  (2) pathfinding (units cannot move - path.cpp), (3) fog/water visuals stubbed.
  ROUND 7 (2026-10-10): **virtscr.cpp FULLY PORTED + palette bug fixed**.
  - virtscr.cpp: the whole ~850-line implementation region (CopyVSToScreen, RenderVSPart,
    RefreshSurface, ShowLimitedSector, RenderVerticalSet, triangle mapping, Execute, Zero,
    RefreshTriangle, RefreshScreen, PrepareLandLocking, AddTHMap...) was `_WIN32`-guarded
    but is PLAIN C++ - un-guarded wholesale. The ONLY real asm was CopyVSPart/
    CopyVSPartMMX (row-copy VS->screen) - translated to memcpy loops. Deleted the
    VirtualScreen::Execute/RefreshScreen/RefreshTriangle + AddTHMap stubs; the GetBmOfst
    stub got the REAL masks.cpp formula: ((i&3)<<6)+((i>>2)<<14). SVSC.Execute() runs
    per frame from TestTriangle (3dgraph.cpp:2285) - the damage-tracked field redraw
    should now recompose the screen under the cursor and over the leftover menu image.
  - **LoadPalette palette-switch bug (the "everything brown" symptom)**: ddini.cpp
    LoadPalette had `if (sdlPal) return;` at the top - the FIRST palette (menu
    2\agew_1.pal) stuck forever, in-game 0\agew_1.pal was never applied. Now the
    palette object is created once and its COLORS are updated in place on every call
    (primarySurface references the same SDL_Palette, so the switch is immediate).
  - REMAINING virtscr gap: PrepareIntersection1/2 (masks.cpp - NOT in build, asm file)
    are still stubs -> mixed-texture terrain CORNERS render via a zeroed ResultMask
    (wrong/no blend) until masks.cpp gets ported. Single-texture tiles are fine.
  NOTE: the saved RN*.m3d files accumulate in the game dir (EraseRND is still the Windows
  stub that should delete "RN? *.m3d" - port it when convenient).
- **Menus are 1024x768, centered, NOT stretched** (intended behavior, not a bug): the UI
  canvas is fixed 1024x768; `menu_x_off/menu_y_off = (screen-1024)/2...` center it on
  larger screens (main.cpp even caps menu res to 1920x1080 with the comment "Fullscreen?
  Prepare for small not stretched menu"). Windowed 1024x768 fills the window exactly;
  fullscreen 1920x1200 shows a centered 1024x768 menu with borders. Verified via runtime
  size dump: window/pixels/rendererOut/texture all = RealLx x RealLy - no HiDPI or
  mode mismatch involved.
- **SelCenter collision**: multi.cpp has array `word SelCenter[8]` (used by brigade/select/
  loadsave/multi); activescenary.cpp had an `extern "C" bool SelCenter(GAMEOBJ*,byte,int)`
  scenario-DLL export with the same symbol name. The function is now `#ifdef _WIN32`-guarded.
- **CCIT/CNAT**: defined in activescenary.cpp, declared extern in megapolis.cpp
  (activescenary.h declares them only under _WIN32).
- **GP sprite cache not ported (performance, not correctness)**: on Windows, first
  draw of a sprite decompresses it into a shared 4.2MB ring buffer (PackCash/GetCash,
  blocks with `[PRefOfs][UDataSize]` headers) and remembers the block in the picture's
  `Pack` field via CASHREF tables; FreeRefs(page) zeroes the headers on UnLoadGP so
  the ring can be reclaimed. On Linux the cache is bypassed: each draw decompresses
  into a temporary `new byte[]` (CASHREF stores 32-bit pointers - unusable on 64-bit).
  `GP_System::FreeRefs` is therefore a documented no-op on Linux. If in-game rendering
  is too slow (every unit sprite decompressed every frame), port the cache: make
  CASHREF pointer-sized, keep GetCash (plain C), port the FreeRefs _WIN32 body 1:1.
  See docs/GPFormat.md section 6 for the cache layout.
- **All GP_System::Show* variants ported (linux_stubs.cpp has no GP stubs left)**:
  the Windows draw helpers differ only in their asm "variation zone" (per-pixel op)
  and whether they draw mirrored. Linux now has `GP_ShowMaskedGeneric` (gp_draw.cpp,
  Linux block) implementing the full masked-line walker with ops:
  0 dst=src, 1 Enc[(src<<8)|dst] (MultiPal), 2 Enc[src] (Pal), 3 Enc[dst] (Shadow),
  4 Enc[(dst<<8)|src] (MultiPalT), 5 Overpoint dither (0 on (y+x) odd cells, no
  data), plus mirror (the *Inv variants: x is the RIGHT edge, dx subtracted,
  right-to-left). Wrappers: PalPict/Inv, MultiPalPictInv, MultiPalTPict/Inv,
  PictShadow/Inv, PictOverpoint/Inv. On top of it: ShowGPTransparent,
  ShowGPTransparentLayers, ShowGPPal, ShowGPPalLayers (Linux impls in gp_draw.cpp,
  temp-buffer decompression like Linux ShowGP; Layers variants gate each compression
  type by a bitmask; RLC paths no-op like all RLC on Linux - ShowRLCItemPal/Grad
  stubbed in gp_draw.cpp). ShowGPRedN/DarkN/Dark/Fired/Mutno/Grad are portable
  wrappers (un-guarded from the family block): RedN=ShowGPPal(yfog+N*256),
  DarkN=ShowGPPal(wfog+(N<<8)), Dark/Fired/Mutno=ShowGPGrad(table), Grad=RLC or
  plain ShowGP. NatUnpackT (2-bit expansion) is the C port of the NatUnpack asm.
  `NatPal[64]` and the `ctables.h` include were moved out of _WIN32 guards.
- **gscbrowser drives the REAL game renderer**: the tool links gp_draw/fastdraw/
  fog/resfile/gtext/exconst and renders GP previews through GPS.Show* itself
  (CMake target updated; tool defines the small globals that live in heavy game
  files: SCRSizeX/Y, RealLx/Ly, offScreenPtr, CurrentCursorGP, LX_fmap, RLCNSpr,
  ErrM, _ExMalloc/_ExFree, NLine, normstr). .gp previews now show a variant
  strip (normal | red | dark | transparent) drawn by ShowGP/ShowGPRedN/
  ShowGPDarkN/ShowGPTransparent on a 512x512 scratch screen, above the
  standalone contact sheet. TAB toggles ANIMATION playback: cycles the page's
  pictures through GPS.ShowGP; frame time adjustable with +/- (default
  100 ms; the game's animated buttons run 40 ms - Video_OnDraw in
  dialogs.cpp). Tool init: FilesInit() + LoadFog(2)
  load the real tables. Validated: all 1086 GP pages render via game code,
  0 crashes.
  TWO REAL GAME BUGS this surfaced (both fixed in gp_draw.cpp, game benefits):
  1) Linux ShowGP was missing the switch cases for national-mask (1),
  transparent (2,3,4) and shadow (5) sprite types - they printed "unhandled"
  and rendered nothing. Ported from the Windows switch (case 5 re-maps the
  screen through themed shadow tables Bright/yfog/Optional1-3 + 7936).
  2) StdUnpack/LZUnpack temp buffers need UnpackLen+10 slack: the Windows cache
  block is GetCash(UnpackLen+18) minus an 8-byte header, i.e. the final
  dictionary copy may legally run past UnpackLen. Exact-size buffers caused
  heap corruption on 41 pages. Nat/Grey temps stay exact (their cache blocks
  are +8-8=0 slack). Applied to all Linux Show* implementations.

## Log noise - not errors

- `gOpenFile: <name>` - was printed for EVERY file open; now OPT-IN via the GSC_SHOW_OPEN=1
  environment variable (arc/gscset.cpp gOpenFile). (`SetLastError` used to print "Setting last error code" too;
  that print was removed - os.cpp now has a silent stub.)
- Archive lookup uppercases both sides (`_strupr` shim works); `all.gsc` stores names like
  `INTERFACE\NEWFLAG.GP`, so backslash mixed-case requests do resolve inside archives.
  The non-archive fallback (gscset.cpp `toLinuxPath`) converts `\` -> `/`.

## Next steps (rough priority)

0.5. **In-game screen**: the game loop runs but nothing renders/updates. Un-comment
     PreDrawGameProcess's body (main.cpp ~1185+; check each call for portability),
     port virtscr.cpp's VirtualScreen::Execute/RefreshScreen asm, and the
     ProcessMapInterface/DrawZones/LoadMapInterface region (interface.cpp guards,
     currently stubbed). ESC-exit sets GameExit only inside a guarded block
     (ProcessGMainMenu case 1) - un-guard when touching that.

0. **tools/gscbrowser.cpp** - small SDL3 archive browser (`cmake --build build --target gscbrowser`,
   run `build/Debug/gscbrowser [archive.gsc]` from the game data directory). Left pane: filterable
   file list (type to filter, Ctrl+U clears, arrows/PgUp/PgDn navigate, Esc quits). Right pane:
   preview - BMPs decoded and displayed (8bpp paletted + 24bpp), GP sprite pages decoded to
   contact sheets, GRD tables as 256-wide palettized images, .pal as 16x16 grid, WAV info,
   text dumps (word-wrapped), hex fallback. Headless: `--list <arch>` prints entries,
   `--dump <arch> <file>` prints preview info. Validated against all 1086 .GP files in all.gsc.
   **Full GP format documentation: docs/GPFormat.md** (headers, StdUnpack dictionary-LZ,
   NatUnpack, per-line mask encoding, NextPict chains, worked examples).
1. Run `build/Debug/cossacks` (windowed and fullscreen) and chase the next crash.
   Priority: press OK in the single-player random map dialog - the game start now
   runs PrepareGameMedia (map gen, nation loading, AI setup) for the first time on
   Linux; expect crashes in that path (4-bytes-per-pointer allocations, pattern #2).
   The `int(ptr)` audit is COMPLETE - all remaining sites are in `_WIN32`-guarded asm
   regions or dead on Linux; new crashes are more likely 4-bytes-per-pointer
   allocations (pattern #2) or asm-translation stride bugs (pattern #3's RestoreMData
   family - always verify translations against the asm's actual row/origin math).
2. Port `path.cpp` -> delete pathfinding stubs from linux_stubs.cpp (units can then move).
3. Port `fog.cpp` + `virtscr.cpp` asm regions (visual correctness).
4. Un-comment `interface.cpp` guarded regions piece by piece (82 guards; many are DirectPlay
   network code which stays guarded - os.h defines NODPLAY).
5. Consider central `\`->`/` conversion for direct `fopen` users (Gopen in gtext.cpp uses
   raw fopen; game-data paths like "Missions\\Missions.txt" in enabled files will fail
   at runtime outside archives).
6. Commit the current state once a session of runtime testing is green.
