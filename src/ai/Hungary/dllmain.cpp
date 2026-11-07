#include <cstdio>
#include "../common/ai_common.h"

#include "objects.h"
// Mine balance tables of the original Vengria.dll (extracted from its .data
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


// Registers Hungary's units/upgrades with the game. Names must match the
// per-nation script VENGRIA.NDS (listed in Nations.lst, packed in all.gsc).
// All strings below are byte-exact from the original Vengria.dll string
// table (Ghidra + raw-string dump; see tools/ai_port.py which generated
// this module from the Hungary template by positional handler alignment).
// Role notes for this nation (from the Vengria.dll decompilation):
//   - 17th c. musketeer: VngSer(VG); mounted 18th c. pick: VngKPnd(VG)
//   - town hall: VngCen(VG); house: VngDom(VG); peasant: Kreposnoi_pruss(VG)
//   - Hungary registers NO MAINVG: the original never researches the
//     XVIII century. Its MAIN slot is repointed to Melnica(VG)GETRES
//     (like the original, which registers that name in two slots), so
//     every MAIN TryUpgrade/UpgIsDone below operates on the mill
//     upgrade, and the 18th c. barracks VngK18(VG) stays gated on it
//     ("ENABLE UNITS GRP 4 VngK18 Dragun_18 VngKPnd Kirasir" in
//     VENGRIA.NDS).
//   - The 18th c. shooter role is DROPPED: the original's per-game pick
//     (GetRandomIndex() & 0x18) == 8 ? "Mushketer(VG)" : "VngPnd(VG)")
//     is only ever fed to dead SafeRegisterUpgrade slots - never a unit
//     registration - so this module registers no shooter and all
//     shooter TryUnits were removed (drop_roles in tools/ai_port.py).
// The water-map AI and routing are 1:1 with the decompilation; the land
// pipeline and the Mediterranean handler keep the Turkey-lineage
// fallbacks (see docs/AI.md "Known gaps").
void RegisterHungary()
{
	RegisterUnitType(&pikiner, "Pikiner_evro(VG)");
	RegisterUnitType(&krestian, "Kreposnoi_pruss(VG)");
	RegisterUnitType(&pehota, "VngSer(VG)");
	RegisterUnitType(&grenader, "Grenader_DIP(VG)");
	RegisterUnitType(&mameluk, "Kirasir(VG)");
	RegisterUnitType(&pushka, "Pushka(VG)");
	RegisterUnitType(&mortira, "Mortira(VG)");
	RegisterUnitType(&mortira_b, "Mortira_b(VG)");
	RegisterUnitType(&lodka, "Lodka(VG)");
	// Hungary's 18th-century barracks. Enabled only by the Melnica(VG)GETRES
	// (XVIII century) research - "ENABLE UNITS GRP 4 VngK18(VG) Dragun_18(VG)
	// VngKPnd(VG) Kirasir(VG)" in VENGRIA.NDS.
	RegisterUnitType(&kazarma, "VngK18(VG)");
	AssignFormUnit("Pikiner_evro(VG)");
	RegisterUnitType(&kozak, "Kozak_loshad_DIP(VG)");
	RegisterUnitType(&oficer, "Officer_evro(VG)");
	RegisterUnitType(&barabanshik, "Barabanshik_ev1(VG)");
	RegisterUpgrade(&officer_ATTACK, "Officer_evro(VG)ATTACK");
	RegisterUpgrade(&officer_SHIELD, "Officer_evro(VG)SHIELD");
	RegisterUnitType(&center, "VngCen(VG)");
	RegisterUnitType(&melnica, "Melnica(VG)");
	RegisterUnitType(&barack, "VngK17(VG)");
	RegisterUnitType(&sclad, "Sclad1(VG)");
	RegisterUnitType(&minaret, "VngAka(VG)");
	RegisterUnitType(&konushnia, "VngKon(VG)");
	RegisterUnitType(&diplomatic, "VngDip(VG)");
	RegisterUnitType(&art_depo, "VngArt(VG)");
	RegisterUnitType(&rinok, "Rinok(VG)");
	RegisterUnitType(&kuznia, "VngKuz(VG)");
	RegisterUnitType(&mechet, "VngHra(VG)");
	RegisterUnitType(&port, "PorE(VG)");
	RegisterUnitType(&galera, "GALERA(VG)");
	RegisterUnitType(&peres_kor, "PERES_KOR(VG)");
	RegisterUnitType(&shebeka, "Fregat(VG)");
	RegisterUnitType(&linkor, "Linkor(VG)");
	RegisterUpgrade(&AKA29, "AKA29VG");
	RegisterUpgrade(&AKA06, "AKA06VG");
	RegisterUpgrade(&Melnica_tuGETRES, "Melnica(VG)GETRES");
	RegisterUpgrade(&Melnica_GETRES2, "Melnica(VG)GETRES2");
	RegisterUpgrade(&KUZ01, "KUZ01VG");
	RegisterUpgrade(&pikiner_ATTACK, "Pikiner_evro(VG)ATTACK");
	RegisterUpgrade(&pikiner_ATTACK3, "Pikiner_evro(VG)ATTACK3");
	RegisterUpgrade(&pikiner_ATTACK4, "Pikiner_evro(VG)ATTACK4");
	RegisterUpgrade(&pikiner_ATTACK5, "Pikiner_evro(VG)ATTACK5");
	RegisterUpgrade(&pikiner_ATTACK6, "Pikiner_evro(VG)ATTACK6");
	RegisterUpgrade(&pikiner_SHIELD, "Pikiner_evro(VG)SHIELD");
	RegisterUpgrade(&pikiner_SHIELD3, "Pikiner_evro(VG)SHIELD3");
	RegisterUpgrade(&pikiner_SHIELD4, "Pikiner_evro(VG)SHIELD4");
	RegisterUpgrade(&pikiner_SHIELD5, "Pikiner_evro(VG)SHIELD5");
	RegisterUpgrade(&pikiner_SHIELD6, "Pikiner_evro(VG)SHIELD6");
	RegisterUpgrade(&pehota_ATTACK, "VngSer(VG)ATTACK");
	RegisterUpgrade(&pehota_ATTACK3, "VngSer(VG)ATTACK3");
	RegisterUpgrade(&pehota_ATTACK4, "VngSer(VG)ATTACK4");
	RegisterUpgrade(&pehota_SHIELD, "VngSer(VG)SHIELD");
	RegisterUpgrade(&pehota_SHIELD3, "VngSer(VG)SHIELD3");
	RegisterUpgrade(&pehota_SHIELD4, "VngSer(VG)SHIELD4");
	RegisterUpgrade(&mameluk_ATTACK, "Kirasir(VG)ATTACK");
	RegisterUpgrade(&mameluk_ATTACK3, "Kirasir(VG)ATTACK3");
	RegisterUpgrade(&mameluk_ATTACK4, "Kirasir(VG)ATTACK4");
	RegisterUpgrade(&mameluk_ATTACK5, "Kirasir(VG)ATTACK5");
	RegisterUpgrade(&mameluk_ATTACK6, "Kirasir(VG)ATTACK6");
	RegisterUpgrade(&mameluk_ATTACK7, "Kirasir(VG)ATTACK7");
	RegisterUpgrade(&mameluk_SHIELD, "Kirasir(VG)SHIELD");
	RegisterUpgrade(&mameluk_SHIELD3, "Kirasir(VG)SHIELD3");
	RegisterUpgrade(&mameluk_SHIELD4, "Kirasir(VG)SHIELD4");
	RegisterUpgrade(&mameluk_SHIELD5, "Kirasir(VG)SHIELD5");
	RegisterUpgrade(&mameluk_SHIELD6, "Kirasir(VG)SHIELD6");
	RegisterUpgrade(&mameluk_SHIELD7, "Kirasir(VG)SHIELD7");
	RegisterUpgrade(&pushka_BUILD, "Pushka(VG)BUILD");
	RegisterUpgrade(&pushka_BUILD3, "Pushka(VG)BUILD3");
	RegisterUpgrade(&pushka_BUILD4, "Pushka(VG)BUILD4");
	RegisterUpgrade(&pushka_BUILD5, "Pushka(VG)BUILD5");
	RegisterUpgrade(&pushka_BUILD6, "Pushka(VG)BUILD6");
	RegisterUpgrade(&mortira_BUILD, "Mortira(VG)BUILD");
	RegisterUpgrade(&mortira_BUILD3, "Mortira(VG)BUILD3");
	RegisterUpgrade(&mortira_BUILD4, "Mortira(VG)BUILD4");
	RegisterUpgrade(&mortira_BUILD5, "Mortira(VG)BUILD5");
	RegisterUpgrade(&KUZ03, "KUZ03VG");
	RegisterUpgrade(&AKA01, "AKA01VG");
	RegisterUpgrade(&AKA02, "AKA02VG");
	RegisterUpgrade(&AKA03, "AKA03VG");
	RegisterUpgrade(&AKA04, "AKA04VG");
	RegisterUpgrade(&AKA08, "AKA08VG");
	RegisterUpgrade(&AKA23, "AKA23VG");
	RegisterUpgrade(&AKA24, "AKA24VG");
	RegisterUpgrade(&AKA10, "AKA10VG");
	RegisterUpgrade(&AKA30, "AKA30VG");
	RegisterUpgrade(&AKA07, "AKA07VG");
	RegisterUpgrade(&AKA20, "AKA20VG");
	RegisterUpgrade(&KUZ02, "KUZ02VG");
	RegisterUpgrade(&AKA09, "AKA09VG");
	RegisterUpgrade(&AKA05, "AKA05VG");
	RegisterUpgrade(&AKA28, "AKA28VG");
	RegisterUpgrade(&KUZ05, "KUZ05VG");
	// XVIII century: unlocks VngK18(VG) + Dragun_18/Gusar_evro/Kirasir.
	RegisterUpgrade(&MAIN, "Melnica(VG)GETRES");

	// ----- The rest of the original Vengria.dll registration set -----
	// Tower, yacht and the 17th/18th c. cavalry.
	RegisterUnitType(&bashnia2, "Bashnia(VG)");
	RegisterUnitType(&yahta, "Yahta(VG)");
	RegisterUnitType(&dragun17, "Dragun(VG)");
	RegisterUnitType(&dragun18, "Dragun_18(VG)");
	RegisterUnitType(&dragun18_dip, "Dragun_18_DIP(VG)");
	RegisterUnitType(&konni_ricar, "VngKPnd(VG)");
	RegisterUnitType(&sveshenik, "Sveshenik_evro(VG)");
	RegisterUnitType(&strelec_algir, "Strelec_Algir_DIP(VG)");
	// The original copies "VngKPnd(VG)" (mounted pick, registered above
	// unconditionally) and a conditional "Mushketer(VG)"/"VngPnd(VG)"
	// shooter pick through per-game buffers, but the shooter name is only
	// ever fed to dead SafeRegisterUpgrade slots - never a unit
	// registration - so no shooter is registered here and the shooter
	// TryUnits were removed from the lineage handlers (see header).
	RegisterUpgrade(&KUZ04, "KUZ04VG");
	RegisterUpgrade(&AKA35, "AKA35VG");
	RegisterUpgrade(&AKA36, "AKA36VG");
	RegisterUpgrade(&AKA32, "AKA32VG");
	RegisterUpgrade(&AKA11, "AKA11VG");
	RegisterUpgrade(&AKA12, "AKA12VG");
	RegisterUpgrade(&AKA13, "AKA13VG");
	RegisterUpgrade(&AKA14, "AKA14VG");
	RegisterUpgrade(&AKA15, "AKA15VG");
	RegisterUpgrade(&AKA31, "AKA31VG");
	RegisterUpgrade(&AKA33, "AKA33VG");
	RegisterUpgrade(&AKA34, "AKA34VG");
	RegisterUpgrade(&KUZ06, "KUZ06VG");
	RegisterUpgrade(&bashnia_PAUSE, "Bashnia(VG)PAUSE");
	RegisterUpgrade(&bashnia_PAUSE3, "Bashnia(VG)PAUSE3");
	RegisterUpgrade(&bashnia_PAUSE4, "Bashnia(VG)PAUSE4");
	RegisterUpgrade(&bashnia_PAUSE5, "Bashnia(VG)PAUSE5");
	RegisterUpgrade(&bashnia_PAUSE6, "Bashnia(VG)PAUSE6");
	RegisterUpgrade(&bashnia_PAUSE7, "Bashnia(VG)PAUSE7");
	RegisterUpgrade(&barabanshik_SHIELD, "Barabanshik_ev1(VG)SHIELD");
	// Unlike Russia (whose 12 "Dragun(RU)..." registrations are dead), the
	// Dragun(VG) and Dragun_18(VG) chains exist in VENGRIA.NDS.
	RegisterUpgrade(&dragun_ATTACK, "Dragun(VG)ATTACK");
	RegisterUpgrade(&dragun_ATTACK3, "Dragun(VG)ATTACK3");
	RegisterUpgrade(&dragun_ATTACK4, "Dragun(VG)ATTACK4");
	RegisterUpgrade(&dragun_ATTACK5, "Dragun(VG)ATTACK5");
	RegisterUpgrade(&dragun_ATTACK6, "Dragun(VG)ATTACK6");
	RegisterUpgrade(&dragun_ATTACK7, "Dragun(VG)ATTACK7");
	RegisterUpgrade(&dragun_SHIELD, "Dragun(VG)SHIELD");
	RegisterUpgrade(&dragun_SHIELD3, "Dragun(VG)SHIELD3");
	RegisterUpgrade(&dragun_SHIELD4, "Dragun(VG)SHIELD4");
	RegisterUpgrade(&dragun_SHIELD5, "Dragun(VG)SHIELD5");
	RegisterUpgrade(&dragun_SHIELD6, "Dragun(VG)SHIELD6");
	RegisterUpgrade(&dragun_SHIELD7, "Dragun(VG)SHIELD7");
	RegisterUpgrade(&dragun18_ATTACK, "Dragun_18(VG)ATTACK");
	RegisterUpgrade(&dragun18_ATTACK3, "Dragun_18(VG)ATTACK3");
	RegisterUpgrade(&dragun18_ATTACK4, "Dragun_18(VG)ATTACK4");
	RegisterUpgrade(&dragun18_ATTACK5, "Dragun_18(VG)ATTACK5");
	RegisterUpgrade(&dragun18_ATTACK6, "Dragun_18(VG)ATTACK6");
	RegisterUpgrade(&dragun18_ATTACK7, "Dragun_18(VG)ATTACK7");
	RegisterUpgrade(&dragun18_SHIELD, "Dragun_18(VG)SHIELD");
	RegisterUpgrade(&dragun18_SHIELD3, "Dragun_18(VG)SHIELD3");
	RegisterUpgrade(&dragun18_SHIELD4, "Dragun_18(VG)SHIELD4");
	RegisterUpgrade(&dragun18_SHIELD5, "Dragun_18(VG)SHIELD5");
	RegisterUpgrade(&dragun18_SHIELD6, "Dragun_18(VG)SHIELD6");
	RegisterUpgrade(&dragun18_SHIELD7, "Dragun_18(VG)SHIELD7");

	// Initial food/wood/stone peasant weights, same as in the rest of the
	// European family. The land pipeline never adjusts them - without this
	// call the peasants idle and starve.
	SetPDistribution(9, 4, 4);
	AssignAmountOfMineUpgrades(6);
	AssignMineUpgrade(0, "shahta(VG)INSIDE", 90);
	AssignMineUpgrade(1, "shahta(VG)INSIDE3", 50);
	AssignMineUpgrade(2, "shahta(VG)INSIDE4", 50);
	AssignMineUpgrade(3, "shahta(VG)INSIDE5", 50);
	AssignMineUpgrade(4, "shahta(VG)INSIDE6", 50);
	AssignMineUpgrade(5, "shahta(VG)INSIDE7", 50);
	AssignPeasant("Kreposnoi_pruss(VG)");
	AssignMine("shahta(VG)");
	AssignHouse("VngDom(VG)");
	AssignWall("WALL_EV(VG)");
	// Original Vengria.dll InitAI: 290/90/90 radii and 300 max mine
	// workers (Austria uses 1000).
	SET_MINE_CAPTURE_RADIUS(290);
	SET_MINE_UPGRADE1_RADIUS(90);
	SET_MINE_UPGRADE2_RADIUS(90);
	SET_DEFAULT_MAX_WORKERS(1000);
	SET_MIN_PEASANT_BRIGADE(5);
}

extern "C" __declspec(dllexport) void InitAI()
{
	RegisterHungary();
}

extern "C" __declspec(dllexport) void OnInit()
{
	InitAI();
}

// Turkey-lineage Mediterranean fallback. Vengria.dll has its own
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
			// Original Vengria.dll ProcessMiddleSeaAI: three ports, no
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
				TryUnit(&pikiner, 800, 20, 100);
			}
			else
			{
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
		TryUpgrade(&AKA09, 95, 100);
		TryUpgrade(&AKA20, 95, 100);
		if (UpgIsDone(&AKA20))
		{
			TryUpgrade(&AKA28, 90, 100);
		}
	}
	else
	{
		TryUnit(&galera, 3, 100, 50);
	}
}

// Turkey-lineage land fallback. Vengria.dll's real land pipeline
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
			// The original Vengria.dll land pipeline never proposes ports
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
			// Hungary has no 18th c. shooter (see header): the original's
			// GetUnits(&strelec) >= 420 branch is unreachable, so only the
			// else branch (pikiner production) remains.
			TryUnit(&pikiner, 400, 20, 40);
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

// Original Vengria.dll FUN_10003210: pike upgrade chain, proposed once the
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

// Original Vengria.dll FUN_100032b0: Mushketer_ev (17th c. musketeer)
// upgrade chain.
void WaterStrelecUpgrades()
{
	TryUpgrade(&pehota_ATTACK, 90, 100);
	TryUpgrade(&pehota_ATTACK3, 90, 100);
	TryUpgrade(&pehota_ATTACK4, 90, 100);
	TryUpgrade(&pehota_SHIELD, 90, 100);
	TryUpgrade(&pehota_SHIELD3, 90, 100);
}

// Original Vengria.dll FUN_10003300: mortar construction upgrades.
void WaterMortiraUpgrades()
{
	TryUpgrade(&mortira_BUILD, 30, 50);
	TryUpgrade(&mortira_BUILD3, 30, 50);
	TryUpgrade(&mortira_BUILD4, 30, 50);
	TryUpgrade(&mortira_BUILD5, 30, 50);
}

// Original Vengria.dll FUN_10006610: pick the mine-upgrade rules table for
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

// Original Vengria.dll FUN_10003340: the AI script for the water map types
// (Peninsulas, Islands, Continents). Ported from the Ghidra decompilation
// of Vengria.dll; TryUnit/TryUpgrade arguments, thresholds and gates are
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
	// (Hungary's dispatcher, like Austria's, has no unconditional barracks
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
