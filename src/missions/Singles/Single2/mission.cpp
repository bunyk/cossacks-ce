// Single mission 2 (Russian escort/defense scenario): the peasant convoy
// ("GK") must survive; enemy waves (Tiagoli) and a defender ring
// (Strelec/Pik_rus) rotate attacks on a GetGlobalTime()%3 schedule, and
// peasants captured on the way convert into Tiagoli workers for the
// player.
//
// Ported 1:1 from Missions/Singles/Single2/Mission.dll (decompilation:
// /tmp/single2_decomp, raw-arg audit vs objdump: /tmp/single2.asm).
// MSVC debug build: __chkesp glue and the dead OnInit duplicate stripped.
// NOTE (decompiler lies caught by the raw-arg audit): the two rotating
// attack loops gate on (index % 3) == (GetGlobalTime() % 3) - Ghidra
// prints a stale "iVar3 % 3" from a rand() register that is NOT involved;
// the div-by-3 in the bytes acts directly on the GetGlobalTime return.
#include "../../common/mission_api.h"
#include <cmath>
#include <ctime>

// ---- MSVC CRT rand (the original seeds it with time() in OnInit) ----
static unsigned int g_randSeed = 1;
static int msvc_rand()
{
	g_randSeed = g_randSeed * 0x343fd + 0x269ec3;
	return ( g_randSeed >> 0x10 ) & 0x7fff;
}

// ---- Registered objects (original DAT_100410xx-100412xx) ----
static GAMEOBJ z1, z2, z3, z4, z5, z6, z7;    // zones Z1..Z7
static GAMEOBJ gk;                            // "GK" - the peasant convoy group
static GAMEOBJ gb2, gb3, gb4, gb5, gb6, gb7;  // building groups GB2..GB7
static GAMEOBJ sel_player;   // 10041100: player's saved selection
static GAMEOBJ sel_tmp;      // 10041160: scratch group
static GAMEOBJ res_units;    // 10040cd8: released/recruited units pool
static GAMEOBJ sel_restore;  // 10041158: unregistered scratch group
static GAMEOBJ form_odin;    // "#ODIN" formation
static GAMEOBJ form_line6;   // "#LINE6" formation
static GAMEOBJ strelec;      // "Strelec(RU)"
static GAMEOBJ strelec_atk;  // "Strelec(RU)ATTACK"
static GAMEOBJ strelec_shld; // "Strelec(RU)SHIELD"
static GAMEOBJ tiagoli_koval; // "Tiagoli_koval_rus(RU)"
static GAMEOBJ tiagoli_kozak; // "Tiagoli_kozak(RU)"
static GAMEOBJ kreposnoi;    // "Kreposnoi(RU)" - the map's peasants
static GAMEOBJ pik_rus;      // "Pik_rus(RU)"

static int difficulty;      // 10041210 (RegisterVar)
static int ringCount;        // 1003ed5c (module-internal, not registered)

// The mission's "fake zone" GAMEOBJ: {word x, word y, dword Type =
// 0x40000000 | radius} - the engine's @-zone form (CreateObject0 reads
// Index/Serial as x<<4/y<<4 and Type & 0xFFFFFF as the radius).
static GAMEOBJ center_pos;  // 10041148/4114c

// Wave/defender slots: 16-byte records (GAMEOBJ group + GAMEOBJ pos),
// registered with RegisterVar(slot, 0x10) like the original.
struct Slot
{
	GAMEOBJ group;
	GAMEOBJ pos;
};
static Slot defenders[50]; // 10040ce8
static Slot waves[10];     // 10041018

// ---- Helper functions (original FUN_10003xxx-10005xxx) ----

// Original FUN_10005e10.
static void ClearGroup( GAMEOBJ* g )
{
	while ( GetTotalAmount0( g ) )
	{
		RemoveUnitFromGroup( g, 0 );
	}
	GetTotalAmount0( g );
}

// Original FUN_10003db0: place `count` defenders in a random ring of
// radius R (= pos->Type & 0xFFFFFF) around the base point; every 3rd is
// a pikeman, the rest strelets; nation 1, #ODIN, into defenders[i].
static void PlaceDefenders( GAMEOBJ* pos, int count )
{
	int R = pos->Type & 0xFFFFFF;
	short baseX = (short) pos->Index;
	short baseY = (short) pos->Serial;
	for ( int i = 0; i < count; i++ )
	{
		int r = msvc_rand() % 10000;
		float ang = ( ( (float) r * 3.1416 ) + ( (float) r * 3.1416 ) ) / 10000.0f;
		pos->Serial = baseY + (short) (int) ( sin( ang ) * R );
		pos->Index = baseX + (short) (int) ( cos( ang ) * R );
		if ( i % 3 == 0 )
		{
			CreateObject0( &defenders[i].group, &form_odin, &pik_rus, 1, pos,
				msvc_rand() % 0xff );
		}
		else
		{
			CreateObject0( &defenders[i].group, &form_odin, &strelec, 1, pos,
				msvc_rand() % 0xff );
		}
	}
}

// Original FUN_10004050: place `count` Tiagoli waves in the ring; nation
// 3, #LINE6, into waves[i].
static void PlaceWaves( GAMEOBJ* pos, int count )
{
	int R = pos->Type & 0xFFFFFF;
	short baseX = (short) pos->Index;
	short baseY = (short) pos->Serial;
	for ( int i = 0; i < count; i++ )
	{
		int r = msvc_rand() % 10000;
		float ang = ( ( (float) r * 3.1416 ) + ( (float) r * 3.1416 ) ) / 10000.0f;
		pos->Serial = baseY + (short) (int) ( sin( ang ) * R );
		pos->Index = baseX + (short) (int) ( cos( ang ) * R );
		CreateObject0( &waves[i].group, &form_line6, &tiagoli_kozak, 3, pos,
			msvc_rand() % 0xff );
	}
}

// Original FUN_10004840: take the map's peasants in the six zones for
// nation 3 (they become the enemy's workforce).
static void TakeMapPeasants()
{
	ClearGroup( &sel_tmp );
	SelectTypeOfUnitsInZone( &z2, &kreposnoi, 3, 0 );
	SelectTypeOfUnitsInZone( &z3, &kreposnoi, 3, 1 );
	SelectTypeOfUnitsInZone( &z4, &kreposnoi, 3, 1 );
	SelectTypeOfUnitsInZone( &z5, &kreposnoi, 3, 1 );
	SelectTypeOfUnitsInZone( &z6, &kreposnoi, 3, 1 );
	SelectTypeOfUnitsInZone( &z7, &kreposnoi, 3, 1 );
	SaveSelectedUnits( 3, &sel_tmp, 0 );
	TakeWood( &sel_tmp );
}

// Original FUN_100041c0: wave-arrival handling. Returns true when this
// wave's units joined the player (the caller converts one then).
static bool WaveArrive( Slot* w, bool nearAlly )
{
	bool result = false;
	if ( GetUnitsByNation( &w->group, 3 ) > 0 )
	{
		UnitsCenter( &w->pos, &w->group, 1000 );
		if ( nearAlly )
		{
			if ( GetUnitsAmount0( &w->pos, 1 ) > 0 )
			{
				// Ally units near: re-target away from the convoy center
				// (mirrored position).
				SelectUnitsInZone( &w->pos, 1, 0 );
				SelCenter( &center_pos, 1, 100 );
				w->pos.Index = (short) w->pos.Index + ( (short) w->pos.Index - (short) center_pos.Index );
				w->pos.Serial = (short) w->pos.Serial + ( (short) w->pos.Serial - (short) center_pos.Serial );
				SelectUnits( &w->group, 0 );
				SelSendTo( 3, &w->pos, msvc_rand() % 0xff, 0 );
			}
		}
		if ( GetUnitsAmount1( &w->pos, &sel_player ) > 0 )
		{
			UnitsCenter( &w->pos, &sel_player, 200 );
			SelSendTo( 3, &w->pos, msvc_rand() % 0xff, 0 );
			UnitsCenter( &w->pos, &w->group, 200 );
			if ( GetUnitsAmount1( &w->pos, &sel_player ) > 0 )
			{
				SelectUnits( &w->group, 0 );
				SelChangeNation( 3, 0 );
				AddResource( 0, 3, 500 );
				result = true;
			}
		}
	}
	return result;
}

// Original FUN_100044d0: send the wave on a random attack run.
static void WaveAttack( Slot* w )
{
	if ( GetUnitsByNation( &w->group, 3 ) > 0 )
	{
		UnitsCenter( &w->pos, &w->group, 400 );
		w->pos.Index = (short) w->pos.Index + ( 1000 - (short) ( msvc_rand() % 2000 ) );
		w->pos.Serial = (short) w->pos.Serial + ( 1000 - (short) ( msvc_rand() % 2000 ) );
		SelectUnits( &w->group, 0 );
		if ( msvc_rand() % 5 == 0 )
		{
			SelectUnits( &sel_player, 0 );
			SelCenter( &w->pos, 0, 400 );
		}
		SelSendAndKill( 3, &w->pos, msvc_rand() % 0xff, 0 );
	}
}

// Original FUN_10004670: are PLAYER units near this defender group?
static bool DefenderAtPlayer( Slot* d )
{
	UnitsCenter( &d->pos, &d->group, 0x96 );
	return GetUnitsAmount0( &d->pos, 0 ) > 0;
}

// Original FUN_10004710: advance a defender group toward the player
// (nation 1) unless the player is already on top of it.
static void DefenderAdvance( Slot* d )
{
	if ( !DefenderAtPlayer( d ) )
	{
		UnitsCenter( &d->pos, &d->group, 400 );
		d->pos.Index = (short) d->pos.Index + ( 200 - (short) ( msvc_rand() % 400 ) );
		d->pos.Serial = (short) d->pos.Serial + ( 200 - (short) ( msvc_rand() % 400 ) );
		SelectUnits( &d->group, 0 );
		SelSendTo( 1, &d->pos, msvc_rand() % 0xff, 0 );
	}
}

// Original FUN_10003f90: does this defender group still exist AND is the
// player near it? (the "enemy broke through" probe)
static bool DefenderEngagedPlayer( Slot* d )
{
	if ( GetTotalAmount0( &d->group ) > 0 )
	{
		UnitsCenter( &d->pos, &d->group, 0x96 );
		return GetUnitsAmount1( &d->pos, &sel_player ) > 0;
	}
	return false;
}

// Original FUN_100049c0: if no nation-1 units remain in the zone, hand
// the zone's peasants and the building group to nation 3. Returns true
// when the capture happened (the caller's cascade short-circuits on it).
static bool CaptureZone( GAMEOBJ* zone, GAMEOBJ* group )
{
	if ( GetUnitsAmount0( zone, 1 ) != 0 )
	{
		return false;
	}
	SelectTypeOfUnitsInZone( zone, &kreposnoi, 3, 0 );
	SelChangeNation( 3, 0 );
	SelectUnits( group, 0 );
	SelChangeNation( 3, 0 );
	return true;
}

extern "C" __declspec(dllexport) void OnInit()
{
	// Seed the CRT rand (original: FUN_1000aec0/FUN_1000ae70 = time()
	// stored into the MSVC rand state).
	g_randSeed = (unsigned int) time( nullptr );

	SetPlayerName( 0, "RUSSIA" );
	SetPlayerName( 1, "RUSSIA" );
	SetPlayerName( 2, "RUSSIA" );
	RegisterZone( &z1, "Z1" );
	RegisterZone( &z6, "Z6" );
	RegisterZone( &z2, "Z2" );
	RegisterZone( &z7, "Z7" );
	RegisterZone( &z3, "Z3" );
	RegisterZone( &z4, "Z4" );
	RegisterZone( &z5, "Z5" );
	RegisterUnits( &gk, "GK" );
	RegisterUnits( &gb2, "GB2" );
	RegisterUnits( &gb3, "GB3" );
	RegisterUnits( &gb4, "GB4" );
	RegisterUnits( &gb5, "GB5" );
	RegisterUnits( &gb6, "GB6" );
	RegisterUnits( &gb7, "GB7" );
	RegisterDynGroup( &sel_tmp );
	RegisterFormation( &form_odin, "#ODIN" );
	RegisterFormation( &form_line6, "#LINE6" );
	RegisterDynGroup( &sel_player );
	RegisterDynGroup( &res_units );
	RegisterVar( &sel_tmp, 8 );
	RegisterVar( &sel_player, 8 );
	RegisterVar( &res_units, 8 );
	RegisterUpgrade( &strelec_atk, "Strelec(RU)ATTACK" );
	RegisterUpgrade( &strelec_shld, "Strelec(RU)SHIELD" );
	RegisterUnitType( &strelec, "Strelec(RU)" );
	RegisterUnitType( &tiagoli_koval, "Tiagoli_koval_rus(RU)" );
	RegisterUnitType( &tiagoli_kozak, "Tiagoli_kozak(RU)" );
	RegisterUnitType( &kreposnoi, "Kreposnoi(RU)" );
	RegisterUnitType( &pik_rus, "Pik_rus(RU)" );
	for ( int i = 0; i < 0x32; i++ )
	{
		RegisterDynGroup( &defenders[i].group );
		RegisterVar( &defenders[i], 0x10 );
	}
	for ( int i = 0; i < 10; i++ )
	{
		RegisterDynGroup( &waves[i].group );
		RegisterVar( &waves[i], 0x10 );
	}
	// The convoy center fake-zone: base point + '@' | radius 1500.
	center_pos.Index = 0x21ca;
	center_pos.Serial = 0x1ce8;
	center_pos.Type = 0x40000000;
	RegisterDynGroup( &sel_restore );
	RegisterVar( &difficulty, 4 );
}

extern "C" __declspec(dllexport) void ProcessScenary()
{
	if ( Trigg( 99 ) )
	{
		difficulty = GetDiff( 0 );
		SetResource( 0, 3, 100 ); // food for the convoy
		SetResource( 0, 1, 0 );
		SetResource( 0, 0, 0 );
		SetResource( 0, 2, 0 );
		SetResource( 0, 5, 0 );
		SetResource( 0, 4, 0 );
		ChangeFriends( 3, 0xb );
		ChangeFriends( 1, 0xb );
		EnableUnit( 0, &strelec, 0 );
		DisableUpgrade( 0, &strelec_atk );
		DisableUpgrade( 0, &strelec_shld );
		SelectUnits( &gk, 0 );
		SaveSelectedUnits( 0, &sel_player, 0 );
		DisableMission( 0x44 );
		DisableMission( 0x45 );
		DisableMission( 0x46 );
		DisableMission( 0x47 );
		DisableMission( 0x48 );
		DisableMission( 0x49 );
		RunTimer( 0x1e, 0x96 );
		SetTrigg( 99, 0 );
	}

	if ( TimerDoneFirst( 0x1e ) )
	{
		ShowPage( "#PAGE0" );
		// Defender ring around the convoy base point (radius 1500).
		center_pos.Index = 0x21ca;
		center_pos.Serial = 0x1ce8;
		center_pos.Type = 0x40000000 | 0x5dc;
		ringCount = difficulty * 5 + 0x23;
		PlaceDefenders( &center_pos, ringCount );
		// Enemy waves further out (radius 5800).
		center_pos.Index = 9000;
		center_pos.Serial = 8000;
		center_pos.Type = 0x40000000 | 0x16a8;
		PlaceWaves( &center_pos, 10 );
		TakeMapPeasants();
		RunTimer( 1, 200 );
		RunTimer( 2, 200 );
	}

	if ( Trigg( 100 ) )
	{
		SaveSelectedUnits( 0, &sel_restore, 0 );

		// Wave arrival: any wave reaching the player converts one
		// peasant of the convoy into a Tiagoli worker (for the player).
		// The "near ally" argument is the CURRENT Trigg(0xd) value (not a
		// gate - the original passes it straight into FUN_100041c0).
		for ( int i = 0; i < 10; i++ )
		{
			bool nearAlly = Trigg( 0xd ) != 0;
			if ( WaveArrive( &waves[i], nearAlly ) )
			{
					if ( Trigg( 0xb ) )
					{
						DisableMission( 0x45 );
						EnableMission( 0x46 );
						EnableMission( 0x47 );
						ShowPage( "#PAGE5" );
						UnitsCenter( &center_pos, &sel_player, 100 );
						SelectUnits( &sel_player, 0 );
						SelErase( 0 );
						CreateObject0( &sel_player, &form_odin, &tiagoli_koval, 0,
							&center_pos, msvc_rand() % 0xff );
						SetTrigg( 0xb, 0 );
						if ( difficulty < 1 )
						{
							SetLightSpot( &z2, 0, 2 );
							SetLightSpot( &z3, 0, 3 );
							SetLightSpot( &z4, 0, 4 );
							SetLightSpot( &z5, 0, 5 );
							SetLightSpot( &z6, 0, 6 );
							SetLightSpot( &z7, 0, 7 );
						}
					}
				}
			}

		// Rotating wave attack schedule: every timer-2 tick one third of
		// the waves (by index % 3 == GetGlobalTime() % 3) attacks.
		if ( TimerDoneFirst( 2 ) )
		{
			for ( int i = 0; i < 10; i++ )
			{
				if ( i % 3 == GetGlobalTime() % 3 )
				{
					WaveAttack( &waves[i] );
				}
			}
			RunTimer( 2, 300 );
		}

		if ( Trigg( 0xd ) )
		{
			// Same rotation for the defender ring advancing (timer 1).
			if ( TimerDoneFirst( 1 ) )
			{
				for ( int i = 0; i < ringCount; i++ )
				{
					if ( i % 3 == GetGlobalTime() % 3 )
					{
						DefenderAdvance( &defenders[i] );
					}
				}
				RunTimer( 1, 0x96 );
			}

			if ( Trigg( 2 ) )
			{
				// Player units in zone Z1: the convoy made it.
				if ( GetUnitsAmount1( &z1, &sel_player ) > 0 )
				{
					RunTimer( 3, 1000 );
					DisableMission( 0x43 );
					EnableMission( 0x44 );
					EnableMission( 0x45 );
					SetTrigg( 2, 0 );
				}
			}

			if ( TimerDoneFirst( 3 ) )
			{
				// The counter-attack: strelets spawn in Z1 as nation 1
				// and charge (the player now commands them).
				ChangeFriends( 1, 2 );
				SetTrigg( 5, 0 );
				ShowPage( "#PAGE6" );
				for ( int i = 0; i < difficulty * 2 + 4; i++ )
				{
					CreateObject0( &sel_tmp, &form_odin, &strelec, 1, &z1,
						msvc_rand() % 0xff );
					RemoveGroup( &sel_tmp, &res_units );
				}
				RunTimer( 7, ( 3 - difficulty ) * 100 + 1000 );
			}

			if ( TimerDoneFirst( 7 ) )
			{
				SelectUnits( &res_units, 0 );
				UnitsCenter( &center_pos, &sel_player, 300 );
				SelSendAndKill( 1, &center_pos, msvc_rand() % 0xff, 2 );
				RunTimer( 7, ( 3 - difficulty ) * 100 + 1000 );
			}

			if ( Trigg( 5 ) )
			{
				for ( int i = 0; i < ringCount; i++ )
				{
					if ( DefenderEngagedPlayer( &defenders[i] ) )
					{
						ShowPage( "#PAGE8" );
						ChangeFriends( 1, 2 );
						SetTrigg( 5, 0 );
					}
				}
			}

			// Zone capture cascade (original: six chained
			// FUN_100049c0 calls, short-circuiting at the FIRST zone
			// that flips to nation 3; the reward block below runs when
			// one was captured this tick).
			bool captured = CaptureZone( &z2, &gb2 );
			if ( !captured ) captured = CaptureZone( &z3, &gb3 );
			if ( !captured ) captured = CaptureZone( &z4, &gb4 );
			if ( !captured ) captured = CaptureZone( &z5, &gb5 );
			if ( !captured ) captured = CaptureZone( &z6, &gb6 );
			if ( !captured ) captured = CaptureZone( &z7, &gb7 );
			if ( captured )
			{
			DisableMission( 0x47 );
			EnableMission( 0x48 );
			EnableMission( 0x49 );
			ChangeFriends( 3, 9 );
			SetTrigg( 0xd, 0 );
			AddResource( 0, 3, difficulty * -3000 + 10000 );
			SetResource( 0, 1, difficulty * -3000 + 10000 );
			SetResource( 0, 0, difficulty * -3000 + 10000 );
			SetResource( 0, 2, difficulty * -3000 + 10000 );
			SetResource( 0, 5, 0 );
			SetResource( 0, 4, difficulty * -3000 + 10000 );
			SetResource( 1, 3, 3000 );
			SetResource( 1, 1, 3000 );
			SetResource( 1, 0, 3000 );
			SetResource( 1, 2, 3000 );
			SetResource( 1, 5, 10000 );
			SetResource( 1, 4, 3000 );
			if ( difficulty < 1 )
			{
				ClearLightSpot( 2 );
				ClearLightSpot( 3 );
				ClearLightSpot( 4 );
				ClearLightSpot( 5 );
				ClearLightSpot( 6 );
				ClearLightSpot( 7 );
			}
			ShowPage( "#PAGE7" );
			StartAI( 1, "RUSSIA.0", 0, 0, 0, difficulty );
			}
		}
	}

	// Victory: the enemy nation fell.
	if ( NationIsErased( 1 ) )
	{
		ShowPage( "#PAGE2" );
		ShowVictory();
		SetTrigg( 100, 0 );
	}

	// Defeat: taken captive. The gate is !Trigg(5) - i.e. AFTER the alarm
	// cleared trigger 5 (byte-verified: asm jumps past the block when
	// Trigg(5) != 0; Ghidra printed the polarity inverted).
	if ( !Trigg( 5 ) )
	{
		if ( GetUnitsByNation( &sel_player, 1 ) == 1 )
		{
			ShowPage( "#PAGE1" );
			LooseGame();
			SetTrigg( 100, 0 );
		}
	}

	// Defeat: the player's group is empty.
	if ( GetTotalAmount0( &sel_player ) == 0 )
	{
		ShowPage( "#PAGE3" );
		LooseGame();
		SetTrigg( 100, 0 );
	}

	// Defeat: the player is erased.
	if ( !Trigg( 0xd ) )
	{
		if ( NationIsErased( 0 ) )
		{
			ShowPage( "#PAGE4" );
			LooseGame();
			SetTrigg( 100, 0 );
		}
	}

	ClearSelection( 0 );
	SelectUnits( &sel_restore, 0 );
}

// Original DllMain(DLL_PROCESS_ATTACH) -> OnInit(); dlopen runs this at
// the same point in the mission load sequence.
__attribute__((constructor)) static void MissionOnLoad()
{
	OnInit();
}
