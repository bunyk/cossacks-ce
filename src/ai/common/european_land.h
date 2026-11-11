#pragma once
// european_land.h - the REAL land-map pipeline and Mediterranean handler of
// the 94208-byte "European" AI DLL family (Russia, Austria, Bavaria + the 13
// pipeline nations + Denmark). Ported 1:1 from the Ghidra decompilation of
// Bavaria.dll; the token streams of Russia.dll and Austria.dll were verified
// identical for every function below (tools/ai_facts.py alignment), so one
// shared body serves all 17 modules.
//
// Include this AFTER objects.h and the module's mine tables in dllmain.cpp.
// Per-nation differences are compile-time macros (define before including):
//   AI_MOUNTED_PICK   - GAMEOBJ of the per-game mounted pick (default konni_ricar)
//   AI_SHOOTER_PICK   - GAMEOBJ of the per-game 18th c. shooter pick (default strelec)
//   AI_SHOOTER_PICK_NONE - define when the original registers no shooter unit
//                         (Hungary): shooter references become no-ops.
//
// Function-to-original map (Bavaria.dll addresses):
//   AiTickPrologue       FUN_10003020   per-tick context + default mine tables
//   EuropeanProcessAI    FUN_10006d20   dispatcher (land-type switch)
//   LandPipeline         FUN_10006bc0   normal land pipeline
//   RichLandPipeline     FUN_10006c20   land pipeline when startRes == 3
//   LandDefenseStance    FUN_100066c0   SetDefenseState toggle
//   LandArmy             FUN_10005dd0   army production (runs twice)
//   LandCannons          FUN_10006710   cannon/mortar projects
//   LandTowers           FUN_10006800   towers + tower PAUSE chain
//   LandBuildings        FUN_10005bb0   building ladder
//   LandBasicBuildings   FUN_10005200   melnica/center/sclad/rinok
//   LandUpgrades         FUN_10005a70   core upgrades incl. TryUpgrade(&MAIN,98,100)
//   LandCombatUpgrades   FUN_10005290   unit upgrade chains (peace < 3 only)
//   LandMarket           FUN_10005fb0   market trades + MAIN hoarding
//   LandDistribution     FUN_100062d0   peasant food/wood/stone split
//   RichAttackToggle     FUN_10006b40   defense toggle for the rich pipeline
//   RichBuildings        FUN_10006930   building ladder for the rich pipeline
//   PikeChain4Done       FUN_10005250   pikiner ATTACK4+SHIELD4 both done
//   ProcessMiddleSeaAI   FUN_100043c0   Mediterranean handler
//   MedPikeChain         FUN_10004210   pike+officer upgrades (food > 1650)
//   MedMusketeerChain    FUN_100042f0   17th c. musketeer upgrades
//   MedCannonBuild       FUN_10004340   cannon/mortar BUILD chain (prob 50)
//   AiHoardFor           FUN_10002f70   top up a resource via market sales
//   RegisterPickChains   InitAI tail    12+12 SafeRegisterUpgrade pick-chain slots

#include <cstdio>

// Per-module hooks defined later in dllmain.cpp.
void ProcessWaterAI();
void ApplyMineUpgradeRules(int land);
void WaterMortiraUpgrades();

#ifndef USAGE_FREGAT
#define USAGE_FREGAT	0x19
#define USAGE_GALERA	0x1B
#define USAGE_IAXTA		0x1C
#endif

#ifndef AI_MOUNTED_PICK
#define AI_MOUNTED_PICK konni_ricar
#endif
#ifndef AI_SHOOTER_PICK
#define AI_SHOOTER_PICK strelec
#endif
#ifdef AI_SHOOTER_PICK_NONE
#define AI_SHOOTER_UNITS 0
#define AI_SHOOTER_TRYUNIT(max, cp, pr) ((void)0)
#else
#define AI_SHOOTER_UNITS GetUnits(&AI_SHOOTER_PICK)
#define AI_SHOOTER_TRYUNIT(max, cp, pr) TryUnit(&AI_SHOOTER_PICK, max, cp, pr)
#endif

// ----- .data tables shared by the whole family (dumped from Bavaria.dll) -----
// Mediterranean mine balance (DAT_1000f030), 19 pairs + {4000,x} terminator.
unsigned short MED_MINE_BALANCE[38] = {
	0, 0, 19, 19, 25, 20, 35, 25, 45, 30, 50, 30, 55, 35, 60, 40, 65, 45, 80, 50,
	105, 60, 125, 65, 150, 70, 175, 75, 200, 80, 225, 80, 250, 80, 500, 100, 4000, 300
};
// Balance used on land maps with the highest start resources
// (DAT_1000f160, SetMineBalanse(3, ...)): 3 pairs; the {4000,45} terminator
// guards the engine's unbounded scan when peasants exceed 1000.
unsigned short RICH_MINE_BALANCE[8] = {
	0, 0, 50, 45, 1000, 45, 4000, 45
};
// Mines-building rules for the highest start resources (DAT_1000f41c,
// 5 pairs): always capture gold mines first.
int START3_BUILD_RULES[10] = {
	0, 1, 1, 1, 2, 1, 3, 1, 4, 1
};

// ----- per-tick/per-game state (the original keeps these in .data slots) -----
int AI_NATION;      // GetAINation(), cached each tick (DAT_10016284)
int AI_LAND;        // GetLandType(), cached each tick (DAT_10015f28)
int AI_STARTRES;    // GetStartRes(), cached each tick (DAT_10013868)
int AI_DIFF;        // GetDifficulty(), cached each tick (_DAT_10016338)
int AI_FREEPROXY;   // peasants minus gold/iron/coal extraction (DAT_10016180)
int aiRandFlag[8];     // per-nation GetRandomIndex()&0x80, set once in InitAI (DAT_10015f68)
int aiMedTradeFlag[8]; // per-nation one-shot market flag (DAT_100165dc)
int aiMedDistMode = 0; // DAT_10016564, never assigned in the DLLs

// ----- per-game pick upgrade chains -----
// The original sprintf's "<pick>"+"ATTACK"... into 12+12 SafeRegisterUpgrade
// slots (order: ATTACK,ATTACK3..ATTACK7,SHIELD,SHIELD3..SHIELD7). Declared in
// each module's objects.h as mounted_pick_chain[12] / shooter_pick_chain[12].
inline void RegisterPickChains(const char* mountedName, const char* shooterName)
{
	static const char* SUFFIX[12] = {
		"ATTACK", "ATTACK3", "ATTACK4", "ATTACK5", "ATTACK6", "ATTACK7",
		"SHIELD", "SHIELD3", "SHIELD4", "SHIELD5", "SHIELD6", "SHIELD7"
	};
	char buf[64];
	for (int i = 0; i < 12; i++)
	{
		sprintf(buf, "%s%s", mountedName, SUFFIX[i]);
		SafeRegisterUpgrade(&mounted_pick_chain[i], buf);
		sprintf(buf, "%s%s", shooterName, SUFFIX[i]);
		SafeRegisterUpgrade(&shooter_pick_chain[i], buf);
	}
}

// ----- helpers -----

// FUN_10005250: pikiner ATTACK4 and SHIELD4 both researched (the original
// checks param == &pikiner first; it is only ever called with &pikiner).
inline bool PikeChain4Done()
{
	return UpgIsDone(&pikiner_ATTACK4) && UpgIsDone(&pikiner_SHIELD4);
}

// FUN_10002f70: top up resource `buy` to at least `want` by selling `sell`
// on the market, in chunks of 100 (at most 15). Returns false when the
// sell stock is insufficient or the yield is negligible.
inline bool AiHoardFor(unsigned char buy, int want, unsigned char sell)
{
	if (GetMoney(buy) >= want)
		return true;
	int deficit = want - GetMoney(buy);
	int yield = GetTorgResult(sell, buy, 1000);
	if (yield <= 0)
		return false;
	int needSell = deficit * 1000 / yield;
	if (GetMoney(sell) <= needSell || needSell <= 999)
		return false;
	int steps = needSell / 15000;
	if (steps > 15)
		steps = 15;
	for (int i = 0; i < steps; i++)
		AI_Torg(sell, buy, 100);
	return true;
}

// ----- land pipeline stages -----

// FUN_100066c0
void LandDefenseStance()
{
	SetDefenseState(1);
	if (PikeChain4Done())
		SetDefenseState(0);
	if (GetMaxPeaceTime() < 1 && GetReadyUnits(&bashnia2) < 1)
		return;
	SetDefenseState(0);
}

// FUN_10005200
void LandBasicBuildings()
{
	TryUnit(&melnica, 1, 100, 100);
	TryUnit(&center, 1, 100, 100);
	TryUnit(&sclad, 1, 100, 100);
	TryUnit(&rinok, 1, 100, 100);
}

// FUN_10005dd0
void LandArmy()
{
	if (GetUnits(&strelec_algir) < 40)
		SetAIRegister(AI_NATION + 8, 0);
	if (GetUnits(&strelec_algir) >= 157 || GetReadyUnits(&konushnia) != 0)
		SetAIRegister(AI_NATION + 8, 1);
	for (int i = 0; i < 2; i++)
	{
		TryUnit(&krestian, 1000, 20, 100);
		if (GetUnits(&pikiner) < 140)
		{
			TryUnit(&pikiner, 1000, 20, 100);
		}
		else
		{
			TryUnit(&pikiner, 1000, 20, 40);
			TryUnit(&pehota, 1000, 20, 100);
		}
		if (GetAIRegister(AI_NATION + 8) == 0 && GetPeaceTimeLeft() < 1)
			TryUnit(&strelec_algir, 160, 20, 100);
		if (GetUnitCost(AI_NATION, &dragun18_dip, GOLD) < 120)
		{
			if (GetMoney(COAL) > 2000 && GetMoney(IRON) > 2000)
			{
				for (int j = 0; j < 30; j++)
					TryUnit(&dragun18_dip, 30, 10, 100);
			}
		}
		TryUnit(&AI_MOUNTED_PICK, 1000, 20, UpgIsDone(&MAIN) ? 30 : 100);
		TryUnit(&dragun18, 1000, 20, 100);
		AI_SHOOTER_TRYUNIT(1000, 20, 100);
	}
}

// FUN_10006710
void LandCannons()
{
	if (UpgIsDone(&pushka_BUILD5))
		TryUnit(&pushka, 30, 30, 60);
	if (UpgIsDone(&mortira_BUILD5))
		TryUnit(&mortira, 20, 30, 60);
	TryUnit(&mortira_b, 60, 80, 100);
	TryUpgrade(&pushka_BUILD, 30, 100);
	TryUpgrade(&pushka_BUILD3, 30, 100);
	TryUpgrade(&pushka_BUILD4, 30, 100);
	TryUpgrade(&pushka_BUILD5, 30, 100);
	TryUpgrade(&pushka_BUILD6, 30, 100);
	TryUpgrade(&mortira_BUILD, 30, 100);
	TryUpgrade(&mortira_BUILD3, 30, 100);
	TryUpgrade(&mortira_BUILD4, 30, 100);
	TryUpgrade(&mortira_BUILD5, 30, 100);
}

// FUN_10006800 (called only when GetPeaceTimeLeft() < 5)
void LandTowers()
{
	int bashniaReady = GetReadyUnits(&bashnia2);
	int barackN = GetUnits(&barack);
	int barackReady = GetReadyUnits(&barack);
	int minaretN = GetUnits(&minaret);
	int minaretReady = GetReadyUnits(&minaret);
	int bashniaN = GetUnits(&bashnia2);
	int eq = (minaretN - minaretReady) - barackReady - bashniaReady + barackN + bashniaN;
	// TEMP DEBUG (printf per AGENTS.md): trace the tower gate, remove after diagnosis
	static int dbgTick = 0;
	bool dbgNow = (++dbgTick % 100 == 1);
	if (dbgNow)
		printf("TOWDBG nat=%d land=%d peace=%d aka=%d/%d bar=%d/%d tow=%d/%d eq=%d freeproxy=%d money w%d g%d s%d f%d i%d c%d towcost w%d g%d s%d i%d c%d\n",
			AI_NATION, AI_LAND, GetPeaceTimeLeft(),
			minaretN, minaretReady, barackN, barackReady, bashniaN, bashniaReady, eq, AI_FREEPROXY,
			GetMoney(WOOD), GetMoney(GOLD), GetMoney(STONE), GetMoney(FOOD), GetMoney(IRON), GetMoney(COAL),
			GetUnitCost(AI_NATION, &bashnia2, WOOD), GetUnitCost(AI_NATION, &bashnia2, GOLD),
			GetUnitCost(AI_NATION, &bashnia2, STONE), GetUnitCost(AI_NATION, &bashnia2, IRON),
			GetUnitCost(AI_NATION, &bashnia2, COAL));
	if (eq == 0 && AI_FREEPROXY > 40)
	{
		bool pr = TryUnit(&bashnia2, 4, 50, 100);
		if (pr || dbgNow)
			printf("TOWDBG nat=%d PROPOSE tow x4 -> %d (eq=%d freeproxy=%d)\n", AI_NATION, (int)pr, eq, AI_FREEPROXY);
		if (UpgIsRun(&KUZ03))
			TryUnit(&bashnia2, 6, 50, 100);
		if (UpgIsRun(&MAIN))
			TryUnit(&bashnia2, 8, 50, 100);
	}
	TryUpgrade(&bashnia_PAUSE, 50, 100);
	TryUpgrade(&bashnia_PAUSE3, 40, 100);
	TryUpgrade(&bashnia_PAUSE4, 30, 100);
	TryUpgrade(&bashnia_PAUSE5, 20, 100);
	TryUpgrade(&bashnia_PAUSE6, 15, 100);
	TryUpgrade(&bashnia_PAUSE7, 10, 100);
}

// FUN_10005bb0
void LandBuildings()
{
	TryUnit(&kazarma, 3, 100, 100);
	if (GetReadyUnits(&diplomatic) != 0)
	{
		TryUnit(&melnica, 2, 100, 100);
		TryUnit(&sclad, 4, 100, 100);
	}
	TryUnit(&melnica, 1, 100, 100);
	TryUnit(&center, 1, 100, 100);
	if (aiRandFlag[AI_NATION] == 0)
	{
		LandBasicBuildings();
		if (GetReadyUnits(&rinok) == 0)
			return;
	}
	TryUnit(&kuznia, 1, 100, 100);
	TryUnit(&center, 2, 100, 100);
	if (GetReadyUnits(&center) < 2 && GetStartRes() != 0)
		return;
	TryUnit(&barack, 2, 100, 100);
	if (GetUnits(&barack) - GetReadyUnits(&barack) < 1)
	{
		TryUnit(&minaret, 1, 100, 100);
		if (GetPeaceTimeLeft() < 3)
		{
			TryUnit(&diplomatic, 1, 100, 100);
			if (GetReadyUnits(&diplomatic) == 0)
				return;
		}
		LandBasicBuildings();
		TryUnit(&center, 4, 100, 100);
		if (GetReadyUnits(&center) > 2)
		{
			TryUnit(&konushnia, 3, 100, 100);
			if (GetUnits(&kazarma) != 0)
				TryUnit(&konushnia, 4, 100, 100);
			TryUnit(&art_depo, 1, 100, 100);
			TryUnit(&art_depo, 3, 40, 100);
			if (UpgIsDone(&AKA10))
				TryUnit(&konushnia, 5, 100, 100);
			TryUnit(&mechet, 1, 100, 100);
		}
	}
}

// FUN_10005a70 - includes the XVIII-century research TryUpgrade(&MAIN, 98, 100)
void LandUpgrades()
{
	TryUpgrade(&Melnica_tuGETRES, 50, 100);
	if (GetUnits(&minaret) != 0)
		TryUpgrade(&KUZ01, 100, 100);
	TryUpgrade(&AKA01, 100, 100);
	if (PikeChain4Done() || (AI_LAND != LT_LAND && AI_LAND != LT_CONTINENT))
		TryUpgrade(&AKA08, 90, 100);
	if (GetUnits(&diplomatic) != 0)
		TryUpgrade(&AKA04, 100, 100);
	if (UpgIsRun(&AKA24))
		TryUpgrade(&AKA02, 100, 100);
	TryUpgrade(&AKA24, 92, 100);
	TryUpgrade(&Melnica_GETRES2, 98, 100);
	if (GetUnits(&konushnia) != 0)
		TryUpgrade(&KUZ03, 98, 100);
	TryUpgrade(&MAIN, 98, 100);
	if (GetUnits(&kazarma) != 0)
		TryUpgrade(&AKA10, 100, 100);
}

// FUN_10005290 (called only when GetPeaceTimeLeft() < 3)
void LandCombatUpgrades()
{
	if (GetUnits(&strelec_algir) != 0 || GetUnits(&mortira_b) != 0)
	{
		TryUpgrade(&AKA11, 100, 100);
		TryUpgrade(&KUZ02, 100, 100);
		TryUpgrade(&AKA09, 100, 100);
	}
	if (GetUnits(&AI_MOUNTED_PICK) >= 21 || GetUnits(&mameluk) >= 21 || GetUnits(&pikiner) >= 141)
	{
		TryUpgrade(&KUZ05, 95, 100);
		TryUpgrade(&AKA34, 95, 100);
		int kuz06 = 95;
		if (GetUnits(&AI_MOUNTED_PICK) < 141 && GetUnits(&mameluk) <= 60)
			kuz06 = 15;
		TryUpgrade(&KUZ06, kuz06, 100);
	}
	if (GetUnits(&pikiner) > 25)
	{
		TryUpgrade(&pikiner_ATTACK, 80, 100);
		TryUpgrade(&pikiner_ATTACK3, 85, 100);
		TryUpgrade(&pikiner_ATTACK4, 90, 100);
		TryUpgrade(&pikiner_SHIELD, 80, 100);
		TryUpgrade(&pikiner_SHIELD3, 85, 100);
		TryUpgrade(&pikiner_SHIELD4, 90, 100);
		if (GetUnits(&pikiner) > 120)
		{
			TryUpgrade(&pikiner_ATTACK5, 90, 100);
			TryUpgrade(&pikiner_ATTACK6, 90, 100);
			TryUpgrade(&pikiner_SHIELD5, 90, 100);
			TryUpgrade(&pikiner_SHIELD6, 90, 100);
		}
	}
	if (GetUnits(&pehota) > 30)
	{
		TryUpgrade(&pehota_ATTACK, 90, 100);
		TryUpgrade(&pehota_ATTACK3, 90, 100);
		TryUpgrade(&pehota_ATTACK4, 90, 100);
		TryUpgrade(&pehota_SHIELD, 30, 100);
		TryUpgrade(&pehota_SHIELD3, 30, 100);
		TryUpgrade(&pehota_SHIELD4, 30, 100);
		TryUpgrade(&AKA31, 90, 100);
		if (GetUnits(&pehota) > 100)
			TryUpgrade(&AKA33, 90, 100);
	}
	if (GetUnits(&oficer) > 4)
	{
		TryUpgrade(&officer_ATTACK, 90, 100);
		TryUpgrade(&officer_SHIELD, 90, 100);
	}
	if (GetUnits(&AI_MOUNTED_PICK) > 20)
	{
		TryUpgrade(&mounted_pick_chain[6], 80, 100);
		TryUpgrade(&mounted_pick_chain[7], 85, 100);
		TryUpgrade(&mounted_pick_chain[8], 90, 100);
		TryUpgrade(&mounted_pick_chain[0], 80, 100);
		TryUpgrade(&mounted_pick_chain[1], 85, 100);
		TryUpgrade(&mounted_pick_chain[2], 90, 100);
		if (GetUnits(&AI_MOUNTED_PICK) > 50)
		{
			TryUpgrade(&mounted_pick_chain[9], 95, 100);
			TryUpgrade(&mounted_pick_chain[10], 95, 100);
			TryUpgrade(&mounted_pick_chain[11], 95, 100);
			TryUpgrade(&mounted_pick_chain[3], 95, 100);
			TryUpgrade(&mounted_pick_chain[4], 95, 100);
			TryUpgrade(&mounted_pick_chain[5], 95, 100);
		}
	}
	if (GetUnits(&mameluk) > 20)
	{
		TryUpgrade(&mameluk_SHIELD, 90, 100);
		TryUpgrade(&mameluk_SHIELD3, 90, 100);
		TryUpgrade(&mameluk_SHIELD4, 90, 100);
		TryUpgrade(&mameluk_ATTACK, 90, 100);
		TryUpgrade(&mameluk_ATTACK3, 90, 100);
		TryUpgrade(&mameluk_ATTACK4, 95, 100);
		TryUpgrade(&mameluk_ATTACK5, 95, 100);
		TryUpgrade(&mameluk_ATTACK6, 95, 100);
		TryUpgrade(&mameluk_ATTACK7, 95, 100);
		if (UpgIsRun(&mameluk_ATTACK7))
		{
			TryUpgrade(&mameluk_SHIELD5, 95, 100);
			TryUpgrade(&mameluk_SHIELD6, 95, 100);
			TryUpgrade(&mameluk_SHIELD7, 95, 100);
		}
	}
	if (GetUnits(&dragun17) > 20 && GetUnits(&dragun18) < 20)
	{
		TryUpgrade(&dragun_ATTACK, 80, 100);
		TryUpgrade(&dragun_ATTACK3, 85, 100);
		TryUpgrade(&dragun_ATTACK4, 90, 100);
		TryUpgrade(&dragun_SHIELD5, 95, 100);
		TryUpgrade(&dragun_SHIELD6, 95, 100);
		TryUpgrade(&dragun_SHIELD7, 95, 100);
		TryUpgrade(&AKA31, 90, 100);
	}
	if (GetUnits(&dragun18) > 20)
	{
		TryUpgrade(&dragun18_ATTACK, 80, 100);
		TryUpgrade(&dragun18_ATTACK3, 85, 100);
		TryUpgrade(&dragun18_ATTACK4, 90, 100);
		TryUpgrade(&dragun18_ATTACK5, 95, 100);
		TryUpgrade(&dragun18_ATTACK6, 95, 100);
		TryUpgrade(&dragun18_ATTACK7, 95, 100);
		TryUpgrade(&dragun18_SHIELD, 10, 100);
		TryUpgrade(&dragun18_SHIELD3, 15, 100);
		TryUpgrade(&dragun18_SHIELD4, 20, 100);
		TryUpgrade(&dragun18_SHIELD5, 25, 100);
		TryUpgrade(&dragun18_SHIELD6, 25, 100);
		TryUpgrade(&dragun18_SHIELD7, 25, 100);
		TryUpgrade(&AKA31, 90, 100);
		TryUpgrade(&AKA33, 90, 100);
	}
	if (AI_SHOOTER_UNITS > 25)
	{
		for (int i = 0; i < 12; i++)
			TryUpgrade(&shooter_pick_chain[i], 90, 100);
		int aka35 = 90;
		if (AI_SHOOTER_UNITS < 61)
		{
			TryUpgrade(&AKA35, 15, 100);
			aka35 = 15;
		}
		else
		{
			TryUpgrade(&AKA35, 90, 100);
		}
		TryUpgrade(&KUZ04, aka35, 100);
		if (UpgIsRun(&AKA35) && UpgIsRun(&KUZ04))
			TryUpgrade(&AKA36, 90, 100);
		TryUpgrade(&AKA31, 90, 100);
		TryUpgrade(&AKA33, 90, 100);
	}
	if (UpgIsDone(&dragun18_ATTACK7))
	{
		TryUpgrade(&AKA15, 90, 100);
		if (UpgIsRun(&AKA15))
		{
			TryUpgrade(&AKA14, 90, 100);
			if (UpgIsRun(&AKA14))
			{
				TryUpgrade(&AKA13, 90, 100);
				if (UpgIsRun(&AKA13))
					TryUpgrade(&AKA12, 90, 100);
			}
		}
	}
}

// FUN_10005fb0
void LandMarket()
{
	if (GetReadyUnits(&rinok) != 0)
	{
		if (GetStartRes() == 3)
		{
			if (GetAIRegister(AI_NATION) == 0)
			{
				AI_Torg(STONE, FOOD, 500000);
				AI_Torg(WOOD, FOOD, 80000);
				SetAIRegister(AI_NATION, 1);
				return;
			}
		}
		else
		{
			if (GetReadyUnits(&rinok) != 0)
			{
				if (GetAIRegister(AI_NATION) == 0)
				{
					AI_Torg(COAL, FOOD, 4000);
					if (GetStartRes() == 2)
					{
						AI_Torg(IRON, WOOD, 1000);
						AI_Torg(IRON, STONE, 1000);
					}
					if (GetStartRes() == 1)
					{
						AI_Torg(IRON, WOOD, 600);
						AI_Torg(IRON, STONE, 600);
					}
					SetAIRegister(AI_NATION, 1);
				}
				if (UpgIsRun(&AKA08) && !UpgIsRun(&AKA24))
				{
					if (GetReadyUnits(&minaret) != 0)
					{
						// Hoard coal for AKA24, selling wood.
						int c = GetUpgradeCost(AI_NATION, &AKA24, COAL);
						AiHoardFor(COAL, c * 11 / 10, WOOD);
					}
				}
				if (UpgIsRun(&AKA24) && !UpgIsRun(&MAIN))
				{
					// Hoard the XVIII-century cost (food/gold/iron), selling stone.
					int c = GetUpgradeCost(AI_NATION, &MAIN, FOOD);
					if (AiHoardFor(FOOD, c * 11 / 10, STONE))
					{
						c = GetUpgradeCost(AI_NATION, &MAIN, GOLD);
						if (AiHoardFor(GOLD, c * 105 / 100, STONE))
						{
							c = GetUpgradeCost(AI_NATION, &MAIN, IRON);
							AiHoardFor(IRON, c * 11 / 10, STONE);
						}
					}
				}
				if (GetMoney(STONE) > 100000)
				{
					AI_Torg(WOOD, STONE, 5000);
					AI_Torg(STONE, GOLD, GetMoney(STONE));
				}
				if (GetMoney(WOOD) > 100000)
				{
					AI_Torg(STONE, WOOD, 5000);
					AI_Torg(WOOD, GOLD, GetMoney(WOOD));
				}
			}
		}
	}
}

// FUN_100062d0
void LandDistribution()
{
	SetPDistribution(0, 4, 4);
	if (!FieldExist())
		return;
	SetPDistribution(9, 4, 4);
	if (GetStartRes() == 3)
		return;
	if (!FieldExist())
	{
		SetPDistribution(0, 6, 3);
		return;
	}
	SetPDistribution(9, 4, 4);
	if (!UpgIsRun(&Melnica_GETRES2))
	{
		int c = GetUpgradeCost(AI_NATION, &Melnica_GETRES2, FOOD);
		if (GetMoney(FOOD) < c * 11 / 10)
			SetPDistribution(9, 4, 4);
		else
			SetPDistribution(9, 6, 6);
	}
	if (!UpgIsRun(&MAIN))
	{
		int c = GetUpgradeCost(AI_NATION, &MAIN, FOOD);
		if (GetMoney(FOOD) < c * 11 / 10)
			SetPDistribution(9, 2, 9);
		else
			SetPDistribution(9, 6, 6);
	}
	if (!UpgIsRun(&AKA24))
	{
		int c = GetUpgradeCost(AI_NATION, &AKA24, FOOD);
		SetPDistribution(GetMoney(FOOD) < c * 13 / 10 ? 9 : 6, 9, 1);
	}
	if ((!UpgIsRun(&pikiner_ATTACK4) || !UpgIsRun(&pikiner_SHIELD4)) && GetMoney(FOOD) < 9000)
		SetPDistribution(9, 4, 1);
	if (GetUnits(&diplomatic) == 0 && GetPeaceTimeLeft() < 2)
	{
		int c = GetUnitCost(AI_NATION, &diplomatic, STONE);
		if (GetMoney(STONE) < c * 11 / 10)
			SetPDistribution(9, 5, 3);
		else
			SetPDistribution(9, 7, 1);
	}
	if (!UpgIsRun(&AKA08))
		SetPDistribution(9, 1, 1);
	if (!UpgIsRun(&AKA01) || !UpgIsRun(&KUZ01))
		SetPDistribution(9, 4, 1);
	if (GetUnits(&minaret) == 0)
		SetPDistribution(6, 4, 4);
	if (!UpgIsRun(&Melnica_tuGETRES))
		SetPDistribution(9, 1, 1);
}

// FUN_10006bc0 - normal land pipeline (startRes != 3)
void LandPipeline()
{
	if (aiRandFlag[AI_NATION] != 0)
		LandDefenseStance();
	LandArmy();
	LandCannons();
	if (GetPeaceTimeLeft() < 5)
		LandTowers();
	LandBuildings();
	LandUpgrades();
	if (GetPeaceTimeLeft() < 3)
		LandCombatUpgrades();
	LandMarket();
	LandDistribution();
	ApplyMineUpgradeRules(AI_LAND);
}

// FUN_10006b40
void RichAttackToggle()
{
	SetDefenseState(1);
	if (GetUnits(&dragun18_dip) < 4)
		SetAIRegister(AI_NATION + 8, 0);
	if (GetUnits(&dragun18_dip) > 95)
		SetAIRegister(AI_NATION + 8, 1);
	SetDefenseState(GetAIRegister(AI_NATION + 8) == 0 ? 1 : 0);
	SetDefenseState(0);
}

// FUN_10006930
void RichBuildings()
{
	if (GetUnits(&kazarma) == GetReadyUnits(&kazarma))
		TryUnit(&kazarma, 5, 100, 100);
	TryUnit(&center, 1, 100, 100);
	TryUnit(&kuznia, 1, 100, 100);
	TryUnit(&barack, 2, 100, 100);
	if (GetUnits(&barack) == GetReadyUnits(&barack))
	{
		TryUnit(&minaret, 1, 100, 100);
		if (GetReadyUnits(&minaret) != 0)
		{
			TryUnit(&sclad, 1, 100, 100);
			TryUnit(&melnica, 1, 100, 100);
			TryUnit(&rinok, 1, 100, 100);
			if (GetReadyUnits(&rinok) != 0)
			{
				TryUnit(&diplomatic, 1, 100, 100);
				TryUnit(&mechet, 1, 100, 100);
				TryUnit(&art_depo, 1, 100, 100);
				TryUnit(&konushnia, 1, 100, 100);
				if (UpgIsRun(&MAIN))
				{
					if (GetUnits(&center) == GetReadyUnits(&center))
						TryUnit(&center, GetExtraction(GOLD) < 100 ? 6 : 4, 100, 100);
					if (GetUnits(&konushnia) == GetReadyUnits(&konushnia))
						TryUnit(&konushnia, 7, 100, 100);
					if (GetUnits(&art_depo) == GetReadyUnits(&art_depo))
						TryUnit(&art_depo, 3, 100, 100);
				}
			}
		}
	}
}

// FUN_10006c20 - land pipeline for the highest start resources (startRes == 3)
void RichLandPipeline()
{
	RichAttackToggle();
	RichBuildings();
	for (int i = 0; i < 2; i++)
	{
		TryUnit(&krestian, 1000, 20, 100);
		TryUnit(&pikiner, 1000, 20, 100);
		if (GetAIRegister(AI_NATION + 8) == 0)
		{
			for (int j = 0; j < 100; j++)
				TryUnit(&dragun18_dip, 110, 20, 100);
		}
		else
		{
			TryUnit(&grenader, 60, 10, 100);
		}
		if (!UpgIsDone(&MAIN))
			TryUnit(&dragun17, 1000, 20, 100);
		TryUnit(&dragun18, 1000, 20, 100);
		AI_SHOOTER_TRYUNIT(1000, 20, 100);
	}
	LandCannons();
	LandMarket();
	LandDistribution();
	LandCombatUpgrades();
	LandUpgrades();
}

// ----- Mediterranean handler -----

// FUN_10004210
void MedPikeChain()
{
	if (GetMoney(FOOD) > 1650)
	{
		TryUpgrade(&pikiner_ATTACK, 40, 100);
		TryUpgrade(&pikiner_ATTACK3, 90, 100);
		TryUpgrade(&pikiner_ATTACK4, 90, 100);
		TryUpgrade(&pikiner_ATTACK5, 90, 100);
		TryUpgrade(&pikiner_ATTACK6, 90, 100);
		TryUpgrade(&pikiner_SHIELD, 90, 100);
		TryUpgrade(&pikiner_SHIELD3, 90, 100);
		TryUpgrade(&pikiner_SHIELD4, 90, 100);
		TryUpgrade(&pikiner_SHIELD5, 90, 100);
		TryUpgrade(&pikiner_SHIELD6, 90, 100);
		TryUpgrade(&officer_ATTACK, 50, 100);
		TryUpgrade(&officer_SHIELD, 50, 100);
	}
}

// FUN_100042f0
void MedMusketeerChain()
{
	TryUpgrade(&pehota_ATTACK, 90, 100);
	TryUpgrade(&pehota_ATTACK3, 90, 100);
	TryUpgrade(&pehota_ATTACK4, 90, 100);
	TryUpgrade(&pehota_SHIELD, 10, 100);
	TryUpgrade(&pehota_SHIELD3, 10, 100);
}

// FUN_10004340
void MedCannonBuild()
{
	TryUpgrade(&pushka_BUILD, 30, 50);
	TryUpgrade(&pushka_BUILD3, 30, 50);
	TryUpgrade(&pushka_BUILD4, 30, 50);
	TryUpgrade(&pushka_BUILD5, 30, 50);
	TryUpgrade(&mortira_BUILD, 30, 50);
	TryUpgrade(&mortira_BUILD3, 30, 50);
	TryUpgrade(&mortira_BUILD4, 30, 50);
	TryUpgrade(&mortira_BUILD5, 30, 50);
}

// ProcessMiddleSeaAI @ FUN_100043c0 (land type 1)
void ProcessMiddleSeaAI()
{
	SetDefenseState(0);
	SetMineBalanse(19, MED_MINE_BALANCE);
	int peasants = GetUnits(&krestian);
	int food = GetMoney(FOOD);
	bool kuz03run = UpgIsRun(&KUZ03);
	int elite = GetUnits(&pikiner) + GetUnits(&mameluk) + GetUnits(&AI_MOUNTED_PICK);
	if (GetDifficulty() > 0)
		TryUnit(&kozak, 15, 20, 40);
	int towersReady = GetReadyUnits(&bashnia2);
	int towers = GetUnits(&bashnia2);
	if (peasants == 0)
		return;
	TryUnit(&melnica, 1, 100, 100);
	if (GetUnits(&melnica) == 0)
		return;
	TryUpgrade(&Melnica_tuGETRES, 100, 100);
	bool fields = FieldExist();
	if (!fields)
	{
		if (food > 700)
		{
			TryUnit(&krestian, 100, 100, 100);
			TryUnit(&pikiner, 100, 100, 100);
		}
		if (food > 1000 && elite > 30)
			MedPikeChain();
		SetPDistribution(0, 1, 1);
	}
	else
	{
		if (aiMedDistMode == 0)
		{
			if (food > 12000)
				SetPDistribution(9, 6, 6);
			SetPDistribution(9, 5, 5);
		}
		else if (aiMedDistMode == 1)
		{
			SetPDistribution(9, 9, 4);
		}
		else if (aiMedDistMode == 2)
		{
			SetPDistribution(9, 4, 9);
		}
		int maxP = kuz03run ? 380 : 450;
		if (peasants < 25 && food > 300)
			TryUnit(&krestian, 100, 100, 100);
		if (peasants >= 25 || food > 300)
		{
			if (food > 300)
			{
				TryUnit(&krestian, maxP, 232, 100);
				if (food > 400)
					TryUnit(&pikiner, maxP, 244, 100);
			}
		}
		if (elite > 30)
			MedPikeChain();
	}
	TryUnit(&center, 1, 100, 100);
	TryUnit(&center, 2, 90, 5);
	TryUnit(&center, 3, 90, 5);
	if (GetUnits(&barack) > 1)
	{
		TryUnit(&center, 2, 80, 5);
		if (GetUnits(&center) > 1)
			TryUnit(&konushnia, 1, 99, 90);
		if (GetReadyUnits(&center) > 1)
			TryUnit(&rinok, 1, 100, 100);
		TryUpgrade(&AKA23, 99, 100);
	}
	if (GetUnits(&krestian) > 35)
		TryUnit(&rinok, 1, 100, 100);
	TryUnit(&kuznia, 1, 100, 100);
	TryUnit(&barack, 1, 100, 100);
	TryUnit(&barack, 2, 100, 100);
	TryUnit(&sclad, 1, 50, 50);
	TryUnit(&kuznia, 2, 10, 5);
	TryUpgrade(&KUZ01, 80, 100);
	if (GetUnits(&minaret) > 0)
		TryUnit(&mechet, 1, 5, 3);
	if (fields)
		TryUnit(&minaret, 1, 90, 90);
	TryUnit(&diplomatic, 1, 99, 10);
	TryUnit(&art_depo, 1, 90, 90);
	if (peasants > 25)
		TryUnit(&melnica, 2, 30, 30);
	TryUnit(&kazarma, 1, 90, 20);
	TryUnit(&kazarma, 2, 80, 5);
	TryUpgrade(&AKA01, 90, 99);
	TryUpgrade(&AKA08, 95, 100);
	TryUpgrade(&KUZ03, 100, 100);
	TryUpgrade(&AKA04, 90, 90);
	TryUpgrade(&AKA02, 95, 100);
	TryUpgrade(&AKA03, 95, 100);
	if (GetReadyUnits(&konushnia) == 0 || kuz03run
		|| (GetMoney(GOLD) > 3700 && GetMoney(IRON) > 5500 && GetMoney(COAL) > 8400))
	{
		SetUpgradeLock(GOLD, 0);
		SetUpgradeLock(IRON, 0);
		SetUpgradeLock(FOOD, 0);
		SetUpgradeLock(GOLD, 0);
	}
	else
	{
		if (GetMoney(WOOD) != 0 && GetMoney(COAL) > 8400)
		{
			if (GetMoney(COAL) > 14000)
				AI_Torg(COAL, GOLD, 5000);
			if (GetMoney(IRON) > 13000)
				AI_Torg(IRON, GOLD, 5000);
		}
	}
	TryUpgrade(&AKA30, 100, 100);
	TryUnit(&port, 1, 50, 90);
	TryUnit(&port, 2, 20, 50);
	TryUnit(&port, 3, 10, 50);
	if (!UpgIsDone(&AKA06))
	{
		int fleet = GetUnitsByUsage(0, USAGE_IAXTA) / 4 + GetUnitsByUsage(0, USAGE_GALERA)
			+ GetUnitsByUsage(0, USAGE_FREGAT);
		if (fleet > 6)
			fleet = 6;
		if (fleet != 0)
			TryUnit(&galera, fleet, 50, 50);
	}
	else
	{
		TryUnit(&shebeka, 10, 50, 50);
	}
	TryUpgrade(&AKA06, 99, 100);
	if (UpgIsRun(&AKA08))
	{
		TryUpgrade(&AKA06, 90, 90);
		TryUpgrade(&AKA05, 99, 100);
	}
	if (UpgIsRun(&AKA06))
	{
		TryUpgrade(&AKA20, 90, 90);
		TryUpgrade(&AKA28, 70, 10);
		WaterMortiraUpgrades();
	}
	SetUpgradeLock(GOLD, 0);
	if (GetMoney(WOOD) > 14000 && UpgIsRun(&AKA30) && GetMoney(GOLD) < 7500
		&& !UpgIsRun(&AKA06))
	{
		SetUpgradeLock(GOLD, 1);
	}
	TryUnit(&lodka, 3, 90, 90);
	if (UpgIsDone(&AKA07))
		TryUnit(&lodka, 20, 10, 90);
	TryUpgrade(&AKA07, 99, 90);
	TryUpgrade(&Melnica_GETRES2, 99, 100);
	TryUpgrade(&bashnia_PAUSE, 50, 50);
	TryUpgrade(&bashnia_PAUSE3, 50, 50);
	TryUpgrade(&bashnia_PAUSE4, 50, 50);
	TryUpgrade(&bashnia_PAUSE5, 50, 50);
	TryUpgrade(&bashnia_PAUSE6, 50, 50);
	TryUpgrade(&bashnia_PAUSE7, 50, 50);
	// TEMP DEBUG (printf per AGENTS.md): Mediterranean tower gate
	{
		static int dbgTickM = 0;
		if (++dbgTickM % 100 == 1)
			printf("TOWDBG-MED nat=%d land=%d peasants=%d tow=%d/%d gate=%d\n",
				AI_NATION, AI_LAND, peasants, towers, towersReady,
				(peasants > 35 && towers == towersReady) ? 1 : 0);
	}
	if (peasants > 35 && towers == towersReady)
	{
		TryUnit(&bashnia2, 1, 80, 10);
		if (peasants > 60)
		{
			TryUnit(&bashnia2, 2, 80, 10);
			TryUnit(&bashnia2, 3, 50, 10);
			TryUnit(&bashnia2, 4, 30, 10);
		}
	}
	if (!kuz03run)
	{
		TryUnit(&grenader, 50, 1, 30);
		TryUnit(&strelec_algir, 20, 1, 30);
	}
	else
	{
		TryUnit(&mortira_b, 50, 10, 100);
		TryUnit(&grenader, 75, 1, 30);
		TryUnit(&strelec_algir, 30, 1, 30);
		TryUpgrade(&AKA34, 95, 90);
		TryUpgrade(&KUZ06, 95, 90);
		TryUpgrade(&KUZ05, 95, 90);
		if (UpgIsDone(&pushka_BUILD5))
			TryUnit(&pushka, 5, 30, 10);
		if (UpgIsDone(&mortira_BUILD5))
			TryUnit(&mortira, 10, 30, 10);
		MedCannonBuild();
	}
	if (UpgIsDone(&MAIN))
	{
		int w = UpgIsDone(&Melnica_GETRES2) ? 4 : 2;
		SetPDistribution(8, w, w);
	}
	if (GetUnits(&konushnia) != 0)
		TryUnit(&lodka, 6, 90, 90);
	if (UpgIsDone(&AKA07))
		TryUnit(&lodka, 20, 10, 90);
	if (kuz03run)
	{
		TryUnit(&konushnia, 2, 95, 20);
		TryUnit(&konushnia, 3, 95, 20);
		TryUnit(&konushnia, 4, 95, 20);
		TryUnit(&konushnia, 5, 40, 20);
		TryUnit(&konushnia, 6, 40, 20);
		TryUpgrade(&MAIN, 99, 100);
		if (GetUnits(&pehota) > 20)
		{
			TryUpgrade(&AKA31, 90, 100);
			TryUpgrade(&AKA33, 80, 70);
		}
		TryUpgrade(&AKA07, 60, 90);
		if (UpgIsRun(&pehota_ATTACK4))
		{
			TryUpgrade(&AKA12, 90, 90);
			TryUpgrade(&AKA13, 90, 90);
			TryUpgrade(&AKA14, 70, 90);
			TryUpgrade(&AKA15, 70, 90);
		}
	}
	TryUpgrade(&AKA30, 99, 100);
	TryUpgrade(&AKA10, 60, 90);
	TryUpgrade(&AKA24, 100, 100);
	if (elite > 350)
	{
		TryUnit(&pehota, 500, 10, 100);
		MedMusketeerChain();
	}
	// market tail
	if (GetReadyUnits(&rinok) == 0)
		return;
	int wood = GetMoney(WOOD);
	int stone = GetMoney(STONE);
	int iron = GetMoney(IRON);
	int coal = GetMoney(COAL);
	int gold = GetMoney(GOLD);
	if (food < 150)
	{
		if (stone > 5000)
			AI_Torg(STONE, FOOD, 3000);
		else if (stone > 1000)
			AI_Torg(STONE, FOOD, 900);
		else if (wood > 5000)
			AI_Torg(WOOD, FOOD, 3000);
		else if (wood > 1000)
			AI_Torg(WOOD, FOOD, 900);
		else if (coal > 2000)
			AI_Torg(COAL, FOOD, 800);
		else if (iron > 2000)
			AI_Torg(IRON, FOOD, 800);
		else if (gold > 1000)
			AI_Torg(GOLD, FOOD, 500);
	}
	if (GetReadyUnits(&minaret) != 0 && gold > 1000 && !UpgIsRun(&AKA08))
	{
		if (iron > 3000)
			AI_Torg(IRON, FOOD, 2000);
		if (coal > 4000)
			AI_Torg(COAL, FOOD, 3000);
	}
	if (GetMoney(STONE) > 120000)
		AI_Torg(STONE, GOLD, 100000);
	if (GetMoney(WOOD) > 100000)
		AI_Torg(WOOD, GOLD, 90000);
	if (GetUnits(&kuznia) != 0 && !UpgIsRun(&KUZ03))
	{
		if (GetMoney(COAL) > 9000 && GetMoney(WOOD) != 0 && GetMoney(GOLD) < 4000)
		{
			if (GetMoney(COAL) > 12000)
				AI_Torg(COAL, GOLD, 3000);
			if (GetMoney(IRON) > 8000)
				AI_Torg(IRON, GOLD, 2000);
			if (GetMoney(STONE) > 20000)
				AI_Torg(STONE, GOLD, 12000);
			if (GetMoney(FOOD) > 30000)
				AI_Torg(FOOD, GOLD, 15000);
		}
		if (GetMoney(COAL) > 9000 && GetMoney(GOLD) != 0 && GetMoney(GOLD) > 4000)
		{
			if (GetMoney(COAL) > 12000)
				AI_Torg(COAL, IRON, 3000);
			if (GetMoney(STONE) > 20000)
				AI_Torg(STONE, IRON, 12000);
			if (GetMoney(FOOD) > 30000)
				AI_Torg(FOOD, IRON, 15000);
		}
	}
	if (aiMedTradeFlag[GetAINation()] == 0)
	{
		AI_Torg(COAL, FOOD, 4000);
		aiMedTradeFlag[GetAINation()] = 1;
	}
}

// ----- per-tick prologue + dispatcher -----

// FUN_10003020
void AiTickPrologue()
{
	AI_NATION = GetAINation();
	AI_FREEPROXY = GetUnits(&krestian) - GetExtraction(GOLD) - GetExtraction(IRON)
		- GetExtraction(COAL);
	if (GetStartRes() == 0)
		MINES_BUILD_RULES[1] = 4;
	SetMineBalanse(19, LAND_MINE_BALANCE);
	SetMinesUpgradeRules(MINE_UPG_RULES_ZERO);
	SetMinesBuildingRules(MINES_BUILD_RULES, 27);
}

// FUN_10006d20 - the ProcessAI dispatcher (plus FUN_100031a0 officers tail)
void EuropeanProcessAI()
{
	AI_LAND = GetLandType();
	AI_DIFF = GetDifficulty();
	AI_STARTRES = GetStartRes();
	AiTickPrologue();
	// TEMP DEBUG (printf per AGENTS.md): which pipeline runs for this nation
	{
		static int dbgTickD = 0;
		if (++dbgTickD % 200 == 1)
			printf("TOWDBG nat=%d DISPATCH land=%d startres=%d diff=%d\n", AI_NATION, AI_LAND, AI_STARTRES, AI_DIFF);
	}
	SetMineBalanse(19, LAND_MINE_BALANCE);
	ApplyMineUpgradeRules(AI_LAND);
	SetMinesBuildingRules(MINES_BUILD_RULES, 27);
	switch (AI_LAND)
	{
	case LT_LAND:
	case LT_CONTINENT:
		if (AI_STARTRES == 3)
		{
			SetMineBalanse(3, RICH_MINE_BALANCE);
			SetMinesUpgradeRules(MINE_UPG_RULES_B);
			SetMinesBuildingRules(START3_BUILD_RULES, 5);
			RichLandPipeline();
		}
		else
		{
			SetMineBalanse(19, LAND_MINE_BALANCE);
			SetMinesUpgradeRules(MINE_UPG_RULES_ZERO);
			if (AI_STARTRES == 0)
				MINES_BUILD_RULES[1] = 4;
			SetMinesBuildingRules(MINES_BUILD_RULES, 27);
			LandPipeline();
		}
		break;
	case LT_MEDITERRANEAN:
		ProcessMiddleSeaAI();
		break;
	case LT_PENINSULAS:
	case LT_ISLANDS:
	case LT_CONTINENTS:
		SET_MINE_CAPTURE_RADIUS(290);
		SET_MINE_UPGRADE1_RADIUS(290);
		SET_MINE_UPGRADE2_RADIUS(290);
		SetMineBalanse(19, WATER_MINE_BALANCE);
		ProcessWaterAI();
		break;
	default:
		break;
	}
	// FUN_100031a0: officers for the pike formations, land types 0 and 1 only
	if (AI_LAND < 2)
	{
		int units = GetUnits(&pikiner);
		if (units / 36 > 0)
		{
			TryUnit(&oficer, units / 36, 50, 50);
			TryUnit(&barabanshik, units / 36, 20, 50);
		}
	}
}
