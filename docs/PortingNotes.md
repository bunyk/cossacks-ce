# Cossacks CE Linux Port - Working Notes

State and findings as of 2026-10-09. Companion to `docs/Ubuntu.md` (build instructions).

## Current state

- **Build succeeds**: `cmake --build build` produces `build/Debug/cossacks` (64-bit ELF,
  links clean) and the `gscbrowser` resource-browser tool.
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
- Still NOT in build / not ported: `shiptrace.cpp` (4 asm),
  `fastdraw.cpp` (12 guards, 33 asm, partially done),
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
- **Terrain triangle artifacts on relief (open, 2026-10-12)**: user reports visible
  triangles in the texture ("like some of them are swapped, or z-fighting") plus
  constant vertical seams on hills. A full static audit against upstream ce8bebc came
  back FAITHFUL for every suspect: 3drandmap texture painting (SqInt, REND/REND1/REND2,
  CostTex - diff is only benign renames), Save3DMap/LoadSurface/Tiles/Sect (raw arrays),
  all triangle rasterizers (addLine, PrecPrepareToRender/AbsolutePrecPrepareToRender,
  PrecRenderTriangle64/128/Dithering, DirectRenderTriangle64Dithering - note the asm
  texture index is ((BMY>>16)&0x3F)<<8 | ((BMX>>16)&0x3F) via `ror ebx,8; mov ah,bh`;
  the fog index folds the pixel into AL: 16384 + ((fog>>8)&~0xFF) + pixel), the dither
  +-32768 alternation, ShowClippedTriangle(+MMX) clipping, the whole masks.cpp port,
  RenderBestTriangle64/DirectRenderTriangle64 (identical C), GetLighting/GetVTex/DTX/
  GetBmOfst/prp34 (mul3(x)=2x is an UPSTREAM quirk, not a port bug), darkfog/trans8
  (loaded verbatim from .grd), randoma (from random.lst), tiles3.bmp load (0x436 seek,
  ntex = size>>12), and the RLC mask-table rebase (same formula as the working sprite
  path). The bug was then bisected dynamically with temporary runtime toggles
  (cycled via Ctrl+Shift+Y: no fog pass / no checkerboard dithering / single
  texture instead of blending / dump next 8 blends to terrain_dump_N.bin).
  TWO LESSONS for future debugging:
  1) event->key.mod is NOT populated by this SDL build - read modifiers via
     GetSDLKeyState (SDL_GetKeyboardState) instead.
  2) The blend dump (per triangle: params bm1..3/x0/y0/s1..3 + ResultMask +
     the three source tile pages) made the bug measurable offline: expected
     models could be scored against actual output (1.00 buggy vs 0.10
     asm-faithful for _2; 0.67 = base+overlay for _1).
  ALL of this instrumentation is REMOVED (fixed 2026-10-12); only the fix and
  this note remain.
  THE DUMPS FOUND THE BUG (fixed 2026-10-12): FastCreateMaskedBitmap64_2's
  Linux translation double-applied the +32 column offset (dst=ResultMask+32
  plus dst[31-i] = col 63-i, while the asm's 'dec edi' lands at col 31-i).
  The mirrored trapezoid was written to ResultMask columns 32..63, but the
  mask overlays (CopyMaskedBitmap, ids 54+) and the triangle sampler both use
  columns 0..31 - so half the mixed-texture triangles (TriType 1/2,
  PrepareIntersection2) rendered STALE ResultMask content from earlier blends
  = the "triangles look swapped / z-fighting" artifact. Fix: dst=ResultMask.
  Verified offline: dump0/2/3/5/7 matched the buggy model 1.00 and the
  asm-faithful model 0.10; _1-path dumps 1/4/6 were already correct (0.67 =
  base + overlay region).
- - **Water: realwater.cpp fully ported (2026-10-12)**: the animated sea was
  entirely absent (TestGP stub -> no HandleWater; WaterDeep/WaterBright pointed
  at a 1MB zero buffer). Ported all 13 asm routines behind #ifdef _WIN32 with
  C++ translations:
  - `ProcessWaves1` + `FastProcess1/_0/_1`: 16-bit wave equation
    `new = (short)(((sum4 - 2*cur) >> k) + cur - prev)` - keep EVERY add
    truncated to (short) like the ax register, and the sign-adjust
    `result -= (short)result >> 16` (16-bit sar -> 0 or -1).
  - `ExtrapolateCell`: 8x16-byte block copies. UPSTREAM QUIRK preserved:
    the asm adds 'dst' to the SOURCE (esi), destination is the plain array
    base, and the destination row stride is 512 bytes (not WaveLx*2=520).
  - `DrawCost`, `DrawCost1`, `DrawCost2`: the water cell renderers. Cell =
    32x16 screen px, 8 wave rows drawn on even rows, each wave sample covers
    4 px and the same dword is smeared one row down at +2 (isometric
    staircase). Gradient = Wave[pos+WaveLx]-Wave[pos-WaveLx] (16-bit wrap!),
    color = Colors4[(grad>>7)+1024+shade]. Shore cells (WaterDeep 128..143):
    per-pixel bilinear z; z<0x800000 -> leave terrain, z>=0x900000 -> solid
    water, else WaterCost[((zband&0xF0)|colorNibble) | (pixel<<8)] blend;
    pixels 1,3 of each 4-pack test against +0x10000 (checkerboard shore
    dither). DrawCost1 blends with the UNDERLYING pixel, DrawCost2 with the
    constant CostCL 0x3D; DrawCost1/2 alternate shades ASHI/ASHI1 per row
    pair (8+4+4 sample split in the solid branch).
  - `CopyWaves`: per-row memmove; the inverse branch must copy rows in
    DESCENDING order (asm rep movsd with std) to avoid inter-row corruption.
  - `DisturbWater`: the asm was EMPTY (push/pop only) - removed entirely
    (no callers, no effect on Windows either).
  - VISUAL NOTE: at darker->lighter water shade transitions the engine draws
    stepped, slightly jagged vertical lines (4-px wave-sample granularity +
    the Colors4 parity dither columns + the +2 isometric row offset). This
    is the authentic renderer look, confirmed by an offline simulation of
    the exact translation logic - NOT a porting bug. The water palette is
    0xB0..0xBF of the active AGEW_1.PAL: the ROOT/0 palette holds the smooth
    blue ramp + foam whites there, while the mod's 2\ palette has REDS at
    those indices (water would render as harsh red noise under palette set 2).
  Wiring: realwater.cpp added to CMake; InitWater un-guarded (main.cpp);
  TestGP moved from gp_draw.cpp (was #ifdef _WIN32) next to HandleWater -
  call chain GFieldShow -> TestTriangle -> SVSC.Execute() + TestGP() ->
  HandleWater() runs per frame and draws straight into ScreenPtr (after the
  VS copy, before panels). Removed linux_stubs: TestGP, SetWaterSpot,
  EraseWaterSpot, CreateWaterLocking, SetBrightSpot, SetupSpot, SpotByUnit,
  WaterDeep/WaterBright static buffer (real arrays come from SetupArrays in
  loadsave.cpp, filled by map generation). WaterBright spots now work
  (unit ripples in newmon.cpp were silently dropped). Minimap water: the
  fork's base source had water recolored to black/grey (0x00/0x01/0xE7/0xEF)
  - restored the original blue ramp 0xB3..0xB7 in 3dmaped.cpp CreateMiniMap.
  NOTE: water colors are hardcoded palette indices 0xB0..0xBF - if the mod's
  palette repurposes those rows, water will take on whatever colors they hold.
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
- **RLC sprite rendering ported (2026-10-12)** - this was the invisible-wheat
  bug. RLC files (GROW0..4 = wheat growth stages, CURSOR_Y, RLC fonts) are
  plain RLE sprites: 4-byte header (short SizeX,SizeY), per scanline a
  segment count byte, per segment (skip,count) + count pixel bytes.
  - fastdraw.cpp had a PREPROCESSOR IMBALANCE: `#ifdef _WIN32` before the
    commented-out ClearScreen never closed - the entire RLC family (asm AND
    the existing #else stubs) sat in the Windows branch and was SILENTLY
    skipped on Linux. Fixed the dangling guard; only after that did the
    per-function #else branches become live.
  - Ported the 8 core blitters as one clean-room generic (ShowRLCGen):
    plain dst=src, pal dst=pal[src], "fon" dst=pal[dst] (mask-only - the
    shadow/white variants), "ht" dst=pal[(src<<8)|dst] (half-transparent:
    trans8/Dark/Fire/WFog/Blue wrappers), each x mirror (Inv) variant.
  - SEGMENT SKIP IS CUMULATIVE (fixed after the "vertical squares" bug):
    the asm keeps ONE running edi per scanline - each segment's skip byte
    advances from the PREVIOUS SEGMENT'S END, not from the line start.
    Reading it as line-absolute shifts every segment after the first left,
    compressing sprites into overlapping blocks ("wheat in vertical squares
    instead of the isometric diamond"). Verified by re-decoding GROW0 with
    cumulative skips: the 5 growth parts compose into a proper diamond.
  - ShowRLCItem (was a no-op stub): full dispatch - GP files route to
    GPS.ShowGP; RLC pictures are at (char*)table + 4 + OfsTable[n]
    (unrebased offsets on 64-bit!). Nation tints route via PAL1..7 (NOTE:
    PAL1..7 are never initialized in this codebase - all-zero; nt>=1 tints
    would render black if ever used; fields use nt=0 plain).
    The 4 unguarded Item dispatchers (DarkN/Pal/Grad/RedN) got the same
    rebase. gp_draw.cpp's duplicate ShowRLCItemPal/Grad no-op stubs
    deleted. ShowRLCItemTrans8/Mutno/Dark/Blue/Fired remain Windows-only
    (unreferenced on Linux so far - waves.cpp's use is commented out).
  - In-game screen palette: `if (CurPalette==2) LoadPalette("2\\agew_1.pal")
    else LoadPalette("0\\agew_1.pal")` - the DK mod's gameplay matches 2\AGEW_1.PAL
    (wheat indices 89/90/241-244 = yellow/green "fresh wheat"; under 0\ they
    are purple-blue - useful for identifying which palette is active).
- **Cursor save/restore (fixed 2026-10-11)**: `RestoreMData` (mouse_x.cpp) had its
  comparison INVERTED vs the asm: asm restores the saved background only where the screen
  still equals the post-cursor save; the port wrote the STALE background wherever the
  cursor had drawn pixels, clobbering fresh content. Symptom: shadow-shaped smears of old
  texture around the mouse cursor (visible while scrolling; the opaque cursor body hides
  its own smear, blended layers don't). gscbrowser can't reproduce this - the mouse
  machinery never runs there.
- **Game menu items enabled (fixed 2026-10-12)**: F12 menu opened but Save/
  Load/Options did nothing - their handlers (ProcessGLoadGame/ProcessGSaveGame/
  ProcessGameOptions + the whole options/load/save dialog layer) sat in five
  `#ifdef _WIN32` regions of interface.cpp (~6183 decls+MM_ProcessMultiPlayer,
  ~6310 options, ~7024 name lists, ~7113 load, ~7764 save) and one in
  dialogs.cpp (~4746 BorderEx) - ALL portable C++ (verified: zero asm, zero
  winapi). Un-guarded all six. CAUTION: use a real brace-matched guard-end
  finder for these regions; a naive "first #endif" match or replace-by-line-
  range eats neighboring declarations (one bad edit destroyed 76 lines of
  decls and had to be reverted with git checkout).
  Supporting ports/stubs: POSIX glob shim for FindFirstFile/FindNextFile/
  FindClose + WIN32_FIND_DATA (interface.cpp, handles Win32 backslash
  patterns), SFLB_CreateGamesList got the real implementation (was a TODO
  printf), PushScreen/PopScreen spliced from the editor-guard region,
  CD-audio API stubbed (devicecd.cpp MCI - GetCDVolume/SetCDVolume/
  StopPlayCD/PlayRandomTrack; sliders inert), MidiSound/RECFILE/ChangeNation/
  MultiTvar/EnterStr/EnterHi duplicates removed from linux_stubs (real defs
  now in un-guarded interface.cpp), processMultiplayer/processBattleMultiplayer/
  ProcessSavingFile/ProcessLoadingFile stubbed (guarded regions, MP/file-picker
  only). Save files land as *.sav next to the executable.
- **Game menu items enabled, round 2 (fixed 2026-10-12)**: after un-guarding
  the dialog definitions, Save/Load/Options STILL did nothing - the CALL SITES
  in ProcessGMainMenu had their OWN separate `#ifdef _WIN32` guards around the
  ProcessGLoadGame()/ProcessGSaveGame()/ProcessGameOptions() calls (and the
  editor menu around ProcessGameOptions). ItemChoose dispatched fine (DBG
  probes showed mcmOptions=0xE7/mcmSave=0xF8/mcmLoad=0xF7/mcmExit=0xFF
  arriving correctly) but the calls were compiled out. Also un-guarded
  IngameYesNoDialog (its whole body was `#ifdef _WIN32` + `#else return false
  TODO`) - that alone broke Exit, since the confirm always answered "no".
  LESSON: definition guards and call-site guards are independent; when a
  feature "still does nothing" after porting its implementation, grep the
  CALL SITES for their own guards. mcm* constants live in dialogs.h
  (mcmExit=0xFF, mcmSave=0xF8, mcmLoad=0xF7, mcmOptions=0xE7...).
- **In-game hotkeys un-guarded (fixed 2026-10-12)**: F12 (main menu), Pause
  (CmdPause -> NOPAUSE toggle) and every letter/F-key hotkey in GameKeyCheck
  did nothing - the whole ~350-line switch block (main.cpp ~1781-2134) was
  `#ifdef _WIN32` even though it was already fully SDL-ported. Un-guarded.
  Needed: a file-scope extern/prototype block before GameKeyCheck (the
  declarations lived in ddex1.cpp's scope, not main.cpp's) - watch the types:
  ChangeNation is a bool FLAG not a function, NeedToPopUp is byte. Two
  callees came from still-guarded regions and were re-implemented/stubbed in
  linux_stubs.cpp: CheckFNSend (+NGSPlayers global; F1-F9 multiplayer-taunt
  check - with chat off it returns false so F1 opens the help menu) and
  ShowStatistics (CapsLock; returns false - the real statistics screens
  ShowStatScreen/ShowUserStat live in interface.cpp's guarded ~12879 region,
  port them separately when needed). GameKeyCheck dispatch note: the PlayGameMode==1
  branch (top of function) is the replay 'exit on any key' path.
- **Unit production from buildings (fixed 2026-10-12)**: clicking a unit button
  in a building's panel did nothing. The whole chain was already ported and
  intact (ShowAbility -> HPR -> CmdProduceObj -> ExBuf cmd 78 -> ExecuteBuffer
  -> ProduceObject -> OneObject::Produce -> order -> ProduceObjLink per frame
  -> CreateNewMonsterAt) - found by DBG probes at each hop. The break:
  `GetQueueMultiplier()` was stubbed to return 0 in linux_stubs.cpp, and HPR
  queues production via `for (i = 0; i < GetQueueMultiplier(); i++)` - zero
  iterations, click silently swallowed. The real function lives in
  newcode/queuemanagement.cpp (modifier keys pick the count: F5=250, Tab=50,
  F2=36, Alt=20, F1=15, Shift=5, default 1) and its SDL port was ALREADY in
  the tree - just never added to CMake. FIX: added to build, deleted the stub.
  LESSON: grep linux_stubs for functions that RETURN VALUES (not just void
  no-ops) - a void stub breaks loudly, a `return 0` stub breaks silently and
  looks like a data/logic bug. Also: in-game controls are already ported:
  Pause/Break key = CmdPause -> NOPAUSE toggle (sim gated in main.cpp),
  F12 = main menu (MenuType=1 -> ProcessGMainMenu, linked), Esc = clear
  current mode (attack/build cancel) - all un-guarded.
- **AV blend-table pointer truncation CRASH (fixed 2026-10-12)**: the KNOWN
  LATENT BUG from the f3aa479 note finally fired: selecting certain buildings/
  walls draws the pulsing highlight via Z-elements with Param1=AV_PULSING|AV_DARK
  etc.; AddOptPoint/AddOptLine/AddToVisual stored the blend table as
  `Param2 = int(wfog)` - a 64-bit pointer truncated to 32 bits - and the draw
  side did `(byte*)par2 + (N<<8)` -> SIGSEGV in GP_ShowMaskedPalPict/
  GP_ShowMaskedGeneric (Encoder read). FIX: the table is fully determined by
  the type+color bits that ALREADY live in Param1 (CType bits 0..3,
  CProp AV_RED/WHITE/DARK bits 4..7 survive `Options & 0xFFFFF0F`), so a new
  `AVBlendTable(Param1)` (zbuffer.cpp, also used by newmon.cpp's UParam draw
  path) rebuilds yfog/wfog/fog+1024(+2048) at draw time; all 27 int(ptr)
  stores replaced with comments. LESSON: never store a pointer through int()
  fields - if an index/enum carries the same information, rebuild at the
  consumer instead of widening the struct.
- **Selected-building shadows (fixed 2026-10-12)**: the THIRD case-5 dispatcher
  had the same f3aa479 branch swap - the LINUX port of `ShowGPPalLayers`
  (gp_draw.cpp ~11152; the selected-buildings path: ShowZElement sees
  ImSelected+NewBuilding -> par1=7432 -> ShowGPPalLayers(wfog+(N<<8))).
  mask bit 64 must select GP_ShowMaskedPictShadow (soft), no bit 64 =
  Overpoint (black column dither) - the Linux port had them swapped, so
  selecting a building turned its shadow into vertical black lines
  (unselected buildings draw via ShowGPLayers, which f3aa479 had already
  fixed - hence "buildings ok, selected buildings broken").
  LESSON: f3aa479 fixed ShowGPLayers + ShowGPTransparentLayers but missed
  the SEPARATE Linux port of ShowGPPalLayers - the file has THREE parallel
  dispatcher families (original WIN32 asm, in-place commented-asm C++
  replacements, and dedicated `// Linux port of ...` functions after
  #endif); audit ALL of them for the same bug pattern when one is found.
  Also fixed 5 latent `lpGPCUR = lpGP + DIFF` layer-advance sites (inside
  WIN32-only regions): the asm adds RAW BYTES (`mov eax,lpGP; add eax,DIFF`),
  pointer arithmetic scales by sizeof(GP_Header)=23 - must be
  `(GP_Header*)((byte*)lpGP + DIFF)` (the Linux ports already did this).
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

## Enemy AI loading layer ported (2026-10-13)

The AI is NOT in the engine: it is a per-nation MODULE the game loads at match
start. Chain: PrepareGameMedia (random map) or loadsave.cpp LoadAI (saves) /
recorder.cpp (replays) -> StartAIEx / LoadAIFromDLL (activescenary.cpp ~5800)
-> LoadLibrary("<nation AI dll>"), call its InitAI export, store
NT->ProcessAIinDLL. The per-tick driver was already ported: City::FindNeedProject
(megapolis.cpp:1212) calls NT->ProcessAIinDLL() when `hLibAI && ProcessAIinDLL`.
The fallback "script AI" (strecog.cpp LoadAI(char*,Nation*) / NT->AI_Req) has NO
callers - dead code, the module path is the only AI.

Ported this round:
- os.h/os.cpp: LoadLibrary/FreeLibrary/GetProcAddress are now REAL (were
  hardcoded NULL): osLoadLibrary = dlopen with Windows-path translation
  (backslash->slash, ".dll" retried as ".so", bare names tried in ./ and the
  exe dir via /proc/self/exe); errors logged to stderr.
- activescenary.cpp LoadAIFromDLL un-guarded: shared body, Linux branch logs
  load failures instead of message-box+assert (a missing AI module leaves the
  nation inert, the megapolis tick checks hLibAI && ProcessAIinDLL).
  linux_stubs.cpp: LoadAIFromDLL stub DELETED.
- interface.cpp: DetermineNationAI un-guarded (was at the tail of the
  Windows-only lobby block; guard boundary moved above it). Stub deleted.
- CMake: `target_link_options(cossacks PRIVATE -rdynamic)` puts the 182
  extern "C" DLLEXPORT AI-API functions (activescenary.cpp) into the exe's
  .dynsym; AI modules resolve them with dlsym(RTLD_DEFAULT) - the Linux
  equivalent of the AI DLLs calling GetProcAddress("DMCR.EXE").
- src/ai/Algeria (the only nation AI SOURCE in this tree; all others ship as
  PE DLLs in the game data): builds as Algeria.so (CMake MODULE target,
  PREFIX "" so no lib prefix). dllmain.cpp got a Linux port: wtypes.h replaced
  by a dlfcn shim, `__declspec(x)` #defined away, DllMain(DLL_PROCESS_ATTACH)
  duplicated as an `__attribute__((constructor)) AI_OnLoad()` (runs at dlopen
  time, calls the same LOAD_FUNCTION list + OnInit). Built copy placed at
  AI/Algeria.so in the game dir.
- RUNTIME CONFIRMED (2026-10-13, user-tested): **Algeria as a random-map
  computer opponent WORKS end-to-end** - map start loads AI/Algeria.so,
  InitAI registers units, the megapolis tick calls ProcessAI. Two fixes
  were needed after the first probe run (see below). NOTE: dlopen CANNOT
  read files inside all.gsc - AI modules must be loose files in the game
  dir (a built copy is at AI/Algeria.so in the repo).

Two bugs found by the first probe run, both fixed:

- **COMPSTART was never filled on Linux** (the silent "nothing happens"):
  the single-player lobby stores computer slots in PINFO[HostID].COMPINFO
  (word: color | team<<4 | difficulty<<8 | nation<<11; the lobby also dumps
  them to comp.opt). On Windows StartIGame (mplayer.cpp, not ported) does
  `memcpy(COMPSTART, PINFO[HostID].COMPINFO, 16)` before the network start
  packet; PrepareGameMedia then decodes COMPSTART into PINFO[i] and calls
  StartAIEx per slot. The Linux SingleOptions #else branch had skipped that
  copy, so the decode loop and the AI loop iterated over zeros. FIX:
  interface.cpp SingleOptions Linux branch now finds the host slot and
  copies COMPINFO -> COMPSTART before PrepareGameMedia (same as StartIGame).
- **Symbol interposition SIGSEGV in the module constructor** (crash at the
  `LOAD_FUNCTION(RegisterUnitType)` line): the module's function-pointer
  globals (typedefs.h: `tpRegisterUnitType* RegisterUnitType` etc.) are
  same-named as the game's exported API functions. With -rdynamic the
  exe's symbols sit FIRST in the global lookup scope, so the module's own
  `RegisterUnitType = <ptr>` store bound to the exe's code symbol and
  wrote into read-only .text. FIX: the module is compiled with
  `-fvisibility=hidden` (CMake target_compile_options) and only the three
  game-facing exports (InitAI/OnInit/ProcessAI) are marked
  `__attribute__((visibility("default")))` - the Linux #else defines
  `__declspec(x)` as that attribute. LESSON for any future dlopen'ed module
  in this codebase: hidden by default, export explicitly; and never let a
  module define globals that mirror game export names.

What the DK mod's data actually wants (learned via the DBG probe):
GetTextByID("<NATION>.0") resolves to STOCK Cossacks AI DLL paths -
`AI\POLAND.dll`, `AI\Algeria.dll` etc. (the mod kept stock ai.dat nation
IDs). The game dir has only AI/Algeria.so so far; every other opponent
nation logs `LoadLibrary(AI\X.dll) failed` and stays inert. `TryUnit`
validates `Type=='UTYP'` so unregistered units are safely ignored - wrong-
nation modules would be inert, not fatal.

ROUND 3 (the actual in-game silence): **GetSound and
SimpleDialog::AssignSound(char*,int) (dialogs.cpp) had their ENTIRE BODIES
inside `#ifdef _WIN32`** - the empty-function trap in non-void form: on
Linux GetSound fell off the end and returned GARBAGE (observed n=19637 in
PlayEffect), so EVERY sound ID in the game was garbage -> every
PlayEffect/PlayCoorEffect call hit `if (n < NSounds)` false -> silence
even though the device was open and 197 buffers were loaded (DBG probes
confirmed: SoundOK=1 NSounds=60 total=197). Un-guarded both (plain
SearchStr/ErrM code), plus their supporting decls: dialogs.cpp's
`#include "cdirsnd.h"/"gsound.h"` (MaxSnd) and `extern char* SoundID[];
extern word NSounds;` were also _WIN32-only.

## Sound + music enabled (2026-10-13)

ROUND 2 (the "I don't hear anything" fix): **the vendored SDL3 had NO audio
drivers** - it was configured before libasound2-dev/libpulse-dev/
libpipewire-0.3-dev were installed, so libSDL3.so contained only the
dummy + disk audio drivers ("No available audio device" - the game then
sets SoundOK=false and silently loads zero sound buffers). After
installing those packages: `cmake -B build -U "ALSA_*" -U "PULSEAUDIO_*"
-U "PIPEWIRE_*" -U "JACK_*" -U "SNDIO_*"` to un-cache the NOTFOUND find
results, rebuild -> libSDL3.so now has pipewire/pulseaudio/alsa.
Verified OFFLINE with a scratchpad harness (sndtest.cpp linking
cdirsnd.cpp+cwave.cpp directly): device opens, wav loads, and GetPos()
advances at exactly 88200 B/s = real time for S16/2ch/22050 - the device
callback IS pulling real data. ALSO fixed two fork bugs in cdirsnd.cpp:
- `IsPlaying` had an unconditional `return false;` at the top (rest dead
  code) - PlayEffect's "don't replay a sound that is already playing"
  logic never worked (every trigger re-fired; harmless but wrong), and
  ControlPan's rand()<350 check never worked.
- `SetPan` was a NO-OP statement (`pSoundBuffer->pan;` - missing `= pan`)
  - all stereo panning was dead; now assigned.
- The byte-vs-word confusion in AudioStreamCallback/StreamAudioBuffer was
  audited: despite the sloppy sizes, the final A bytes put on the stream
  are the real first-A-bytes of source with volume/pan applied (the
  garbage the loop computes beyond that is simply never copied out) -
  output is correct; the FIXMEs remain cosmetic.

The engine's audio backends were ALREADY SDL3-ported (gamesound.cpp fully
un-guarded; cdirsnd.cpp = SDL_OpenAudioDevice + per-buffer
SDL_OpenAudioDeviceStream with a mixing callback; cwave.cpp = SDL_LoadWAV +
SDL_CreateAudioStream conversion to S16/2ch/22050; devicecd.cpp = full
SDL3+stb_vorbis rework of the CD player, playing Tracks/Track_XX.ogg).
What was missing (all fixed this round):

- **The data**: the DK mod's all.gsc contains only SOUNDLIST.TXT + TRACKS.CD -
  NO wav/ogg files (SDL_LoadWAV/stb_vorbis read loose files, they CANNOT go
  through the GSC archive). Copied the loose Sound/ (4.9M, 114 wavs - all 112
  names the mod's SOUNDLIST.TXT references are present) and Tracks/ (32M,
  18 oggs) from the GOG Back to War install into the game dir.
- **cwave.cpp**: SoundList paths are Windows-style ("Sound\ResKam1.wav") -
  now converted backslash->slash before SDL_LoadWAV.
- **devicecd.cpp was NEVER IN THE BUILD**: added it + newcode/stb_vorbis.c
  (implementation TU; the .h is STB_VORBIS_HEADER_ONLY wrapper) to CMake.
  Fixed there: include case ("sdl3/sdl.h" -> "SDL3/SDL.h"), devicecd.h used
  DWORD/SDL types without includes (now includes SDL3/os.h), "Tracks\\..." ->
  "Tracks/...", and the LAZY-INIT fix: CDeviceCD CDPLAY is a GLOBAL - its
  ctor ran before SDL_Init/FilesInit, so Open() failed silently at startup
  and nothing retried; the ctor is now empty and Play() lazily reads
  Tracks.cd (via Gopen, works with the archive) + opens the device.
- **linux_stubs.cpp**: GetCDVolume/SetCDVolume/StopPlayCD/PlayRandomTrack
  stubs DELETED (real defs now compile). MidiSound stays (options-slider
  state var only; interface.cpp's def is _WIN32-guarded).
- **Volume defaults**: WarSound/WorkSound/OrderSound were 0 = near-SILENT
  (PlayEffect does vol -= (100-WarSound)*40 in DirectSound units;
  0 -> -4000 -> 1% gain). Windows loads these from mode.dat (guard kept);
  Linux defaults are now 100 (full). MidiSound likewise 100.

Music flow: Options dialog (main menu or F12) Music combo -> PlayMode=1 ->
PlayRandomTrack -> PlayCDTrack(TracksMask[...]) -> stb_vorbis decode ->
SDL audio stream; the stream's get-callback re-arms PlayRandomTrack when
drained, so it plays continuously. CDPLAY.GetVolume/SetVolume map the
options slider. SDL_InitSubSystem returns BOOL (true=success) in
SDL3 - the checks in cdirsnd/devicecd are correct as written.


ROUND 4 - CDirSound redesigned to ONE stream (the intermittent
"audio disappears mid-session" bug): the fork's design opened ONE
SDL_OpenAudioDeviceStream PER SOUND BUFFER (~200 streams on one PipeWire
client, all created by LoadSounds at startup). PipeWire shed the client
intermittently (the sink input vanished while the app ran). Redesigned
DirectSound-style: ONE master stream (m_masterStream) whose single
get-callback (MasterStreamCallback) mixes every playing SoundBuffer into
each block (StreamAudioBuffer = proper S16 mixer with clamping, per-buffer
DirectSound-style volume/pan; CreateSoundBuffer/DuplicateSoundBuffer/
CopyWaveToBuffer no longer open streams). Verified offline via the
scratchpad sndtest.cpp harness (GetPos advances at 88200 B/s, clips end
exactly at size).

ROUND 5 - in-game silence was FOG GATING + two 64-bit font crashes:
- **Fog gates**: PrepareToGame (ddex1.cpp:1416) sets FogMode=1 at EVERY
  match start, and the unported GetFog stub returned 0 = "all fogged", so
  every Add*Effect with a fog check returned before playing anything.
  INTERIM (REMOVE WITH THE FOG.CPP PORT): linux_stubs GetFog now returns
  1000 (visible), and GetV_fmap (loadsave.cpp) is documented all-visible
  (it feeds only the three sound gates). DBG probes proved triggers fired
  while PlaySingleEffect was never reached until these were fixed.
- **64-bit font crashes (pattern #3 family), ASLR-dependent - they came
  and went between runs and did NOT reproduce under gdb**: the RLC-font API
  encodes "font as small GP-handle" vs "font as RLCTable*" with
  `int(lpr) < 4096` - on 64-bit a truncated heap pointer can be NEGATIVE
  and passes the test. Fixed with `(uintptr_t)lpr < 4096` in fastdraw.cpp
  GetRLCWidth, GetRLCHeight, GetRLCWidthUNICODE (the width side; crashed
  inside GP_System::LoadGP case 2 via the rom10 width table) and
  ShowRLCItem (the DRAW side; crashed via ShowString -> ShowChar ->
  ShowGP(FileIndex=-777319232)). Both hit first from the in-game
  unit-info panel (CreateUNITINF_UNIT, its "25" text button).
- **Main-menu Options button did nothing**: the dispatch
  `ItemChoose == mcmEdit -> ProcessMenuOptions()` sat in the shared _WIN32
  dispatch block with the editor/video/load items (call-site guard vs
  ported definition - same lesson as the F12 menu round). Guard boundary
  moved so only mcmEdit runs on Linux; mcmVideo (editor media) / mcmLoad /
  mcmExit STAY guarded - the main-menu Load and Exit buttons are still
  dead (TODO).

ROUND 6 - SOUND + MUSIC CONFIRMED WORKING END-TO-END (user-tested):
menu clicks, in-game effects (orders/combat/ambient with panning), and
background music (Options -> Music combo -> PlayRandomTrack ->
CDeviceCD::Play -> stb_vorbis decodes Tracks/Track_XX.ogg (5.9M samples
at 44.1kHz observed) -> its own SDL stream, auto-advancing when drained).
ALL DBG probes from this round are STRIPPED. The offline probe harness
lives in the session scratchpad (sndtest.cpp - links cdirsnd.cpp+cwave.cpp,
plays any Sound/*.wav without launching the game; reuse for future audio
regressions).


ROUND 8 - national music in SINGLE PLAYER never got a nation: CurrentNation
(interface.cpp) was only ever set by the multiplayer lobby's player-slot
page (MNATION combo, ~4351); single-player left it -1, so NATMUS fell into
the random path (or worse, into Track_00 if the lobby default leaked).
MyNation is a COLOR SLOT, not a nation type - the ai.dat index the track
table wants comes from DetermineNationAI (the same lookup replays use:
scan the player's units' peasant ID -> GlobalAI.Ai[j]). PrepareGameMedia
now sets CurrentNation = DetermineNationAI(MyNation) right after
Load3DMap; -1 makes NATMUS fall back to random. Austria = ai.dat index 2
-> TracksMask[2] = Track_05.ogg.


ROUND 9 - music volume slider reset to 0 on every options reopen: the
volume lived ONLY on the SDL stream gain. Before the first track ever
played, CDPLAY.audioStream is NULL (lazy init), so SetVolume silently
failed and GetVolume turned the -1.0f "no stream" error gain into 0 -
the dialog re-read 0 and showed the slider at 0. CDeviceCD now caches
m_volume (0..100, default 100): SetVolume stores it and applies it only
if a stream exists; Open() applies it to fresh streams; GetVolume returns
the cached value. Volume now persists across dialog sessions and across
music on/off.


ROUND 10 - "still no track playing" regression + Track_05 verified: the
ROUND 7 random-picker rewrite had a range bug: ((tick&0xFFF)+attempts*131)
escaped the 12-bit range as attempts grew, so candidates could exceed the
last track number (all decode failures), and with an unlucky tick phase
EVERY candidate in the bounded loop missed -> PlayRandomTrack returned
false -> the callback disarmed itself -> silence (national music at the
main menu falls into this fallback too, CurrentNation==-1 there). Fixed:
mask the varied seed back into 12 bits (((base + attempts*131) & 0xFFF)
* NTracks >> 12) + StartTrack). Track_05.ogg (Austria's national track)
verified good OFFLINE with the game's decoder (stb_vorbis: 6.39M samples
stereo 44.1kHz) and played audibly via a scratchpad harness (oggtest.cpp).
Added a permanent low-frequency log: "Music: playing <file>" on every
track start (devicecd.cpp).


ROUND 11 - music NEVER auto-started on Linux: the startup call
`if (PlayMode) PlayRandomTrack();` sits in the _WIN32 block at the end of
SDL_AppInit (with SetupMultiplayer/SetupHint/mode.dat write). PlayMode
DEFAULTS TO 2 (ddex1.cpp: "int PlayMode = 2" - random music ON), so on
Windows music starts with the app; on Linux nothing called PlayRandomTrack
at startup - music only played when the user cycled the options Music
combo (the PrevPlayMode != ... transition gate fires the call), which is
why the first music test worked and later sessions were silent. Moved a
Linux copy of the call right after LoadPalette (PlayMode/PlayRandomTrack
are not declared in any header - externs added at the call site).
NOTE: the rest of that _WIN32 block (SetupHint UI color masking, tmtmt/
REALTIME/KeyPressed resets, OnMouseMoveRedraw) is still not ported -
SetupHint may matter for in-game UI hints, check when hints are tested.

## Enemy AI - TODO to continue later

- **Strip the temporary DBG probe**: `fprintf(stderr, "DBG LoadAIFromDLL:
  Nat=%d Name='%s'")` at the top of LoadAIFromDLL (activescenary.cpp) -
  keep the failure logs in os.cpp/osLoadLibrary, those are permanent.
- **Update this file** when AI for other nations works.
- **AI for the other 19 nations**: NO public source exists - this tree has
  only src/ai/Algeria; the game data (and the GOG install at
  ~/.wine/.../AI/) ships 20 Windows PE DLLs (32-bit, cannot dlopen).
  Checked MisterCoderman/cossacks-1.52-platform: its ai/ folder is the
  same PE binaries; the "statically recompiled x86->C" AI is internal to
  their wasm build, not published as source. Realistic options:
  a) quick stopgap: copy/symlink Algeria.so as AI/<OtherNation>.so -
     registrations against nation-0 unit names mostly fail, AI stays
     inert (safe but useless);
  b) hand-port the PE DLLs: they all follow the Algeria template
     (LoadFunction + nation-specific Register* lists + ProcessAI decision
     tree) - extract each DLL's string table + decompile TryUnit call
     sequences (each is maybe a day of work per nation);
  c) an x86-32 interpreter for the PE DLLs (what cossacks-1.52-platform
     did in-wasm for mission DLLs) - big machinery.
- **Verify AI behavior in-game**: production seen, peasants working,
  attacks? If the AI builds nothing despite the module ticking, next
  suspects are the per-tick path (City::FindNeedProject is gated by
  CT->AI_DEVELOPMENT, set by megapolis.cpp:194 / SET_AI_DEVELOPMENT
  scenario cmd, activescenary.cpp:4117) and the safenet.cpp stubs.
- **safenet.cpp GetNextDiv/Gre/SafeCell still STUBBED** (linux_stubs.cpp)
  = AI long-range pathing across areas; AI units will stall on large maps
  until ported. THIS is the next real porting task for AI gameplay.
- **Difficulty**: StartAIEx passes Diff-1; CITY[Nat].Difficulty drives AI
  resource cheat levels (check it feeds through with the DK mod).
- **Saves/replays**: loadsave.cpp LoadAI (CT->AIID path) and recorder.cpp
  (DetermineNationAI + LoadAIFromDLL) are now live code on Linux - test
  that a saved game with an AI opponent restores the module (module is
  re-dlopen'ed on load; watch for stale NT->hLibAI across games - the
  loadsave.cpp:208 FreeLibrary(hLibAI) path was for the scenario DLL).
- **Scenario DLLs** (UserMissions\CMS_start.dll, activescenary.cpp ~425)
  now ALSO go through the real dlopen path - they will fail gracefully
  until someone ports/ships them as .so.


## Remaining port surface audit (2026-10-13)

"Remove all guards in interface.cpp" is the wrong goal. Audited: 67 _WIN32
regions / 9908 lines = 18 regions ~7000 lines MULTIPLAYER (lobby phase 2,
GSC online, chat, DirectPlay callers - blocked on mplayer.cpp/ir_main.cpp,
~12k lines needing a DirectPlay -> UDP/SDL_net replacement - its own
project), 47 regions ~2860 lines plain C++, 2 regions 68 lines winapi
(GetComputerName - trivial shim; mission-list scan - the FindFirstFile
glob shim already exists). The big asm files (gp_draw ~10k, fastdraw 2k,
3dgraph/realwater/path/masks ~5k) are ALREADY PORTED - their guards keep
the original asm as reference by convention and should never be deleted.

Genuinely left, by value:
- mode.dat read/write (main.cpp WIN32 block) ~60 lines - settings
  persistence (volumes/resolution survive restarts)
- Statistics screens (ShowStatScreen/ShowUserStat) ~300 - replaces the
  ShowStatistics stub
- SelectSingleMission (~12420, known bounce bug), mission list browser +
  history battles (ProcessOneBattle ~963), random map dialog (~355)
- fog.cpp ~1350 asm lines - real fog-of-war; removes the GetFog/GetV_fmap
  interims, restores AI visibility semantics
- safenet.cpp GetNextDiv/Gre/SafeCell - AI long-range pathing
- mplayer.cpp + ir_main.cpp - multiplayer project
- bmptool.cpp stays guarded forever (pure Win GDI)


## fog.cpp PORTED (2026-10-13) - real fog of war, interims removed

All live fog asm translated; the interim hacks are GONE (GetFog returning
1000, GetV_fmap all-visible comment, linux_stubs fog/UnitLight/FogMode
stubs - all deleted; FogMode is defined in fog.cpp now).

What was translated (asm + #else per convention):
- ProcessFog1_1/2/3 + ProcessFog1: 16-bit 5-point blur with decay
  (out = (sum4>>2) - (sum4>>10), sums wrap to 16 bits) over the active
  sub-rect (cols/rows 1..mlx-1/mly-1 of the (msx/4+6) x (msy/4+6) region);
  the three asm variants differ ONLY in the row stride FMSX2_C (bytes) =
  2*FMSX_C, FMSX_C = 134<<(ADDSH-1) - one shared FogBlurPass(S) covers
  all three. Starts at word index S+1 (= asm byte offset 2+FMSX2_C).
- ShowSuperFluentFog32_160_16 (DrawFog's per-cell renderer): 32x16 block,
  bilinear corner fog (b=(z2-z1)>>5, c=(z3-z1)>>4, d=(z1+z4-z3-z2)>>9),
  per pixel darkfog[((fog>>8)&0xFF00)+pixel] - the same level-pixel fold
  as the terrain rasterizers; all corners <=63 skips, >=96 fills 0.
- DrawMiniFog (minimap): fmap <= 1300 blacks the pixel + its lower twin;
  64-BIT FIX: the original int(ScreenPtr)/int(fmap) byte offsets are real
  pointers now (fp is word*, so the asm byte strides DDDX/F_add halve).
Plain C++ un-guarded: SetupFog, GetF, TurnFogOn/Off, UnitLight (per-unit
vision spots by VisionType), WFSC/SetWF/CreateFogImage (4x upscale of the
viewport fog), DrawFog, FogSpot, GetFog, FOGOFFS init in LoadFog.
KEPT WINDOWS-ONLY (dead - zero callers in the tree, verified by grep):
SetScreenFog16x16, ProcessScreenFog16x16 (old BFog system),
ShowSuperFluentFog16_160, ShowSuperFluentFog12_160 (DrawFog only uses the
32x16 variant), SetLightPoint, ShowSuperFog decl; LoadBlobs() stays
guarded (shiptrace.cpp not ported).

PIPELINE (all live): PreDrawGameProcess -> ProcessNewMonsters ->
CalculateMotion -> UnitLight(own units, newmon:8020) -> FogSpot writes
8000; GFieldShow per frame (mapa:~1878, gated FogMode && !Vision) ->
ProcessFog1 (blur/diffuse) -> CreateFogImage (upscale to WFSC) ->
DrawFog (32x16 blocks via GetF -> 5 visible / fden[] dark);
DrawMiniFog on the minimap (mapa:~2570); GetFog/GetV_fmap return REAL
values now (sound gates: unexplored areas silent - CORRECT behavior now,
not a hack).

VERIFIED OFFLINE (fogtest.cpp harness in the session scratchpad, links
fog.cpp with game-global shims): FogBlurPass == independent model of the
asm (0 mismatches), ShowSuperFluentFog32_160_16 bilinear == model (0),
full-fog fill == model (0). gscbrowser needed new inert shims for the
now-live fog globals (msx/msy/mapx/mapy/smaplx/smaply/smapx/smapy/ADDSH/
MiniMode/MiniLx/MiniLy/MiniX/MiniY/minix/miniy/GetHeight).


## mode.dat / rec.dat settings persistence ported (2026-10-13)

The settings load in SDL_AppInit and the save at shutdown were entirely
_WIN32-guarded (portable Gopen/Gscanf/Gprintf code), so on Linux the
volumes, resolution, scroll speed, speed mode, music preference and
record settings reset every session. Guard boundaries moved:
- READ (SDL_AppInit ~350): InitObjs3 + the volume zeroing + mode.dat (12
  values: window res, fullscreen res, WarSound, OrderSound x2, MidiSound,
  dummy-7th, ScrollSpeed, exFMode, PlayMode) + rec.dat now load on Linux -
  IDENTICAL to the original including the zero-then-load volume behavior
  (linux_stubs defaults reverted to 0 to match; first run is silent until
  the user moves a slider, exactly like Windows). PlayMode loads BEFORE
  the ROUND-11 music autostart, so the saved music preference auto-starts.
- WRITE (SDL_AppQuit ~890): resolution packing + mode.dat/rec.dat saves
  are portable now; the Windows-only cleanup (ClearScreen/UnLoading/
  CloseExplosions/ShutdownMultiplayer/FilesExit/FinishInternetExplorer)
  stays guarded. StopPlayCD() + FreeDDObjects() now run on Linux at exit.
- Both un-guarded sites needed extern decl blocks (the symbols were only
  visible to the guarded code): WarSound/WorkSound/OrderSound/MidiSound/
  ScrollSpeed/PlayMode/RECFILE/RecordMode/InitObjs3/StopPlayCD.
The "Linux doesn't read mode.dat" gap from the sound rounds is CLOSED.


## Save/Load game fixed (2026-10-13)

Load-from-F12 works end-to-end now. Three bugs, found via offline chunk
walks of the .m3d and the user's gdb backtrace:

- **PEAC (peace-time) chunk size off by 4** (savenewmap.cpp
  SavePeaceTimeInfo): `sz = 4 + N + 4 + 8 + 3` but SEVEN 1-byte state
  flags are written - the size field was 4 short, so the m3d chunk walk
  (stride = signature + 4 + size, verified against every other chunk in
  the file) landed 4 bytes early on the next chunk, read a garbage
  signature, fell into the unknown-chunk RSeek(garbage) branch and looped
  forever (RSeek 4GB -> the "load game froze"). Diagnosed entirely
  offline by walking the m3d bytes and comparing with the real signature
  positions.
- **LoadPeaceTimeInfo used RBlockWrite for the 7 state flags** (upstream
  bug - writes to the read handle: flags never loaded AND 7-byte stream
  desync). Fixed to RBlockRead.
- **NewAnm/HiLayer/LoLayer saved as int(ptr)-offsets** (loadsave.cpp
  1233/1297, pattern #3): SAVE computed int(NewAnm)-int(newMons) from
  TRUNCATED pointers, LOAD rebuilt via (NewAnimation*)(int(newMons)+pp) -
  a 32-bit-truncated base pointer = invalid pointer -> SIGSEGV in
  BuildObjLink (OBJ->NewAnm->NFrames) on the first frame after load.
  File format keeps the 4-byte offset field; save computes it in 64-bit
  ((intptr_t)a-(intptr_t)b), load applies it to (byte*)newMons. NOTE: old
  Linux saves usually stored CORRECT offsets (same-heap pointers), so
  they load after just the load-side fix.
- **EXIT HANG in the audio teardown**: the global CDeviceCD destructor
  (runs at exit()) closed the music stream, but the audio device thread
  could be mid-PlayRandomTrack (a multi-second whole-track stb_vorbis
  decode) whose Play() RE-ATTACHES the get-callback at its end - so the
  main thread's clear-then-destroy raced against the re-arm and
  SDL_CloseAudioDevice's thread-join waited forever. Fix: g_musicShutdown
  flag set FIRST in CDeviceCD::Close, checked in MusicStreamCallback and
  PlayCDTrack (no decode, no re-arm during teardown); CDirSound's
  ReleaseAll also detaches its master-stream callback before destroying
  (defensive).
The Load3DMapLandOnly chunk-walk DBG probe is STRIPPED (verified clean:
VIEW SURF TILE SECT TREE UNI3 WAL2 LOC1 SEA2 RESU TOP1 TOPW ZON1 FOR1
.DLL AIIA PSMS PEAC RIV1 ENDM). Note the map files are written fresh at
every random-map start; old m3ds written by pre-PEAC-fix builds stay
unloadable. EraseRND is now PORTED (was a printf stub): dirent scan for
"RN? *.m3d", mtime sort, deletes all but the 3 NEWEST (same behavior as
the Windows FindFirstFile+SortClass version); the call in SDL_AppInit is
un-guarded.


## Options-slider sync + EraseRND (2026-10-13)

"Main menu options show music volume 100, sound 0, random track":
- The music slider is initialized from GetCDVolume() - the DEVICE volume
  (CDPLAY.m_volume), not the persisted MidiSound; nothing applied the
  loaded MidiSound to the device at startup, so the slider showed the
  device default (100). FIX: SDL_AppInit applies SetCDVolume(MidiSound)
  right after the mode.dat load - slider, device and file now agree.
- "Sound 0": the mode.dat from earlier sessions holds zeros - those
  sessions ended in the audio-teardown hang (Ctrl-C before the exit
  write), so the good values never saved. With the exit fixed, quitting
  writes the real values.


QUIRK (not a bug): the options volume sliders use the original VScrollBar
semantics - marker at TOP = 0 (silent), marker at BOTTOM = max (loudest);
dragging DOWN increases the value (VScrollBar_MouseOver maps SPos from
the marker's Y directly). Identical on Windows. mode.dat confirmed the
persistence round-trips (user drags "up" save 0/6 exactly as dragged).


## Loose-file priority bug (settings never loaded - 2026-10-14)

THE ROOT CAUSE of "settings don't load back": CGSCset::gOpenFile's
loose-file priority check (Windows: FindFirstFile, then archive lookup)
was _WIN32-only, so on Linux the ARCHIVE was always searched first - and
the DK mod's all.gsc CONTAINS ITS OWN STALE MODE.DAT (WarSound=0). The
game read the archive's copy while Gopen("wt") writes the LOOSE file -
settings wrote to disk and loaded from 1998. Diagnosed with headless
Gopen/Gprintf/Gscanf round-trip harnesses (gwtest/gwread in the session
scratchpad: both paths verified perfect in isolation) + in-game load
probes (file=98, loaded=0). FIXED: portable check - `access(F_OK)` marks
a loose file present, mirroring the Windows priority. NOTE: this also
affects ANY loose file that shares a name with an archive entry
(overrides the earlier "loose-file fallback" behavior, which only
kicked in on archive MISS). The load/options probes are stripped again
after verification.

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
