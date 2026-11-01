#pragma once

// ============================================================================
// The game's AI API, resolved at dlopen() time from the main executable.
//
// How the AI tick works: the engine calls the module's exported ProcessAI()
// once per AI tick for the nation being serviced. Inside that call, the
// TryUnit()/TryUpgrade() calls are *proposals*; the engine evaluates every
// proposal the same tick:
//   - upgrades start immediately at their producing building;
//   - units are queued at their producing building;
//   - buildings are placed ONE AT A TIME (one "present project" per city);
//     further building proposals in the same tick are dropped, and a
//     proposal whose placement fails (no space / unreachable shore for a
//     port) is silently retried on a later tick.
// Everything else (Get*/Set*) reads or configures the engine state for the
// CURRENT nation only (the engine sets it before calling ProcessAI).
//
// All functions resolve via dlsym(RTLD_DEFAULT, "<name>"); see LOAD_FUNCTION
// in dllmain.cpp. A name the game does not export logs to stderr and leaves
// the pointer null (crash on call) - the set below matches the exports of
// activescenary.cpp.
// ============================================================================

struct GAMEOBJ;

// Binds the GAMEOBJ to the nation's unit/building by its NDS name
// (e.g. "Pikiner_evro(au)"). Must be called (InitAI) before the object is
// used with TryUnit/GetUnits. Unknown name: returns false silently and the
// GAMEOBJ stays unbound (its later use pops an error dialog at runtime).
typedef bool tpRegisterUnitType(GAMEOBJ* GOBJ, const char* Name);
tpRegisterUnitType* RegisterUnitType = nullptr;

// Same for a research/upgrade by its NDS name (e.g. "AKA08RU", no parens).
// Unknown name: returns false silently, same consequences.
typedef bool tpRegisterUpgrade(GAMEOBJ* GOBJ, const char* Name);
tpRegisterUpgrade* RegisterUpgrade = nullptr;

// Like RegisterUpgrade, but intended for names that may legitimately not
// exist in the nation's NDS: returns false without side effects. (Some
// original AIs register "maybe existing" upgrade names through it.)
typedef bool tpSafeRegisterUpgrade(GAMEOBJ* GOBJ, const char* Name);
tpSafeRegisterUpgrade* SafeRegisterUpgrade = nullptr;

// Market trade: sell SellAmount of SellRes for BuyRes (resource ids below).
// Executed immediately (PerformTorg). GetTorgResult() previews the buy yield
// without trading.
typedef void tpAI_Torg(unsigned char SellRes, unsigned char BuyRes, int SellAmount);
tpAI_Torg* AI_Torg = nullptr;

// Propose building/producing one more of the registered unit or building.
//   Max          - propose only while the nation owns FEWER than Max of it;
//                  the count includes unfinished (under-construction) ones
//                  (that is GetUnits below), so Max is the total target.
//   CostPercent  - the largest share of a CURRENT resource stockpile the
//                  AI accepts to spend on this. The engine computes, per
//                  resource, cost[res]*100/stock[res] and takes the worst;
//                  the proposal passes only if CostPercent >= that value.
//                  1..20   = "only when it is nearly free vs my stock"
//                  ~100    = "whenever I can pay it from stock"
//                  >100    = "even if it exceeds my stock" (e.g. 232 for
//                            the peasant push; the engine lets it through)
//   Probability  - per-tick chance (0..100) that the proposal is made at
//                  all. High = competes every tick; low = sporadic, which
//                  spreads production and leaves room for other projects.
// Only one BUILDING project is placed per city at a time; unit proposals
// queue freely. NDS buildability still applies: a building locked behind a
// research (ENABLE UNITS GRP) is never placed until that research is done.
typedef bool tpTryUnit(GAMEOBJ* UnitType, int Max, unsigned char CostPercent, unsigned char Probability);
tpTryUnit* TryUnit = nullptr;

// Propose starting a research. Same CostPercent/Probability rules as
// TryUnit, but a successful proposal starts the research IMMEDIATELY at
// its producing building (no queue, no one-at-a-time limit). The engine
// inflates the cost percent to 1000 (i.e. unaffordable) for a resource
// locked with SetUpgradeLock when the upgrade costs >600 of it - that is
// how the AI hoards gold for the XVIII-century research.
typedef bool tpTryUpgrade(GAMEOBJ* Upgrade, int CostPercent, int Probability);
tpTryUpgrade* TryUpgrade = nullptr;

// Mine-vs-food peasant balance: N PAIRS of
//   {total peasants, peasants wanted on food+wood+stone}.
// The engine interpolates the second value between neighboring pairs and
// sends the remainder to gold/iron/coal mines (City::CalculateBalance).
// The engine scan ignores N and walks while pair[0] <= peasant count, so
// the last pair MUST be a {4000, ~300..400} terminator. Re-issued every
// tick by the original AIs (SetMinesBuildingRules/SetMinesUpgradeRules
// too) - they are plain per-tick state, not one-time setup.
typedef void tpSetMineBalanse(int N, unsigned short* Bal);
tpSetMineBalanse* SetMineBalanse = nullptr;

// Relative weights for peasants working food / wood / stone (SetPDistribution
// sets the same numbers; this is the "who works where" ratio, the remainder
// of the balance above goes to mines). Ratios, not percentages (e.g. 9,4,4).
typedef void tpSetPDistribution(int OnFood, int OnWood, int OnStone);
tpSetPDistribution* SetPDistribution = nullptr;

// How many mine-deepening researches exist (originals: 6).
typedef void tpAssignAmountOfMineUpgrades(int MU);
tpAssignAmountOfMineUpgrades* AssignAmountOfMineUpgrades = nullptr;

// The i-th mine-deepening research: NDS upgrade name + its cost in percent.
typedef bool tpAssignMineUpgrade(unsigned short U, const char* Str, unsigned val);
tpAssignMineUpgrade* AssignMineUpgrade = nullptr;

// Role bindings (once, in InitAI): which registered object serves as
//   AssignPeasant  - the peasant/worker unit
//   AssignMine     - the mine building
//   AssignHouse    - the population-support house
//   AssignWall     - the wall the AI builds
//   AssignFormUnit - the unit that forms up with officers (the pike)
typedef void tpAssignMine(const char* Name);
tpAssignMine* AssignMine = nullptr;

typedef void tpAssignPeasant(const char* Name);
tpAssignPeasant* AssignPeasant = nullptr;

typedef void tpAssignHouse(const char* Name);
tpAssignHouse* AssignHouse = nullptr;

typedef void tpAssignWall(const char* Name);
tpAssignWall* AssignWall = nullptr;

typedef void tpAssignFormUnit(const char* Name);
tpAssignFormUnit* AssignFormUnit = nullptr;

// How far (game cells) from its town center the AI captures free mines /
// builds the first mine upgrade / builds further mine upgrades.
typedef void tpSET_MINE_CAPTURE_RADIUS(int x);
tpSET_MINE_CAPTURE_RADIUS* SET_MINE_CAPTURE_RADIUS = nullptr;

typedef void tpSET_MINE_UPGRADE1_RADIUS(int x);
tpSET_MINE_UPGRADE1_RADIUS* SET_MINE_UPGRADE1_RADIUS = nullptr;

typedef void tpSET_MINE_UPGRADE2_RADIUS(int x);
tpSET_MINE_UPGRADE2_RADIUS* SET_MINE_UPGRADE2_RADIUS = nullptr;

// Cap on the number of FREE (unassigned) peasants the AI keeps: peasant
// production proposals stop while free peasants exceed this.
typedef void tpSET_DEFAULT_MAX_WORKERS(int x);
tpSET_DEFAULT_MAX_WORKERS* SET_DEFAULT_MAX_WORKERS = nullptr;

// Minimum free peasants before the AI detaches a brigade to capture a new
// mine (it waits for spare workers instead of stripping the economy).
typedef void tpSET_MIN_PEASANT_BRIGADE(int x);
tpSET_MIN_PEASANT_BRIGADE* SET_MIN_PEASANT_BRIGADE = nullptr;

// Current stockpile of a resource: 0=wood 1=gold 2=stone 3=food 4=iron
// 5=coal. (WOOD/GOLD/... defines in dllmain.cpp use the same order.)
typedef int tpGetMoney(unsigned char id);
tpGetMoney* GetMoney = nullptr;

// Owned count of a registered unit/building, INCLUDING unfinished ones.
typedef int tpGetUnits(GAMEOBJ* UnitType);
tpGetUnits* GetUnits = nullptr;

// Owned count of a registered unit/building with construction COMPLETE.
typedef int tpGetReadyUnits(GAMEOBJ* UnitType);
tpGetReadyUnits* GetReadyUnits = nullptr;

// Research fully completed.
typedef bool tpUpgIsDone(GAMEOBJ* Upgrade);
tpUpgIsDone* UpgIsDone = nullptr;

// Research done OR in progress (engine: Done || IsDoing - NOT strictly
// "running"; the original AIs use it as "this research is underway").
typedef bool tpUpgIsRun(GAMEOBJ* Upgrade);
tpUpgIsRun* UpgIsRun = nullptr;

// True once the AI has a sown field anywhere (its mills make fields).
typedef bool tpFieldExist();
tpFieldExist* FieldExist = nullptr;

// Game settings: difficulty (0..3), starting-resources option, land type
// (0=Land, 1=Mediterranean, 2=Peninsulas, 3=Islands, 4=Continents,
// 5=Continent - see LT_ defines in dllmain.cpp).
typedef int tpGetDifficulty();
tpGetDifficulty* GetDifficulty = nullptr;

typedef int tpGetStartRes();
tpGetStartRes* GetStartRes = nullptr;

typedef int tpGetLandType();
tpGetLandType* GetLandType = nullptr;

// 32 scratch int slots per AI nation, kept for the whole game - the
// originals use them as "did this once" flags (e.g. fire a one-time trade).
typedef void tpSetAIRegister(int Reg, int Val);
tpSetAIRegister* SetAIRegister = nullptr;

typedef int tpGetAIRegister(int Reg);
tpGetAIRegister* GetAIRegister = nullptr;

// Defender pool caps (city "guards"): new combat units are sorted into
// aggressor/defender/guard pools (RegisterNewUnit, megapolis.cpp); these
// cap the guard pools (AbsMaxGuards is the hard limit).
typedef void tpSetDefSettings(int MaxGuards, int AbsMaxGuards);
tpSetDefSettings* SetDefSettings = nullptr;

// Nonzero = "defense mode": the engine switches the brigade-composition
// thresholds to their defensive values when forming armies. The water-map
// handlers clear it every tick; some land paths set it while hoarding.
typedef void tpSetDefenseState(unsigned char State);
tpSetDefenseState* SetDefenseState = nullptr;

// Temporarily forbid upgrades costing >600 of Res (Val=1) / reallow
// (Val=0). Used to hoard gold for the XVIII-century research: a locked
// resource makes every such upgrade "unaffordable" (cost percent 1000).
// Res: 0=wood 1=gold 2=stone 3=food 4=iron 5=coal.
typedef void tpSetUpgradeLock(unsigned char Res, unsigned char Val);
tpSetUpgradeLock* SetUpgradeLock = nullptr;

// Which resource the NEXT captured mine must yield: NElm PAIRS of
// {RSM = gold + (iron<<8) + (coal<<16) counts of owned mines,
//  ResID wanted next: 1=gold 4=iron}. Exact-match scan, last match wins,
// no match = take the nearest mine of any resource.
typedef void tpSetMinesBuildingRules(int* Table, int NElm);
tpSetMinesBuildingRules* SetMinesBuildingRules = nullptr;

// Share of gold/iron/coal mines to keep un-upgraded at upgrade tier 1/2/3:
// 27 ints = MU1{gold,iron,coal}% + MU2{...}% + MU3{...}%.
typedef void tpSetMinesUpgradeRules(int* Table);
tpSetMinesUpgradeRules* SetMinesUpgradeRules = nullptr;

// The largest per-mine worker capacity the AI currently has - it grows
// with the mine-deepening researches (5 -> 10 -> ... -> 95). The original
// water AIs key their late-game behavior on it (battleships at 95).
typedef int tpGetMaxPeasantsInMines();
tpGetMaxPeasantsInMines* GetMaxPeasantsInMines = nullptr;

// Count units of a given newMons->Usage id across a nation:
// 0x19 Fregat, 0x1B Galera, 0x1C Iaxta, 0x1F Parom, 0x0E Linkor, ...
// (full list in mapdiscr.h). Nat is a player slot mapped through
// AssignTBL - the original AIs always pass 0.
typedef int tpGetUnitsByUsage(unsigned char Nat, unsigned char Usage);
tpGetUnitsByUsage* GetUnitsByUsage = nullptr;

// Per-game pseudo-random index (the original AIs use it at InitAI to pick
// per-game unit variants). Deterministic in play/record mode (derived from
// unit coordinates), otherwise a time-seeded counter.
typedef int tpGetRandomIndex();
tpGetRandomIndex* GetRandomIndex = nullptr;
