// Single mission 59 (Turkish defense scenario): hold the gates against
// the Ukrainian AI, then flip the captured units to your side.
//
// Ported 1:1 from Missions/Singles/Single59/Mission.dll (decompilation:
// /tmp/single59_decomp). The original imports the engine scenary API
// from dmcr.exe; here the calls are direct (mission_api.h).
#include "../../common/mission_api.h"

// Original FUN_10001000: the AI attack-timer delay per difficulty.
static int AttackDelay()
{
	switch (GetDiff(0))
	{
	case 0: return 50000;
	case 1: return 20000;
	case 2: return 15000;
	case 3: return 10000;
	default: return 100;
	}
}

static GAMEOBJ ggates;          // "Ggates" - the gates group
static GAMEOBJ pehota_turki;    // "Pehota_turki(TU)"
static GAMEOBJ pikiner_turki;    // "Pikiner_turki(TU)"
static GAMEOBJ turkey_pikiner;  // "TUrkey_pikiner(TU)"
static GAMEOBJ bashnia_3;       // "Bashnia_3(TU)"
static GAMEOBJ wall_tu;         // "WALL_TU(TU)"
static GAMEOBJ kuznica_ua;      // "Kuznica_ua(UA)"
static GAMEOBJ konushnia_ua;    // "konushnia_ua(UA)"
static GAMEOBJ strelcovaia;     // "Strelcovaia_Izba(UA)"
static GAMEOBJ grenader_dip;    // "Grenader_DIP(TU)"
static GAMEOBJ strelec_algir;   // "Strelec_Algir_DIP(TU)"
static GAMEOBJ kozak_loshad;    // "Kozak_loshad_DIP(TU)"
static GAMEOBJ dragun_18_dip;   // "Dragun_18_DIP(TU)"

extern "C" __declspec(dllexport) void OnInit()
{
	RegisterUnits(&ggates, "Ggates");
	RegisterUnitType(&pehota_turki, "Pehota_turki(TU)");
	RegisterUnitType(&pikiner_turki, "Pikiner_turki(TU)");
	RegisterUnitType(&turkey_pikiner, "TUrkey_pikiner(TU)");
	RegisterUnitType(&bashnia_3, "Bashnia_3(TU)");
	RegisterUnitType(&wall_tu, "WALL_TU(TU)");
	RegisterUnitType(&kuznica_ua, "Kuznica_ua(UA)");
	RegisterUnitType(&konushnia_ua, "konushnia_ua(UA)");
	RegisterUnitType(&strelcovaia, "Strelcovaia_Izba(UA)");
	RegisterUnitType(&grenader_dip, "Grenader_DIP(TU)");
	RegisterUnitType(&strelec_algir, "Strelec_Algir_DIP(TU)");
	RegisterUnitType(&kozak_loshad, "Kozak_loshad_DIP(TU)");
	RegisterUnitType(&dragun_18_dip, "Dragun_18_DIP(TU)");
	SetPlayerName(5, "COSSACKS");
	SetPlayerName(6, "COSSACKS");
}

extern "C" __declspec(dllexport) void ProcessScenary()
{
	if (Trigg(1))
	{
		SetTrigg(1, 0);
		ShowPage("#PAGE1");
		ShowPage("#PAGE2");
		SelectUnits(&ggates, 0);
		SelOpenGates(5);
		ChangeFriends(5, 0x60);
		RunTimer(1, AttackDelay() + 5000);
		// Starting stock for the enemy (player 5).
		SetResource(5, 3, 5000);
		SetResource(5, 1, 15000);
		SetResource(5, 4, 5000);
		SetResource(5, 5, 5000);
		SetResource(5, 0, 5000);
		SetResource(5, 2, 5000);
		// Player stock + AI difficulty by game difficulty (player 0).
		if (GetDiff(0) == 0)
		{
			SetResource(0, 3, 50000);
			SetResource(0, 1, 10000);
			SetResource(0, 4, 10000);
			SetResource(0, 5, 10000);
			SetResource(0, 0, 10000);
			SetResource(0, 2, 10000);
			StartAI(5, "UKRAINE.0", 0, 1, 2, 1);
		}
		if (GetDiff(0) == 1)
		{
			SetResource(0, 3, 7000);
			SetResource(0, 1, 7000);
			SetResource(0, 4, 7000);
			SetResource(0, 5, 7000);
			SetResource(0, 0, 1000);
			SetResource(0, 2, 1000);
			StartAI(5, "UKRAINE.0", 0, 1, 2, 2);
		}
		if (GetDiff(0) == 2)
		{
			SetResource(0, 3, 5000);
			SetResource(0, 1, 5000);
			SetResource(0, 4, 5000);
			SetResource(0, 5, 5000);
			SetResource(0, 0, 500);
			SetResource(0, 2, 500);
			StartAI(5, "UKRAINE.0", 0, 1, 2, 3);
		}
		if (GetDiff(0) == 3)
		{
			SetResource(0, 3, 2000);
			SetResource(0, 1, 2000);
			SetResource(0, 4, 2000);
			SetResource(0, 5, 4000);
			SetResource(0, 0, 500);
			SetResource(0, 2, 500);
			StartAI(5, "UKRAINE.0", 0, 1, 2, 3);
		}
	}
	// The player cannot build these (all EnableUnit flags stay 0).
	EnableUnit(0, &pehota_turki, 0);
	EnableUnit(0, &pikiner_turki, 0);
	EnableUnit(0, &turkey_pikiner, 0);
	EnableUnit(0, &bashnia_3, 0);
	EnableUnit(0, &wall_tu, 0);
	EnableUnit(0, &kuznica_ua, 0);
	EnableUnit(0, &konushnia_ua, 0);
	EnableUnit(0, &strelcovaia, 0);

	// Defeat: the player is erased.
	if (Trigg(6) && NationIsErased(0))
	{
		SetTrigg(6, 0);
		ShowPage("#PAGE3");
		LooseGame();
	}

	// Victory: the enemy is erased.
	if (Trigg(8) && NationIsErased(5))
	{
		SetTrigg(8, 0);
		ShowPage("#PAGE5");
		ShowVictory();
	}

	// The AI timer expired: capture the gate defenders to player 6 and
	// restart the attack timer.
	if (Trigg(9) && TimerDone(1))
	{
		SetTrigg(9, 0);
		ShowPage("#PAGE6");
		SetTrigg(10, 1);
		SelectUnitsType(&grenader_dip, 0, 0);
		SelectUnitsType(&strelec_algir, 0, 1);
		SelectUnitsType(&kozak_loshad, 0, 1);
		SelectUnitsType(&dragun_18_dip, 0, 1);
		SelChangeNation(0, 6);
		RunTimer(1, AttackDelay());
	}

	// Alternate the two timer gates (the original flips 9 and 10 each
	// time the timer completes).
	if (Trigg(10) && TimerDone(1))
	{
		SetTrigg(10, 0);
		SetTrigg(9, 1);
	}
}

// Original DllMain(DLL_PROCESS_ATTACH) -> OnInit(); dlopen runs this at
// the same point in the mission load sequence.
__attribute__((constructor)) static void MissionOnLoad()
{
	OnInit();
}
