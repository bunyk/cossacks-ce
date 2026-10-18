#include <cstdio>

// Linux port: the game dlopen()s this module; game functions are resolved
// from the main executable (linked with -rdynamic) via dlsym(RTLD_DEFAULT).
// The module itself is compiled with -fvisibility=hidden so its own
// same-named globals (the function pointers in typedefs.h) are NOT
// interposed by the game's exported functions.
#include <dlfcn.h>
#include <unistd.h>
typedef void* HINSTANCE;
#define __declspec(x) __attribute__((visibility("default")))

#include "objects.h"
#include "typedefs.h"

#define LOAD_FUNCTION(x) x=(tp##x*)LoadFunction(#x)

#define WOOD  0
#define GOLD  1
#define STONE 2
#define FOOD  3
#define IRON  4
#define COAL  5


#define LT_LAND				0
#define LT_MEDITERRANEAN	1
#define LT_PENINSULAS		2
#define LT_ISLANDS			3
#define LT_CONTINENTS		4
#define LT_CONTINENT		5

struct GAMEOBJ;

HINSTANCE hLib = nullptr;

unsigned short LAND_BALANCE[38] = {
	0, 0, 0x13, 0x13, 0x19, 0x14, 0x21, 0x14, 0x2B, 0x14, 0x30, 0x1E, 0x35, 0x20, 0x3A, 0x23, 0x41, 0x23, 0x50,
	0x23, 0x69, 0x28, 0x7D, 0x32, 0x96, 0x3C, 0x0AF, 0x46, 0x0C8, 0x50, 0x0E1, 0x50, 0x0FA, 0x50, 0x1F4, 0x64, 0x0FA0,
	0x12C
};

void* LoadFunction(const char* Name)
{
	// The main executable exports its extern "C" AI-API functions
	// (activescenary.cpp) into the global symbol scope.
	void* fn = dlsym(RTLD_DEFAULT, Name);
	if (!fn)
	{
		fprintf(stderr, "AI module: game does not export function: %s\n", Name);
	};
	return fn;
};


// Registers Turkey's units/upgrades with the game. Names must match the
// per-nation script TURKISH.NDS (listed in Nations.lst, packed in all.gsc):
// every object is "<base>(<NatCharHi>)" - for Turkey the suffix is (TU).
// Cross-checked against the strings of the original AI/Turcia.dll.
// Ported from Algeria by role mapping:
//   Strelec_Algir -> Ianithar          (shooter)
//   Mameluk       -> TUrkey_pikiner    (melee with ATTACK/SHIELD 3..7 chain)
//   Center_Algir  -> Center_Turki, Dom_Algir -> Dom_Turki
// Not yet ported from Turcia.dll (it registers more than Algeria does):
//   Bashnia_3 + PAUSE3..7 tower upgrades, Dragun_18_DIP, Strelec_Algir_DIP,
//   Ianithar ATTACK/SHIELD upgrades, AKA11..15, AKA31, AKA33.
void RegisterTurkey()
{
	RegisterUnitType(&pikiner, "Pikiner_turki(TU)");
	RegisterUnitType(&krestian, "Krestian_Turki(TU)");
	RegisterUnitType(&pehota, "Pehota_turki(TU)");
	RegisterUnitType(&grenader, "Grenader_DIP(TU)");
	RegisterUnitType(&strelec, "Ianithar(TU)");
	RegisterUnitType(&mameluk, "TUrkey_pikiner(TU)");
	RegisterUnitType(&pushka, "Pushka(TU)");
	RegisterUnitType(&mortira, "Mortira(TU)");
	RegisterUnitType(&mortira_b, "Mortira_b(TU)");
	RegisterUnitType(&lodka, "Lodka(TU)");
	AssignFormUnit("Pikiner_turki(TU)");
	RegisterUnitType(&strelec, "Ianithar(TU)");
	RegisterUnitType(&kozak, "Kozak_loshad_DIP(TU)");
	RegisterUnitType(&oficer, "Oficer_turki(TU)");
	RegisterUnitType(&barabanshik, "Barabanshik_Turki(TU)");
	RegisterUpgrade(&officer_ATTACK, "Oficer_turki(TU)ATTACK");
	RegisterUpgrade(&officer_SHIELD, "Oficer_turki(TU)SHIELD");
	RegisterUnitType(&center, "Center_Turki(TU)");
	RegisterUnitType(&melnica, "Melnica_tu(TU)");
	RegisterUnitType(&barack, "Barack_Turki(TU)");
	RegisterUnitType(&sclad, "Sclad4(TU)");
	RegisterUnitType(&minaret, "Minaret(TU)");
	RegisterUnitType(&konushnia, "Konushnia_Turki(TU)");
	RegisterUnitType(&diplomatic, "Diplomatic_Turki(TU)");
	RegisterUnitType(&art_depo, "Art_Depo_Turki(TU)");
	RegisterUnitType(&rinok, "Rinok_Turki(TU)");
	RegisterUnitType(&kuznia, "Kuznia_Turki(TU)");
	RegisterUnitType(&mechet, "Mechet_Turki(TU)");
	RegisterUnitType(&port, "Port(TU)");
	RegisterUnitType(&galera, "GALERA(TU)");
	RegisterUnitType(&peres_kor, "PERES_KOR(TU)");
	RegisterUnitType(&shebeka, "Shebeka(TU)");
	RegisterUnitType(&linkor, "Linkor(TU)");
	RegisterUpgrade(&AKA29, "AKA29TU");
	RegisterUpgrade(&AKA06, "AKA06TU");
	RegisterUpgrade(&Melnica_tuGETRES, "Melnica_tu(TU)GETRES");
	RegisterUpgrade(&KUZ01, "KUZ01TU");
	RegisterUpgrade(&pikiner_ATTACK, "Pikiner_turki(TU)ATTACK");
	RegisterUpgrade(&pikiner_ATTACK3, "Pikiner_turki(TU)ATTACK3");
	RegisterUpgrade(&pikiner_ATTACK4, "Pikiner_turki(TU)ATTACK4");
	RegisterUpgrade(&pikiner_ATTACK5, "Pikiner_turki(TU)ATTACK5");
	RegisterUpgrade(&pikiner_ATTACK6, "Pikiner_turki(TU)ATTACK6");
	RegisterUpgrade(&pikiner_SHIELD, "Pikiner_turki(TU)SHIELD");
	RegisterUpgrade(&pikiner_SHIELD3, "Pikiner_turki(TU)SHIELD3");
	RegisterUpgrade(&pikiner_SHIELD4, "Pikiner_turki(TU)SHIELD4");
	RegisterUpgrade(&pikiner_SHIELD5, "Pikiner_turki(TU)SHIELD5");
	RegisterUpgrade(&pikiner_SHIELD6, "Pikiner_turki(TU)SHIELD6");
	RegisterUpgrade(&pehota_ATTACK, "Pehota_turki(TU)ATTACK");
	RegisterUpgrade(&pehota_ATTACK3, "Pehota_turki(TU)ATTACK3");
	RegisterUpgrade(&pehota_ATTACK4, "Pehota_turki(TU)ATTACK4");
	RegisterUpgrade(&pehota_SHIELD, "Pehota_turki(TU)SHIELD");
	RegisterUpgrade(&pehota_SHIELD3, "Pehota_turki(TU)SHIELD3");
	RegisterUpgrade(&pehota_SHIELD4, "Pehota_turki(TU)SHIELD4");
	RegisterUpgrade(&mameluk_ATTACK, "TUrkey_pikiner(TU)ATTACK");
	RegisterUpgrade(&mameluk_ATTACK3, "TUrkey_pikiner(TU)ATTACK3");
	RegisterUpgrade(&mameluk_ATTACK4, "TUrkey_pikiner(TU)ATTACK4");
	RegisterUpgrade(&mameluk_ATTACK5, "TUrkey_pikiner(TU)ATTACK5");
	RegisterUpgrade(&mameluk_ATTACK6, "TUrkey_pikiner(TU)ATTACK6");
	RegisterUpgrade(&mameluk_ATTACK7, "TUrkey_pikiner(TU)ATTACK7");
	RegisterUpgrade(&mameluk_SHIELD, "TUrkey_pikiner(TU)SHIELD");
	RegisterUpgrade(&mameluk_SHIELD3, "TUrkey_pikiner(TU)SHIELD3");
	RegisterUpgrade(&mameluk_SHIELD4, "TUrkey_pikiner(TU)SHIELD4");
	RegisterUpgrade(&mameluk_SHIELD5, "TUrkey_pikiner(TU)SHIELD5");
	RegisterUpgrade(&mameluk_SHIELD6, "TUrkey_pikiner(TU)SHIELD6");
	RegisterUpgrade(&mameluk_SHIELD7, "TUrkey_pikiner(TU)SHIELD7");
	RegisterUpgrade(&pushka_BUILD, "Pushka(TU)BUILD");
	RegisterUpgrade(&pushka_BUILD3, "Pushka(TU)BUILD3");
	RegisterUpgrade(&pushka_BUILD4, "Pushka(TU)BUILD4");
	RegisterUpgrade(&pushka_BUILD5, "Pushka(TU)BUILD5");
	RegisterUpgrade(&pushka_BUILD6, "Pushka(TU)BUILD6");
	RegisterUpgrade(&mortira_BUILD, "Mortira(TU)BUILD");
	RegisterUpgrade(&mortira_BUILD3, "Mortira(TU)BUILD3");
	RegisterUpgrade(&mortira_BUILD4, "Mortira(TU)BUILD4");
	RegisterUpgrade(&mortira_BUILD5, "Mortira(TU)BUILD5");
	RegisterUpgrade(&KUZ03, "KUZ03TU");
	RegisterUpgrade(&AKA01, "AKA01TU");
	RegisterUpgrade(&AKA02, "AKA02TU");
	RegisterUpgrade(&AKA03, "AKA03TU");
	RegisterUpgrade(&AKA04, "AKA04TU");
	RegisterUpgrade(&AKA08, "AKA08TU");
	RegisterUpgrade(&AKA23, "AKA23TU");
	RegisterUpgrade(&AKA24, "AKA24TU");
	RegisterUpgrade(&AKA10, "AKA10TU");
	RegisterUpgrade(&AKA30, "AKA30TU");
	RegisterUpgrade(&AKA07, "AKA07TU");
	RegisterUpgrade(&AKA20, "AKA20TU");
	RegisterUpgrade(&KUZ02, "KUZ02TU");
	RegisterUpgrade(&AKA09, "AKA09TU");
	RegisterUpgrade(&AKA05, "AKA05TU");
	RegisterUpgrade(&AKA28, "AKA28TU");
	RegisterUpgrade(&KUZ05, "KUZ05TU");
	RegisterUpgrade(&AKA16, "AKA16TU");
	RegisterUpgrade(&AKA17, "AKA17TU");
	RegisterUpgrade(&AKA27, "AKA27TU");

	SetPDistribution(9, 4, 4);
	AssignAmountOfMineUpgrades(6);
	AssignMineUpgrade(0, "shahta(TU)INSIDE", 90);
	AssignMineUpgrade(1, "shahta(TU)INSIDE3", 50);
	AssignMineUpgrade(2, "shahta(TU)INSIDE4", 50);
	AssignMineUpgrade(3, "shahta(TU)INSIDE5", 50);
	AssignMineUpgrade(4, "shahta(TU)INSIDE6", 50);
	AssignMineUpgrade(5, "shahta(TU)INSIDE7", 50);
	AssignPeasant("Krestian_Turki(TU)");
	AssignMine("shahta(TU)");
	AssignHouse("Dom_Turki(TU)");
}

extern "C" __declspec(dllexport) void InitAI()
{
	RegisterTurkey();
	SET_MINE_CAPTURE_RADIUS(290);
	SET_MINE_UPGRADE1_RADIUS(260);
	SET_MINE_UPGRADE2_RADIUS(130);
	SET_DEFAULT_MAX_WORKERS(300);
	SET_MIN_PEASANT_BRIGADE(5);
}

extern "C" __declspec(dllexport) void OnInit()
{
	InitAI();
}

void ProcessNonLandAI(int landType)
{
	SetMineBalanse(19, LAND_BALANCE);

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
			int units = GetUnits(&port);
			if (GetReadyUnits(&port) == units && GetUnits(&konushnia))
			{
				TryUnit(&port, 2, 50, 10);
				TryUnit(&port, 3, 20, 10);
			}
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
		TryUpgrade(&AKA09, 95, 100);
		TryUpgrade(&AKA20, 95, 100);
		if (UpgIsDone(&AKA20))
		{
			TryUpgrade(&AKA28, 90, 100);
			TryUpgrade(&AKA27, 90, 100);
			TryUpgrade(&AKA16, 90, 100);
			TryUpgrade(&AKA17, 90, 100);
		}
	}
	else
	{
		TryUnit(&galera, 3, 100, 50);
	}
}

void ProcessLandAI()
{
	SetMineBalanse(19, LAND_BALANCE);
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
			if (GetReadyUnits(&port) == GetUnits(&port) && GetUnits(&konushnia))
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

extern "C" __declspec(dllexport) void ProcessAI()
{
	int land = GetLandType();
	int diff = GetDifficulty();
	int startRes = GetStartRes();
	switch (land)
	{
	case LT_LAND:
		ProcessLandAI();
		break;
	case LT_MEDITERRANEAN:
	case LT_PENINSULAS:
	case LT_ISLANDS:
	case LT_CONTINENTS:
		ProcessNonLandAI(land);
		break;
	default:
		break;
	}

	if (land == 0 || land == 1)
	{
		int units = GetUnits(&pikiner);
		if (units / 36 > 0)
		{
			TryUnit(&oficer, units / 36, 50, 50);
			TryUnit(&barabanshik, units / 36, 10, 50);
		}
	}
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
	LOAD_FUNCTION(AssignFormUnit);
	LOAD_FUNCTION(GetAIRegister);
	LOAD_FUNCTION(SetAIRegister);
	OnInit();
}
