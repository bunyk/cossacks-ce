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


// Registers Russia's units/upgrades with the game. Names must match the
// per-nation script RUSSIA.NDS (listed in Nations.lst, packed in all.gsc):
// every object is "<base>(<NatCharHi>)" - for Russia the suffix is (RU).
// Cross-checked against the strings of the original AI/Russia.dll.
// Ported from the Turkey module (Algeria lineage) by role mapping:
//   Pikiner_turki -> Pik_rus        (pike,   ATTACK/SHIELD 3..6 chain)
//   Pehota_turki  -> Strelec        (17th c. musketeer)
//   Ianithar      -> Mushketer      (18th c. musketeer)
//   TUrkey_pikiner-> Kirasir        (18th c. melee, ATTACK/SHIELD 3..7 chain)
//   Oficer_turki  -> Russki_Komandir, Barabanshik_Turki -> Barabanshik_1
//   Center_Turki  -> Russki_Center, Barack_Turki -> Rus_Strel_Dom
//     (NOT Kazarma: RUSSIA.NDS gates Kazarma behind the MAINRU "XVIII
//     century" research - "ENABLE UNITS GRP 4 Kazarma Dragun_18 Gusar_evro
//     Kirasir". Rus_Strel_Dom is the 17th c. barracks producing Pik_rus/
//     Strelec; Kazarma is registered separately and built after MAIN.)
//   Minaret       -> akademia_RU (academy), Mechet_Turki -> Cerkov_RU
//   Kuznia_Turki  -> Kuznica, Konushnia_Turki -> konushnia
//   Diplomatic_Turki -> Dip_korpus_RU, Art_Depo_Turki -> artileri_depo
//   Rinok_Turki   -> Rinok, Sclad4 -> Sclad3, Melnica_tu -> Melnica_rus
//   Dom_Turki     -> ruskaia_izba, Krestian_Turki -> Kreposnoi (peasant)
//   Port          -> PorR, Shebeka -> Fregat (mid-tier warship)
// AKA16/AKA17/AKA27 were dropped: registered by the Algeria/Turkey AIs but
// NOT by Russia.dll (its academy set is AKA01..15, 20, 23, 24, 28..36).
// Not yet ported from Russia.dll (it registers more than Turkey does):
//   KUZ04/KUZ06, AKA11..15,31..36, WALL_KR, Bashnia_2 + PAUSE3..7 tower
//   upgrades, Dragun/Dragun_18 + their ATTACK/SHIELD chains, Grenader(RU),
//   Tiagoli_koval_rus, Tiagoli_kozak, Sveshenik_ukr, Strelec_Algir_DIP,
//   Dragun_18_DIP, Mushketer upgrades, Melnica_rus GETRES2, Yahta,
//   Pushka_mnogostvolka.
void RegisterRussia()
{
	RegisterUnitType(&pikiner, "Pik_rus(RU)");
	RegisterUnitType(&krestian, "Kreposnoi(RU)");
	RegisterUnitType(&pehota, "Strelec(RU)");
	RegisterUnitType(&grenader, "Grenader_DIP(RU)");
	RegisterUnitType(&strelec, "Mushketer(RU)");
	RegisterUnitType(&mameluk, "Kirasir(RU)");
	RegisterUnitType(&pushka, "Pushka(RU)");
	RegisterUnitType(&mortira, "Mortira(RU)");
	RegisterUnitType(&mortira_b, "Mortira_b(RU)");
	RegisterUnitType(&lodka, "Lodka(RU)");
	// Russia's 18th-century barracks. Enabled only by the MAINRU (XVIII
	// century) research - built in ProcessAI once MAIN is done.
	RegisterUnitType(&kazarma, "Kazarma(RU)");
	AssignFormUnit("Pik_rus(RU)");
	RegisterUnitType(&strelec, "Mushketer(RU)");
	RegisterUnitType(&kozak, "Kozak_loshad_DIP(RU)");
	RegisterUnitType(&oficer, "Russki_Komandir(RU)");
	RegisterUnitType(&barabanshik, "Barabanshik_1(RU)");
	RegisterUpgrade(&officer_ATTACK, "Russki_Komandir(RU)ATTACK");
	RegisterUpgrade(&officer_SHIELD, "Russki_Komandir(RU)SHIELD");
	RegisterUnitType(&center, "Russki_Center(RU)");
	RegisterUnitType(&melnica, "Melnica_rus(RU)");
	RegisterUnitType(&barack, "Rus_Strel_Dom(RU)");
	RegisterUnitType(&sclad, "Sclad3(RU)");
	RegisterUnitType(&minaret, "akademia_RU(RU)");
	RegisterUnitType(&konushnia, "konushnia(RU)");
	RegisterUnitType(&diplomatic, "Dip_korpus_RU(RU)");
	RegisterUnitType(&art_depo, "artileri_depo(RU)");
	RegisterUnitType(&rinok, "Rinok(RU)");
	RegisterUnitType(&kuznia, "Kuznica(RU)");
	RegisterUnitType(&mechet, "Cerkov_RU(RU)");
	RegisterUnitType(&port, "PorR(RU)");
	RegisterUnitType(&galera, "GALERA(RU)");
	RegisterUnitType(&peres_kor, "PERES_KOR(RU)");
	RegisterUnitType(&shebeka, "Fregat(RU)");
	RegisterUnitType(&linkor, "Linkor(RU)");
	RegisterUpgrade(&AKA29, "AKA29RU");
	RegisterUpgrade(&AKA06, "AKA06RU");
	RegisterUpgrade(&Melnica_tuGETRES, "Melnica_rus(RU)GETRES");
	RegisterUpgrade(&KUZ01, "KUZ01RU");
	RegisterUpgrade(&pikiner_ATTACK, "Pik_rus(RU)ATTACK");
	RegisterUpgrade(&pikiner_ATTACK3, "Pik_rus(RU)ATTACK3");
	RegisterUpgrade(&pikiner_ATTACK4, "Pik_rus(RU)ATTACK4");
	RegisterUpgrade(&pikiner_ATTACK5, "Pik_rus(RU)ATTACK5");
	RegisterUpgrade(&pikiner_ATTACK6, "Pik_rus(RU)ATTACK6");
	RegisterUpgrade(&pikiner_SHIELD, "Pik_rus(RU)SHIELD");
	RegisterUpgrade(&pikiner_SHIELD3, "Pik_rus(RU)SHIELD3");
	RegisterUpgrade(&pikiner_SHIELD4, "Pik_rus(RU)SHIELD4");
	RegisterUpgrade(&pikiner_SHIELD5, "Pik_rus(RU)SHIELD5");
	RegisterUpgrade(&pikiner_SHIELD6, "Pik_rus(RU)SHIELD6");
	RegisterUpgrade(&pehota_ATTACK, "Strelec(RU)ATTACK");
	RegisterUpgrade(&pehota_ATTACK3, "Strelec(RU)ATTACK3");
	RegisterUpgrade(&pehota_ATTACK4, "Strelec(RU)ATTACK4");
	RegisterUpgrade(&pehota_SHIELD, "Strelec(RU)SHIELD");
	RegisterUpgrade(&pehota_SHIELD3, "Strelec(RU)SHIELD3");
	RegisterUpgrade(&pehota_SHIELD4, "Strelec(RU)SHIELD4");
	RegisterUpgrade(&mameluk_ATTACK, "Kirasir(RU)ATTACK");
	RegisterUpgrade(&mameluk_ATTACK3, "Kirasir(RU)ATTACK3");
	RegisterUpgrade(&mameluk_ATTACK4, "Kirasir(RU)ATTACK4");
	RegisterUpgrade(&mameluk_ATTACK5, "Kirasir(RU)ATTACK5");
	RegisterUpgrade(&mameluk_ATTACK6, "Kirasir(RU)ATTACK6");
	RegisterUpgrade(&mameluk_ATTACK7, "Kirasir(RU)ATTACK7");
	RegisterUpgrade(&mameluk_SHIELD, "Kirasir(RU)SHIELD");
	RegisterUpgrade(&mameluk_SHIELD3, "Kirasir(RU)SHIELD3");
	RegisterUpgrade(&mameluk_SHIELD4, "Kirasir(RU)SHIELD4");
	RegisterUpgrade(&mameluk_SHIELD5, "Kirasir(RU)SHIELD5");
	RegisterUpgrade(&mameluk_SHIELD6, "Kirasir(RU)SHIELD6");
	RegisterUpgrade(&mameluk_SHIELD7, "Kirasir(RU)SHIELD7");
	RegisterUpgrade(&pushka_BUILD, "Pushka(RU)BUILD");
	RegisterUpgrade(&pushka_BUILD3, "Pushka(RU)BUILD3");
	RegisterUpgrade(&pushka_BUILD4, "Pushka(RU)BUILD4");
	RegisterUpgrade(&pushka_BUILD5, "Pushka(RU)BUILD5");
	RegisterUpgrade(&pushka_BUILD6, "Pushka(RU)BUILD6");
	RegisterUpgrade(&mortira_BUILD, "Mortira(RU)BUILD");
	RegisterUpgrade(&mortira_BUILD3, "Mortira(RU)BUILD3");
	RegisterUpgrade(&mortira_BUILD4, "Mortira(RU)BUILD4");
	RegisterUpgrade(&mortira_BUILD5, "Mortira(RU)BUILD5");
	RegisterUpgrade(&KUZ03, "KUZ03RU");
	RegisterUpgrade(&AKA01, "AKA01RU");
	RegisterUpgrade(&AKA02, "AKA02RU");
	RegisterUpgrade(&AKA03, "AKA03RU");
	RegisterUpgrade(&AKA04, "AKA04RU");
	RegisterUpgrade(&AKA08, "AKA08RU");
	RegisterUpgrade(&AKA23, "AKA23RU");
	RegisterUpgrade(&AKA24, "AKA24RU");
	RegisterUpgrade(&AKA10, "AKA10RU");
	RegisterUpgrade(&AKA30, "AKA30RU");
	RegisterUpgrade(&AKA07, "AKA07RU");
	RegisterUpgrade(&AKA20, "AKA20RU");
	RegisterUpgrade(&KUZ02, "KUZ02RU");
	RegisterUpgrade(&AKA09, "AKA09RU");
	RegisterUpgrade(&AKA05, "AKA05RU");
	RegisterUpgrade(&AKA28, "AKA28RU");
	RegisterUpgrade(&KUZ05, "KUZ05RU");
	// XVIII century: unlocks Kazarma + the 18th c. units (Mushketer,
	// Kirasir, Grenader, Dragun_18, Gusar_evro).
	RegisterUpgrade(&MAIN, "MAINRU");

	SetPDistribution(9, 4, 4);
	AssignAmountOfMineUpgrades(6);
	AssignMineUpgrade(0, "shahta(RU)INSIDE", 90);
	AssignMineUpgrade(1, "shahta(RU)INSIDE3", 50);
	AssignMineUpgrade(2, "shahta(RU)INSIDE4", 50);
	AssignMineUpgrade(3, "shahta(RU)INSIDE5", 50);
	AssignMineUpgrade(4, "shahta(RU)INSIDE6", 50);
	AssignMineUpgrade(5, "shahta(RU)INSIDE7", 50);
	AssignPeasant("Kreposnoi(RU)");
	AssignMine("shahta(RU)");
	AssignHouse("ruskaia_izba(RU)");
}

extern "C" __declspec(dllexport) void InitAI()
{
	RegisterRussia();
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
		// Russia: research XVIII century once the late-game academy
		// chain is underway, then build the 18th c. barracks so the
		// 18th c. units (Mushketer, Kirasir) become trainable.
		TryUpgrade(&MAIN, 100, 100);
		if (UpgIsDone(&MAIN))
		{
			TryUnit(&kazarma, 1, 100, 100);
		}
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
