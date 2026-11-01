// Ukraine AI - ported 1:1 from the original Ukraine.dll (GOG "Cossacks:
// Back To War", AI/Ukraine.dll, 53248 bytes; Ghidra decompilation in
// /tmp/ukraine_decomp). Ukraine is the OLD Algeria/Turcia AI generation:
// same InitAI constants (mine radii 290/260/130, MAX_WORKERS 300,
// MIN_PBRIG 5), no SetMinesBuildingRules/SetMinesUpgradeRules (those APIs
// did not exist yet - the engine defaults apply), a single 19-pair mine
// balance table, and two handlers: land 0 -> the exported ProcessLandAI,
// land 1..4 -> FUN_100019f0 (ProcessNonLandAI here). Land 5+ is ignored,
// and unlike Algeria there is NO officers/drummer tail in ProcessAI.
// Ukraine has no XVIII century at all (no MAINUA); GALERA(UA) is gated by
// AKA06UA which the original never registers, so its TryUnits are dead
// in the original too (kept 1:1).
//
// Role map (Ukraine.dll name -> Algeria-lineage var):
//   strelec  <- Kozacki_Strelec(UA)   17th c. musketeer (from barack)
//   krestian <- Kres_ukranian(UA)     peasant
//   pikiner  <- Kozak_pikiner(UA)     pikeman (also AssignFormUnit target
//                                     in Algeria; Ukraine assigns nothing)
//   grenader <- Grenader_DIP(UA)      mercenary grenadier
//   barack   <- Strelcovaia_Izba(UA)  17th c. "shooting izba" barracks
//   center   <- CenterUR(UA)          town hall
//   melnica  <- Melnica_rus(UA)       mill
//   port     <- Poru(UA), galera <- GALERA(UA), peres_kor <- PERES_KOR(UA)
//   art_depo <- artileri_depo_UA(UA), kuznia <- Kuznica_ua(UA), etc.
#include <cstdio>
#include "../common/ai_common.h"

#include "objects.h"

// SetMineBalanse table of the original Ukraine.dll (DAT_10009030, .data),
// 19 pairs {total peasants, peasants wanted on food+wood+stone}; the rest
// go to mines. The engine scan has no bound check, so the {4000, 300}
// terminator must stay. Differs slightly from Algeria's table.
unsigned short MINE_BALANCE[38] = {
	0, 0, 19, 19, 25, 20, 33, 20, 43, 20, 48, 30, 53, 32, 58, 35, 65, 35, 80, 35,
	105, 40, 125, 50, 150, 60, 175, 70, 200, 80, 225, 80, 250, 80, 500, 100, 4000, 300
};

// Original InitAI registration order (FUN_10002150-equivalent chains are
// inlined as the original's three helper functions below).
void RegisterUkraine()
{
	RegisterUnitType(&strelec, "Kozacki_Strelec(UA)");
	RegisterUnitType(&krestian, "Kres_ukranian(UA)");
	RegisterUnitType(&grenader, "Grenader_DIP(UA)");
	RegisterUnitType(&pikiner, "Kozak_pikiner(UA)");
	RegisterUnitType(&pushka, "Pushka(UA)");
	RegisterUnitType(&mortira, "Mortira(UA)");
	RegisterUnitType(&mortira_b, "Mortira_b(UA)");
	RegisterUnitType(&lodka, "Lodka(UA)");
	RegisterUnitType(&center, "CenterUR(UA)");
	RegisterUnitType(&melnica, "Melnica_rus(UA)");
	RegisterUnitType(&barack, "Strelcovaia_Izba(UA)");
	RegisterUnitType(&sclad, "Sclad3(UA)");
	RegisterUnitType(&akademia, "akademia_UA(UA)");
	RegisterUnitType(&konushnia, "konushnia_ua(UA)");
	RegisterUnitType(&diplomatic, "Dip_korpus_UA(UA)");
	RegisterUnitType(&art_depo, "artileri_depo_UA(UA)");
	RegisterUnitType(&rinok, "Rinok(UA)");
	RegisterUnitType(&kuznia, "Kuznica_ua(UA)");
	RegisterUnitType(&cerkov, "Cerkov_UA(UA)");
	RegisterUnitType(&port, "Poru(UA)");
	RegisterUnitType(&galera, "GALERA(UA)");
	RegisterUnitType(&peres_kor, "PERES_KOR(UA)");
	RegisterUpgrade(&melnica_GETRES, "Melnica_rus(UA)GETRES");
	RegisterUpgrade(&melnica_GETRES2, "Melnica_rus(UA)GETRES2");
	RegisterUpgrade(&KUZ01, "KUZ01UA");
	RegisterUpgrade(&strelec_ATTACK, "Kozacki_Strelec(UA)ATTACK");
	RegisterUpgrade(&strelec_ATTACK3, "Kozacki_Strelec(UA)ATTACK3");
	RegisterUpgrade(&strelec_ATTACK4, "Kozacki_Strelec(UA)ATTACK4");
	RegisterUpgrade(&strelec_ATTACK5, "Kozacki_Strelec(UA)ATTACK5");
	RegisterUpgrade(&strelec_ATTACK6, "Kozacki_Strelec(UA)ATTACK6");
	RegisterUpgrade(&strelec_SHIELD, "Kozacki_Strelec(UA)SHIELD");
	RegisterUpgrade(&strelec_SHIELD3, "Kozacki_Strelec(UA)SHIELD3");
	RegisterUpgrade(&strelec_SHIELD4, "Kozacki_Strelec(UA)SHIELD4");
	RegisterUpgrade(&strelec_SHIELD5, "Kozacki_Strelec(UA)SHIELD5");
	RegisterUpgrade(&strelec_SHIELD6, "Kozacki_Strelec(UA)SHIELD6");
	RegisterUpgrade(&pikiner_ATTACK, "Kozak_pikiner(UA)ATTACK");
	RegisterUpgrade(&pikiner_ATTACK3, "Kozak_pikiner(UA)ATTACK3");
	RegisterUpgrade(&pikiner_ATTACK4, "Kozak_pikiner(UA)ATTACK4");
	RegisterUpgrade(&pikiner_ATTACK5, "Kozak_pikiner(UA)ATTACK5");
	RegisterUpgrade(&pikiner_ATTACK6, "Kozak_pikiner(UA)ATTACK6");
	RegisterUpgrade(&pikiner_ATTACK7, "Kozak_pikiner(UA)ATTACK7");
	RegisterUpgrade(&pikiner_SHIELD, "Kozak_pikiner(UA)SHIELD");
	RegisterUpgrade(&pikiner_SHIELD3, "Kozak_pikiner(UA)SHIELD3");
	RegisterUpgrade(&pikiner_SHIELD4, "Kozak_pikiner(UA)SHIELD4");
	RegisterUpgrade(&pikiner_SHIELD5, "Kozak_pikiner(UA)SHIELD5");
	RegisterUpgrade(&pikiner_SHIELD6, "Kozak_pikiner(UA)SHIELD6");
	RegisterUpgrade(&pikiner_SHIELD7, "Kozak_pikiner(UA)SHIELD7");
	RegisterUpgrade(&pushka_BUILD, "Pushka(UA)BUILD");
	RegisterUpgrade(&pushka_BUILD3, "Pushka(UA)BUILD3");
	RegisterUpgrade(&pushka_BUILD4, "Pushka(UA)BUILD4");
	RegisterUpgrade(&pushka_BUILD5, "Pushka(UA)BUILD5");
	RegisterUpgrade(&pushka_BUILD6, "Pushka(UA)BUILD6");
	RegisterUpgrade(&mortira_BUILD, "Mortira(UA)BUILD");
	RegisterUpgrade(&mortira_BUILD3, "Mortira(UA)BUILD3");
	RegisterUpgrade(&mortira_BUILD4, "Mortira(UA)BUILD4");
	RegisterUpgrade(&mortira_BUILD5, "Mortira(UA)BUILD5");
	RegisterUpgrade(&KUZ03, "KUZ03UA");
	RegisterUpgrade(&AKA01, "AKA01UA");
	RegisterUpgrade(&AKA02, "AKA02UA");
	RegisterUpgrade(&AKA03, "AKA03UA");
	RegisterUpgrade(&AKA04, "AKA04UA");
	RegisterUpgrade(&AKA08, "AKA08UA");
	RegisterUpgrade(&AKA24, "AKA24UA");
	RegisterUpgrade(&AKA33, "AKA33UA");
	RegisterUpgrade(&AKA12, "AKA12UA");
	RegisterUpgrade(&AKA13, "AKA13UA");
	RegisterUpgrade(&AKA14, "AKA14UA");
	RegisterUpgrade(&AKA15, "AKA15UA");
	RegisterUpgrade(&AKA10, "AKA10UA");
	RegisterUpgrade(&AKA07, "AKA07UA");
	RegisterUpgrade(&KUZ02, "KUZ02UA");
	RegisterUpgrade(&AKA09, "AKA09UA");
	RegisterUpgrade(&AKA05, "AKA05UA");
	RegisterUpgrade(&AKA28, "AKA28UA");
	RegisterUpgrade(&KUZ05, "KUZ05UA");
	RegisterUpgrade(&AKA23, "AKA23UA");
	RegisterUpgrade(&AKA31, "AKA31UA");
	RegisterUpgrade(&AKA30, "AKA30UA");
	RegisterUpgrade(&AKA20, "AKA20UA");
}

extern "C" __declspec(dllexport) void InitAI()
{
	RegisterUkraine();
	// Mine upgrade tiers (Ukraine.dll DAT_100090xx, same as Algeria's):
	// capture radius 290, upgrade1 260, upgrade2 130.
	SET_MINE_CAPTURE_RADIUS(290);
	SET_MINE_UPGRADE1_RADIUS(260);
	SET_MINE_UPGRADE2_RADIUS(130);
	SET_DEFAULT_MAX_WORKERS(300);
	SET_MIN_PEASANT_BRIGADE(5);
	// SetPDistribution(9,4,4) + AssignAmountOfMineUpgrades(6) +
	// the six AssignMineUpgrade tiers (original InitAI tail).
	SetPDistribution(9, 4, 4);
	AssignAmountOfMineUpgrades(6);
	AssignMineUpgrade(0, "shahta(UA)INSIDE", 90);
	AssignMineUpgrade(1, "shahta(UA)INSIDE3", 50);
	AssignMineUpgrade(2, "shahta(UA)INSIDE4", 50);
	AssignMineUpgrade(3, "shahta(UA)INSIDE5", 50);
	AssignMineUpgrade(4, "shahta(UA)INSIDE6", 50);
	AssignMineUpgrade(5, "shahta(UA)INSIDE7", 50);
	AssignPeasant("Kres_ukranian(UA)");
	AssignMine("shahta(UA)");
	AssignHouse("Ukrainska_hata(UA)");
}

extern "C" __declspec(dllexport) void OnInit()
{
	InitAI();
}

// Original FUN_100021f0: the Pushka + Mortira BUILD upgrade chain
// (artillery cost/time reductions), proposed at (30, 50) each.
static void TryArtilleryBuildUpgrades()
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

// Original FUN_10002150 (land variant) / FUN_10001950 (non-land variant):
// the Kozacki_Strelec ATTACK/SHIELD chain. The two handlers propose it
// with slightly different CostPercent values, so both bodies are kept.
static void TryStrelecUpgradesLand()
{
	TryUpgrade(&strelec_ATTACK, 60, 100);
	TryUpgrade(&strelec_ATTACK3, 60, 100);
	TryUpgrade(&strelec_ATTACK4, 60, 100);
	TryUpgrade(&strelec_ATTACK5, 70, 100);
	TryUpgrade(&strelec_ATTACK6, 70, 100);
	TryUpgrade(&strelec_SHIELD, 60, 100);
	TryUpgrade(&strelec_SHIELD3, 60, 100);
	TryUpgrade(&strelec_SHIELD4, 60, 100);
	TryUpgrade(&strelec_SHIELD5, 20, 100);
	TryUpgrade(&strelec_SHIELD6, 10, 100);
}

static void TryStrelecUpgradesNonLand()
{
	TryUpgrade(&strelec_ATTACK, 60, 100);
	TryUpgrade(&strelec_ATTACK3, 60, 100);
	TryUpgrade(&strelec_ATTACK4, 60, 100);
	TryUpgrade(&strelec_ATTACK5, 60, 100);
	TryUpgrade(&strelec_ATTACK6, 60, 100);
	TryUpgrade(&strelec_SHIELD, 60, 100);
	TryUpgrade(&strelec_SHIELD3, 10, 100);
	TryUpgrade(&strelec_SHIELD4, 10, 100);
	TryUpgrade(&strelec_SHIELD5, 10, 100);
	TryUpgrade(&strelec_SHIELD6, 10, 100);
}

// Original exported ProcessLandAI @ 10002270: the land (type 0) handler.
extern "C" __declspec(dllexport) void ProcessLandAI()
{
	SetMineBalanse(19, MINE_BALANCE);
	int foodLast = GetMoney(FOOD);
	int woodLast = GetMoney(WOOD);
	int stoneLast = GetMoney(STONE);
	int goldLast = GetMoney(GOLD);
	int coalLast = GetMoney(COAL);
	int ironLast = GetMoney(IRON);

	// Build order: mill -> town hall -> blacksmith -> barracks x3 ->
	// stable x4 -> artillery depot -> storehouse -> market -> academy.
	TryUnit(&melnica, 1, 100, 100);
	if (GetUnits(&melnica))
	{
		TryUnit(&center, 2, 100, 100);
		if (GetUnits(&center))
		{
			TryUnit(&kuznia, 1, 100, 100);
			TryUnit(&barack, 2, 100, 100);
			if (GetUnits(&center) > 1)
			{
				if (foodLast > 1000)
				{
					TryUnit(&barack, 3, 100, 100);
				}
				TryUnit(&barack, 4, 50, 100);
				TryUnit(&barack, 5, 30, 100);
				TryUnit(&konushnia, 2, 100, 100);
				TryUnit(&konushnia, 3, 90, 100);
				TryUnit(&konushnia, 4, 90, 100);
				TryUnit(&konushnia, 5, 90, 100);
				TryUnit(&art_depo, 1, 100, 100);
			}
			TryUnit(&sclad, 1, 100, 100);
			if (foodLast > 700 || goldLast > 5000)
			{
				TryUnit(&rinok, 1, 100, 100);
				TryUnit(&akademia, 1, 90, 100);
			}
			if (GetUnits(&barack))
			{
				TryUnit(&melnica, 2, 10, 10);
			}
			TryUnit(&diplomatic, 1, 90, 10);
		}
	}

	// Mill resource upgrades gate the unit production.
	TryUpgrade(&melnica_GETRES, 100, 100);
	TryUpgrade(&melnica_GETRES2, 99, 100);
	if (UpgIsRun(&melnica_GETRES))
	{
		if (foodLast > 700)
		{
			TryUnit(&strelec, 500, 100, 100);
		}
		if (foodLast > 200)
		{
			TryUnit(&krestian, 1000, 100, 100);
		}
		if (foodLast > 300)
		{
			TryUnit(&krestian, 1000, 100, 100);
			TryUnit(&pikiner, 600, 20, 100);
			TryUnit(&pikiner, 600, 20, 100);
			if (goldLast > 1000)
			{
				TryUnit(&grenader, 70, 100, 100);
			}
		}
		TryUpgrade(&AKA01, 99, 100);
		TryUpgrade(&KUZ01, 99, 100);
	}
	TryUpgrade(&KUZ03, 100, 100);
	if (UpgIsDone(&melnica_GETRES2))
	{
		TryUpgrade(&AKA08, 99, 100);
		TryUpgrade(&AKA23, 99, 100);
	}
	TryUpgrade(&AKA04, 99, 100);
	if (UpgIsDone(&KUZ03))
	{
		TryUpgrade(&AKA24, 99, 100);
		TryUpgrade(&AKA01, 99, 100);
		TryUpgrade(&AKA02, 99, 100);
		TryUpgrade(&AKA03, 99, 100);
		TryUpgrade(&AKA31, 95, 100);
		TryUpgrade(&AKA33, 95, 100);
		TryUpgrade(&KUZ05, 95, 100);
		TryArtilleryBuildUpgrades();
		if (UpgIsDone(&mortira_BUILD5))
		{
			TryUnit(&mortira, 10, 10, 50);
		}
		if (UpgIsDone(&pushka_BUILD5))
		{
			TryUnit(&pushka, 10, 20, 50);
			TryUnit(&pushka, 20, 20, 50);
		}
		TryUnit(&mortira_b, 20, 10, 50);
		TryStrelecUpgradesLand();
		TryUpgrade(&pikiner_ATTACK, 60, 50);
		TryUpgrade(&pikiner_ATTACK3, 60, 50);
		TryUpgrade(&pikiner_ATTACK4, 60, 50);
		TryUpgrade(&pikiner_ATTACK5, 60, 50);
		TryUpgrade(&pikiner_ATTACK6, 60, 50);
		TryUpgrade(&pikiner_ATTACK7, 60, 50);
		TryUpgrade(&pikiner_SHIELD, 30, 50);
		TryUpgrade(&pikiner_SHIELD3, 30, 50);
		TryUpgrade(&pikiner_SHIELD4, 30, 50);
		TryUpgrade(&pikiner_SHIELD5, 30, 50);
		TryUpgrade(&pikiner_SHIELD6, 30, 50);
		TryUpgrade(&pikiner_SHIELD7, 30, 50);
		if (UpgIsDone(&strelec_ATTACK6))
		{
			TryUpgrade(&AKA12, 40, 20);
			TryUpgrade(&AKA13, 40, 20);
			TryUpgrade(&AKA14, 40, 20);
			TryUpgrade(&AKA15, 40, 20);
		}
	}

	// Trade tail (only with a working market).
	if (GetReadyUnits(&rinok))
	{
		if (goldLast > 2500 && !UpgIsDone(&melnica_GETRES2))
		{
			AI_Torg(GOLD, FOOD, 500);
		}
		if (stoneLast > 100000)
		{
			AI_Torg(STONE, GOLD, 90000);
		}
		if (woodLast > 1000000)
		{
			AI_Torg(WOOD, GOLD, 90000);
		}
		// Emergency food buys, one source per tick, when food runs low
		// and fields already exist.
		if (foodLast < 500 && FieldExist())
		{
			if (coalLast > 20000)
			{
				AI_Torg(COAL, FOOD, 3000);
			}
			else if (ironLast < 20001)
			{
				if (goldLast < 15001)
				{
					if (stoneLast < 70001)
					{
						if (woodLast < 40001)
						{
							if (ironLast < 10001)
							{
								if (coalLast < 10001)
								{
									if (ironLast > 2000)
									{
										AI_Torg(IRON, FOOD, 900);
									}
									else if (coalLast > 2000)
									{
										AI_Torg(COAL, FOOD, 900);
									}
								}
								else
								{
									AI_Torg(COAL, FOOD, 1000);
								}
							}
							else
							{
								AI_Torg(IRON, FOOD, 1000);
							}
						}
						else
						{
							AI_Torg(WOOD, FOOD, 10000);
						}
					}
					else
					{
						AI_Torg(STONE, FOOD, 30000);
					}
				}
				else
				{
					AI_Torg(GOLD, FOOD, 3000);
				}
			}
			else
			{
				AI_Torg(IRON, FOOD, 3000);
			}
		}
	}

	// Peasant distribution: no fields -> grow them; else keep peasants
	// low until the second mill upgrade, then balance out.
	if (!FieldExist())
	{
		SetPDistribution(0, 5, 5);
		return;
	}
	if (GetUnits(&krestian) < 20 && foodLast < 2000)
	{
		SetPDistribution(10, 2, 2);
		return;
	}
	if (!UpgIsDone(&melnica_GETRES2))
	{
		SetPDistribution(10, 4, 2);
		return;
	}
	SetPDistribution(7, 6, 3);
}

// Original FUN_100019f0: the land 1..4 (Mediterranean..continents)
// handler. landType is the global the original's ProcessAI stored
// (DAT_1000abc8); the original checks it for the mortar boat / ferry /
// galera branches.
static void ProcessNonLandAI(int landType)
{
	SetMineBalanse(19, MINE_BALANCE);
	int foodLast = GetMoney(FOOD);
	int woodLast = GetMoney(WOOD);
	int stoneLast = GetMoney(STONE);
	int goldLast = GetMoney(GOLD);
	int coalLast = GetMoney(COAL);
	int ironLast = GetMoney(IRON);

	// Same build order as the land handler, plus the port: up to three
	// ports, one extra when the market is finished and gold allows.
	TryUnit(&melnica, 1, 100, 100);
	if (GetUnits(&melnica))
	{
		TryUnit(&center, 2, 100, 100);
		if (GetUnits(&center))
		{
			TryUnit(&kuznia, 1, 100, 100);
			TryUnit(&barack, 2, 100, 100);
			if (GetUnits(&center) > 1)
			{
				if (foodLast > 1000)
				{
					TryUnit(&barack, 3, 100, 100);
				}
				TryUnit(&barack, 4, 50, 100);
				TryUnit(&barack, 5, 30, 100);
			}
			TryUnit(&sclad, 1, 100, 100);
			TryUnit(&rinok, 1, 100, 100);
			TryUnit(&konushnia, 2, 100, 100);
			TryUnit(&konushnia, 3, 90, 100);
			TryUnit(&konushnia, 4, 90, 100);
			TryUnit(&konushnia, 5, 90, 100);
			TryUnit(&art_depo, 1, 100, 100);
			TryUnit(&akademia, 1, 90, 100);
			if (GetUnits(&port) == GetReadyUnits(&port))
			{
				TryUnit(&port, 2, 50, 10);
				TryUnit(&port, 3, 20, 10);
			}
			if (GetUnits(&barack))
			{
				TryUnit(&melnica, 2, 10, 10);
			}
			TryUnit(&diplomatic, 1, 90, 10);
		}
	}

	TryUpgrade(&melnica_GETRES, 100, 100);
	TryUpgrade(&melnica_GETRES2, 99, 100);
	if (UpgIsRun(&melnica_GETRES))
	{
		if (foodLast > 700)
		{
			TryUnit(&strelec, 500, 20, 100);
		}
		if (foodLast > 200)
		{
			TryUnit(&krestian, 1000, 100, 100);
		}
		if (foodLast > 300)
		{
			TryUnit(&krestian, 1000, 100, 100);
			TryUnit(&pikiner, 600, 20, 100);
			TryUnit(&pikiner, 600, 20, 100);
			// The original here checks the land-type global; for this
			// handler it is always 1..4, so the land==0 branch is dead
			// and the grenadier needs only gold > 1000.
			if (goldLast > 1000)
			{
				TryUnit(&grenader, 70, 100, 100);
			}
		}
		TryUpgrade(&AKA01, 99, 100);
		TryUpgrade(&KUZ01, 99, 100);
	}
	TryUpgrade(&KUZ03, 100, 100);
	if (UpgIsDone(&melnica_GETRES2))
	{
		TryUpgrade(&AKA08, 99, 100);
		TryUpgrade(&AKA23, 99, 100);
	}
	TryUpgrade(&AKA04, 99, 100);
	TryUpgrade(&AKA30, 100, 100);
	if (UpgIsDone(&KUZ03))
	{
		// Mediterranean (land 1) only: mortar boats.
		if (landType == LT_MEDITERRANEAN)
		{
			TryUnit(&mortira_b, 40, 10, 50);
		}
		TryUpgrade(&AKA24, 99, 100);
		TryUpgrade(&AKA07, 99, 100);
		if (UpgIsDone(&AKA07))
		{
			TryUnit(&lodka, 40, 10, 50);
			TryUpgrade(&AKA05, 99, 100);
			TryUpgrade(&AKA01, 99, 100);
			TryUpgrade(&AKA02, 99, 100);
			TryUpgrade(&AKA03, 99, 100);
			TryUpgrade(&AKA31, 95, 100);
			TryUpgrade(&AKA33, 95, 100);
			TryUpgrade(&KUZ05, 95, 100);
			TryArtilleryBuildUpgrades();
			if (UpgIsDone(&mortira_BUILD5))
			{
				TryUnit(&mortira, 10, 10, 50);
			}
			if (UpgIsDone(&pushka_BUILD5))
			{
				TryUnit(&pushka, 10, 20, 50);
				TryUnit(&pushka, 20, 20, 50);
			}
			TryUnit(&mortira_b, 20, 10, 50);
			TryStrelecUpgradesNonLand();
			TryUpgrade(&pikiner_ATTACK, 30, 50);
			TryUpgrade(&pikiner_ATTACK3, 30, 50);
			TryUpgrade(&pikiner_ATTACK4, 30, 50);
			TryUpgrade(&pikiner_ATTACK5, 30, 50);
			TryUpgrade(&pikiner_ATTACK6, 30, 50);
			TryUpgrade(&pikiner_ATTACK7, 30, 50);
			TryUpgrade(&pikiner_SHIELD, 30, 50);
			TryUpgrade(&pikiner_SHIELD3, 30, 50);
			TryUpgrade(&pikiner_SHIELD4, 30, 50);
			TryUpgrade(&pikiner_SHIELD5, 30, 50);
			TryUpgrade(&pikiner_SHIELD6, 30, 50);
			TryUpgrade(&pikiner_SHIELD7, 30, 50);
			if (UpgIsDone(&strelec_ATTACK6))
			{
				TryUpgrade(&AKA12, 40, 20);
				TryUpgrade(&AKA13, 40, 20);
				TryUpgrade(&AKA14, 40, 20);
				TryUpgrade(&AKA15, 40, 20);
			}
			// Fishing boats; the ferry only away from the Mediterranean.
			TryUnit(&lodka, 3, 50, 100);
			if (landType != LT_MEDITERRANEAN)
			{
				TryUnit(&peres_kor, 3, 90, 100);
			}
		}
	}

	// Trade tail: same three gold/stone/wood sells as the land handler.
	if (GetReadyUnits(&rinok))
	{
		if (goldLast > 2500 && !UpgIsDone(&melnica_GETRES2))
		{
			AI_Torg(GOLD, FOOD, 500);
		}
		if (stoneLast > 100000)
		{
			AI_Torg(STONE, GOLD, 90000);
		}
		if (woodLast > 1000000)
		{
			AI_Torg(WOOD, GOLD, 90000);
		}
	}

	// Peasant distribution (same logic as the land handler, but the
	// original computes it through three temporaries).
	int pdFood, pdWood, pdStone;
	if (!FieldExist())
	{
		pdFood = 0; pdWood = 5; pdStone = 5;
	}
	else if (GetUnits(&krestian) < 20 && foodLast < 2000)
	{
		pdFood = 10; pdWood = 2; pdStone = 2;
	}
	else if (!UpgIsDone(&melnica_GETRES2))
	{
		pdFood = 10; pdWood = 4; pdStone = 2;
	}
	else
	{
		pdFood = 7; pdWood = 6; pdStone = 3;
	}
	SetPDistribution(pdFood, pdWood, pdStone);

	// GALERA(UA) is gated by AKA06UA, which the original never registers
	// (dead TryUnits there too - kept for fidelity).
	if (!UpgIsDone(&AKA30))
	{
		TryUnit(&galera, 3, 100, 50);
		return;
	}
	SetPDistribution(6, 7, 3);
	TryUnit(&galera, 7, 70, 50);
}

extern "C" __declspec(dllexport) void ProcessAI()
{
	// The original also reads GetDifficulty()/GetStartRes() into two
	// never-read global pairs (10/30/100 and 30/50/100 knobs) - dead
	// state, omitted here (same call pattern exists in Turcia.dll).
	int land = GetLandType();
	if (land == LT_LAND)
	{
		ProcessLandAI();
		return;
	}
	// Land types 5+ (continents-large) are not handled at all.
	if (land < 1 || land > 4)
	{
		return;
	}
	ProcessNonLandAI(land);
}

#ifdef _WIN32
BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		{
			hLib = GetModuleHandleA("DMCR.EXE");
			if (hLib == nullptr)
			{
				MessageBoxA(NULL, "Can not detect DMCR.EXE", "ERROR!", 0);
				return TRUE;
			}
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
			OnInit();
		}
		break;
	default:
		break;
	}
	return TRUE;
};
#else
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
	OnInit();
}
#endif
