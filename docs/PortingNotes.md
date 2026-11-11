# Cossacks CE Linux Port — Working Notes

State as of 2026-11-11. Historical round-by-round logs removed; resolved bugs
live in git history. What remains here: current state, reusable debugging
techniques, and the recurring-bug checklist for newly enabled code.

## Current state

- **Build**: `cmake --build build` → `build/Debug/cossacks` (64-bit ELF) +
  `gscbrowser` tool. All 82 `src/game/*.cpp` compile, including `mplayer.cpp`,
  `ir_main.cpp`, and the CommCore library. `linux_stubs.cpp` is deleted —
  real implementations live in their original files.
- **No assembly in game code**: the last x86 asm was replaced with C++
  (commit 625a208). No `__asm`/`asm volatile` blocks in `src/game/`, `src/ai/`,
  or `src/missions/` (one MSVC debug-break remains in vendored stb_vorbis.c).
- **Single player works end-to-end**: random map start, full render+sim loop,
  save/load (F12), end-of-game statistics, ESC menu, map editor (`/MAPEDITOR`).
- **All 20 nation AI modules ported** (src/ai/* → gamedata/AI/*.so) — see
  **docs/AI.md** for the per-nation handbook.
- **Missions**: loading machinery works, 2/106 mission modules ported — see
  **docs/missions.md**.
- **Replays**: recorded-vs-replayed worlds stay hash-identical. The record's
  RunData carries the startup speed (SpeedSh); old records default to FrmDec=2.
- **65 `#ifdef _WIN32` guard lines remain** (19 files in `src/`, concentrated
  in interface.cpp) — all are Windows-API-specific, see docs/architecture.md
  "Still Windows-Only".

## Debugging techniques (proven)

- **RNG divergence tracing**: `#define rando() RandNew(__FILE__, __LINE__)` —
  every RNG call carries its site. A printf in RandNew gated by an env var
  gives a line-aligned trace to diff live vs replay; the first differing call
  is the culprit.
- **Per-tick world-state hash**: FNV hash over every live object's fields
  (x/y/RealX/Y/Dest/NewCurSprite/StandTime/LocalOrder/Selected/anim NFrames)
  finds the first diverging tick; per-object detail dumps then name the unit
  and field. Never print raw animation pointers — ASLR randomizes them across
  processes.
- **Ticks == frames**: the sim advances per frame (`tmtmt++` in
  PreDrawGameProcess); the PostDrawGameProcess tick gate only exchanges command
  buffers. "Ticks" and "frames" are the same currency in traces.
- **Offline binary format diagnosis**: walk the `.m3d`/`.gsc` bytes directly
  (chunk stride = signature + 4 + size; FAT entry → `~m_Offset` +
  dataStart(14+81*N), XOR key 0x87) and compare with real signature positions.
  `tools/gsc_extract.py gamedata/all.gsc "PATH\\FILE"` pulls any archive file.

## Recurring bug patterns (checklist for any newly enabled file)

1. **`sizeof TypeName` without parens** — legal on MSVC, error on GCC →
   `sizeof(TypeName)`.
2. **4-bytes-per-pointer allocations** — 32-bit assumption in `**` casts with
   `* 4`, `<< 2`, `4 *`. Find with:
   `grep -rnE "\*\*) *(calloc|malloc|realloc)" src/game/*.cpp | grep -vE "sizeof"`
3. **`int(ptr)` casts** — pointer truncation on 64-bit. Grep for
   `int(ScreenPtr)`/`int(RealScreenPtr)`/`int(offScreenPtr)` in newly enabled
   code; use `byte*`/`intptr_t` arithmetic instead.
4. **`bool b++;`** — illegal in C++17 → `b = true;`.
5. **MSVC token pasting**: `#define PRC(x) ((32768*##x##)/100)` → remove `##`.
6. **`DLLEXPORTint`** — needs a space after DLLEXPORT.
7. **`#include "safety.h "`** — trailing space in include name.
8. **Empty-loop trap**: an event loop with its entire body inside
   `#ifdef _WIN32` compiles to `do { } while (...)` — busy loop, frozen window.
   Same in function form: a non-void function whose body is guarded returns
   garbage on Linux (was the total-silence bug via `GetSound`).
9. **Nested sub-guards**: after un-guarding a `switch`, check each `case` for
   its own `#ifdef _WIN32` sub-block (was the ESC-after-victory bug).
10. **`goto` over variable initialization**: MSVC allows it, GCC does not.
    Hoist declarations to the top of the loop body when un-guarding loops
    with `goto FinCLC`.

## Structural conventions of the port

- **os.h shims** (src/game/newcode/os.h): `LoadLibrary`/`FreeLibrary`/
  `GetProcAddress` are real (dlopen/dlclose/dlsym with path translation),
  `GlobalAlloc/GlobalFree` → malloc/free, `HRESULT` = int, `_strupr`.
  Do NOT put `min` here (breaks `<algorithm>` — use ternaries locally).
- **pack(1) leaks**: `gp_draw.h`, `topograf.h`, `playerinfo.h`, `bmptool.h`,
  `arc/gsctypes.h` use `pack(push,1)`…`pack(pop)` so their file-format structs
  stay packed without leaking state to later includes. `SampleRoot` (14 B) and
  `TerrainData` (66 B) in 3drandmap.cpp are packed file-format structs too.
  Layout canaries: `static_assert(offsetof(SimpleDialog, OnUserClick) == 136)`
  (dialogs.h), `static_assert(sizeof(SingleAI_Desc) == 56)` (nature.h).
  If either fires, a header is leaking pack state. For any struct raw-read/
  raw-written to data files, verify layout against actual archive bytes.
- **`div(x,y)` is redefined** to `SecureDivision(x,y)` (mapdiscr.h:14,
  securedivision.cpp). Returns `div_t` with `.quot` — do not "fix" call sites.
- **VertBuf layout** (3dgraph.cpp rasterizer, 16 bytes/line):
  `+0 word Dx, +2 word Lx, +4 int BMX, +8 int BMY|BMxy, +12 int fog`.
- **Guard-boundary moves, not code moves**: when un-guarding, move
  `#ifdef`/`#endif` boundaries around functions in place rather than moving
  code (see AGENTS.md).
- **dlopen'ed modules**: compile `-fvisibility=hidden`, export only
  game-facing entry points, never define globals mirroring game export names.
  Modules must be loose files (dlopen cannot read inside .gsc archives).

## Fixed bugs worth remembering (root causes, one line each)

- **Replay desync**: record never carried startup game speed → FrmDec mismatch
  → animation gates fire at different ticks → RNG cursor divergence. Fixed by
  appending SpeedSh to the record's RunData.
- **Save/load freeze**: PEAC chunk size field was 4 bytes short (7 flag bytes
  written, 3 counted) → chunk walk desync → infinite RSeek. Also
  `LoadPeaceTimeInfo` used RBlockWrite on the read handle.
- **Save/load crash**: NewAnm/HiLayer/LoLayer saved as 32-bit-truncated
  int(ptr) offsets → invalid pointers after load. Save in 64-bit
  (`(intptr_t)a-(intptr_t)b`), apply to `(byte*)base` on load.
- **Loose-file priority**: archive was searched before loose files on Linux →
  stale archive copies shadowed disk writes. Fixed with `access(F_OK)` check.
- **Exit hang / deadlock in audio teardown**: audio thread mid-decode
  re-arms the stream callback while the main thread destroys it →
  SDL thread-join deadlock. Fixed with `g_musicShutdown` flag checked at
  entry AND after decode; `~CDeviceCD` does not destroy the stream at all.
- **Double-free at exit**: `memset(Obj3Map, 0, B3SZ*4)` zeroed half the
  pointer array on 64-bit + free-without-null → second `ClearObjs3`
  double-freed. Fixed with full-size memset + null-after-free.
- **COMPSTART empty on Linux**: single-player lobby stores computer slots in
  `PINFO[HostID].COMPINFO`; the Linux start path must `memcpy` them to
  `COMPSTART` before `PrepareGameMedia` (as Windows `StartIGame` does).
- **Module constructor SIGSEGV**: module globals same-named as game exports
  bind to the exe's .text under `-rdynamic`. Fixed with
  `-fvisibility=hidden` on modules.

## Cross-references

- **docs/AI.md** — nation AI porting handbook (all 20 ported, per-nation status).
- **docs/missions.md** — mission module porting (2/106 ported, engine side works).
- **docs/GPFormat.md** — GP sprite file format and cache layout.
- **docs/unit_rendering.md** — unit/building render pipeline details.
- **README.md** — build & run instructions, game data layout.
