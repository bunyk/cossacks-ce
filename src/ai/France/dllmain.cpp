#include <cstdio>
#include "../common/ai_common.h"

#include "objects.h"
// Mine balance tables of the original France.dll (extracted from its .data
// with Ghidra; SetMineBalanse(19, ...) is called with these every AI tick).
// 19 pairs {total peasants, peasants wanted on food+wood+stone}; the engine
// interpolates between pairs and the rest go to mines. The scan in
// megapolis.cpp has no bound check, so the 4000 terminator must stay.
// (Identical to Russia's/Austria's tables - the European AIs share this data.)
unsigned short LAND_MINE_BALANCE[38] = {
	0, 0, 15, 15, 20, 15, 25, 15, 30, 15, 50, 30, 55, 35, 65, 40, 70, 40, 80, 50,
	105, 60, 125, 65, 150, 70, 175, 75, 200, 80, 225, 90, 250, 100, 500, 200, 4000, 400
};
unsigned short WATER_MINE_BALANCE[38] = {
	0, 0, 19, 19, 25, 20, 33, 20, 43, 23, 48, 28, 53, 33, 58, 35, 65, 40, 80, 50,
	105, 55, 125, 60, 150, 75, 175, 90, 200, 100, 225, 100, 250, 120, 500, 140, 4000, 300
};

// SetMinesBuildingRules table (original DAT_1000f244), 27 pairs
// {RSM = gold + (iron << 8) + (coal << 16) mine counts, ResID of the next
// mine to build: 1=gold 4=iron}. The last matching pair wins. Rule [1] is
// raised from 1 to 4 every tick while the game uses the lowest start
// resources. (Same values as Russia's.)
int MINES_BUILD_RULES[54] = {
	0, 1, 256, 1, 65536, 1, 65792, 1, 512, 1, 131072, 1, 66048, 1, 131328, 1, 131584, 1,
	1, 1, 257, 1, 65537, 1, 65793, 1, 513, 1, 131073, 1, 66049, 1, 131329, 1, 131585, 1,
	2, 4, 258, 1, 65538, 4, 65794, 1, 514, 1, 131074, 4, 66050, 1, 131330, 1, 131586, 1
};

// SetMinesUpgradeRules tables (27 ints: MU1{G,I,C}% MU2{G,I,C}% MU3{G,I,C}%,
// percentages of gold/iron/coal mines to keep at upgrade tiers 1/2/3).
int MINE_UPG_RULES_ZERO[27] = { 0 };
int MINE_UPG_RULES_B[27] = { 90 };
int MINE_UPG_RULES_C[27] = {
	90, 80, 70, 25, 20, 15, 25, 20, 15,
	90, 50, 30, 25, 20, 15, 25, 20, 15,
	90, 90, 90, 30, 30, 30, 30, 30, 30
};

// newMons->Usage IDs (mapdiscr.h) for GetUnitsByUsage
#define USAGE_LINKOR	0x0E
#define USAGE_FREGAT	0x19
#define USAGE_GALERA	0x1B
#define USAGE_IAXTA		0x1C
#define USAGE_PAROM		0x1F

#include "../common/european_land.h"



// Registers France's units/upgrades with the game. Names must match the
// per-nation script FRANCE.NDS (listed in Nations.lst, packed in all.gsc).
// All strings below are byte-exact from the original France.dll string
// table (Ghidra + raw-string dump; see tools/ai_port.py which generated
// this module from the France template by positional handler alignment).
// Role notes for this nation:
//   - 17th c. musketeer: Mushketer_ev(fr); 18th c. shooter pick: Grenader(fr), Konni_Ricar(fr)
//   - town hall: Center_France(fr); house: Dom_France(fr); peasant: Krestian_Sved(fr)
//   - 18th c. barracks Kazarma_FR(fr) is MAIN-gated (ENABLE UNITS GRP 4)
// The water-map AI is 1:1 with the decompilation; the land pipeline,
// Mediterranean handler and dispatcher come from the shared European
// skeleton in common/european_land.h (token-identical across the family).
void RegisterFrance()
{
	// Per-game rand flag (original InitAI: GetRandomIndex() & 0x80,
	// stored per nation in a .data array; one module serves one
	// nation, so fill all slots). Must precede the pick GetRandomIndex
	// calls below, as in the original.
	{
		int randFlag = GetRandomIndex() & 0x80;
		for (int fi = 0; fi < 8; fi++)
			aiRandFlag[fi] = randFlag;
	}
	RegisterPickChains("Konni_Ricar(fr)", "Grenader(fr)");

	RegisterUnitType(&pikiner, "Pikiner_evro(fr)");
	RegisterUnitType(&krestian, "Krestian_Sved(fr)");
	RegisterUnitType(&pehota, "Mushketer_ev(fr)");
	RegisterUnitType(&grenader, "Grenader_DIP(fr)");
	RegisterUnitType(&mameluk, "Kirasir(fr)");
	RegisterUnitType(&pushka, "Pushka(fr)");
	RegisterUnitType(&mortira, "Mortira(fr)");
	RegisterUnitType(&mortira_b, "Mortira_b(fr)");
	RegisterUnitType(&lodka, "Lodka(fr)");
	// France's 18th-century barracks. Enabled only by the MAINFR (XVIII
	// century) research - "ENABLE UNITS GRP 4 Kazarma_FR(fr) Dragun_France(fr)
	// Gusar_evro(BA) Kirasir(fr)" in FRANCE.NDS.
	RegisterUnitType(&kazarma, "Kazarma_FR(fr)");
	AssignFormUnit("Pikiner_evro(fr)");
	RegisterUnitType(&kozak, "Kozak_loshad_DIP(fr)");
	RegisterUnitType(&oficer, "Officer_evro(fr)");
	RegisterUnitType(&barabanshik, "Barabanshik_ev1(fr)");
	RegisterUpgrade(&officer_ATTACK, "Officer_evro(fr)ATTACK");
	RegisterUpgrade(&officer_SHIELD, "Officer_evro(fr)SHIELD");
	RegisterUnitType(&center, "Center_France(fr)");
	RegisterUnitType(&melnica, "Melnica(fr)");
	RegisterUnitType(&barack, "Kazarma_1_FR(fr)");
	RegisterUnitType(&sclad, "Sclad2(fr)");
	RegisterUnitType(&minaret, "akademia_FR(fr)");
	RegisterUnitType(&konushnia, "Konushnia_FR(fr)");
	RegisterUnitType(&diplomatic, "Dip_korpus_FR(fr)");
	RegisterUnitType(&art_depo, "artileri_depo_FR(fr)");
	RegisterUnitType(&rinok, "Rinok(fr)");
	RegisterUnitType(&kuznia, "Kuznica_FR(fr)");
	RegisterUnitType(&mechet, "NotrDam(fr)");
	RegisterUnitType(&port, "PorE(fr)");
	RegisterUnitType(&galera, "GALERA(fr)");
	RegisterUnitType(&peres_kor, "PERES_KOR(fr)");
	RegisterUnitType(&shebeka, "Fregat(fr)");
	RegisterUnitType(&linkor, "Linkor(fr)");
	RegisterUpgrade(&AKA29, "AKA29FR");
	RegisterUpgrade(&AKA06, "AKA06FR");
	RegisterUpgrade(&Melnica_tuGETRES, "Melnica(fr)GETRES");
	RegisterUpgrade(&Melnica_GETRES2, "Melnica(fr)GETRES2");
	RegisterUpgrade(&KUZ01, "KUZ01FR");
	RegisterUpgrade(&pikiner_ATTACK, "Pikiner_evro(fr)ATTACK");
	RegisterUpgrade(&pikiner_ATTACK3, "Pikiner_evro(fr)ATTACK3");
	RegisterUpgrade(&pikiner_ATTACK4, "Pikiner_evro(fr)ATTACK4");
	RegisterUpgrade(&pikiner_ATTACK5, "Pikiner_evro(fr)ATTACK5");
	RegisterUpgrade(&pikiner_ATTACK6, "Pikiner_evro(fr)ATTACK6");
	RegisterUpgrade(&pikiner_SHIELD, "Pikiner_evro(fr)SHIELD");
	RegisterUpgrade(&pikiner_SHIELD3, "Pikiner_evro(fr)SHIELD3");
	RegisterUpgrade(&pikiner_SHIELD4, "Pikiner_evro(fr)SHIELD4");
	RegisterUpgrade(&pikiner_SHIELD5, "Pikiner_evro(fr)SHIELD5");
	RegisterUpgrade(&pikiner_SHIELD6, "Pikiner_evro(fr)SHIELD6");
	RegisterUpgrade(&pehota_ATTACK, "Mushketer_ev(fr)ATTACK");
	RegisterUpgrade(&pehota_ATTACK3, "Mushketer_ev(fr)ATTACK3");
	RegisterUpgrade(&pehota_ATTACK4, "Mushketer_ev(fr)ATTACK4");
	RegisterUpgrade(&pehota_SHIELD, "Mushketer_ev(fr)SHIELD");
	RegisterUpgrade(&pehota_SHIELD3, "Mushketer_ev(fr)SHIELD3");
	RegisterUpgrade(&pehota_SHIELD4, "Mushketer_ev(fr)SHIELD4");
	RegisterUpgrade(&mameluk_ATTACK, "Kirasir(fr)ATTACK");
	RegisterUpgrade(&mameluk_ATTACK3, "Kirasir(fr)ATTACK3");
	RegisterUpgrade(&mameluk_ATTACK4, "Kirasir(fr)ATTACK4");
	RegisterUpgrade(&mameluk_ATTACK5, "Kirasir(fr)ATTACK5");
	RegisterUpgrade(&mameluk_ATTACK6, "Kirasir(fr)ATTACK6");
	RegisterUpgrade(&mameluk_ATTACK7, "Kirasir(fr)ATTACK7");
	RegisterUpgrade(&mameluk_SHIELD, "Kirasir(fr)SHIELD");
	RegisterUpgrade(&mameluk_SHIELD3, "Kirasir(fr)SHIELD3");
	RegisterUpgrade(&mameluk_SHIELD4, "Kirasir(fr)SHIELD4");
	RegisterUpgrade(&mameluk_SHIELD5, "Kirasir(fr)SHIELD5");
	RegisterUpgrade(&mameluk_SHIELD6, "Kirasir(fr)SHIELD6");
	RegisterUpgrade(&mameluk_SHIELD7, "Kirasir(fr)SHIELD7");
	RegisterUpgrade(&pushka_BUILD, "Pushka(fr)BUILD");
	RegisterUpgrade(&pushka_BUILD3, "Pushka(fr)BUILD3");
	RegisterUpgrade(&pushka_BUILD4, "Pushka(fr)BUILD4");
	RegisterUpgrade(&pushka_BUILD5, "Pushka(fr)BUILD5");
	RegisterUpgrade(&pushka_BUILD6, "Pushka(fr)BUILD6");
	RegisterUpgrade(&mortira_BUILD, "Mortira(fr)BUILD");
	RegisterUpgrade(&mortira_BUILD3, "Mortira(fr)BUILD3");
	RegisterUpgrade(&mortira_BUILD4, "Mortira(fr)BUILD4");
	RegisterUpgrade(&mortira_BUILD5, "Mortira(fr)BUILD5");
	RegisterUpgrade(&KUZ03, "KUZ03FR");
	RegisterUpgrade(&AKA01, "AKA01FR");
	RegisterUpgrade(&AKA02, "AKA02FR");
	RegisterUpgrade(&AKA03, "AKA03FR");
	RegisterUpgrade(&AKA04, "AKA04FR");
	RegisterUpgrade(&AKA08, "AKA08FR");
	RegisterUpgrade(&AKA23, "AKA23FR");
	RegisterUpgrade(&AKA24, "AKA24FR");
	RegisterUpgrade(&AKA10, "AKA10FR");
	RegisterUpgrade(&AKA30, "AKA30FR");
	RegisterUpgrade(&AKA07, "AKA07FR");
	RegisterUpgrade(&AKA20, "AKA20FR");
	RegisterUpgrade(&KUZ02, "KUZ02FR");
	RegisterUpgrade(&AKA09, "AKA09FR");
	RegisterUpgrade(&AKA05, "AKA05FR");
	RegisterUpgrade(&AKA28, "AKA28FR");
	RegisterUpgrade(&KUZ05, "KUZ05FR");
	// XVIII century: unlocks Kazarma_FR(fr) + Dragun_18/Gusar_evro/Kirasir.
	RegisterUpgrade(&MAIN, "MAINFR");

	// ----- The rest of the original France.dll registration set -----
	// Tower, yacht and the 17th/18th c. cavalry.
	RegisterUnitType(&bashnia2, "Bashnia(fr)");
	RegisterUnitType(&yahta, "Yahta(fr)");
	RegisterUnitType(&dragun17, "Dragun(fr)");
	RegisterUnitType(&dragun18, "Dragun_France(fr)");
	RegisterUnitType(&dragun18_dip, "Dragun_18_DIP(fr)");
	RegisterUnitType(&konni_ricar, "Konni_Ricar(fr)");
	RegisterUnitType(&sveshenik, "Sveshenik_evro(fr)");
	RegisterUnitType(&strelec_algir, "Strelec_Algir_DIP(fr)");
	// The original copies "Konni_Ricar(fr)" and "Grenader(fr)" through
	// per-game buffers after a GetRandomIndex() call, but the copies are
	// unconditional (vestigial Russia-style pick code), so the
	// registrations are unconditional here too. Grenader(fr) is the
	// 18th c. musketeer from the MAINFR-gated Kazarma.
	RegisterUnitType(&strelec, "Grenader(fr)");
	RegisterUpgrade(&KUZ04, "KUZ04FR");
	RegisterUpgrade(&AKA35, "AKA35FR");
	RegisterUpgrade(&AKA36, "AKA36FR");
	RegisterUpgrade(&AKA32, "AKA32FR");
	RegisterUpgrade(&AKA11, "AKA11FR");
	RegisterUpgrade(&AKA12, "AKA12FR");
	RegisterUpgrade(&AKA13, "AKA13FR");
	RegisterUpgrade(&AKA14, "AKA14FR");
	RegisterUpgrade(&AKA15, "AKA15FR");
	RegisterUpgrade(&AKA31, "AKA31FR");
	RegisterUpgrade(&AKA33, "AKA33FR");
	RegisterUpgrade(&AKA34, "AKA34FR");
	RegisterUpgrade(&KUZ06, "KUZ06FR");
	RegisterUpgrade(&bashnia_PAUSE, "Bashnia(fr)PAUSE");
	RegisterUpgrade(&bashnia_PAUSE3, "Bashnia(fr)PAUSE3");
	RegisterUpgrade(&bashnia_PAUSE4, "Bashnia(fr)PAUSE4");
	RegisterUpgrade(&bashnia_PAUSE5, "Bashnia(fr)PAUSE5");
	RegisterUpgrade(&bashnia_PAUSE6, "Bashnia(fr)PAUSE6");
	RegisterUpgrade(&bashnia_PAUSE7, "Bashnia(fr)PAUSE7");
	RegisterUpgrade(&barabanshik_SHIELD, "Barabanshik_ev1(fr)SHIELD");
	// Unlike Russia (whose 12 "Dragun(RU)..." registrations are dead), the
	// Dragun(fr) and Dragun_France(fr) chains exist in FRANCE.NDS.
	RegisterUpgrade(&dragun_ATTACK, "Dragun(fr)ATTACK");
	RegisterUpgrade(&dragun_ATTACK3, "Dragun(fr)ATTACK3");
	RegisterUpgrade(&dragun_ATTACK4, "Dragun(fr)ATTACK4");
	RegisterUpgrade(&dragun_ATTACK5, "Dragun(fr)ATTACK5");
	RegisterUpgrade(&dragun_ATTACK6, "Dragun(fr)ATTACK6");
	RegisterUpgrade(&dragun_ATTACK7, "Dragun(fr)ATTACK7");
	RegisterUpgrade(&dragun_SHIELD, "Dragun(fr)SHIELD");
	RegisterUpgrade(&dragun_SHIELD3, "Dragun(fr)SHIELD3");
	RegisterUpgrade(&dragun_SHIELD4, "Dragun(fr)SHIELD4");
	RegisterUpgrade(&dragun_SHIELD5, "Dragun(fr)SHIELD5");
	RegisterUpgrade(&dragun_SHIELD6, "Dragun(fr)SHIELD6");
	RegisterUpgrade(&dragun_SHIELD7, "Dragun(fr)SHIELD7");
	RegisterUpgrade(&dragun18_ATTACK, "Dragun_France(fr)ATTACK");
	RegisterUpgrade(&dragun18_ATTACK3, "Dragun_France(fr)ATTACK3");
	RegisterUpgrade(&dragun18_ATTACK4, "Dragun_France(fr)ATTACK4");
	RegisterUpgrade(&dragun18_ATTACK5, "Dragun_France(fr)ATTACK5");
	RegisterUpgrade(&dragun18_ATTACK6, "Dragun_France(fr)ATTACK6");
	RegisterUpgrade(&dragun18_ATTACK7, "Dragun_France(fr)ATTACK7");
	RegisterUpgrade(&dragun18_SHIELD, "Dragun_France(fr)SHIELD");
	RegisterUpgrade(&dragun18_SHIELD3, "Dragun_France(fr)SHIELD3");
	RegisterUpgrade(&dragun18_SHIELD4, "Dragun_France(fr)SHIELD4");
	RegisterUpgrade(&dragun18_SHIELD5, "Dragun_France(fr)SHIELD5");
	RegisterUpgrade(&dragun18_SHIELD6, "Dragun_France(fr)SHIELD6");
	RegisterUpgrade(&dragun18_SHIELD7, "Dragun_France(fr)SHIELD7");

	// Original InitAI (unlike Bavaria, which omits this): an initial
	// food/wood/stone peasant distribution of 9/4/4 before the mine
	// upgrade setup; the per-tick handlers adjust it afterwards.
	SetPDistribution(9, 4, 4);
	AssignAmountOfMineUpgrades(6);
	AssignMineUpgrade(0, "shahta(fr)INSIDE", 90);
	AssignMineUpgrade(1, "shahta(fr)INSIDE3", 50);
	AssignMineUpgrade(2, "shahta(fr)INSIDE4", 50);
	AssignMineUpgrade(3, "shahta(fr)INSIDE5", 50);
	AssignMineUpgrade(4, "shahta(fr)INSIDE6", 50);
	AssignMineUpgrade(5, "shahta(fr)INSIDE7", 50);
	AssignPeasant("Krestian_Sved(fr)");
	AssignMine("shahta(fr)");
	AssignHouse("Dom_France(fr)");
	AssignWall("WALL_EV(fr)");
	// Original France.dll InitAI: 290/90/90 radii and 300 max mine
	// workers (Austria uses 1000).
	SET_MINE_CAPTURE_RADIUS(290);
	SET_MINE_UPGRADE1_RADIUS(90);
	SET_MINE_UPGRADE2_RADIUS(90);
	SET_DEFAULT_MAX_WORKERS(300);
	SET_MIN_PEASANT_BRIGADE(5);
}

extern "C" __declspec(dllexport) void InitAI()
{
	RegisterFrance();
}

extern "C" __declspec(dllexport) void OnInit()
{
	InitAI();
}

// Original France.dll FUN_10003210: pike upgrade chain, proposed once the
// AI has a port and food income on water maps. (Unlike Russia's chain this
// one includes ATTACK4 and no tower-pause upgrade.)
void WaterPikeUpgrades()
{
	TryUpgrade(&pikiner_ATTACK, 60, 100);
	TryUpgrade(&pikiner_ATTACK3, 60, 100);
	TryUpgrade(&pikiner_ATTACK4, 60, 100);
	TryUpgrade(&pikiner_ATTACK5, 20, 100);
	TryUpgrade(&pikiner_ATTACK6, 10, 100);
	TryUpgrade(&pikiner_SHIELD, 60, 100);
	TryUpgrade(&pikiner_SHIELD3, 60, 100);
	TryUpgrade(&pikiner_SHIELD4, 60, 100);
	TryUpgrade(&pikiner_SHIELD5, 20, 100);
	TryUpgrade(&pikiner_SHIELD6, 10, 100);
}

// Original France.dll FUN_100032b0: Mushketer_ev (17th c. musketeer)
// upgrade chain.
void WaterStrelecUpgrades()
{
	TryUpgrade(&pehota_ATTACK, 90, 100);
	TryUpgrade(&pehota_ATTACK3, 90, 100);
	TryUpgrade(&pehota_ATTACK4, 90, 100);
	TryUpgrade(&pehota_SHIELD, 90, 100);
	TryUpgrade(&pehota_SHIELD3, 90, 100);
}

// Original France.dll FUN_10003300: mortar construction upgrades.
void WaterMortiraUpgrades()
{
	TryUpgrade(&mortira_BUILD, 30, 50);
	TryUpgrade(&mortira_BUILD3, 30, 50);
	TryUpgrade(&mortira_BUILD4, 30, 50);
	TryUpgrade(&mortira_BUILD5, 30, 50);
}

// Original France.dll FUN_10006610: pick the mine-upgrade rules table for
// this tick. ZERO is the idle default, B (gold-only) runs while the pike
// ATTACK4/SHIELD4 upgrades are in progress, C is the full table for water
// maps (and for land maps once AKA24 is running).
void ApplyMineUpgradeRules(int land)
{
	SetMinesUpgradeRules(MINE_UPG_RULES_ZERO);
	if (UpgIsRun(&pikiner_ATTACK4) && UpgIsRun(&pikiner_SHIELD4))
	{
		SetMinesUpgradeRules(MINE_UPG_RULES_B);
		return;
	}
	if (land == LT_PENINSULAS || land == LT_ISLANDS || land == LT_CONTINENTS)
	{
		SetMinesUpgradeRules(MINE_UPG_RULES_C);
		return;
	}
	if (UpgIsRun(&AKA08) && !UpgIsRun(&AKA24))
	{
		SetMinesUpgradeRules(MINE_UPG_RULES_ZERO);
		return;
	}
	if (UpgIsRun(&AKA24))
	{
		SetMinesUpgradeRules(MINE_UPG_RULES_C);
	}
}

// Original France.dll FUN_10003340: the AI script for the water map types
// (Peninsulas, Islands, Continents). Ported from the Ghidra decompilation
// of France.dll; TryUnit/TryUpgrade arguments, thresholds and gates are
// kept as-is.
void ProcessWaterAI()
{
	// City guard budget (SetDefSettings -> MaxGuards/AbsMaxGuards) and
	// non-defensive stance, as the original does every water-map tick
	SetDefSettings(300, 400);
	SetDefenseState(0);

	int galeras = GetUnitsByUsage(0, USAGE_GALERA);
	int fregats = GetUnitsByUsage(0, USAGE_FREGAT);
	int yachts = GetUnitsByUsage(0, USAGE_IAXTA);
	int linkors = GetUnitsByUsage(0, USAGE_LINKOR);
	bool mainDone = UpgIsDone(&MAIN);
	// War-fleet strength proxy used for the frigate targets below
	int fleet = (yachts >> 2) + fregats + linkors * 4;

	int portReady = GetReadyUnits(&port);
	int centers = GetUnits(&center);
	int centersReady = GetReadyUnits(&center);
	int dips = GetUnits(&diplomatic);
	int dipsReady = GetReadyUnits(&diplomatic);
	int kons = GetUnits(&konushnia);
	int konsReady = GetReadyUnits(&konushnia);
	int ports = GetUnits(&port);
	// Own buildings still under construction (used to pace new projects)
	int building = (kons - konsReady) - dipsReady - centersReady - portReady + dips + ports + centers;

	// XVIII century is researched on water maps, unconditionally
	TryUpgrade(&MAIN, 95, 100);

	// While hoarding resources for the XVIII century, don't spam peasants
	bool wantPeasants = !(GetMoney(FOOD) > 55000 && GetMoney(GOLD) > 10000
		&& GetMoney(IRON) > 7000 && !mainDone);

	int peasants = GetUnits(&krestian);
	int food = GetMoney(FOOD);
	bool kuz03Running = UpgIsRun(&KUZ03) != 0;
	int strelecs = GetUnits(&pehota);
	// Strike-group proxy: Konni_Ricar + Kirasir + (unlike Russia, which
	// counts grenadiers here) the 17th c. pikemen - kept as decompiled
	int elite = GetUnits(&konni_ricar) + GetUnits(&mameluk) + GetUnits(&pikiner);

	TryUpgrade(&AKA23, 99, 80);
	TryUpgrade(&AKA24, 98, 80);
	int towersReady = GetReadyUnits(&bashnia2);
	int towers = GetUnits(&bashnia2);

	if (peasants != 0)
	{
		TryUnit(&melnica, 1, 100, 100);
		if (!GetUnits(&melnica))
		{
			return;
		}
		TryUpgrade(&Melnica_tuGETRES, 100, 100);
		if (!FieldExist())
		{
			// No fields yet: quick peasants and pikes, everyone on food
			if (food > 700)
			{
				TryUnit(&krestian, 100, 100, 100);
				TryUnit(&pikiner, 100, 100, 100);
			}
			SetPDistribution(0, 1, 1);
		}
		else
		{
			// Peasant distribution by food stock and AKA30 (market trade)
			int dFood, dWood, dStone;
			if (!UpgIsDone(&AKA30))
			{
				if (food < 12001)
				{
					if (food < 6001)
					{
						dStone = 4; dWood = 4; dFood = 10;
					}
					else
					{
						dStone = 5; dWood = 5; dFood = 9;
					}
				}
				else
				{
					dStone = 8; dWood = 8; dFood = 9;
				}
			}
			else if (food < 12001)
			{
				if (food < 6001)
				{
					dStone = 1; dWood = 4; dFood = 9;
				}
				else
				{
					dStone = 3; dWood = 5; dFood = 9;
				}
			}
			else
			{
				dStone = 6; dWood = 8; dFood = 9;
			}
			SetPDistribution(dFood, dWood, dStone);

			int pikeTarget = kuz03Running ? 300 : 350;
			if (peasants < 25 && food > 300)
			{
				TryUnit(&krestian, 100, 100, 100);
			}
			if (food > 200)
			{
				if (wantPeasants)
				{
					TryUnit(&krestian, 1000, 232, 100);
				}
				if (food > 300)
				{
					TryUnit(&pikiner, pikeTarget, 100, 30);
				}
			}
			if (UpgIsDone(&AKA08) && food > 1000 && GetUnits(&port) != 0)
			{
				WaterPikeUpgrades();
			}
		}
		// The rest of the tick runs regardless of FieldExist(): a
		// fresh town without fields still builds its hall, store,
		// barracks and ports (only the peasant distribution differs).

		// Galleys: grow the fleet slowly from what is already afloat
		int galTarget = galeras < 5 ? 5 : galeras + 2;
		TryUnit(&galera, galTarget, 70, 50);
		if (fleet < 5)
		{
			TryUnit(&shebeka, 12, 70, 50);
		}
		else
		{
			TryUnit(&shebeka, fleet + 6, 30, 50);
			TryUnit(&shebeka, fleet + 9, 15, 50);
			TryUnit(&shebeka, fleet + 10, 10, 50);
		}
		if (GetUnitsByUsage(0, USAGE_PAROM) != 0)
		{
			// Has ferries: upgrade the towers
			TryUpgrade(&bashnia_PAUSE, 50, 50);
			TryUpgrade(&bashnia_PAUSE3, 50, 50);
			TryUpgrade(&bashnia_PAUSE4, 50, 50);
			TryUpgrade(&bashnia_PAUSE5, 50, 50);
			TryUpgrade(&bashnia_PAUSE6, 50, 50);
			TryUpgrade(&bashnia_PAUSE7, 50, 50);
		}
		TryUnit(&peres_kor, 3, 90, 10);
		TryUnit(&center, 1, 100, 100);
		TryUnit(&center, 2, 95, 5);
		if (building == 0)
		{
			TryUnit(&center, 3, 95, 5);
		}
		TryUnit(&kuznia, 1, 100, 100);
		TryUnit(&barack, 1, 100, 100);
		TryUnit(&sclad, 1, 50, 50);
		TryUnit(&kuznia, 2, 10, 5);
		TryUnit(&konushnia, 1, 99, 90);
		TryUpgrade(&KUZ01, 80, 100);
		if (GetUnits(&minaret) != 0)
		{
			TryUnit(&rinok, 1, 90, 25);
			TryUnit(&mechet, 1, 5, 3);
		}
		TryUnit(&minaret, 1, 90, 90);
		if (building == 0)
		{
			TryUnit(&diplomatic, 1, 99, 10);
		}
		TryUnit(&art_depo, 1, 90, 90);
		if (peasants > 25)
		{
			TryUnit(&melnica, 2, 30, 30);
		}
		TryUnit(&kazarma, 1, 90, 20);
		TryUnit(&kazarma, 2, 50, 5);
		TryUpgrade(&AKA01, 90, 99);
		TryUpgrade(&AKA02, 90, 99);
		TryUpgrade(&AKA03, 90, 99);
		TryUpgrade(&AKA04, 90, 99);
		TryUpgrade(&AKA08, 95, 100);
		TryUpgrade(&KUZ03, 99, 100);
		// Ports: up to five, no gate
		TryUnit(&port, 1, 80, 20);
		TryUnit(&port, 2, 50, 5);
		TryUnit(&port, 3, 30, 5);
		TryUnit(&port, 5, 20, 5);
		TryUpgrade(&AKA06, 95, 100);
		int mineCap = GetMaxPeasantsInMines();
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
		if (UpgIsDone(&mortira_BUILD5))
		{
			TryUnit(&mortira, 10, 10, 90);
			TryUnit(&mortira, 15, 3, 10);
		}
		// Gold-upgrade lock: forbid gold-costly upgrades while saving
		// up for the XVIII-century research
		SetUpgradeLock(GOLD, 0);
		if (GetMoney(WOOD) > 14000 && UpgIsRun(&AKA30) && GetMoney(GOLD) < 7500
			&& !UpgIsRun(&AKA06))
		{
			SetUpgradeLock(GOLD, 1);
		}
		if (UpgIsRun(&AKA06) && UpgIsRun(&AKA07) && UpgIsDone(&KUZ03))
		{
			// mineCap values are the max workers per (upgraded) mine
			if ((mineCap == 10 || mineCap == 18) && food > 50000
				&& GetMoney(GOLD) < 8200)
			{
				SetUpgradeLock(GOLD, 1);
			}
			if (!UpgIsRun(&MAIN) && food > 60000 && GetMoney(GOLD) < 11000)
			{
				SetUpgradeLock(GOLD, 1);
			}
			if (UpgIsDone(&MAIN))
			{
				if (mineCap == 28)
				{
					if (food > 50000 && GetMoney(COAL) > 7300
						&& GetMoney(GOLD) < 20000)
					{
						SetUpgradeLock(GOLD, 1);
					}
				}
				else if (mineCap == 40)
				{
					if (food > 50000 && GetMoney(COAL) > 9800
						&& GetMoney(GOLD) < 24000)
					{
						SetUpgradeLock(GOLD, 1);
					}
				}
				else if (mineCap == 55)
				{
					if (food > 80000 && GetMoney(GOLD) < 26000)
					{
						SetUpgradeLock(GOLD, 1);
					}
				}
			}
		}
		// Fully upgraded mines (capacity 95): battleships + AKA29
		if (mineCap == 95)
		{
			TryUpgrade(&AKA29, 70, 10);
			TryUnit(&linkor, 1, 50, 30);
			TryUnit(&linkor, 2, 30, 30);
			TryUnit(&linkor, 3, 20, 30);
			TryUnit(&linkor, 5, 10, 30);
		}
		// Towers once the economy is large and all towers are ready
		if (peasants > 80 && towers == towersReady)
		{
			TryUnit(&bashnia2, 1, 80, 10);
			if (peasants > 95)
			{
				TryUnit(&bashnia2, 2, 80, 10);
				TryUnit(&bashnia2, 3, 50, 10);
				TryUnit(&bashnia2, 4, 30, 10);
			}
		}
		// 18th c. units once the academy chain is underway
		if (kuz03Running && UpgIsDone(&AKA08))
		{
			TryUnit(&grenader, 75, 1, 30);
			// (the original also queries these two and drops the
			// results; kept to mirror the flow)
			UpgIsDone(&pushka_BUILD5);
			UpgIsDone(&mortira_BUILD5);
		}
		// (original queries the art depository here and drops it)
		GetUnits(&art_depo);
		TryUnit(&lodka, 3, 90, 90);
		if (UpgIsDone(&AKA07))
		{
			TryUnit(&lodka, 20, 10, 90);
		}
		if (kuz03Running && UpgIsDone(&AKA08))
		{
			if (building == 0)
			{
				TryUnit(&konushnia, 2, 90, 20);
				TryUnit(&konushnia, 3, 90, 20);
				TryUnit(&konushnia, 4, 90, 20);
			}
			if (strelecs > 20)
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
		// Enough strike troops -> mass 17th c. musketeers
		if (GetUnits(&barack) > 1 && elite > 300)
		{
			if (strelecs * 5 < elite - 50)
			{
				TryUnit(&pehota, 500, 10, 10);
			}
			WaterStrelecUpgrades();
		}
	}

	if (kuz03Running)
	{
		TryUpgrade(&AKA34, 95, 90);
		TryUpgrade(&KUZ06, 95, 90);
		TryUpgrade(&KUZ05, 95, 90);
		TryUnit(&barack, 2, 95, 90);
	}
	if (UpgIsRun(&AKA30))
	{
		TryUpgrade(&KUZ02, 95, 90);
		TryUpgrade(&AKA09, 95, 90);
	}

	// ----- Market trades (original tail of FUN_100033e0) -----
	if (GetReadyUnits(&rinok) == 0)
	{
		return;
	}
	if (GetUnits(&minaret) != 0 && !UpgIsRun(&AKA08) && GetMoney(GOLD) > 800
		&& GetMoney(FOOD) < 6000 && GetMoney(IRON) > 3000)
	{
		AI_Torg(IRON, FOOD, 3000);
	}
	if (GetMoney(FOOD) < 1000 && GetMoney(STONE) > 5000)
	{
		AI_Torg(STONE, FOOD, 4000);
	}
	if (!UpgIsDone(&KUZ02) || !UpgIsDone(&AKA09))
	{
		if (GetMoney(STONE) >= 110001)
		{
			AI_Torg(STONE, GOLD, 90000);
		}
	}
	else
	{
		if (GetMoney(STONE) > 110000)
		{
			AI_Torg(STONE, GOLD, 100000);
		}
		if (GetMoney(WOOD) != 0 && GetMoney(GOLD) != 0 && GetMoney(IRON) < 5000)
		{
			if (GetMoney(COAL) < GetMoney(IRON))
			{
				AI_Torg(STONE, COAL, 25000);
				AI_Torg(STONE, IRON, 25000);
			}
			else
			{
				AI_Torg(STONE, IRON, 25000);
				AI_Torg(STONE, COAL, 25000);
			}
		}
		else if (GetMoney(WOOD) != 0)
		{
			// The original issues this trade twice when coal is low and
			// there is wood (kept as-is)
			if (GetMoney(COAL) < 5000)
			{
				AI_Torg(STONE, COAL, 50000);
			}
			if (GetMoney(COAL) <= 4999)
			{
				AI_Torg(STONE, COAL, 50000);
			}
		}
	}
	if (GetMoney(WOOD) > 150000)
	{
		AI_Torg(WOOD, GOLD, 100000);
	}
	if (GetMoney(IRON) > 80000)
	{
		AI_Torg(IRON, GOLD, 40000);
	}
	if (GetMoney(COAL) > 80000)
	{
		AI_Torg(COAL, GOLD, 40000);
	}
}

extern "C" __declspec(dllexport) void ProcessAI()
{
	// Original dispatcher FUN_10006d20 (shared European skeleton),
	// including the real land pipeline and Mediterranean handler.
	EuropeanProcessAI();
}

// Linux: this constructor runs at dlopen() time, like DllMain(DLL_PROCESS_ATTACH).
__attribute__((constructor)) void AI_OnLoad()
{
	LOAD_FUNCTION(RegisterUnitType);
	LOAD_FUNCTION(RegisterUpgrade);
	LOAD_FUNCTION(AI_Torg);
	LOAD_FUNCTION(TryUnit);
	LOAD_FUNCTION(TryUpgrade);
	LOAD_FUNCTION(SetMineBalanse);
	LOAD_FUNCTION(SetPDistribution);
	LOAD_FUNCTION(AssignAmountOfMineUpgrades);
	LOAD_FUNCTION(AssignMineUpgrade);
	LOAD_FUNCTION(AssignMine);
	LOAD_FUNCTION(AssignPeasant);
	LOAD_FUNCTION(AssignHouse);
	LOAD_FUNCTION(AssignWall);
	LOAD_FUNCTION(AssignFormUnit);
	LOAD_FUNCTION(SET_MINE_CAPTURE_RADIUS);
	LOAD_FUNCTION(SET_MINE_UPGRADE1_RADIUS);
	LOAD_FUNCTION(SET_MINE_UPGRADE2_RADIUS);
	LOAD_FUNCTION(SET_DEFAULT_MAX_WORKERS);
	LOAD_FUNCTION(SET_MIN_PEASANT_BRIGADE);
	LOAD_FUNCTION(GetMoney);
	LOAD_FUNCTION(GetUnits);
	LOAD_FUNCTION(GetReadyUnits);
	LOAD_FUNCTION(UpgIsDone);
	LOAD_FUNCTION(UpgIsRun);
	LOAD_FUNCTION(FieldExist);
	LOAD_FUNCTION(GetDifficulty);
	LOAD_FUNCTION(GetStartRes);
	LOAD_FUNCTION(GetLandType);
	LOAD_FUNCTION(GetAIRegister);
	LOAD_FUNCTION(SetAIRegister);
	LOAD_FUNCTION(SetDefSettings);
	LOAD_FUNCTION(SetDefenseState);
	LOAD_FUNCTION(SetUpgradeLock);
	LOAD_FUNCTION(SetMinesBuildingRules);
	LOAD_FUNCTION(SetMinesUpgradeRules);
	LOAD_FUNCTION(GetMaxPeasantsInMines);
	LOAD_FUNCTION(GetUnitsByUsage);
	LOAD_FUNCTION(GetRandomIndex);
	LOAD_FUNCTION_EUROPEAN();
	OnInit();
}
