// Tutorial 1: the first Russian tutorial mission (build the town center,
// mill, store and market, then gather resources).
//
// Ported 1:1 from the original Missions/Tutorial/Tutorial1/Mission.dll
// (decompilation: /tmp/tutorial1_decomp). The original is an MSVC DEBUG
// build: every API call is followed by a __chkesp() stack check and the
// DLL carries dead duplicates of OnInit/ProcessScenary (FUN_10001090 /
// FUN_100014a0). Both are stripped here; the call sequence, constants and
// control flow are otherwise exact.
//
// Engine contract (activescenary.cpp): the game LoadLibrary()s this
// module, DllMain runs OnInit() immediately (here: a load constructor),
// and ProcessScenary() is called every mission tick.
#include "../../common/mission_api.h"

// Registered engine-visible variables (original DAT_100303e8/e4/e0):
// the wood/stone/food stock at the moment tutorial step 15 ends - the
// PAGE16/17 checks compare against them to see what the player gathered.
static int resWoodStart;
static int resStoneStart;
static int resFoodStart;

// Mission objects (original DAT_100304xx slots, registration order kept).
static GAMEOBJ russki_center;   // "Russki_Center(RU)"  town hall
static GAMEOBJ ruskaia_izba;     // "ruskaia_izba(RU)"  house
static GAMEOBJ melnica_rus;
static GAMEOBJ sclad3;
static GAMEOBJ shahta;
static GAMEOBJ kuznica;
static GAMEOBJ konushnia;
static GAMEOBJ cerkov_ru;
static GAMEOBJ akademia_ru;
static GAMEOBJ dip_korpus_ru;
static GAMEOBJ rus_strel_dom;
static GAMEOBJ kazarma;
static GAMEOBJ artileri_depo;
static GAMEOBJ rinok;
static GAMEOBJ porr;
static GAMEOBJ kreposnoi;
static GAMEOBJ wall_ukr;
static GAMEOBJ wall_kr;
static GAMEOBJ bashnia_2;
static GAMEOBJ peasants_group; // dynamic group: currently selected units
static GAMEOBJ main_upg1;  // "MAINRU" - registered twice, like the original
static GAMEOBJ main_upg2;  // (DAT_10030490 / DAT_10030498)

extern "C" __declspec(dllexport) void OnInit()
{
	RegisterVar(&resWoodStart, 4);
	RegisterVar(&resStoneStart, 4);
	RegisterVar(&resFoodStart, 4);
	RegisterUnitType(&russki_center, "Russki_Center(RU)");
	RegisterUnitType(&ruskaia_izba, "ruskaia_izba(RU)");
	RegisterUnitType(&melnica_rus, "Melnica_rus(RU)");
	RegisterUnitType(&sclad3, "Sclad3(RU)");
	RegisterUnitType(&shahta, "shahta(RU)");
	RegisterUnitType(&kuznica, "Kuznica(RU)");
	RegisterUnitType(&konushnia, "konushnia(RU)");
	RegisterUnitType(&cerkov_ru, "Cerkov_RU(RU)");
	RegisterUnitType(&akademia_ru, "akademia_RU(RU)");
	RegisterUnitType(&dip_korpus_ru, "Dip_korpus_RU(RU)");
	RegisterUnitType(&rus_strel_dom, "Rus_Strel_Dom(RU)");
	RegisterUnitType(&kazarma, "Kazarma(RU)");
	RegisterUnitType(&artileri_depo, "artileri_depo(RU)");
	RegisterUnitType(&rinok, "Rinok(RU)");
	RegisterUnitType(&porr, "PorR(RU)");
	RegisterUnitType(&kreposnoi, "Kreposnoi(RU)");
	RegisterUnitType(&wall_ukr, "WALL_UKR(RU)");
	RegisterUnitType(&wall_kr, "WALL_KR(RU)");
	RegisterUnitType(&bashnia_2, "Bashnia_2(RU)");
	RegisterDynGroup(&peasants_group);
	RegisterVar(&peasants_group, 8);
	RegisterUpgrade(&main_upg1, "MAINRU");
	RegisterUpgrade(&main_upg2, "MAINRU");
	ChangeFriends(0, 7);
	ChangeFriends(1, 0xff);
	ChangeFriends(2, 0xff);
}

extern "C" __declspec(dllexport) void ProcessScenary()
{
	if (Trigg(1))
	{
		SetTrigg(1, 0);
		// Lock the XVIII century and every building until the tutorial
		// enables them step by step.
		DisableUpgrade(0, &main_upg1);
		EnableUnit(0, &russki_center, 0);
		EnableUnit(0, &ruskaia_izba, 0);
		EnableUnit(0, &melnica_rus, 0);
		EnableUnit(0, &sclad3, 0);
		EnableUnit(0, &shahta, 0);
		EnableUnit(0, &kuznica, 0);
		EnableUnit(0, &konushnia, 0);
		EnableUnit(0, &cerkov_ru, 0);
		EnableUnit(0, &akademia_ru, 0);
		EnableUnit(0, &dip_korpus_ru, 0);
		EnableUnit(0, &rus_strel_dom, 0);
		EnableUnit(0, &kazarma, 0);
		EnableUnit(0, &artileri_depo, 0);
		EnableUnit(0, &rinok, 0);
		EnableUnit(0, &porr, 0);
		EnableUnit(0, &wall_ukr, 0);
		EnableUnit(0, &wall_kr, 0);
		EnableUnit(0, &bashnia_2, 0);
		SetResource(0, 3, 3000); // food
		SetResource(0, 1, 350);  // gold (original 0x15e)
		SetResource(0, 4, 10);   // iron
		SetResource(0, 2, 1000); // stone
		SetResource(0, 0, 1000); // wood
		SetResource(0, 5, 10);   // coal
		// Disable the objectives (mission hint letters) until reached.
		DisableMission(0x42);
		DisableMission(0x43);
		DisableMission(0x44);
		DisableMission(0x45);
		DisableMission(0x46);
		DisableMission(0x48);
		DisableMission(0x49);
		DisableMission(0x47);
		DisableMission(0x4b);
		DisableMission(0x4c);
		DisableMission(0x4d);
		DisableMission(0x4e);
		DisableMission(0x4f);
		DisableMission(0x50);
		// Clear every script trigger except 4 (the intro timer gate).
		SetTrigg(2, 0);
		SetTrigg(5, 0);
		SetTrigg(6, 0);
		SetTrigg(7, 0);
		SetTrigg(8, 0);
		SetTrigg(9, 0);
		SetTrigg(10, 0);
		SetTrigg(0xb, 0);
		SetTrigg(0xc, 0);
		SetTrigg(0xd, 0);
		SetTrigg(0xe, 0);
		SetTrigg(0xf, 0);
		SetTrigg(0x10, 0);
		SetTrigg(0x11, 0);
		SetTrigg(0x12, 0);
		SetTrigg(0x13, 0);
		SetTrigg(0x14, 0);
		SetTrigg(0x15, 0);
		SetTrigg(0x16, 0);
		SetTrigg(0x17, 0);
		SetTrigg(0x18, 0);
		SetTrigg(0x19, 0);
		SetTrigg(0x1a, 0);
		SetTrigg(0x1b, 0);
		SetTrigg(0x1c, 0);
		RunTimer(1, 80);
	}

	// Intro pages after the initial 80-frame timer.
	if (Trigg(4) && TimerDone(1))
	{
		SetTrigg(4, 0);
		ShowPage("#PAGE1");
		ShowPage("#PAGE2");
		ClearSelection(0);
		SetTrigg(2, 1);
		RunTimer(4, 300);
	}

	// Track the player's selection while step "select your peasants" runs.
	if (Trigg(2))
	{
		SaveSelectedUnits(0, &peasants_group, 0);
	}

	// Peasants selected -> advance to the town-center step.
	if (Trigg(3) && GetTotalAmount0(&peasants_group) > 0 && TimerDone(4))
	{
		SelectUnits1(0, &peasants_group, 0);
		SetTrigg(2, 0);
		SetTrigg(3, 0);
		SetTrigg(5, 1);
		SetTrigg(6, 1);
		ShowPage("#PAGE3");
		ClearSelection(0);
	}

	if (Trigg(5))
	{
		SaveSelectedUnits(0, &peasants_group, 0);
	}

	// 10+ units selected -> enable the house objective.
	if (Trigg(6) && GetTotalAmount0(&peasants_group) > 9)
	{
		DisableMission(0x41);
		EnableMission(0x42);
		SetTrigg(6, 0);
		SetTrigg(5, 0);
		ShowPage("#PAGE4");
		RunTimer(2, 350); // original 0x15e
		SetTrigg(7, 1);
	}

	// Wait, then unlock the mill.
	if (Trigg(7) && TimerDone(2))
	{
		SetTrigg(7, 0);
		ShowPage("#PAGE5");
		ShowPage("#PAGE6");
		ClearSelection(0);
		EnableUnit(0, &melnica_rus, 1);
		SetTrigg(8, 1);
	}

	// Mill placed -> next page.
	if (Trigg(8) && GetTotalAmount1(&melnica_rus, 0) > 0)
	{
		SetTrigg(8, 0);
		ShowPage("#PAGE7");
		SetTrigg(9, 1);
	}

	// Mill built (ready) -> unlock the store.
	if (Trigg(9) && GetReadyAmount(&melnica_rus, 0) > 0)
	{
		SetTrigg(9, 0);
		EnableMission(0x43);
		EnableMission(0x42);
		DisableMission(0x42);
		ShowPage("#PAGE8");
		RunTimer(3, 400);
		SetTrigg(10, 1);
	}

	if (Trigg(10) && TimerDone(3))
	{
		SetTrigg(10, 0);
		ShowPage("#PAGE9");
		DisableUpgrade(0, &main_upg2);
		RunTimer(5, 500);
		SetTrigg(0xb, 1);
	}

	// Unlock the town center.
	if (Trigg(0xb) && TimerDone(5))
	{
		SetTrigg(0xb, 0);
		ShowPage("#PAGE10");
		ShowPage("#PAGE11");
		ClearSelection(0);
		EnableUnit(0, &russki_center, 1);
		SetTrigg(0xc, 1);
	}

	// Town center built -> unlock the store building.
	if (Trigg(0xc) && GetReadyAmount(&russki_center, 0) > 0)
	{
		SetTrigg(0xc, 0);
		ShowPage("#PAGE12");
		EnableMission(0x44);
		EnableMission(0x4c);
		DisableMission(0x43);
		RunTimer(6, 400);
		EnableUnit(0, &sclad3, 1);
		SetTrigg(0xd, 1);
	}

	if (Trigg(0xd) && TimerDone(6))
	{
		SetTrigg(0xd, 0);
		ShowPage("#PAGE13");
		SetTrigg(0xe, 1);
	}

	// Store built -> unlock the mine.
	if (Trigg(0xe) && GetReadyAmount(&sclad3, 0) > 0)
	{
		SetTrigg(0xe, 0);
		SetTrigg(0xf, 1);
		EnableMission(0x45);
		EnableMission(0x4d);
		DisableMission(0x44);
		ShowPage("#PAGE14");
		RunTimer(7, 600);
	}

	// Snapshot the resource stock for the "gather more" checks below.
	if (Trigg(0xf) && TimerDone(7))
	{
		SetTrigg(0xf, 0);
		ShowPage("#PAGE15");
		resWoodStart = GetResource(0, 0);
		resStoneStart = GetResource(0, 2);
		resFoodStart = GetResource(0, 3);
		SetTrigg(0x10, 1);
	}

	// Any resource grown -> tell the player to keep gathering.
	if (Trigg(0x10))
	{
		if (!(GetResource(0, 0) <= resWoodStart
			&& GetResource(0, 2) <= resStoneStart
			&& GetResource(0, 3) <= resFoodStart))
		{
			SetTrigg(0x10, 0);
			ShowPage("#PAGE16");
			RunTimer(8, 300);
			SetTrigg(0x11, 1);
		}
	}

	// Wood < 2048 or stone < 2100 -> "keep gathering" page (the original
	// tests > 2047 / > 2099 to skip; kept exactly).
	if (Trigg(0x11) && TimerDone(8))
	{
		if (!(GetResource(0, 0) > 0x7f7 && GetResource(0, 2) > 0x833))
		{
			SetTrigg(0x11, 0);
			ShowPage("#PAGE17");
			SetTrigg(0x12, 1);
		}
	}

	// Wood > 2048 and stone > 2100 -> next step.
	if (Trigg(0x12))
	{
		if (GetResource(0, 0) > 0x7f8 && GetResource(0, 2) > 0x834)
		{
			SetTrigg(0x12, 0);
			ShowPage("#PAGE18");
			SetTrigg(0x13, 1);
		}
	}

	// Second town center (a house for the new peasants) built.
	if (Trigg(0x13) && GetReadyAmount(&russki_center, 0) > 1)
	{
		SetTrigg(0x13, 0);
		ShowPage("#PAGE19");
		EnableMission(0x46);
		EnableMission(0x4e);
		DisableMission(0x45);
		SetTrigg(0x14, 1);
		SetTrigg(0x15, 1);
	}

	if (Trigg(0x14))
	{
		SaveSelectedUnits(0, &peasants_group, 0);
	}

	// Selected peasants sent to the town center -> market step.
	if (Trigg(0x15) && GetTotalAmount2(&peasants_group, &russki_center, 0) > 0)
	{
		SetTrigg(0x15, 0);
		ShowPage("#PAGE20");
		SetTrigg(0x16, 1);
		EnableMission(0x48);
		DisableMission(0x46);
		RunTimer(9, 150); // original 0x96
	}

	if (Trigg(0x16) && TimerDone(9))
	{
		SetTrigg(0x16, 0);
		ShowPage("#PAGE21");
		SetTrigg(0x17, 1);
		RunTimer(10, 500);
	}

	// 450+ wood gathered -> unlock the market.
	if (Trigg(0x17) && TimerDone(10) && GetResource(0, 0) > 0x1c2)
	{
		SetTrigg(0x17, 0);
		ClearSelection(0);
		EnableUnit(0, &rinok, 1);
		EnableMission(0x47);
		EnableMission(0x4f);
		DisableMission(0x48);
		SetTrigg(0x1c, 1);
		ShowPage("#PAGE22");
	}

	// Market built -> unlock the mine.
	if (Trigg(0x1c) && GetReadyAmount(&rinok, 0) > 0)
	{
		SetTrigg(0x1c, 0);
		ShowPage("#PAGE23");
		SetTrigg(0x18, 1);
		RunTimer(0xb, 500);
	}

	if (Trigg(0x18) && TimerDone(0xb))
	{
		SetTrigg(0x18, 0);
		ShowPage("#PAGE24");
		SetTrigg(0x19, 1);
		RunTimer(0xc, 400);
	}

	// Unlock the gold mine (shahta).
	if (Trigg(0x19) && TimerDone(0xc))
	{
		SetTrigg(0x19, 0);
		ShowPage("#PAGE25");
		ClearSelection(0);
		EnableUnit(0, &shahta, 1);
		EnableMission(0x49);
		DisableMission(0x47);
		SetTrigg(0x1a, 1);
	}

	// Mine built -> final pages.
	if (Trigg(0x1a) && GetReadyAmount(&shahta, 0) > 0)
	{
		SetTrigg(0x1a, 0);
		ShowPage("#PAGE26");
		ShowPage("#PAGE28");
		EnableMission(0x50);
		DisableMission(0x49);
		SetTrigg(0x1b, 1);
		RunTimer(0xd, 500);
	}

	if (Trigg(0x1b) && TimerDone(0xd))
	{
		SetTrigg(0x1b, 0);
		ShowPage("#PAGE27");
		SetTrigg(99, 0);
	}

	// Defeat: the player's town is destroyed.
	if (Trigg(100) && NationIsErased(0))
	{
		SetTrigg(100, 0);
		ShowPage("#PAGE100");
	}

	// Victory (fires once trigger 99 is cleared by the last step).
	if (!Trigg(99))
	{
		SetTrigg(99, 0);
		ShowPage("#PAGE99");
		ShowVictory();
	}
}

// The original's DllMain(DLL_PROCESS_ATTACH) calls OnInit; dlopen runs
// this constructor at the same point in the mission load sequence.
__attribute__((constructor)) static void MissionOnLoad()
{
	OnInit();
}
