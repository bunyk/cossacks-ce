#include <cstdio>
#include "../common/ai_common.h"

#include "objects.h"
// Mine balance tables of the original England.dll (extracted from its .data
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


// Registers England's units/upgrades with the game. Names must match the
// per-nation script ENGLAND.NDS (listed in Nations.lst, packed in all.gsc).
// All strings below are byte-exact from the original England.dll string
// table (Ghidra + raw-string dump; see tools/ai_port.py which generated
// this module from the England template by positional handler alignment).
// Role notes for this nation:
//   - 17th c. musketeer: Mushketer_ev(en); 18th c. shooter pick: Grenader(en), Konni_Ricar(en)
//   - town hall: Center_British(en); house: Dom_English(en); peasant: Krestian_Sved(en)
//   - 18th c. barracks Kazarma_BR(en) is MAIN-gated (ENABLE UNITS GRP 4)
// The water-map AI and routing are 1:1 with the decompilation; the land
// pipeline and the Mediterranean handler keep the Turkey-lineage
// fallbacks (see docs/AI.md "Known gaps").
void RegisterEngland()
{
	RegisterUnitType(&pikiner, "Pikiner_evro(en)");
	RegisterUnitType(&krestian, "Krestian_Sved(en)");
	RegisterUnitType(&pehota, "Mushketer_ev(en)");
	RegisterUnitType(&grenader, "Grenader_DIP(en)");
	RegisterUnitType(&mameluk, "Kirasir(en)");
	RegisterUnitType(&pushka, "Pushka(en)");
	RegisterUnitType(&mortira, "Mortira(en)");
	RegisterUnitType(&mortira_b, "Mortira_b(en)");
	RegisterUnitType(&lodka, "Lodka(en)");
	// England's 18th-century barracks. Enabled only by the MAINEN (XVIII
	// century) research - "ENABLE UNITS GRP 4 Kazarma_BR(en) Dragun_18(en)
	// Gusar_evro(BA) Kirasir(en)" in ENGLAND.NDS.
	RegisterUnitType(&kazarma, "Kazarma_BR(en)");
	AssignFormUnit("Pikiner_evro(en)");
	RegisterUnitType(&kozak, "Kozak_loshad_DIP(en)");
	RegisterUnitType(&oficer, "Officer_evro(en)");
	RegisterUnitType(&barabanshik, "Barabanshik_ev1(en)");
	RegisterUpgrade(&officer_ATTACK, "Officer_evro(en)ATTACK");
	RegisterUpgrade(&officer_SHIELD, "Officer_evro(en)SHIELD");
	RegisterUnitType(&center, "Center_British(en)");
	RegisterUnitType(&melnica, "Melnica(en)");
	RegisterUnitType(&barack, "Kazarma_1_BR(en)");
	RegisterUnitType(&sclad, "Sclad1(en)");
	RegisterUnitType(&minaret, "akademia_BR(en)");
	RegisterUnitType(&konushnia, "konushnia_BR(en)");
	RegisterUnitType(&diplomatic, "Dip_korpus_BR(en)");
	RegisterUnitType(&art_depo, "artileri_depo_BR(en)");
	RegisterUnitType(&rinok, "Rinok(en)");
	RegisterUnitType(&kuznia, "Kuznica_BR(en)");
	RegisterUnitType(&mechet, "Cerkov_BR(en)");
	RegisterUnitType(&port, "PorE(en)");
	RegisterUnitType(&galera, "GALERA(en)");
	RegisterUnitType(&peres_kor, "PERES_KOR(en)");
	RegisterUnitType(&shebeka, "Fregat(en)");
	RegisterUnitType(&linkor, "Linkor(en)");
	RegisterUpgrade(&AKA29, "AKA29EN");
	RegisterUpgrade(&AKA06, "AKA06EN");
	RegisterUpgrade(&Melnica_tuGETRES, "Melnica(en)GETRES");
	RegisterUpgrade(&Melnica_GETRES2, "Melnica(en)GETRES2");
	RegisterUpgrade(&KUZ01, "KUZ01EN");
	RegisterUpgrade(&pikiner_ATTACK, "Pikiner_evro(en)ATTACK");
	RegisterUpgrade(&pikiner_ATTACK3, "Pikiner_evro(en)ATTACK3");
	RegisterUpgrade(&pikiner_ATTACK4, "Pikiner_evro(en)ATTACK4");
	RegisterUpgrade(&pikiner_ATTACK5, "Pikiner_evro(en)ATTACK5");
	RegisterUpgrade(&pikiner_ATTACK6, "Pikiner_evro(en)ATTACK6");
	RegisterUpgrade(&pikiner_SHIELD, "Pikiner_evro(en)SHIELD");
	RegisterUpgrade(&pikiner_SHIELD3, "Pikiner_evro(en)SHIELD3");
	RegisterUpgrade(&pikiner_SHIELD4, "Pikiner_evro(en)SHIELD4");
	RegisterUpgrade(&pikiner_SHIELD5, "Pikiner_evro(en)SHIELD5");
	RegisterUpgrade(&pikiner_SHIELD6, "Pikiner_evro(en)SHIELD6");
	RegisterUpgrade(&pehota_ATTACK, "Mushketer_ev(en)ATTACK");
	RegisterUpgrade(&pehota_ATTACK3, "Mushketer_ev(en)ATTACK3");
	RegisterUpgrade(&pehota_ATTACK4, "Mushketer_ev(en)ATTACK4");
	RegisterUpgrade(&pehota_SHIELD, "Mushketer_ev(en)SHIELD");
	RegisterUpgrade(&pehota_SHIELD3, "Mushketer_ev(en)SHIELD3");
	RegisterUpgrade(&pehota_SHIELD4, "Mushketer_ev(en)SHIELD4");
	RegisterUpgrade(&mameluk_ATTACK, "Kirasir(en)ATTACK");
	RegisterUpgrade(&mameluk_ATTACK3, "Kirasir(en)ATTACK3");
	RegisterUpgrade(&mameluk_ATTACK4, "Kirasir(en)ATTACK4");
	RegisterUpgrade(&mameluk_ATTACK5, "Kirasir(en)ATTACK5");
	RegisterUpgrade(&mameluk_ATTACK6, "Kirasir(en)ATTACK6");
	RegisterUpgrade(&mameluk_ATTACK7, "Kirasir(en)ATTACK7");
	RegisterUpgrade(&mameluk_SHIELD, "Kirasir(en)SHIELD");
	RegisterUpgrade(&mameluk_SHIELD3, "Kirasir(en)SHIELD3");
	RegisterUpgrade(&mameluk_SHIELD4, "Kirasir(en)SHIELD4");
	RegisterUpgrade(&mameluk_SHIELD5, "Kirasir(en)SHIELD5");
	RegisterUpgrade(&mameluk_SHIELD6, "Kirasir(en)SHIELD6");
	RegisterUpgrade(&mameluk_SHIELD7, "Kirasir(en)SHIELD7");
	RegisterUpgrade(&pushka_BUILD, "Pushka(en)BUILD");
	RegisterUpgrade(&pushka_BUILD3, "Pushka(en)BUILD3");
	RegisterUpgrade(&pushka_BUILD4, "Pushka(en)BUILD4");
	RegisterUpgrade(&pushka_BUILD5, "Pushka(en)BUILD5");
	RegisterUpgrade(&pushka_BUILD6, "Pushka(en)BUILD6");
	RegisterUpgrade(&mortira_BUILD, "Mortira(en)BUILD");
	RegisterUpgrade(&mortira_BUILD3, "Mortira(en)BUILD3");
	RegisterUpgrade(&mortira_BUILD4, "Mortira(en)BUILD4");
	RegisterUpgrade(&mortira_BUILD5, "Mortira(en)BUILD5");
	RegisterUpgrade(&KUZ03, "KUZ03EN");
	RegisterUpgrade(&AKA01, "AKA01EN");
	RegisterUpgrade(&AKA02, "AKA02EN");
	RegisterUpgrade(&AKA03, "AKA03EN");
	RegisterUpgrade(&AKA04, "AKA04EN");
	RegisterUpgrade(&AKA08, "AKA08EN");
	RegisterUpgrade(&AKA23, "AKA23EN");
	RegisterUpgrade(&AKA24, "AKA24EN");
	RegisterUpgrade(&AKA10, "AKA10EN");
	RegisterUpgrade(&AKA30, "AKA30EN");
	RegisterUpgrade(&AKA07, "AKA07EN");
	RegisterUpgrade(&AKA20, "AKA20EN");
	RegisterUpgrade(&KUZ02, "KUZ02EN");
		RegisterUpgrade(&AKA05, "AKA05EN");
		RegisterUpgrade(&KUZ05, "KUZ05EN");
	// XVIII century: unlocks Kazarma_BR(en) + Dragun_18/Gusar_evro/Kirasir.
	RegisterUpgrade(&MAIN, "MAINEN");

	// ----- The rest of the original England.dll registration set -----
	// Tower, yacht and the 17th/18th c. cavalry.
	RegisterUnitType(&bashnia2, "Bashnia(en)");
	RegisterUnitType(&yahta, "Yahta(en)");
	RegisterUnitType(&dragun17, "Dragun(en)");
	RegisterUnitType(&dragun18, "Dragun_18(en)");
	RegisterUnitType(&dragun18_dip, "Dragun_18_DIP(en)");
	RegisterUnitType(&konni_ricar, "Konni_Ricar(en)");
	RegisterUnitType(&sveshenik, "Sveshenik_evro(en)");
	RegisterUnitType(&strelec_algir, "Strelec_Algir_DIP(en)");
	// The original copies "Konni_Ricar(en)" and "Grenader(en)" through
	// per-game buffers after a GetRandomIndex() call, but the copies are
	// unconditional (vestigial Russia-style pick code), so the
	// registrations are unconditional here too. Grenader(en) is the
	// 18th c. musketeer from the MAINEN-gated Kazarma.
	RegisterUnitType(&strelec, "Grenader(en)");
	RegisterUpgrade(&KUZ04, "KUZ04EN");
	RegisterUpgrade(&AKA35, "AKA35EN");
	RegisterUpgrade(&AKA36, "AKA36EN");
	RegisterUpgrade(&AKA32, "AKA32EN");
	RegisterUpgrade(&AKA11, "AKA11EN");
	RegisterUpgrade(&AKA12, "AKA12EN");
	RegisterUpgrade(&AKA13, "AKA13EN");
	RegisterUpgrade(&AKA14, "AKA14EN");
		RegisterUpgrade(&AKA31, "AKA31EN");
	RegisterUpgrade(&AKA33, "AKA33EN");
	RegisterUpgrade(&AKA34, "AKA34EN");
	RegisterUpgrade(&KUZ06, "KUZ06EN");
	RegisterUpgrade(&bashnia_PAUSE, "Bashnia(en)PAUSE");
	RegisterUpgrade(&bashnia_PAUSE3, "Bashnia(en)PAUSE3");
	RegisterUpgrade(&bashnia_PAUSE4, "Bashnia(en)PAUSE4");
		RegisterUpgrade(&bashnia_PAUSE6, "Bashnia(en)PAUSE6");
	RegisterUpgrade(&bashnia_PAUSE7, "Bashnia(en)PAUSE7");
	RegisterUpgrade(&barabanshik_SHIELD, "Barabanshik_ev1(en)SHIELD");
	// Unlike Russia (whose 12 "Dragun(RU)..." registrations are dead), the
	// Dragun(en) and Dragun_18(en) chains exist in ENGLAND.NDS.
	RegisterUpgrade(&dragun_ATTACK, "Dragun(en)ATTACK");
	RegisterUpgrade(&dragun_ATTACK3, "Dragun(en)ATTACK3");
	RegisterUpgrade(&dragun_ATTACK4, "Dragun(en)ATTACK4");
	RegisterUpgrade(&dragun_ATTACK5, "Dragun(en)ATTACK5");
	RegisterUpgrade(&dragun_ATTACK6, "Dragun(en)ATTACK6");
	RegisterUpgrade(&dragun_ATTACK7, "Dragun(en)ATTACK7");
	RegisterUpgrade(&dragun_SHIELD, "Dragun(en)SHIELD");
	RegisterUpgrade(&dragun_SHIELD3, "Dragun(en)SHIELD3");
	RegisterUpgrade(&dragun_SHIELD4, "Dragun(en)SHIELD4");
	RegisterUpgrade(&dragun_SHIELD5, "Dragun(en)SHIELD5");
	RegisterUpgrade(&dragun_SHIELD6, "Dragun(en)SHIELD6");
	RegisterUpgrade(&dragun_SHIELD7, "Dragun(en)SHIELD7");
	RegisterUpgrade(&dragun18_ATTACK, "Dragun_18(en)ATTACK");
	RegisterUpgrade(&dragun18_ATTACK3, "Dragun_18(en)ATTACK3");
	RegisterUpgrade(&dragun18_ATTACK4, "Dragun_18(en)ATTACK4");
	RegisterUpgrade(&dragun18_ATTACK5, "Dragun_18(en)ATTACK5");
	RegisterUpgrade(&dragun18_ATTACK6, "Dragun_18(en)ATTACK6");
	RegisterUpgrade(&dragun18_ATTACK7, "Dragun_18(en)ATTACK7");
	RegisterUpgrade(&dragun18_SHIELD, "Dragun_18(en)SHIELD");
	RegisterUpgrade(&dragun18_SHIELD3, "Dragun_18(en)SHIELD3");
	RegisterUpgrade(&dragun18_SHIELD4, "Dragun_18(en)SHIELD4");
	RegisterUpgrade(&dragun18_SHIELD5, "Dragun_18(en)SHIELD5");
	RegisterUpgrade(&dragun18_SHIELD6, "Dragun_18(en)SHIELD6");
	RegisterUpgrade(&dragun18_SHIELD7, "Dragun_18(en)SHIELD7");

	// Original InitAI (unlike Bavaria, which omits this): an initial
	// food/wood/stone peasant distribution of 9/4/4 before the mine
	// upgrade setup; the per-tick handlers adjust it afterwards.
	SetPDistribution(9, 4, 4);
	AssignAmountOfMineUpgrades(6);
	AssignMineUpgrade(0, "shahta(en)INSIDE", 90);
	AssignMineUpgrade(1, "shahta(en)INSIDE3", 50);
	AssignMineUpgrade(2, "shahta(en)INSIDE4", 50);
	AssignMineUpgrade(3, "shahta(en)INSIDE5", 50);
	AssignMineUpgrade(4, "shahta(en)INSIDE6", 50);
	AssignMineUpgrade(5, "shahta(en)INSIDE7", 50);
	AssignPeasant("Krestian_Sved(en)");
	AssignMine("shahta(en)");
	AssignHouse("Dom_English(en)");
	AssignWall("WALL_EV(en)");
	// Original England.dll InitAI: 290/90/90 radii and 300 max mine
	// workers (Austria uses 1000).
	SET_MINE_CAPTURE_RADIUS(290);
	SET_MINE_UPGRADE1_RADIUS(90);
	SET_MINE_UPGRADE2_RADIUS(90);
	SET_DEFAULT_MAX_WORKERS(300);
	SET_MIN_PEASANT_BRIGADE(5);
}

extern "C" __declspec(dllexport) void InitAI()
{
	RegisterEngland();
}

extern "C" __declspec(dllexport) void OnInit()
{
	InitAI();
}

// Turkey-lineage Mediterranean fallback. England.dll has its own
// ProcessMiddleSeaAI@100043c0 (not yet ported); the port/MAIN values below
// are its real ones.
void ProcessNonLandAI(int landType)
{
	SetMineBalanse(19, LAND_MINE_BALANCE);

	int wood = GetMoney(0);
	int gold = GetMoney(1);
	int stone = GetMoney(2);
	int food = GetMoney(3);
	int iron = GetMoney(4);
	int coal = GetMoney(5);
	int stoneLast = stone;

	TryUnit(&melnica, 1, 100, 100);
	if (GetUnits(&melnica))
	{
		TryUnit(&center, 2, 100, 100);
		if (GetUnits(&center))
		{
			TryUnit(&kuznia, 1, 100, 100);
			TryUnit(&barack, 1, 100, 100);
			if (GetUnits(&minaret))
			{
				TryUnit(&barack, 2, 100, 100);
				if (GetUnits(&center) > 1)
				{
					if (food > 1000)
					{
						TryUnit(&barack, 3, 100, 100);
					}
					TryUnit(&barack, 4, 50, 100);
					TryUnit(&barack, 5, 30, 100);
				}
			}
			TryUnit(&sclad, 1, 100, 100);
			if (FieldExist())
			{
				if (food > 1200)
				{
					TryUnit(&sclad, 3, 10, 2);
					if (food > 2000)
					{
						TryUnit(&sclad, 5, 10, 1);
						TryUnit(&sclad, 6, 3, 1);
						TryUnit(&sclad, 7, 2, 1);
						TryUnit(&rinok, 1, 100, 100);
					}
				}
			}
			if (GetUnits(&minaret) && GetUnits(&barack) > 1)
			{
				TryUnit(&konushnia, 2, 100, 100);
				TryUnit(&konushnia, 3, 90, 100);
				TryUnit(&konushnia, 4, 90, 100);
				TryUnit(&konushnia, 5, 90, 100);
				TryUnit(&art_depo, 1, 100, 100);
			}
			TryUnit(&minaret, 1, 90, 100);
			// Original England.dll ProcessMiddleSeaAI: three ports, no
			// extra gate.
			TryUnit(&port, 1, 50, 90);
			TryUnit(&port, 2, 20, 50);
			TryUnit(&port, 3, 10, 50);
			if (GetUnits(&barack))
				TryUnit(&melnica, 2, 10, 10);
			TryUnit(&diplomatic, 1, 90, 10);
			stone = stoneLast;
		}
	}
	TryUpgrade(&Melnica_tuGETRES, 100, 100);
	if (UpgIsRun(&Melnica_tuGETRES))
	{
		if (food > 700)
		{
			if (landType == LT_MEDITERRANEAN && GetUnits(&pehota) < 100)
			{
				TryUnit(&pikiner, 400, 20, 40);
				TryUnit(&pehota, 200, 10, 100);
			}
			else if (landType == LT_MEDITERRANEAN)
			{
				TryUnit(&strelec, 800, 20, 60);
				TryUnit(&pikiner, 800, 20, 100);
			}
			else
			{
				TryUnit(&strelec, 400, 20, 60);
				TryUnit(&pikiner, 400, 20, 100);
			}
		}
		if (food > 250)
			TryUnit(&krestian, 1000, 100, 100);
		if (food > 300)
		{
			TryUnit(&mameluk, 400, 20, 100);
			TryUnit(&mameluk, 400, 20, 100);
			if (gold > 1000)
				TryUnit(&grenader, 70, 100, 100);
		}
		TryUpgrade(&AKA01, 99, 100);
		TryUpgrade(&KUZ01, 99, 100);
	}
	if (landType == LT_MEDITERRANEAN && GetUnits(&krestian) > 45)
	{
		TryUpgrade(&pehota_ATTACK, 30, 50);
		TryUpgrade(&pehota_ATTACK3, 30, 50);
		TryUpgrade(&pehota_ATTACK4, 30, 50);
		TryUpgrade(&pehota_SHIELD, 30, 50);
		TryUpgrade(&pehota_SHIELD3, 30, 50);
		TryUpgrade(&pehota_SHIELD4, 30, 50);
	}
	TryUpgrade(&KUZ03, 100, 100);
	TryUpgrade(&AKA08, 98, 100);
	TryUpgrade(&AKA23, 99, 100);
	TryUpgrade(&AKA04, 99, 100);
	TryUpgrade(&AKA01, 99, 100);
	TryUpgrade(&AKA02, 99, 100);
	TryUpgrade(&AKA03, 99, 100);
	TryUpgrade(&AKA30, 100, 100);
	TryUpgrade(&AKA06, 99, 100);
	if (UpgIsDone(&KUZ03))
	{
		if (landType == 1)
		{
			TryUnit(&mortira_b, 40, 10, 50);
		}
		TryUpgrade(&AKA24, 99, 100);
		TryUpgrade(&AKA07, 99, 100);
		if (UpgIsDone(&AKA07))
		{
			TryUnit(&lodka, 40, 10, 50);
			TryUpgrade(&AKA05, 99, 100);
		}
		TryUpgrade(&KUZ05, 95, 100);
		if (landType == LT_MEDITERRANEAN)
		{
			TryUpgrade(&pushka_BUILD, 30, 50);
			TryUpgrade(&pushka_BUILD3, 30, 50);
			TryUpgrade(&pushka_BUILD4, 30, 50);
			TryUpgrade(&pushka_BUILD5, 30, 50);
		}
		TryUpgrade(&mortira_BUILD, 30, 50);
		TryUpgrade(&mortira_BUILD3, 30, 50);
		TryUpgrade(&mortira_BUILD4, 30, 50);
		TryUpgrade(&mortira_BUILD5, 30, 50);
		if (UpgIsDone(&mortira_BUILD5))
		{
			TryUnit(&mortira, 10, 10, 50);
		}
		if (UpgIsDone(&pushka_BUILD5))
		{
			TryUnit(&pushka, 12, 10, 50);
		}
		TryUnit(&mortira_b, 20, 10, 50);
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
		if (landType == LT_MEDITERRANEAN)
		{
			TryUpgrade(&pehota_ATTACK, 30, 50);
			TryUpgrade(&pehota_ATTACK3, 30, 50);
			TryUpgrade(&pehota_ATTACK4, 30, 50);
			TryUpgrade(&pehota_SHIELD, 30, 50);
			TryUpgrade(&pehota_SHIELD3, 30, 50);
			TryUpgrade(&pehota_SHIELD4, 30, 50);
		}
		TryUpgrade(&mameluk_ATTACK, 30, 50);
		TryUpgrade(&mameluk_ATTACK3, 30, 50);
		TryUpgrade(&mameluk_ATTACK4, 30, 50);
		TryUpgrade(&mameluk_ATTACK5, 30, 50);
		TryUpgrade(&mameluk_ATTACK6, 30, 50);
		TryUpgrade(&mameluk_ATTACK7, 30, 50);
		TryUpgrade(&mameluk_SHIELD, 30, 50);
		TryUpgrade(&mameluk_SHIELD3, 30, 50);
		TryUpgrade(&mameluk_SHIELD4, 30, 50);
		TryUpgrade(&mameluk_SHIELD5, 30, 50);
		TryUpgrade(&mameluk_SHIELD6, 30, 50);
		TryUpgrade(&mameluk_SHIELD7, 30, 50);
	}
	TryUnit(&lodka, 3, 50, 50);
	if (landType == LT_MEDITERRANEAN)
	{
		TryUnit(&shebeka, 6, 99, 10);
		TryUnit(&shebeka, 12, 60, 10);
		TryUnit(&galera, 6, 90, 20);
	}
	else
	{
		TryUnit(&peres_kor, 3, 90, 50);
		TryUnit(&shebeka, 6, 99, 10);
		TryUnit(&shebeka, 12, 60, 10);
		TryUnit(&shebeka, 15, 50, 10);
		TryUnit(&galera, 6, 90, 20);
	}
	// market
	if (GetReadyUnits(&rinok))
	{
		if (stone > 100000)
		{
			AI_Torg(STONE, GOLD, 90000);
		}
		if (wood > 1000000)
		{
			AI_Torg(WOOD, GOLD, 90000);
		}
		if (food < 300)
		{
			if (coal > 12000)
			{
				AI_Torg(COAL, FOOD, 1000);
			}
			else if (gold > 3000)
			{
				AI_Torg(GOLD, FOOD, 1000);
			}
			else if (stone > 1500)
			{
				AI_Torg(STONE, FOOD, 1500);
			}
			else if (wood > 1500)
			{
				AI_Torg(WOOD, FOOD, 1000);
			}
		}
		if (!UpgIsDone(&AKA08) && gold > 800)
		{
			if (coal > 9000)
				AI_Torg(COAL, FOOD, 3000);
			if (iron > 9000)
				AI_Torg(IRON, FOOD, 3000);
		}
	}
	if (!FieldExist())
	{
		SetPDistribution(10, 5, 2);
	}
	else
	{
		if (GetUnits(&krestian) >= 20 || food >= 2000)
		{
			if (UpgIsDone(&AKA03) && food >= 15000)
			{
				SetPDistribution(10, 6, 4);
			}
			else
			{
				SetPDistribution(10, 4, 4);
			}
		}
		else
		{
			SetPDistribution(10, 5, 2);
		}
	}
	if (UpgIsDone(&AKA30))
	{
		SetPDistribution(6, 7, 3);
		TryUnit(&galera, 7, 70, 50);
		TryUpgrade(&KUZ02, 95, 100);
		TryUpgrade(&AKA20, 95, 100);
		if (UpgIsDone(&AKA20))
		{
		}
	}
	else
	{
		TryUnit(&galera, 3, 100, 50);
	}
}

// Turkey-lineage land fallback. England.dll's real land pipeline
// (FUN_10006bc0/6c20) is not yet ported; the original never TryUnits ports
// on land maps.
void ProcessLandAI()
{
	SetMineBalanse(19, LAND_MINE_BALANCE);
	int foodLast = GetMoney(FOOD);
	int goldLast = GetMoney(GOLD);
	TryUnit(&melnica, 1, 100, 100);
	if (GetUnits(&melnica))
	{
		TryUnit(&center, 2, 100, 100);
		if (GetUnits(&center))
		{
			TryUnit(&kuznia, 1, 100, 100);
			TryUnit(&barack, 1, 100, 100);
			if (GetUnits(&minaret))
			{
				TryUnit(&barack, 2, 100, 100);
				if (GetUnits(&center) > 1)
				{
					if (foodLast > 1000)
					{
						TryUnit(&barack, 3, 100, 100);
					}
					TryUnit(&barack, 4, 60, 100);
					TryUnit(&barack, 5, 60, 100);
				}
			}
			TryUnit(&sclad, 1, 100, 100);
			if (FieldExist() && foodLast > 1200)
			{
				TryUnit(&sclad, 3, 10, 2);
				if (foodLast > 2000)
				{
					TryUnit(&sclad, 5, 10, 1);
					TryUnit(&sclad, 6, 3, 1);
					TryUnit(&sclad, 7, 2, 1);
				}
				if (GetUnits(&krestian) > 24)
				{
					TryUnit(&rinok, 1, 100, 100);
				}
			}
			if (GetUnits(&minaret) && GetUnits(&barack) > 1)
			{
				TryUnit(&konushnia, 2, 100, 100);
				TryUnit(&konushnia, 3, 90, 100);
				TryUnit(&konushnia, 4, 90, 100);
				TryUnit(&konushnia, 5, 90, 100);
				TryUnit(&art_depo, 1, 100, 100);
			}
			TryUnit(&minaret, 1, 90, 100);
			// The original England.dll land pipeline never proposes ports
			// (the old block here was Turkey-lineage code).
			if (GetUnits(&barack))
			{
				TryUnit(&melnica, 2, 10, 10);
			}
			TryUnit(&diplomatic, 1, 90, 10);
		}
	}
	TryUpgrade(&Melnica_tuGETRES, 100, 100);
	if (UpgIsRun(&Melnica_tuGETRES))
	{
		if (foodLast > 700)
		{
			if (GetUnits(&strelec) >= 420)
			{
				TryUnit(&strelec, 600, 5, 100);
				if (GetUnits(&pikiner) > 250)
				{
					TryUnit(&strelec, 800, 20, 60);
				}
				TryUnit(&pikiner, 800, 20, 100);
			}
			else
			{
				TryUnit(&pikiner, 400, 20, 40);
				TryUnit(&strelec, 600, 5, 100);
			}
		}
		if (foodLast > 250)
		{
			TryUnit(&krestian, 1000, 100, 100);
		}
		if (foodLast > 300)
		{
			TryUnit(&mameluk, 400, 20, 100);
			TryUnit(&mameluk, 400, 20, 100);
			if (goldLast > 1500)
			{
				TryUnit(&grenader, 70, 100, 100);
			}
		}
		TryUpgrade(&AKA01, 99, 100);
		TryUpgrade(&KUZ01, 99, 100);
	}

	TryUpgrade(&KUZ03, 100, 100);
	TryUpgrade(&AKA08, 98, 100);
	TryUpgrade(&AKA23, 99, 100);
	TryUpgrade(&AKA04, 99, 100);
	TryUpgrade(&AKA01, 99, 100);
	TryUpgrade(&AKA02, 99, 100);
	TryUpgrade(&AKA03, 99, 100);
	TryUpgrade(&AKA30, 100, 100);
	if (UpgIsDone(&KUZ03))
	{
		TryUpgrade(&KUZ05, 95, 100);
		TryUpgrade(&pushka_BUILD, 30, 50);
		TryUpgrade(&pushka_BUILD3, 30, 50);
		TryUpgrade(&pushka_BUILD4, 30, 50);
		TryUpgrade(&pushka_BUILD5, 30, 50);
		TryUpgrade(&mortira_BUILD, 30, 50);
		TryUpgrade(&mortira_BUILD3, 30, 50);
		TryUpgrade(&mortira_BUILD4, 30, 50);
		TryUpgrade(&mortira_BUILD5, 30, 50);
		if (UpgIsDone(&mortira_BUILD5))
		{
			TryUnit(&mortira, 10, 10, 50);
		}
		if (UpgIsDone(&pushka_BUILD5))
		{
			TryUnit(&pushka, 12, 10, 50);
		}
		TryUnit(&mortira_b, 50, 10, 50);
		TryUpgrade(&mameluk_ATTACK, 30, 50);
		TryUpgrade(&mameluk_ATTACK3, 30, 50);
		TryUpgrade(&mameluk_ATTACK4, 30, 50);
		TryUpgrade(&mameluk_ATTACK5, 30, 50);
		TryUpgrade(&mameluk_ATTACK6, 30, 50);
		TryUpgrade(&mameluk_ATTACK7, 30, 50);
		TryUpgrade(&mameluk_SHIELD, 30, 50);
		TryUpgrade(&mameluk_SHIELD3, 30, 50);
		TryUpgrade(&mameluk_SHIELD4, 30, 50);
		TryUpgrade(&mameluk_SHIELD5, 30, 50);
		TryUpgrade(&mameluk_SHIELD6, 30, 50);
		TryUpgrade(&mameluk_SHIELD7, 30, 50);
	}
	if (GetUnits(&krestian) > 35)
	{
		TryUpgrade(&pikiner_ATTACK, 90, 100);
		TryUpgrade(&pikiner_ATTACK3, 90, 100);
		TryUpgrade(&pikiner_ATTACK4, 90, 100);
		TryUpgrade(&pikiner_ATTACK5, 90, 100);
		TryUpgrade(&pikiner_ATTACK6, 90, 100);
		TryUpgrade(&pikiner_SHIELD, 90, 100);
		TryUpgrade(&pikiner_SHIELD3, 90, 100);
		TryUpgrade(&pikiner_SHIELD4, 90, 100);
		TryUpgrade(&pikiner_SHIELD5, 90, 100);
		TryUpgrade(&pikiner_SHIELD6, 90, 100);
	}
	// market
	if (GetReadyUnits(&rinok))
	{
		int wood = GetMoney(WOOD);
		int stone = GetMoney(STONE);
		int iron = GetMoney(IRON);
		int coal = GetMoney(COAL);
		int gold = GetMoney(GOLD);
		int food = GetMoney(FOOD);
		if (food < 150)
		{
			if (stone > 5000)
			{
				AI_Torg(STONE, FOOD, 3000);
			}
			else if (stone > 1000)
			{
				AI_Torg(STONE, FOOD, 900);
			}
			else if (wood > 5000)
			{
				AI_Torg(WOOD, FOOD, 3000);
			}
			else if (wood > 1000)
			{
				AI_Torg(WOOD, FOOD, 900);
			}
			else if (coal > 2000)
			{
				AI_Torg(COAL, FOOD, 800);
			}
			else if (iron > 2000)
			{
				AI_Torg(IRON, FOOD, 800);
			}
			else if (gold > 1000)
			{
				AI_Torg(GOLD, FOOD, 500);
			}
		}
		if (GetReadyUnits(&minaret) && gold > 1000 && !UpgIsRun(&AKA08))
		{
			if (iron > 3000)
			{
				AI_Torg(IRON, FOOD, 2000);
			}
			if (coal > 4000)
			{
				AI_Torg(COAL, FOOD, 3000);
			}
		}
		if (GetMoney(STONE) > 120000)
		{
			AI_Torg(STONE, GOLD, 100000);
		}
		if (GetMoney(WOOD) > 100000)
		{
			AI_Torg(WOOD, GOLD, 90000);
		}
		if (GetUnits(&kuznia) && !UpgIsRun(&KUZ03))
		{
			if (GetMoney(COAL) > 9000 && GetMoney(WOOD) && GetMoney(GOLD) < 4000)
			{
				if (GetMoney(COAL) > 12000)
				{
					AI_Torg(COAL, GOLD, 3000);
				}
				if (GetMoney(IRON) > 8000)
				{
					AI_Torg(IRON, GOLD, 2000);
				}
				if (GetMoney(STONE) > 20000)
				{
					AI_Torg(STONE, GOLD, 12000);
				}
				if (GetMoney(FOOD) > 30000)
				{
					AI_Torg(FOOD, GOLD, 15000);
				}
			}
			if (GetMoney(COAL) > 9000 && GetMoney(GOLD) && GetMoney(GOLD) > 4000)
			{
				if (GetMoney(COAL) > 12000)
				{
					AI_Torg(COAL, IRON, 3000);
				}
				if (GetMoney(STONE) > 20000)
				{
					AI_Torg(STONE, IRON, 12000);
				}
				if (GetMoney(FOOD) > 30000)
				{
					AI_Torg(FOOD, IRON, 15000);
				}
			}
		}
		if (!GetAIRegister(1))
		{
			AI_Torg(COAL, FOOD, 4000);
			SetAIRegister(1, 1);
		}
	}
}

// Original England.dll FUN_10003210: pike upgrade chain, proposed once the
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

// Original England.dll FUN_100032b0: Mushketer_ev (17th c. musketeer)
// upgrade chain.
void WaterStrelecUpgrades()
{
	TryUpgrade(&pehota_ATTACK, 90, 100);
	TryUpgrade(&pehota_ATTACK3, 90, 100);
	TryUpgrade(&pehota_ATTACK4, 90, 100);
	TryUpgrade(&pehota_SHIELD, 90, 100);
	TryUpgrade(&pehota_SHIELD3, 90, 100);
}

// Original England.dll FUN_10003300: mortar construction upgrades.
void WaterMortiraUpgrades()
{
	TryUpgrade(&mortira_BUILD, 30, 50);
	TryUpgrade(&mortira_BUILD3, 30, 50);
	TryUpgrade(&mortira_BUILD4, 30, 50);
	TryUpgrade(&mortira_BUILD5, 30, 50);
}

// Original England.dll FUN_10006610: pick the mine-upgrade rules table for
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

// Original England.dll FUN_10003340: the AI script for the water map types
// (Peninsulas, Islands, Continents). Ported from the Ghidra decompilation
// of England.dll; TryUnit/TryUpgrade arguments, thresholds and gates are
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
	// This nation registers no Bashnia(xx)PAUSE5 - the second gate is a
	// dead slot in the original too (always "not done"), so the trade
	// below runs whenever KUZ02 is not done.
	if (!UpgIsDone(&KUZ02) || true)
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
	int land = GetLandType();

	// Per-tick economy setup (original: FUN_100030c0 prologue +
	// FUN_100066b0). With the lowest start resources the second
	// mines-building rule is raised to 4, as in the original.
	if (GetStartRes() == 0)
	{
		MINES_BUILD_RULES[1] = 4;
	}
	ApplyMineUpgradeRules(land);
	SetMinesBuildingRules(MINES_BUILD_RULES, 27);

	switch (land)
	{
	case LT_LAND:
	case LT_CONTINENT:
		// Original dispatcher runs the land pipeline for land types 0 and 5
		SetMineBalanse(19, LAND_MINE_BALANCE);
		ProcessLandAI();
		// Original land path (FUN_10005b10/FUN_10005c50): research the
		// XVIII century, then up to three 18th c. barracks (the original
		// TryUnits Kazarma(au) ungated; gated here because the building
		// is MAINAU-locked in AUSTRIA.NDS anyway)
		TryUpgrade(&MAIN, 98, 100);
		if (UpgIsDone(&MAIN))
		{
			TryUnit(&kazarma, 3, 100, 100);
		}
		break;
	case LT_MEDITERRANEAN:
		SetMineBalanse(19, LAND_MINE_BALANCE);
		ProcessNonLandAI(land);
		// Original ProcessMiddleSeaAI: XVIII century + two barracks
		TryUpgrade(&MAIN, 99, 100);
		if (UpgIsDone(&MAIN))
		{
			TryUnit(&kazarma, 1, 90, 20);
			TryUnit(&kazarma, 2, 80, 5);
		}
		break;
	case LT_PENINSULAS:
	case LT_ISLANDS:
	case LT_CONTINENTS:
		// The original dispatcher re-arms ALL THREE mine radii to 290
		// (the European AIs other than Russia differ from its 290/90/90)
		// and switches to the water mine balance before running the
		// water-map script
		SET_MINE_CAPTURE_RADIUS(290);
		SET_MINE_UPGRADE1_RADIUS(290);
		SET_MINE_UPGRADE2_RADIUS(290);
		SetMineBalanse(19, WATER_MINE_BALANCE);
		ProcessWaterAI();
		break;
	default:
		break;
	}

	// Officers for the pike formations (original FUN_10003240):
	// land types 0 and 1 only
	if (land == LT_LAND || land == LT_MEDITERRANEAN)
	{
		int units = GetUnits(&pikiner);
		if (units / 36 > 0)
		{
			TryUnit(&oficer, units / 36, 50, 50);
			TryUnit(&barabanshik, units / 36, 20, 50);
		}
	}
	// (England's dispatcher, like Austria's, has no unconditional barracks
	// TryUnit tail, unlike Russia's wrapper)
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
	OnInit();
}
