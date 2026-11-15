# Brigade AI: unit orders, city pools, and the frozen-workers trap

This chapter covers the engine side of the computer player's military and
worker management: the per-unit order chain, the city's named brigade pools,
how finished units are sorted into them, and one concrete bug pattern — AI
workers frozen in place — diagnosed from `gamedata/test.sav`. The nation
module side (what gets produced and researched) is in
[060_national_ai.md](060_national_ai.md); this chapter is what the engine does
with those units every AI tick. Main files: `src/game/brigade.cpp`,
`src/game/megapolis.cpp`, `src/game/mine.cpp`, `src/game/newmon.cpp`,
`src/game/build.cpp`.

## 0. The 60-second version

```
City::ProcessCreation()  (megapolis.cpp, once per 8 ticks per nation)
  |
  ├─ ExecuteBrigades()   each enabled Brigade runs BOrder->BLink
  │                     (B_MakeBattleLink = attack, etc., brigade.cpp)
  ├─ HandleBrigades()    rebalance pools Free/OnField/OnWood/OnStone via
  │                     CalculateBalance (SetMineBalanse table) + Check* CFN
  ├─ TakeNewMine / UpgradeMines   peasant brigades to capture/deepen mines
  ├─ SendAgressors / HandleDefending / SmartGamer   raids, forts, armies
  └─ FindNeedProject()   -> the nation module's ProcessAI (chapter 060)

Every unit: OneObject::LocalOrder -> Order1 chain, each with a DoLink from
ORDLIST (32 kinds, loadsave.cpp). The motion lock:
  UnlimitedMotion=1 + SetUnlimited/ClearUnlimited orders  <- newborn units,
  mine walkers; PUSE(OB) = UnlimitedMotion || boxed-in  -> unit is NEVER
  drafted into any pool (RegisterNewUnit + all Check* bail out) -> frozen
  forever if the ClearUnlimited order is lost.
```

## 1. The order chain of one unit

Every `OneObject` has `LocalOrder`, a linked list of `Order1` (mapdiscr.h).
Each `Order1` carries `OrderType`, `PrioryLevel`, and a `DoLink` function
pointer from a fixed 32-entry table `ORDLIST` (loadsave.cpp) — the save game
stores just the table index (`GetOrderKind`), which is also why the sav is
only loadable by the same build. The orders that matter for workers and
soldiers:

* `NewMonsterSendToLink` / `NewMonsterSmartSendToLink` — walk to a point;
* `TakeResLink` — gather a resource (field/wood/stone/mine sprite);
* `GoToMineLink` / `LeaveMineLink` — enter/leave a mine (mine.cpp);
* `BuildObjLink` — build a construction site (brigade.cpp:984);
* `AttackObjLink`, `AI_AttackPointLink`, `PatrolLink`, ... — combat;
* `SetUnlimitedLink` / `ClearUnlimitedLink` — the motion-lock pair:
  `OneObject::Produce` (build.cpp) creates a newborn with
  `UnlimitedMotion = true`, queues `SetOrderedUnlimitedMotion(0)`, the
  send-to-born-point orders, then `ClearOrderedUnlimitedMotion(2, GID)`;
  `ClearUnlimitedLink` (newmon.cpp) clears the flag once the unit has stood
  (`StandTime >= 5`) and is not boxed in (`FINDNEAREMPTY` if it is).

## 2. The city's named pools

`City` keeps named `Brigade` pools for workers and soldiers: `Free`,
`OnField`, `OnWood`, `OnStone`, `Builders`, `FreeArmy`, `Guards`, `Defenders`,
`Agressors` (plus `Settlers` for colonists). Every tick `City::HandleBrigades`
(megapolis.cpp) calls `CalculateBalance()` — which interpolates the
`SetMineBalanse` pair table the nation module installed (see chapter 060) into
`NeedPF/NeedPW/NeedPS` — then moves peasants between `Free` and
`OnField/OnWood/OnStone` to match, and runs each pool's `CheckMembers` with a
callback (`CFN`): `CheckFree`, `CheckFieldWorker`, `CheckWoodWorker`,
`CheckStoneWorker`, `CheckBuilders`. The callback inspects the unit's current
order (`BuildObjLink` -> move to `Builders`; `TakeResLink` with `FoodID` ->
stays on fields; ...) and issues new orders: field workers get
`TakeResource(field)`, wood/stone workers walk to the nearest storehouse
(`WoodSkladID`/`StoneSkladID`).

## 3. Sorting finished units: `RegisterNewUnit`

When a unit finishes (or is captured), `City::RegisterNewUnit` sorts it:
peasants -> `Free`; officers/drummers -> `SearchArmy`; transports ->
diversion brigades; artillery/ranged/mounted (`PushkaID`, `StrelokID`,
`FastHorseID`, ...) -> `Agressors` with `DoNotCall = true` (they only fight
via `SendAgressors`, which dispatches raids while `NAgressors < 10`, then
saturates at 15000); other melee -> `FreeArmy` / `Guards` / `Defenders` /
`Agressors` by dice, with the `MaxGuards`/`AbsMaxGuards` caps the AI module
set via `SetDefSettings`. Ships go to water brigades (`MakeShipBattle`).
`SmartGamer` then forms defensive armies from `Agressors`
(`CheckMinArmyCreationAbility` -> `CreateMinimalArmyFromBrigade` -> `Parad` +
`MakeBattle`), and `HandleDefending` posts `Defenders`/`Guards` to threatened
points. `Brigade::AddInRadius` (brigade.cpp) drafts nearby units that are not
`DoNotCall`/`InArmy`/already building into attack brigades.

## 4. Brigade-level orders: `BOrder` and `[Local sent to]`

A `Brigade` has its own order chain `BOrder` (head of a `BrigadeOrder` list,
`DeleteBOrder` pops it), executed each tick through the order's `BLink`
callback. Unlike unit orders, brigade orders carry a message string shown in
the UI — that is where `[Local sent to]` comes from.

`Brigade::LocalSendTo(x, y, prio, OrdType)` (brigade.cpp) means "whole
brigade to this point, keeping formation":

* it creates a `Brigade_LST` order (brigade.cpp:569 — `N`, `StartTime`, three
  `PrevSumm*` stuck-detection slots, and `Position[30]`) with
  `BLink = B_LocalSendToLink` and message `"[Local sent to]"`;
* per-member destination slots `Position[3i..3i+2]` = target plus the
  brigade's convoy offsets `posX[i]/posY[i]` (from `CreateConvoy`: a square
  grid, 32 cells apart, centered on the target), plus member id packed with
  its serial (`id | serial << 13`). `CorrectPositions` spiral-searches
  (`Rarr` rings) any slot that lands in a barred cell out to a free one;
* `LinearLocalSendTo` instead lays the members on a line perpendicular to the
  approach direction (70-cell spacing); `WideLocalSendTo` uses 128 spacing;
* each member then gets a plain unit order `NewMonsterSendTo(slot << 4, prio)`.
  War brigades (`WarType`) skip all this and do `HumanLocalSendTo` — an
  attack-move in the brigade's facing direction.

`B_LocalSendToLink` (brigade.cpp:584) runs every tick while the order exists:

* hard timeout: `tmtmt - StartTime > 300` -> `DeleteBOrder()`;
* stuck detection: it sums member `RealX + RealY`; if the sum is unchanged
  across three consecutive checks after 50 ticks -> `DeleteBOrder()`;
* if any member locked onto an enemy (`EnemyID != 0xFFFF`) -> `DeleteBOrder()`
  (combat takes over);
* re-issue: members more than 250 cells from their slot and without a current
  order get `NewMonsterSendTo` again; members busy with `TakeRes` are re-sent
  to their slot.

Who issues it: the topology walk links. `B_GlobalSendToLink`
(brigade.cpp:1080) resolves the brigade's and the destination's topology
areas (`GetTopology`, `MotionLinks` adjacency table); same area ->
`LocalSendTo` to the exact point, otherwise `LocalSendTo` to the center of
the next area (`(AR->x << 6) + 32`) — waypoint hopping across the map.
`B_CaptureMineLink` (brigade.cpp:1177) walks to a mine the same way, creating
the mine (`UID_MINE`) if absent, or `MoveAway` + retry with `time = 5` when
placement fails.

The trap: all three `DeleteBOrder()` exits above never call `Rospusk()`, so
members keep `DoNotCall = 1` and — once their individual `SendTo` orders
finish, or were never re-issued because they stood within 250 cells of their
slot — they are left with no orders at all. That is the stranded freeze of
the next section, now swept by `City::ExecuteBrigades`.

## 5. The frozen-workers trap (diagnosed from `gamedata/test.sav`)

The key rule: **`PUSE(OB)` = `OB->UnlimitedMotion || CheckBar(OB->x, OB->y,
OB->Lx, OB->Lx)`** (megapolis.cpp:400) — "passive/unusable". A unit in
unlimited motion or boxed in is never drafted — `RegisterNewUnit` returns
early for it, and every `Check*` callback returns -1 ("stay") for it. So if
`UnlimitedMotion` stays set while the `ClearUnlimited` order is gone, the
unit is frozen forever — it looks exactly like "was going somewhere but no
longer does".

The two PUSE halves:

* `UnlimitedMotion` — a scripted move in progress: newborn units walking to
  their born point after `OneObject::Produce`, peasants walking into a mine
  (`GoToMineLink` sets it). Normally a `ClearUnlimited` order
  (`ClearUnlimitedLink`, newmon.cpp) clears the flag once the unit has stood
  still 5 ticks and is not boxed.
* **Boxed in** — `CheckBar(x, y, Lx, Lx)` asks the motion-block bitfield
  `MFIELDS` (`MotionField::CheckBar`, path.cpp) whether *any cell of the
  unit's own Lx×Lx footprint rectangle is blocked*. Bars are set by terrain,
  water and buildings at map build/placement, and by other standing units
  (`UnitsField.BSetBar(OB->x, OB->y, OB->Lx)` in newmon.cpp). So a unit
  surrounded by buildings or packed units has its footprint barred — it
  cannot path anywhere, and PUSE keeps it out of every pool.

`gamedata/test.sav` shows this pattern in every nation. The save is a
4-player game (players "Very hard"/"Taras"/"Normal"/"Easy", colors 0/1/2/4,
identity `NatRefTBL`; the tail's `MyNation` byte says the human is nation 1,
"Taras"). Motion-locked units with an **empty order list** per nation:

| nation | who | UM units | UM with no orders | brigade ids | biggest clusters (units at same cell) |
|---|---|---|---|---|---|
| 0 | AI "Very hard" | 31 | 30 | in `Brigs[]` pools | 9 at (161,872), 5 at (71,777), 5 at (90,917) |
| 1 | human "Taras" | 61 | 60 | all `0xFFFF` | 9 at (905,647), 9 at (921,919), 9 at (841,735) |
| 2 | AI "Normal" | 11 | 10 | in `Brigs[]` pools | 4 at (201,157), 4 at (184,184) |

So the frozen groups of 9 standing at one cell are produced units that never
left their born points; in each nation only ~1 motion-locked unit still has a
pending `ClearUnlimited`/`NewMonsterSendTo` order that could release it. The
fix belongs in whatever drops the order chain while the flag stays set (e.g.
`ClearOrders()` callers in newmon.cpp), not in the nation AI module.

To investigate live: **Ctrl+PrintScreen** in game calls `DumpSelectedUnits()`
(main.cpp), which prints to stdout the player nation and AI/difficulty per
nation, each nation's named-pool brigade IDs, a per-nation census of
`UnlimitedMotion` units (with and without orders), and for every selected
unit its position, flags, `PUSE` evaluation (`UnlimitedMotion || CheckBar`),
brigade state (members, pending `BOrder` + its message string), and the full
order chain with `DoLink` names.

Live diagnosis from `gamedata/test.sav` (player = nation 2 "Normal") shows
two distinct freeze flavors:

1. **Stranded `Brigs[]` brigade members.** Peasants with `DoNotCall=1`, no
   unit orders, sitting in a regular brigade (`Brig=2`, `CFN=no`) whose
   `BOrder` was `[Local sent to]` (`B_LocalSendToLink`). That link deletes
   its order on timeout (`tmtmt - StartTime > 300`) or when the members stop
   moving — via plain `DeleteBOrder(); return;`, never calling `Rospusk()` —
   so the members are never returned to `Free`; with `DoNotCall=1` they are
   also skipped by `Brigade::AddInRadius` re-drafting. They stand "as if
   going somewhere" forever. **Fix:** `City::ExecuteBrigades` now sweeps
   non-pool brigades (`i < MaxBrig - 11`) whose army is gone (`ArmyID ==
   0xFFFF` or `!ARMS[ArmyID].Enabled` — an orphaned brigade with `ArmyID=0`
   is exactly the frozen grenadier group), that are not war brigades
   (`WarType`) or mine-defender brigades (their `[Protect mines]` BOrder is
   a permanent no-op), have members and no `BrigDelay`, and are stranded:
   either no `BOrder` at all (order timed out), or the `BOrder` keeps
   refreshing while every member stands orderless (`!LocalOrder &&
   StandTime >= 100`) — the capture-mine walk loop case. Stranded members
   are returned with `AllowBuilder` + `DeleteBOrder` + `Rospusk()` (named
   pools are the last 11 slots, so they are never caught); `Rospusk()` also
   clears `DoNotCall` so the pool `Check*` callbacks re-task the units.
   Verified against the initial commit (`ce8bebc`, the original sources):
   `PUSE`, `City::RegisterNewUnit`, `City::HandleBrigades` and
   `B_LocalSendToLink` are unchanged by the port, and the original
   `ExecuteBrigades` likewise only disbanded *empty* brigades — so the
   stranded-member freeze is original-game behavior, not a porting bug; the
   sweep above is a deliberate fix on top of it.
2. **Boxed-in units (`PUSE` via `CheckBar`).** E.g. a `Builders`-pool peasant
   with `boxed=1`: every `Check*` callback returns -1 on `PUSE`, so it keeps
   its stale order and `DoNotCall` and cannot move. `CheckBar` reads the
   static motion field `MFIELDS`, barred by terrain/water at map build, walls
   (`walls.cpp`), resource sprites and buildings — `CheckBuilding` (the
   placement test) is unchanged from the original, so a building placed over
   a standing worker boxes it, in the original game too. **Fix:**
   `City::UnboxWorkers()` (megapolis.cpp, called each AI tick after
   `HandleBrigades`) walks boxed-in peasants out to the nearest free cell
   (`FindUnitPosition`) using the same `SetOrderedUnlimitedMotion` /
   `NewMonsterSendTo` / `ClearOrderedUnlimitedMotion` sequence as newborn
   units — `UnlimitedMotion` lets the unit path through the bars, and the
   `ClearUnlimited` order drops the flag once it stands on a free cell. It
   only fires for units that stand still (`!InMotion && !BackMotion &&
   StandTime >= 30`) — a moving unit bars its own current cell, so without
   that gate it would be re-sent to random cells forever.
3. **Pathfinding-level freeze flush against an obstacle** (`CreatePrePath` /
   `CreatePrePath2`, path.cpp). When a moving unit's next step is blocked, the
   per-tick retry in newmon.cpp calls `CreatePrePath(OB->PathX[OB->NIPoints-1],
   ...)` — always with `PathX` already set. The old code bailed out immediately
   on `InLocked && PathX` (unit's own footprint cell is barred while a path is
   active), and also gave up when no cell on the direct line to the target was
   free (`uu >= Pps -> return false`). A unit pressed against a building or
   packed into a crowd therefore failed pathing on *every* retry and stood
   frozen forever, still holding its move order — visually "was going
   somewhere but no longer does". Fix (ported from the 1.52-platform engine
   fixes): the early bail-out is removed so the `InLocked` recovery block
   always runs; when the direct line has no free cell, the unit steps to the
   free neighbour cell closest to the target (`bestDir` over `nidrx/nidry`);
   only if boxed in on all 8 sides does it fall through to the wall-following
   trace seeded at its own position (`escapeFromLocked`, `Rpp = 0`).

Note the census's `UnlimitedMotion=1` + no-orders units are NOT frozen: they
are miners hidden inside mines (`Hidden=1`, normal state after `HideMe()`).

Related UI note: clicking a unit that belongs to a `Brigs[]` brigade selects
the **whole brigade** — `ImSelBrigade` (multi.cpp) adds `Brig->Memb[2..NMemb]`
(slots 0/1 are the officer/drummer pair). So clicking one frozen peasant of a
5-member brigade selects 3 line units — that is what the debug dump's `NSL`
shows. When such a brigade captures a mine, members get `GoToMine` and become
the hidden `UnlimitedMotion=1` miners; a member that stays outside is either
still walking or was left stranded (see above).
