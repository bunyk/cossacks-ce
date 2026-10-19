# Cossacks CE Linux Port - Working Notes

State and findings as of 2026-10-09. Companion to `docs/Ubuntu.md` (build instructions).

## Current state

- **Build succeeds**: `cmake --build build` produces `build/Debug/cossacks` (64-bit ELF,
  links clean) and the `gscbrowser` resource-browser tool.

## src/game/linux_stubs.cpp - intentionally missing functionality

No-op stubs with exact header signatures so it links. Each stub is a TODO:

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

UPDATE 2026-10-18 (option b is working, see docs/AI.md - now the full
porting handbook):

- Turkey (AI/Turcia.so) and Russia (AI/Russia.so) are ported from the
  Algeria template by role-mapping every registered name to the target
  nation, validated against the original DLL's string table (the .so
  output name must match the DLL name the text tables resolve, e.g.
  Turcia for Turkey).
- CMake now emits all nation AI MODULEs straight into the game dir's
  AI/ folder (LIBRARY_OUTPUT_DIRECTORY).
- Biggest lesson: a name can register fine and still never be built -
  RUSSIA.NDS gates Kazarma behind the MAINRU "XVIII century" research
  (ENABLE UNITS GRP), and TryUnit fails SILENTLY. Check the .NDS
  [ACCESSCONTROL]/ENABLE-GRP before mapping buildings. Russia also
  needed MAINRU registered + researched (Turkey/Algeria have no XVIII
  century at all).
- tools/gsc_extract.py extracts any file from all.gsc (negated offsets,
  0x87 byte cipher) - use it to pull <NATION>.NDS.
- The 20 original DLLs cluster by size (16 x 94208 European incl.
  Russia; Turcia 69632; Algeria 57344; Ukraine 53248; Denmark 49152):
  one Ghidra/BinDiff pass over the 94208 cluster likely covers 16
  nations' real ProcessAI.





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


## ShowStatistics un-stubbed (2026-10-14) - end-of-game statistics screens

The statistics trio was NOT in a guarded region by itself: it sat at the
tail of the big _WIN32 block whose head is the battles screens
(ProcessOneBattle/ProcessWars/SlideShow, interface.cpp L12972+). Guard
SPLIT: #endif moved up right after SlideShow - the stats block
(PrintNum/ShowStatistics/CreateCommName/CalculateStatsScreenOffsets/
ShowStatScreen/ShowUnitsList/ShowUserStat + LPAL/ROME globals) is
cross-platform; the battles screens stay guarded (their call sites are
in guarded menus anyway). Verified via nm: all three symbols now compile
into interface.cpp.o.
Supporting un-guards (all plain C++):
- PlayGame's end-of-match block `if (ShowStat) { ShowStatistics() ... }`
  (interface.cpp ~9228) - THE call site that matters: CmdEndGame ->
  mapa.cpp CheckVictory sets ShowStat=1 on win/loss (un-guarded), then
  PlayGame's exit path shows the 3-screen stats (arrows navigate, ESC
  closes; res==3 means exit to menu). The J hotkey in GameKeyCheck only
  fires for PlayGameMode==2 (recorded replay viewings).
- GetPName + Names/tmm* globals block (interface.cpp ~2040) - stub
  deleted from linux_stubs.
- dialogs.cpp: both addListBox overloads (SQPicture variant + LxLy
  variant, L2242-2331) and ListBox_OnMouseOver (L2178) - plain C++,
  no asm/winapi; used by stats (and later by chat).
- interface.cpp got extern decls for lines.cpp's Hline/Vline/Xbar/DrawLine
  (in the build since long ago; header not included there).
- linux_stubs.cpp: ShowStatistics stub DELETED.
NOTE: ShowUserStat reads per-unit statistics (units built/lost per
type) - verify in-game that the numbers look sane with the DK mod.


ROUND 2 (2026-10-14): ESC after Victory/Defeat did nothing. The ESC case in
GameKeyCheck (main.cpp ~1819) had a SECOND sub-block guarded: the mode resets
(SetDestMode/GoAndAttackMode/GUARDMODE/PATROLMODE/AttGrMode/ShowGameScreen)
AND the `if (WaitState == 1) WaitState = 2;` advance. Chain: CheckVictory
(mapa.cpp ~1140) sets WaitState=1 (via WaitState 0->1 assignment at ~1219)
and shows "press ESC"; ESC must bump WaitState to 2, then the next
CheckVictory pass sets GameExit=true + ShowStat=1 -> PlayGame's exit path
-> ShowStatistics. All globals (defined in mine.cpp/multi.cpp/selprop.cpp/
mapa.cpp) needed local extern decls; GUARDMODE/PATROLMODE are BOOL.
LESSON repeated: even after un-guarding a whole switch once (the hotkeys
round), check the INDIVIDUAL cases for their own nested sub-guards.


## Statistics round 3 (2026-10-14): ESC key + exit crash + background

- ESC after Victory/Defeat did nothing: the ESC case in GameKeyCheck
  (main.cpp) had its own NESTED sub-guard inside the already-un-guarded
  hotkey switch - mode resets AND `if (WaitState == 1) WaitState = 2;`
  (the advance that lets ESC skip the victory screen; CheckVictory then
  sets GameExit/ShowStat). Lesson: after un-guarding a switch, check each
  CASE for nested sub-guards.
- EXIT CRASH "free(): invalid pointer" in ClearObjs3 (via UnLoading ->
  _ExFree - note ddini.h #defines free as _ExFree everywhere): TWO bugs:
  (a) InitObjs3 memset(Obj3Map, 0, B3SZ*4) zeroed only HALF the pointer
  array on 64-bit (pattern #2) -> after the first ClearObjs3 the second
  half held DANGLING cell pointers; (b) ClearObjs3 freed Obj3Map[i]
  WITHOUT nulling them (the OBARS loop does null). UnLoading runs after
  EVERY match, so the second pass double-freed -> abort at exit.
  Fixed: full-size memset (B3SZ*sizeof(word*)) + null-after-free.
- Stats screen BACKGROUND see-through (map visible under the graph):
  likely ORIGINAL LOOK, not a port bug - verified the stats code is
  byte-identical to upstream ce8bebc (no ClearScreen before
  ShowStatistics; Xbar is a 1-PIXEL BORDER box, NOT a fill - the grid
  cells are transparent by design; the frozen game frame was in the
  framebuffer on Windows too). VERIFY: run ./build/Debug/gscbrowser,
  open INTERFACE\STATISTIC_SCREEN.GP and look at frame 2 (the
  "Backgrounds for UI elements" GPS.ShowGP frame) - if it is a partial
  graphic with transparent areas, the see-through is authentic.


## Main-menu Load button un-guarded (2026-10-14)

The mcmLoad dispatch (interface.cpp ~6988) sat in the same _WIN32 block as
mcmExit (SlideShow - battles block, still guarded) and the user-mission
handling. Guard split so ONLY mcmLoad runs on Linux: processMLoadGame is
already ported (the F12 Load uses the same function). mcmExit (main-menu
Exit button -> SlideShow) and the ItemChoose==43 user-mission dispatch
STAY guarded - the Exit button is the last dead main-menu item.
Also note: the stats screens' REPLAY button requires PlayGameMode=2
recorded replay support (recorder.cpp replays) - not related to this.


## Music exit hang ROUND 3 (2026-10-14) - the real deadlock mechanism

The ROUND-9 g_musicShutdown flag was necessary but not sufficient: the
audio device thread could be INSIDE CDeviceCD::Play's stb_vorbis decode
(seconds long) when the main thread's atexit teardown ran. After the
decode, Play called SDL_ClearAudioStream/PutAudioStreamData/
SetAudioStreamGetCallback - SDL_DestroyAudioStream (main thread) takes
the stream lock and closes the device (device-thread JOIN). The audio
thread blocking on the stream lock + the main thread joining the device
thread = DEADLOCK; the join never completes (gdb: SDL_SYS_WaitThread
from ClosePhysicalAudioDevice from ~CDeviceCD at exit).
DEFINITIVE FIX (two layers):
- CDeviceCD::Play checks g_musicShutdown at entry AND AGAIN AFTER the
  decode - if shutdown began meanwhile it frees the buffer and returns
  WITHOUT touching the stream.
- CDeviceCD::Close (runs ONLY from ~CDeviceCD at exit): sets the flag,
  detaches the get-callback, NULLs the pointers - and does NOT destroy
  or pause the stream at all. No destroy -> no join -> no deadlock; the
  process is exiting anyway and the OS reclaims the audio device.
DESIGN NOTE (upstream, kept): the whole-track decode on the audio thread
is the root flaw - a future rework should decode on a worker thread and
feed the stream incrementally.


## Resizable window (2026-10-14)

Window mode now sets SDL_WINDOW_RESIZABLE. The render path already scaled
(window mode: FlipPages -> BlitSurfaceScaled -> primaryTexture (RealLx x
RealLy) -> RenderTexture(nullptr, nullptr) = stretch to window), so the
picture scales for free. Mouse input did NOT: SDL reports window-space
coordinates while the game works in logical RealLx x RealLy space - added
g_wndW/g_wndH tracking (set after window creation + on every
SDL_EVENT_WINDOW_RESIZED/MOVED) and ScaleMouseToLogical() applied to all
mouse event handlers (motion + both button cases) in SDL_AppEvent.
NOTE: the picture is STRETCHED to the window aspect (same convention as
the fullscreen path); letterboxing would need a dst-rect in RenderTexture
- not done, keep behavior consistent with fullscreen.


## Resizable window round 2 (2026-10-14): aspect letterbox + no snap-back

- Menu/game resolution changes snapped the window back to 1024x768:
  ResizeAndCenterWindow SDL_SetWindowSize(RealLx, RealLy) ran from
  SetGameDisplayModeAnyway (menu return) and every SetGameDisplayMode.
  DECOUPLED: the window size is now USER-ONLY (OS resize grip);
  ResizeAndCenterWindow only centers/clips/warps (warp uses the real
  window size). doInit still creates the window at RealLx x RealLy.
- Aspect-preserving letterbox instead of stretch: ddini.cpp exports
  g_letterbox (SDL_Rect, window space) + ComputeLetterbox() (fits
  RealLx x RealLy into the window, bars on the surplus axis).
  FlipPages window path: RenderClear(black) + RenderTexture with the
  letterbox dst rect (SDL_FRect - RenderTexture takes floats).
  ScaleMouseToLogical (main.cpp) maps through the same rect:
  (x-lb.x)*RealLx/lb.w, clamped - clicks land under the cursor inside
  the letterboxed area; cursor in the black bars clamps to the edge.
  ComputeLetterbox is recomputed per flip AND per mouse event (both
  resolution and window size can change at any time; the math is 4
  divisions).



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
