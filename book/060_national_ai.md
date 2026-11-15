# National AI: how a computer player is built

This chapter explains how Cossacks drives a non-player nation: the per-nation
AI modules under `src/ai/<Nation>/`, the engine API they call, the tick that
calls them, and how "strategy" is actually encoded. It answers four practical
questions: which functions exist to call, which functions get called when the
AI decides something, whether there is an evaluation function to maximise, and
whether the nation-level AI rules only the economy or also the military. Two
nations are used as running examples: Ukraine (`src/ai/Ukraine/`, the old
standalone family) and England (`src/ai/England/`, the shared "European"
skeleton in `src/ai/common/european_land.h`).

## 0. The 60-second version

A nation AI is a **dlopen'd module with three exports** — `InitAI`, `OnInit`,
`ProcessAI` — plus a table of function pointers it resolves from the main
executable at load time (`src/ai/common/ai_common.h`, `LoadFunction` via
`dlsym(RTLD_DEFAULT, ...)`). Once per AI tick the engine calls `ProcessAI()`;
inside that call the module only **proposes** projects with `TryUnit` /
`TryUpgrade` and reads state with `Get*` queries; the engine evaluates and
executes the proposals right after the call returns:

```
main loop (main.cpp): every 8th internal tick tmtmt%8==N services CITY[N]
  |
  v
City::ProcessCreation()  (megapolis.cpp) - per nation, if AI_Enabled
  |  SmartGamer, HandleBrigades, TakeNewMine, UpgradeMines, SendAgressors,
  |  HandleDefending ...   <- the ENGINE's military/economy automation
  |  EnumProp()            <- computes each project's affordability percent
  v
City::FindNeedProject()   (megapolis.cpp)
  sets CCIT/CNAT/CurAINation, AiIsRunNow=true
  calls Nation::ProcessAIinDLL()  ->  the module's exported ProcessAI()
      module reads GetMoney/GetUnits/UpgIsDone/...   (current nation only)
      module proposes TryUnit(&barack, 3, 100, 100) / TryUpgrade(&AKA01, ...)
      each proposal is gated by: Max > owned count, rando() < Probability,
      CostPercent >= project percent (cost*100/stock, worst resource)
  AiIsRunNow=false, returns NBestProjects (the queued BestProj[] indices)
  |
  v
same function then executes BestProj[]: upgrades start immediately
(PerformUpgrade), units queue at the producer (ProduceByAI), ONE building
at a time is placed (PresentProject / FindAppropriateBuildingPlace)
```

There is **no utility function to maximise**. The only automatic "score" is
affordability: the engine computes, per project, the worst `cost[res]*100 /
stock[res]` over the 8 resources (`Branch::GetMonsterCostPercent` /
`GetUpgradeCostPercent`) and a proposal passes only if the module's
`CostPercent` argument is >= that. Everything else — build order, unit
counts, research order, when to attack — is hand-written thresholds in the
module (`GetMoney(FOOD) > 700`, `UpgIsRun(&KUZ03)`, `GetPeaceTimeLeft() < 3`,
...).

The module rules **economy, production and research only**. The military —
brigade formation, raids, defence, mine capture, naval battles — is engine
code (`City::RegisterNewUnit`, `SmartGamer`, `SendAgressors`,
`HandleDefending`, `TakeNewMine`, `UpgradeMines`, `Brigade::MakeBattle` in
brigade.cpp), steered indirectly by what the module produces and by
`SetDefSettings` / `SetDefenseState` / `SetUpgradeLock`.

## 1. The two halves: module and engine API

Each nation gets its own shared library, `gamedata/AI/<Name>.so`, built from
`src/ai/<Nation>/dllmain.cpp` (see `docs/AI.md` for the porting history; the
name must match `TEXT\AI.TXT` packed in `all.gsc`). The module includes
`src/ai/common/ai_common.h`, which:

* shims the Windows bits (`HINSTANCE`, `__declspec`) for Linux,
* declares the whole AI API as global function pointers in
  `src/ai/common/typedefs.h` — **this file is the authoritative per-function
  reference**, every pointer documented inline,
* provides `LoadFunction(name)` = `dlsym(RTLD_DEFAULT, name)` (the game is
  linked `-rdynamic`, so its `extern "C" DLLEXPORT` functions in
  `activescenary.cpp` are resolvable), and the `LOAD_FUNCTION(x)` macro.

Loading: the engine dlopens the module, the module's constructor
(`AI_OnLoad`, an `__attribute__((constructor))` standing in for `DllMain`)
runs `LOAD_FUNCTION(...)` for every API function (a missing export logs
`AI module: game does not export function: ...` to stderr) and then calls
`OnInit()` -> `InitAI()`, where the module registers its unit/upgrade names
and sets its constants. Later the engine resolves the module's exports
(`ProcessAI`, activescenary.cpp: `NT->ProcessAIinDLL = GetProcAddress(...,
"ProcessAI")`); if the module failed to load, `ProcessAIinDLL` stays null and
`City::FindNeedProject` falls back to the old built-in `AI_Req`/`AI_Cmd`
script tables — or, with no module at all, the city logic simply never
engages.

The engine-side context: the API functions are **legal only during the AI
tick** (`AiIsRunNow != 0`); calling them outside pops `AI_Error` with a
backtrace. During the tick the engine has set `CCIT` (the city being
serviced), `CNAT` (its nation), `CurAINation` — so every `Get*`/`Set*`/`Try*`
applies to the current nation only.

## 2. The tick: who calls what when the AI decides

The chain from the main loop to the module:

1. `main.cpp` game loop: `tmtmt % 8 == N` services `CITY[N].ProcessCreation()`
   — each nation's city is processed once per 8 internal ticks.
2. `City::ProcessCreation()` (megapolis.cpp): refreshes units/abilities, runs
   brigades, then for AI nations (`Nat->AI_Enabled`, `CenterFound`) runs the
   engine automation — `SmartGamer` (army creation from the aggressor pool),
   `HandleConstructions`, `HandleFields`, `HandleBrigades`, `TakeNewMine`
   (if `AI_MINES_CPTURE`), `HandleIdeas`, `UpgradeMines`, `SendAgressors`,
   `HandleDefending` — then `EnumProp()` and `FindNeedProject()`.
3. `City::FindNeedProject()`: zeroes `NBestProjects`, sets
   `AiIsRunNow = true; CCIT = this; CNAT = Nat; CurAINation = Nat->NNUM;`
   and calls `NT->ProcessAIinDLL()` — the module's `ProcessAI()`. On return
   it hands back the queued `BestProj[]` indices.
4. Back in `ProcessCreation`, each `BestProj[k]` is executed: upgrade
   projects (`PKind == 1`) call `POB->PerformUpgrade(...)` immediately;
   non-building units call `ProduceByAI(...)` (queued at the producer);
   buildings go through the one-at-a-time `PresentProject` placement
   (`FindAppropriateBuildingPlace`, with the house/farm special case, then
   `CreateTeamOfPeasants` + `BuildWithSelected`).

So "the AI makes a decision" happens exactly inside `ProcessAI()`: the module
reads the world through the `Get*` functions and expresses decisions as
`Try*` proposals, which the engine has already priced in `EnumProp()` into
`CCIT->Prop[].Percent`.

## 3. The API surface: what functions there are to call

All declared in `src/ai/common/typedefs.h` with per-function docs; resolved in
each module's `AI_OnLoad`. Grouped by purpose:

| Group | Functions | Purpose |
|---|---|---|
| Registration (once, in `InitAI`) | `RegisterUnitType`, `RegisterUpgrade`, `SafeRegisterUpgrade` | bind a `GAMEOBJ` handle to an NDS name, e.g. `"Pikiner_evro(en)"`, `"AKA08EN"` |
| Role binding (once) | `AssignPeasant`, `AssignMine`, `AssignHouse`, `AssignWall`, `AssignFormUnit` | tell the engine which registered object plays each role (worker, mine, house, wall, pike for officer formations) |
| Proposals (every tick) | `TryUnit(GAMEOBJ*, Max, CostPercent, Probability)`, `TryUpgrade(GAMEOBJ*, CostPercent, Probability)` | the only way to make things happen — queue a unit/building or start a research |
| Economy shape (every tick) | `SetMineBalanse(N, pairs)`, `SetPDistribution(food, wood, stone)`, `SetMinesBuildingRules`, `SetMinesUpgradeRules`, `AssignAmountOfMineUpgrades`, `AssignMineUpgrade` | peasant split between farms and mines; which mine to capture/build next; which mines to upgrade |
| Economy knobs (once) | `SET_MINE_CAPTURE_RADIUS`, `SET_MINE_UPGRADE1_RADIUS`, `SET_MINE_UPGRADE2_RADIUS`, `SET_DEFAULT_MAX_WORKERS`, `SET_MIN_PEASANT_BRIGADE` | radii (game cells) for mine capture/upgrade, free-peasant cap, minimum peasants before a capture brigade is detached |
| Market | `AI_Torg(sell, buy, amount)`, `GetTorgResult(sell, buy, amount)` | trade now / preview the yield |
| Queries | `GetMoney(res)`, `GetUnits`, `GetReadyUnits`, `UpgIsDone`, `UpgIsRun`, `FieldExist`, `GetDifficulty`, `GetStartRes`, `GetLandType`, `GetAINation`, `GetExtraction(res)`, `GetUnitCost`, `GetUpgradeCost`, `GetMaxPeasantsInMines`, `GetUnitsByUsage`, `GetRandomIndex`, `GetMaxPeaceTime`, `GetPeaceTimeLeft` | read the current nation's stockpiles, counts (incl. unfinished), research state, map/game settings |
| Scratch state | `SetAIRegister(i, v)`, `GetAIRegister(i)` | 32 per-nation int slots kept for the whole game ("did this once" flags) |
| Military levers | `SetDefSettings(maxGuards, absMaxGuards)`, `SetDefenseState(state)`, `SetUpgradeLock(res, val)` | cap the city guard pools; switch brigade composition to defensive (`DefenceStage`); forbid expensive upgrades in a resource (hoarding) |

Resource ids are `WOOD 0, GOLD 1, STONE 2, FOOD 3, IRON 4, COAL 5`
(`ai_common.h`); land types `LT_LAND 0, LT_MEDITERRANEAN 1, LT_PENINSULAS 2,
LT_ISLANDS 3, LT_CONTINENTS 4, LT_CONTINENT 5`.

## 4. How a decision is made: proposals, not orders

`TryUnit(&barack, 3, 100, 100)` does not build a barracks; it *proposes* one.
The engine-side `TryUnit` (activescenary.cpp) applies, in order:

* guard: `AiIsRunNow`, `UnitType->Type == 'UTYP'`, `Probability <= 100`,
  fewer than 127 queued projects (`NBestProjects >= 127` drops silently);
* `CannonState == 1` blocks artillery/tower proposals;
* `Max > CCIT->UnitAmount[UID]` — the count includes unfinished units, so
  `Max` is the total target, not the queue length;
* the dice: `rando() < PERCONV[Probability]` where `PERCONV[p] =
  32768*p/100`;
* affordability: `CostPercent >= PRP->Percent`, where `Percent` was computed
  by `EnumProp()` via `Branch::GetMonsterCostPercent` — the **worst**
  `cost[res]*100/stock[res]` over the 8 resources (min 1; 101 if the project
  is free);
* placement: if the city already has a `PresentProject` and the proposal is
  a building, it is dropped (one building per city per tick; a failed
  placement is silently retried next tick); unit proposals queue freely.

`TryUpgrade` has the same dice and affordability rule (via
`Branch::GetUpgradeCostPercent`) but a successful proposal starts the
research **immediately** at its producer (`PerformUpgrade`) — no queue, no
one-at-a-time limit. The hoarding trick: `SetUpgradeLock(GOLD, 1)` makes
`GetUpgradeCostPercent` report 1000 for every upgrade costing >600 gold, i.e.
"unaffordable", so `TryUpgrade` skips them while the nation saves for the
XVIII-century research (`MAIN`).

So the decision procedure per tick is: read state (`GetMoney`, `GetUnits`,
`UpgIsRun`...), then walk a hand-written ladder of gates, each gate a
`TryUnit`/`TryUpgrade` with its own `Max`/`CostPercent`/`Probability`. The
three arguments are the whole "AI tuning surface": `Max` = army size target,
`CostPercent` = how much of the stockpile may be spent (1..20 = only when
nearly free, ~100 = whenever affordable, >100 = even above stock, e.g. 232
for the peasant push), `Probability` = per-tick chance, which paces
production when several proposals compete.

## 5. Is there an evaluation function to maximise?

**No.** There is no economy-strength or military-strength score anywhere in
the engine or the modules. The only automatic evaluation is the affordability
percent described above (`cost*100/stock`, worst resource, plus the
`SetUpgradeLock` 1000-inflation and the probability dice). Strategy quality
comes entirely from the constants and thresholds the module author writes:

* resource thresholds — Ukraine: `foodLast > 700` gates musketeer
  production, `goldLast > 1000` gates grenadiers;
* research gates — `if (UpgIsRun(&melnica_GETRES)) { TryUnit(&strelec, ...) }`,
  `if (UpgIsDone(&KUZ03)) { ... mid-game upgrades ... }`;
* count gates — England's water handler: `if (elite > 300)` where
  `elite = GetUnits(&konni_ricar) + GetUnits(&mameluk) + GetUnits(&pikiner)`,
  then mass musketeers `TryUnit(&pehota, 500, 10, 10)`; fleet strength
  proxy `fleet = (yachts >> 2) + fregats + linkors*4` paces frigate targets;
* time gates — `GetPeaceTimeLeft() < 5` unlocks towers, `< 3` unlocks combat
  upgrades and the diplomatic corps (the European land pipeline);
* hoarding — `LandMarket` computes `GetUpgradeCost(NAT, &MAIN, res)` and
  tops up via `AiHoardFor` (a market top-up helper in european_land.h using
  `GetTorgResult`); England's water handler instead raises
  `SetUpgradeLock(GOLD, 1)` under stockpile conditions.

If you design a new AI and want a "maximise X" objective, you must encode X
yourself as thresholds over the `Get*` queries — the engine will never
compute it for you.

## 6. Economy or also military?

The nation module rules **economy, production and research**. The military is
run by engine code in the same tick, and the module influences it only
indirectly:

* **What exists to fight with.** Every finished combat unit is sorted by
  `City::RegisterNewUnit` (megapolis.cpp) into pools: peasants -> `Free`;
  officers/drummers -> `SearchArmy`; transports -> diversion brigades;
  artillery/ranged/mounted (`PushkaID`, `StrelokID`, `FastHorseID`, ...) ->
  `Agressors` with `DoNotCall`; other melee -> `FreeArmy` / `Guards` /
  `Defenders` / `Agressors` by dice, with the `MaxGuards`/`AbsMaxGuards`
  caps the module set via `SetDefSettings`. Ships (`LockType`,
  `AI_WATER_BATTLE`) get their own water brigades (`MakeShipBattle`).
* **What the engine does with the pools.** `SmartGamer` (the engine's
  "main strategical brain") forms defensive armies from `Agressors`
  (`CheckMinArmyCreationAbility` -> `CreateMinimalArmyFromBrigade` ->
  `Parad` + `MakeBattle`), sends settler diversions (`SHIP_DIVERSIA`), fast
  horse diversions (`CheckDiversants`), and scans enemy army info
  (`GAINF`) for danger to buildings (`AI_TOWN_DEFENCE` -> `AddDefending`).
  `SendAgressors` dispatches small raids while `NAgressors < 10` (starts at
  5, then saturates at 15000 — so only the first couple of raids per game).
  `HandleDefending` posts `Defenders`/`Guards` to threatened points.
  `Brigade::MakeBattle` (brigade.cpp) is the actual attack order.
* **Economy automation the module only configures.** `TakeNewMine` detaches
  a peasant brigade to capture a free mine (needs `FreePS >=
  SET_MIN_PEASANT_BRIGADE`, wood+stone >= 100, mine within
  `SET_MINE_CAPTURE_RADIUS`); `UpgradeMines` deepens mines within
  `SET_MINE_UPGRADE1/2_RADIUS` using the `AssignMineUpgrade` tiers;
  `City::CalculateBalance` consumes the `SetMineBalanse` pair table
  (interpolating between `{totalPeasants, wantedOnFoodWoodStone}` pairs —
  the engine scan ignores N and walks to the mandatory `{4000, x}`
  terminator — and sends the rest to gold/iron/coal mines).
* **The module's direct military levers** are only: the unit mix and counts
  it produces, `SetDefSettings` (guard caps), `SetDefenseState` (sets
  `City::DefenceStage`, which makes army formation use the defensive
  `DefBriMin` thresholds instead of `BriMin`), `SetUpgradeLock`, and the
  European officers tail (`TryUnit(&oficer, pikinerCount/36, ...)` +
  drummer, so pike blocks get their officers).

## 7. Designing a new national AI: how strategy is chosen

There is no planner to configure — you write a per-tick decision ladder. The
practical recipe (what the existing modules all do):

1. **Dispatch on the map.** `ProcessAI` reads `GetLandType()` (and
   `GetStartRes()`, `GetDifficulty()`) and switches handlers: land,
   Mediterranean, peninsulas/islands/continents (water), continent-large.
   England does this in the shared `EuropeanProcessAI` (european_land.h);
   Ukraine in its own `ProcessAI` (land 0 vs 1..4, land 5+ ignored).
2. **Build order as a nested ladder.** Each tick re-issue the ladder:
   `TryUnit(&melnica, 1, 100, 100); if (GetUnits(&melnica)) { TryUnit(&center,
   2, 100, 100); if (GetUnits(&center)) { ... } }` — prerequisites are owned
   counts, so the ladder self-paces and one-building-per-tick makes the order
   real.
3. **Unit targets and pacing.** `Max` per unit type = army size;
   `Probability` spreads competing proposals; low `CostPercent` (10..30)
   makes elite units wait until they are cheap relative to stock.
4. **Research order.** `TryUpgrade` chains gated on `UpgIsDone`/`UpgIsRun`
   of earlier researches; the big era research (`MAIN` = XVIII century,
   e.g. `MAINEN` for England) is hoarded for via `SetUpgradeLock` +
   `GetUpgradeCost`/`AiHoardFor`.
5. **Economy shaping.** `SetMineBalanse` pair table + `SetPDistribution`
   per phase (no fields yet -> everyone on food `(0,5,5)`; late game ->
   balanced `(9,4,4)`), mine capture/upgrade rules, radii and worker caps
   in `InitAI`.
6. **Market policy** as a tail of `AI_Torg` calls gated on
   `GetReadyUnits(&rinok)` (market built): sell surplus stone/wood for gold,
   emergency food buys when `food < 500 && FieldExist()`.
7. **Timing.** Use `GetPeaceTimeLeft()`/`GetMaxPeaceTime()` to switch from
   boom to war (towers at <5, combat upgrades + diplomacy at <3 in the
   European land pipeline).
8. **Per-game variety.** `GetRandomIndex()` once in `InitAI` to pick unit
   variants per game (England: `Konni_Ricar(en)` mounted / `Grenader(en)`
   18th-c. shooter chains via `RegisterPickChains`).
9. **Register everything that might not exist with `SafeRegisterUpgrade`**
   (e.g. England drops `AKA09EN`/`AKA15EN`/`AKA28EN`), because an unbound
   `GAMEOBJ` pops an error dialog on use. Names must match the nation's NDS
   byte-exactly, and remember `ENABLE UNITS GRP` gating: a `TryUnit` of a
   locked building (18th-c. barracks behind `MAIN`) is a harmless no-op.

Pitfalls that have bitten real ports (see `docs/AI.md`): the mine-balance
tables are pair-based and the engine scan ignores the count N — always keep
the `{4000, x}` terminator; the module `.so` file name must match
`TEXT\AI.TXT` exactly (case-sensitive on Linux, and it is the DLL name, e.g.
`Vengria`, not the nation name) or the load fails silently per nation; the
one-building-per-tick rule means a wrong gate can stall the whole build
order; `TryUnit` of a building while `PresentProject` is set is dropped.

## 8. Two examples: Ukraine and England

### Ukraine — the old standalone family (`src/ai/Ukraine/dllmain.cpp`)

Ukraine predates the shared skeleton: no XVIII century at all (no `MAIN`),
no `SetMinesBuildingRules`/`SetMinesUpgradeRules`, its own two handlers.

* `InitAI` -> `RegisterUkraine()` registers ~23 units/buildings and ~50
  upgrades by NDS name (`Kozacki_Strelec(UA)`, `Kres_ukranian(UA)`,
  `Strelcovaia_Izba(UA)`, `AKA01UA`...), then sets mine radii 290/260/130,
  `SET_DEFAULT_MAX_WORKERS(300)`, `SET_MIN_PEASANT_BRIGADE(5)`,
  `SetPDistribution(9,4,4)`, 6 mine-upgrade tiers (`shahta(UA)INSIDE`...), and
  the role bindings (`AssignPeasant("Kres_ukranian(UA)")`,
  `AssignMine("shahta(UA)")`, `AssignHouse("Ukrainska_hata(UA)")`).
* `ProcessAI` reads `GetLandType()`: `LT_LAND` -> `ProcessLandAI`, land 1..4
  -> `ProcessNonLandAI(land)` (adds ports, mortar boats on Mediterranean,
  fishing boats, ferry, galleys), land 5+ does nothing.
* `ProcessLandAI` is the textbook ladder: `SetMineBalanse(19, MINE_BALANCE)`
  (a 19-pair table ending in `{4000, 300}`), snapshot all six resources with
  `GetMoney`, then mill -> town hall -> blacksmith -> barracks x2..5 ->
  stables x2..5 -> artillery depot -> storehouse, market + academy once
  food/gold allow. Mill upgrades `Melnica_rus(UA)GETRES/GETRES2` gate unit
  production: `UpgIsRun(&melnica_GETRES)` -> strelec up to 500 (food>700),
  peasants up to 1000 (food>200/300), pikemen up to 600, grenadiers up to 70
  (gold>1000). `UpgIsDone(&KUZ03)` unlocks the mid-game block: academy
  upgrades, artillery BUILD chains, and only then cannons/mortars
  (`UpgIsDone(&pushka_BUILD5)` -> `TryUnit(&pushka, 10/20, 20, 50)`), plus
  the pike/strelec ATTACK/SHIELD chains. The tail is the market: sell
  gold->food while the second mill upgrade is pending, stone/wood->gold on
  surplus, and a cascade of emergency food buys when `foodLast < 500 &&
  FieldExist()`. Finally `SetPDistribution` by phase: no fields `(0,5,5)`,
  few peasants `(10,2,2)`, pre-GETRES2 `(10,4,2)`, else `(7,6,3)`.

### England — the shared European skeleton (`src/ai/England/dllmain.cpp` + `src/ai/common/european_land.h`)

England's `ProcessAI` is just `EuropeanProcessAI()` (european_land.h), the
pipeline shared by 17 nations:

* Prologue `AiTickPrologue()` caches `GetAINation/GetLandType/GetStartRes/
  GetDifficulty` into module globals, sets the land mine balance and the
  27-pair `SetMinesBuildingRules` (gold mines first; rule [1] raised to 4
  on the lowest start resources).
* Dispatch: `LT_LAND`/`LT_CONTINENT` -> `LandPipeline` (or
  `RichLandPipeline` when `GetStartRes()==3`, with gold-first mine rules);
  `LT_MEDITERRANEAN` -> `ProcessMiddleSeaAI`; peninsulas/islands/continents
  -> re-arm mine radii to 290/290/290, water balance table, then England's
  own `ProcessWaterAI()`.
* `LandPipeline` is the staged strategy: `LandDefenseStance` (defense mode
  until pike ATTACK4+SHIELD4 are done), `LandArmy` (peasants, pikes, then
  musketeers once pikes >= 140, mounted, 18th-c. dragoons, the per-game
  shooter pick), `LandCannons`, `LandTowers` (peace < 5), `LandBuildings`
  (the building ladder), `LandUpgrades` (incl. `TryUpgrade(&MAIN, 98, 100)` —
  England's `MAIN` is `MAINEN`, the XVIII century that unlocks
  `Kazarma_BR(en)`), `LandCombatUpgrades` (peace < 3: all unit upgrade
  chains), `LandMarket` (start-res one-shot trades via `SetAIRegister`,
  hoarding coal for AKA24 and food/gold/iron for MAIN via `AiHoardFor`,
  surplus->gold), `LandDistribution` (peasant split adjusted by what
  research is running and its `GetUpgradeCost`), `ApplyMineUpgradeRules`.
  After the switch, the officers tail adds `oficer`/`barabanshik` at
  `pikinerCount/36` on land maps.
* England's `ProcessWaterAI` (in its dllmain.cpp) is the naval strategy:
  `SetDefSettings(300, 400)`, fleet proxies from `GetUnitsByUsage`
  (galleys grow `+2` per tick, frigates scale with the `fleet` proxy,
  battleships `linkor` only when `GetMaxPeasantsInMines() == 95`), ports up
  to 5, `TryUpgrade(&MAIN, 95, 100)` unconditionally, and gold hoarding
  for the XVIII century via `SetUpgradeLock(GOLD, 1)` under stockpile
  thresholds keyed on the mine-cap tier (10/18/28/40/55/95).
* England's `InitAI` -> `RegisterEngland()` is the registration block: the
  per-game pick flag (`GetRandomIndex() & 0x80` into a per-nation array),
  `SafeRegisterUpgrade` for the dropped names, `RegisterPickChains(
  "Konni_Ricar(en)", "Grenader(en)")` (12+12 upgrade chains built by
  sprintf'ing `ATTACK`/`ATTACK3`.../`SHIELD7` suffixes), all unit/upgrade
  registrations, then radii 290/90/90, `SET_DEFAULT_MAX_WORKERS(300)`,
  `SET_MIN_PEASANT_BRIGADE(5)`, `SetPDistribution(9,4,4)`, the 6 mine
  tiers, and `AssignPeasant/AssignMine/AssignHouse/AssignWall/AssignFormUnit`.

The contrast is the lesson: Ukraine hand-rolls everything per land type and
has no era research; England inherits a staged land pipeline (economy ->
upgrades -> war on the peace timer), a Mediterranean handler, a water handler
with fleet scaling and battleship unlock, and the shared mine tables — the
only per-nation content being names, picks, and a few constants.

## 9. Difficulty levels: engine-owned, with one per-nation hook

Difficulty is **primarily engine state, not per-nation script**. The setup
dialog combo `DIFF` (`@RMID_EASY` / `@RMID_NORMAL` / `@RMID_HARD` /
`@RMID_VERYHARD`, interface.cpp) stores its line 0..3 into `CITY[i].Difficulty`
for every nation at game start; scenarios can override per nation via
`StartAI(Nat, Name, Land, Money, ResOnMap, Difficulty)`; the byte is saved in
the `.sav` (loadsave.cpp) and restored on load. `gamedata/test.sav`, for
example, is a 4-player "Very hard" (= 3) game.

Engine effects of `City::Difficulty`:

* **Production speed** — the produce order (`build.cpp`) rescales its stage
  count: `NStages = ProduceStages * DiffP[Difficulty] >> 8` for AI nations,
  skipped at difficulty 3. `DiffP` comes from `Ai\df.dat` = `3000 2048 512
  256`, i.e. easy AI produces ~11x slower, normal ~8x, hard ~2x, very hard
  at full speed.
* **Capture resolution** — `newmon.cpp`: `Easy = Difficulty < 3`; when an
  AI-owned object is captured, peasant-type buildings are destroyed unless
  the difficulty is very hard.
* **Raids** — `CheckDiversants` (megapolis.cpp) returns immediately on
  difficulty 0: no fast-horse diversion raids on easy.

Per-nation modules can *read* it through the `GetDifficulty()` API, but almost
none do: the European skeleton caches it in `AI_DIFF` each tick and never uses
it; the only real module-side use is in `ProcessMiddleSeaAI` (european_land.h):
`if (GetDifficulty() > 0) TryUnit(&kozak, 15, 20, 40);` — mercenary cavalry on
Mediterranean maps only above easy. Turkey, Algeria and Ukraine read
`GetDifficulty()` into locals they never use. So when designing a national AI,
treat difficulty as something the engine already handles; use `GetDifficulty()`
only if you want a behavior switch of your own.

## 10. Where the military side lives

The engine-side half of the story — per-unit order chains, the city's brigade
pools (`Free`/`OnField`/`OnWood`/`OnStone`/`Builders`/`Agressors`/...), and
the frozen-workers trap found in `gamedata/test.sav` — is chapter
[070_brigade_ai.md](070_brigade_ai.md).
