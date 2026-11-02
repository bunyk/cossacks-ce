// Single mission 1: rescue the imprisoned officer ("Prisoner in the
// Turkish camp"): build up the Venice garrison, break the Turkish prison,
// free the officer and hold out until the fleets are wiped out.
//
// Ported 1:1 from Missions/Singles/Single1/Mission.dll (decompilation:
// /tmp/single1_decomp). MSVC debug build - the __chkesp() glue, the
// 0xcc fill loops and the dead duplicate of OnInit (FUN_100018b0) and
// dead helper variants (FUN_10001540/10001650/100016a0/10001780/100017f0,
// unreferenced by ProcessScenary) are stripped; everything else is
// exact, including the duplicated RegisterZone calls.
#include "../../common/mission_api.h"

// Engine-visible variables (original DAT_100323f8 / DAT_10032570).
static int difficulty; // GetDiff(0) snapshot, drives counts everywhere
static int resVar;     // only registered, used by dead code in the original

// ---- Registered objects (original DAT_100324xx-100326xx, init order) ----
static GAMEOBJ g_erase;        // "GErase"
static GAMEOBJ sel_player;     // dyn group: player's saved selection
static GAMEOBJ spawn;          // dyn group: CreateObject0 scratch group
static GAMEOBJ unused1, unused2, unused3; // registered, never used
static GAMEOBJ prisoners;      // dyn group: captured-prisoner pool (GBar/GPrison output)
static GAMEOBJ z0, z1, z2, z3, z4, z5;
static GAMEOBJ alone;          // "#ALONE" formation
static GAMEOBJ g_bar;          // "GBar"   - barracks zone units
static GAMEOBJ g_prison;       // "GPrison" - prison guards; player captures them
static GAMEOBJ oficer_new;     // "Oficer_NEW(UN)" - the rescue target unit
static GAMEOBJ pikiner_turki;  // "Pikiner_turki(TU)"
static GAMEOBJ oficer_pool;    // dyn group: freed officers
static GAMEOBJ kazarma_evro;   // "Kazarma_evro(VE)"
static GAMEOBJ konushnia_swesair; // "Konushnia_Swesair(VE)"
static GAMEOBJ krestian_turki; // "Krestian_Turki(TU)"
static GAMEOBJ pushka_tu;      // "Pushka(TU)"
static GAMEOBJ pushka1, pushka2, pushka3; // dyn+var artillery groups
static GAMEOBJ pike1, pike2, pike3;       // dyn assault-pike groups
static GAMEOBJ z1p, z2p, z3p, z1a, z2a, z3a;
static GAMEOBJ gal1, gal2, gal3;  // dyn+var galley groups
static GAMEOBJ galera_tu;      // "GALERA(TU)"
static GAMEOBJ z4a, z1g, z2g, z3g;
static GAMEOBJ gal_fleet;       // dyn+var: merged enemy fleet (RemoveGroup target)
static GAMEOBJ gal5;           // dyn+var galley group
static GAMEOBJ z6, z7;
static GAMEOBJ sel_z5;          // dyn+var: units selected in zone Z5
static GAMEOBJ fregat_pi;       // "Fregat(PI)"
static GAMEOBJ fregat_pool;    // dyn+var
static GAMEOBJ shebeka_tu;      // "Shebeka(TU)"
static GAMEOBJ shebeka_pool;   // dyn+var
static GAMEOBJ z10, z11, z12, z13, z14, z15;
static GAMEOBJ g_ve;            // "GVe" - player's Venice buildings
static GAMEOBJ g_food;         // "GFood"
static GAMEOBJ g_wood;         // "GWood"
static GAMEOBJ sp_pi1, sp_pi2, sp_pi3, sp_pi4;   // Venice spawn pools
static GAMEOBJ sp_tu1, sp_tu2, sp_tu3, sp_tu4;   // Turkish spawn pools
static GAMEOBJ pikiner_evro_pi; // "Pikiner_evro(PI)"
static GAMEOBJ mushketer_ev_pi; // "Mushketer_ev(PI)"
static GAMEOBJ grenader_dip_pi; // "Grenader_DIP(PI)"
static GAMEOBJ strelec_algir_pi; // "Strelec_Algir_DIP(PI)"
static GAMEOBJ z20, z21, z22, z23, z01, z16, z30, z31, z32, z33;
static GAMEOBJ ianithar_tu;     // "Ianithar(TU)"
static GAMEOBJ grenader_dip_tu; // "Grenader_DIP(TU)"
static GAMEOBJ strelec_algir_tu; // "Strelec_Algir_DIP(TU)"
static GAMEOBJ g_pi;            // "GPi"
static GAMEOBJ g_tu;            // "GTu"
static GAMEOBJ g_ball;          // "GBAll"
static GAMEOBJ g_uall;          // "GUAll"
static GAMEOBJ main_ve;         // "MAINVE"
static GAMEOBJ aka06ve;         // "AKA06VE"
static GAMEOBJ aka29ve;         // "AKA29VE"

// ---- Helper functions (original FUN_100011xx-100016xx) ----

// Original FUN_10001120: top up every resource of Nat to 200.
static void GiveMinRes( byte nat )
{
	for ( int r = 0; r < 6; r++ )
	{
		if ( GetResource( nat, r ) < 200 )
		{
			AddResource( nat, r, 200 );
		}
	}
}

// Original FUN_100011d0: AI nations get an effectively endless stock.
static void EnsureBigRes( byte nat, byte res )
{
	if ( GetResource( nat, res ) < 100000 )
	{
		SetResource( nat, res, 10000000 );
	}
}

// Original FUN_10001260.
static void GiveBigRes( byte nat )
{
	for ( int r = 0; r < 6; r++ )
	{
		EnsureBigRes( nat, r );
	}
}

// Original FUN_100012d0: the nation that owns the group's units
// (returns 8 when nobody does, like the original loop).
static byte GroupOwner( GAMEOBJ* group )
{
	byte nat = 0;
	for ( nat = 0; nat < 8; nat++ )
	{
		if ( GetUnitsByNation( group, nat ) > 0 ) break;
	}
	return nat;
}

// Original FUN_10001350: send the group's owner units to die in the zone.
static void SendGroup( GAMEOBJ* group, GAMEOBJ* zone, byte dir, byte type )
{
	byte nat = GroupOwner( group );
	if ( GetUnitsByNation( group, nat ) != 0 )
	{
		SelectUnits1( nat, group, 0 );
		SetStandGround( nat, 0 );
		AllowAttack( nat, 1 );
		SelSendAndKill( nat, zone, dir, type );
	}
}

// Original FUN_100014a0 / FUN_10001450 / FUN_100014f0 wrappers.
static void SendKill2( GAMEOBJ* g, GAMEOBJ* z, byte dir ) { SendGroup( g, z, dir, 2 ); }
static void SendKill200_2( GAMEOBJ* g, GAMEOBJ* z ) { SendGroup( g, z, 200, 2 ); }
static void SendKill200_0( GAMEOBJ* g, GAMEOBJ* z ) { SendGroup( g, z, 200, 0 ); }

// Original FUN_100016f0: create unitType as nation dir inside zone,
// move it into dst (through the scratch spawn group).
static void SpawnInto( GAMEOBJ* dst, GAMEOBJ* unitType, GAMEOBJ* zone, byte nat )
{
	CreateObject0( &spawn, &alone, unitType, nat, zone, 100 );
	RemoveGroup( &spawn, dst );
}

extern "C" __declspec(dllexport) void OnInit()
{
	RegisterVar( &difficulty, 4 );
	RegisterVar( &resVar, 4 );
	SetPlayerName( 1, "PIEMOUNT" );
	SetPlayerName( 2, "TURKEY" );
	SetPlayerName( 3, "PIEMOUNT" );
	SetPlayerName( 4, "TURKEY" );
	RegisterUnits( &g_erase, "GErase" );
	RegisterDynGroup( &sel_player );
	RegisterDynGroup( &spawn );
	RegisterDynGroup( &unused1 );
	RegisterDynGroup( &unused2 );
	RegisterDynGroup( &unused3 );
	RegisterDynGroup( &prisoners );
	RegisterVar( &prisoners, 8 );
	RegisterZone( &z0, "Z0" );
	RegisterZone( &z1, "Z1" );
	RegisterZone( &z2, "Z2" );
	RegisterZone( &z3, "Z3" );
	RegisterZone( &z4, "Z4" );
	RegisterZone( &z5, "Z5" );
	RegisterFormation( &alone, "#ALONE" );
	RegisterUnits( &g_bar, "GBar" );
	RegisterUnits( &g_prison, "GPrison" );
	RegisterUnitType( &oficer_new, "Oficer_NEW(UN)" );
	RegisterUnitType( &pikiner_turki, "Pikiner_turki(TU)" );
	RegisterDynGroup( &oficer_pool );
	RegisterVar( &oficer_pool, 8 );
	RegisterUnitType( &kazarma_evro, "Kazarma_evro(VE)" );
	RegisterUnitType( &konushnia_swesair, "Konushnia_Swesair(VE)" );
	RegisterUnitType( &krestian_turki, "Krestian_Turki(TU)" );
	RegisterUnitType( &pushka_tu, "Pushka(TU)" );
	RegisterDynGroup( &pushka1 );
	RegisterVar( &pushka1, 8 );
	RegisterDynGroup( &pushka2 );
	RegisterVar( &pushka2, 8 );
	RegisterDynGroup( &pushka3 );
	RegisterVar( &pushka3, 8 );
	RegisterDynGroup( &pike1 );
	RegisterDynGroup( &pike2 );
	RegisterDynGroup( &pike3 );
	RegisterZone( &z1p, "Z1P" );
	RegisterZone( &z2p, "Z2P" );
	RegisterZone( &z3p, "Z3P" );
	RegisterZone( &z1a, "Z1A" );
	RegisterZone( &z2a, "Z2A" );
	RegisterZone( &z3a, "Z3A" );
	RegisterDynGroup( &gal1 );
	RegisterVar( &gal1, 8 );
	RegisterDynGroup( &gal2 );
	RegisterVar( &gal2, 8 );
	RegisterDynGroup( &gal3 );
	RegisterVar( &gal3, 8 );
	RegisterUnitType( &galera_tu, "GALERA(TU)" );
	RegisterZone( &z4a, "Z4A" );
	RegisterZone( &z1g, "Z1G" );
	RegisterZone( &z2g, "Z2G" );
	RegisterZone( &z3g, "Z3G" );
	RegisterDynGroup( &gal_fleet );
	RegisterVar( &gal_fleet, 8 );
	RegisterDynGroup( &gal5 );
	RegisterVar( &gal5, 8 );
	RegisterZone( &z6, "Z6" );
	RegisterZone( &z7, "Z7" );
	RegisterDynGroup( &sel_z5 );
	RegisterVar( &sel_z5, 8 );
	RegisterUnitType( &fregat_pi, "Fregat(PI)" );
	RegisterDynGroup( &fregat_pool );
	RegisterVar( &fregat_pool, 8 );
	RegisterUnitType( &shebeka_tu, "Shebeka(TU)" );
	RegisterDynGroup( &shebeka_pool );
	RegisterVar( &shebeka_pool, 8 );
	RegisterZone( &z10, "Z10" );
	RegisterZone( &z11, "Z11" );
	RegisterZone( &z12, "Z12" );
	RegisterZone( &z13, "Z13" );
	RegisterZone( &z14, "Z14" );
	RegisterZone( &z15, "Z15" );
	RegisterUnits( &g_ve, "GVe" );
	RegisterUnits( &g_food, "GFood" );
	RegisterUnits( &g_wood, "GWood" );
	RegisterDynGroup( &sp_pi1 );
	RegisterDynGroup( &sp_pi2 );
	RegisterDynGroup( &sp_pi3 );
	RegisterDynGroup( &sp_pi4 );
	RegisterUnitType( &pikiner_evro_pi, "Pikiner_evro(PI)" );
	RegisterUnitType( &mushketer_ev_pi, "Mushketer_ev(PI)" );
	RegisterUnitType( &grenader_dip_pi, "Grenader_DIP(PI)" );
	RegisterUnitType( &strelec_algir_pi, "Strelec_Algir_DIP(PI)" );
	RegisterZone( &z20, "Z20" );
	RegisterZone( &z21, "Z21" );
	RegisterZone( &z22, "Z22" );
	RegisterZone( &z23, "Z23" );
	RegisterZone( &z01, "Z01" );
	RegisterZone( &z16, "Z16" );
	RegisterZone( &z30, "Z30" );
	RegisterZone( &z31, "Z31" );
	RegisterZone( &z32, "Z32" );
	RegisterZone( &z33, "Z33" );
	RegisterDynGroup( &sp_tu1 );
	RegisterDynGroup( &sp_tu2 );
	RegisterDynGroup( &sp_tu3 );
	RegisterDynGroup( &sp_tu4 );
	RegisterUnitType( &ianithar_tu, "Ianithar(TU)" );
	RegisterUnitType( &grenader_dip_tu, "Grenader_DIP(TU)" );
	RegisterUnitType( &strelec_algir_tu, "Strelec_Algir_DIP(TU)" );
	RegisterZone( &z20, "Z20" );
	RegisterZone( &z21, "Z21" );
	RegisterZone( &z22, "Z22" );
	RegisterZone( &z23, "Z23" );
	RegisterUnits( &g_pi, "GPi" );
	RegisterUnits( &g_tu, "GTu" );
	RegisterUnits( &g_ball, "GBAll" );
	RegisterUnits( &g_uall, "GUAll" );
	RegisterUpgrade( &main_ve, "MAINVE" );
	RegisterUpgrade( &aka06ve, "AKA06VE" );
	RegisterUpgrade( &aka29ve, "AKA29VE" );
}

extern "C" __declspec(dllexport) void ProcessScenary()
{
	// AI upkeep pulse: while the intro timer 10 is running, top the
	// allied nations' stock up (min for 1/2, endless for 3/4).
	if ( TimerDone( 10 ) )
	{
		GiveMinRes( 1 );
		GiveMinRes( 2 );
		GiveBigRes( 3 );
		GiveBigRes( 4 );
		RunTimer( 10, 500 );
	}

	if ( !Trigg( 0 ) ) goto AfterInit;

	// ---- one-time setup (trigger 0) ----
	TakeFood( &g_food );
	TakeWood( &g_wood );
	InitialUpgrade( "GFood", "AKA30VE" );
	DisableUpgrade( 0, &aka06ve );
	DisableUpgrade( 0, &aka29ve );
	DisableUpgrade( 0, &main_ve );
	EnableUnit( 0, &oficer_new, 1 );
	EnableUnit( 0, &kazarma_evro, 0 );
	EnableUnit( 0, &konushnia_swesair, 0 );
	EnableUnit( 0, &krestian_turki, 0 );
	EnableUnit( 4, &oficer_new, 1 );
	ChangeFriends( 1, 0x2e );
	ChangeFriends( 2, 0x2e );
	ChangeFriends( 3, 0x2e );
	ChangeFriends( 4, 0x2e );

	difficulty = GetDiff( 0 );
	SetResource( 0, 0, ( difficulty * -2 + 0xf ) * 1000 );
	// Player: every resource set to the wood stock.
	SetResource( 0, 3, GetResource( 0, 0 ) );
	SetResource( 0, 2, GetResource( 0, 0 ) );
	SetResource( 0, 1, GetResource( 0, 0 ) );
	SetResource( 0, 4, GetResource( 0, 0 ) );
	SetResource( 0, 5, GetResource( 0, 0 ) );
	// Nation 1 (PIEMOUNT): on easiest difficulty a third of the food,
	// otherwise all of it; everything else mirrors its wood.
	if ( difficulty == 0 )
	{
		SetResource( 1, 0, GetResource( 0, 3 ) / 3 );
	}
	else
	{
		SetResource( 1, 0, GetResource( 0, 3 ) );
	}
	SetResource( 1, 3, GetResource( 1, 0 ) );
	SetResource( 1, 2, GetResource( 1, 0 ) );
	SetResource( 1, 1, GetResource( 1, 0 ) );
	SetResource( 1, 4, GetResource( 1, 0 ) );
	SetResource( 1, 5, GetResource( 1, 0 ) );
	// Nation 2 (TURKEY): mirrors nation 1's food.
	SetResource( 2, 0, GetResource( 1, 3 ) );
	SetResource( 2, 3, GetResource( 2, 0 ) );
	SetResource( 2, 2, GetResource( 2, 0 ) );
	SetResource( 2, 1, GetResource( 2, 0 ) );
	SetResource( 2, 4, GetResource( 2, 0 ) );
	SetResource( 2, 5, GetResource( 2, 0 ) );
	// Nations 3/4: everything at 1000000.
	SetResource( 3, 0, 1000000 );
	SetResource( 3, 3, GetResource( 3, 0 ) );
	SetResource( 3, 2, GetResource( 3, 0 ) );
	SetResource( 3, 1, GetResource( 3, 0 ) );
	SetResource( 3, 4, GetResource( 3, 0 ) );
	SetResource( 3, 5, GetResource( 3, 0 ) );
	SetResource( 4, 0, GetResource( 3, 3 ) );
	SetResource( 4, 3, GetResource( 3, 0 ) );
	SetResource( 4, 2, GetResource( 3, 0 ) );
	SetResource( 4, 1, GetResource( 3, 0 ) );
	SetResource( 4, 4, GetResource( 3, 0 ) );
	SetResource( 4, 5, GetResource( 3, 0 ) );

	// Land armies (nation 1 Venice, nation 2 Turkey) into the landing
	// zones, then send them at the player.
	for ( int i = 0; i < difficulty * 5 + 5; i++ )
	{
		SpawnInto( &sp_pi1, &pikiner_evro_pi, &z21, 1 );
		if ( difficulty != 0 )
		{
			SpawnInto( &sp_pi1, &pikiner_evro_pi, &z22, 1 );
		}
		if ( difficulty != 0 )
		{
			SpawnInto( &sp_pi1, &mushketer_ev_pi, &z22, 1 );
		}
		SpawnInto( &sp_tu1, &pikiner_turki, &z31, 2 );
		if ( difficulty != 0 )
		{
			SpawnInto( &sp_tu1, &pikiner_turki, &z32, 2 );
		}
		if ( difficulty != 0 )
		{
			SpawnInto( &sp_tu1, &ianithar_tu, &z32, 2 );
		}
	}
	SendKill2( &sp_pi1, &z21, 0x40 );
	SendKill2( &sp_pi1, &z20, 0x40 );
	SendKill2( &sp_pi1, &z01, 0 );
	SelectUnits( &sp_pi1, 0 );
	DoNotUseSelInAI( 1 );
	ClearSelection( 1 );
	SendKill2( &sp_tu1, &z31, 0x40 );
	SendKill2( &sp_tu1, &z30, 0x40 );
	SendKill2( &sp_tu1, &z01, 0x80 );
	SelectUnits( &sp_tu1, 0 );
	DoNotUseSelInAI( 2 );
	ClearSelection( 2 );

	// Artillery + pike assault waves (none on the easiest difficulty:
	// the loop bound is difficulty*2-1).
	for ( int i = 0; i < difficulty * 2 - 1; i++ )
	{
		SpawnInto( &pushka1, &pushka_tu, &z1, 4 );
		SpawnInto( &pushka2, &pushka_tu, &z2, 4 );
		SpawnInto( &pushka3, &pushka_tu, &z3, 4 );
		for ( int j = 0; j < 4; j++ )
		{
			SpawnInto( &pike1, &pikiner_turki, &z4, 4 );
			SpawnInto( &pike2, &pikiner_turki, &z4, 4 );
			SpawnInto( &pike3, &pikiner_turki, &z4, 4 );
		}
	}

	// The enemy fleet.
	for ( int i = 0; i < difficulty + 2; i++ )
	{
		SpawnInto( &gal1, &galera_tu, &z1g, 4 );
		SpawnInto( &gal2, &galera_tu, &z2g, 4 );
		SpawnInto( &gal3, &galera_tu, &z3g, 4 );
		// NOTE: the Ghidra decompilation misattributes these two zone
		// args as Z4A/Z6 (hoisted pushes around the difficulty branch
		// below); the raw assembly pushes 0x100324c8 (Z6) for
		// gal_fleet and 0x100324d0 (Z7) for gal5 - verified in objdump
		// after the beached-galley report.
		SpawnInto( &gal_fleet, &galera_tu, &z6, 4 );
		SpawnInto( &gal5, &galera_tu, &z7, 4 );
		if ( difficulty < 2 )
		{
			i++;
		}
	}

	// Position the artillery and pike groups, merge the assault pikes
	// into the prisoner pool and the galleys into the fleet pool.
	SendKill2( &pushka1, &z1, 10 );
	SendKill2( &pike1, &z1p, 10 );
	SendKill2( &pushka2, &z2, 0x40 );
	SendKill2( &pike2, &z2p, 0x40 );
	SendKill2( &pushka3, &z3, 0xa0 );
	SendKill2( &pike3, &z3p, 0xa0 );
	RemoveGroup( &pike1, &prisoners );
	RemoveGroup( &pike2, &prisoners );
	RemoveGroup( &pike3, &prisoners );
	SendKill2( &gal1, &z1g, 0x40 );
	SendKill2( &gal2, &z2g, 0x40 );
	SendKill2( &gal3, &z3g, 0x40 );
	SendKill2( &gal_fleet, &z6, 0x28 );
	SendKill2( &gal5, &z7, 0x58 );
	RemoveGroup( &gal1, &gal_fleet );
	RemoveGroup( &gal2, &gal_fleet );
	RemoveGroup( &gal3, &gal_fleet );
	RemoveGroup( &gal5, &gal_fleet );

	// Pre-researched upgrades by difficulty (original goto chains).
	if ( difficulty == 0 )
	{
		InitialUpgrade( "GFood", "AKA16VE" );
		InitialUpgrade( "GFood", "AKA20VE" );
		InitialUpgrade( "GFood", "AKA12VE" );
		InitialUpgrade( "GFood", "AKA13VE" );
		InitialUpgrade( "GFood", "AKA32VE" );
		InitialUpgrade( "GFood", "AKA17VE" );
		InitialUpgrade( "GFood", "AKA27VE" );
		InitialUpgrade( "GFood", "AKA28VE" );
		InitialUpgrade( "GFood", "AKA14VE" );
		InitialUpgrade( "GFood", "AKA15VE" );
		InitialUpgrade( "GFood", "AKA33VE" );
		InitialUpgrade( "GFood", "AKA34VE" );
	}
	else if ( difficulty == 1 )
	{
		InitialUpgrade( "GFood", "AKA17VE" );
		InitialUpgrade( "GFood", "AKA27VE" );
		InitialUpgrade( "GFood", "AKA28VE" );
		InitialUpgrade( "GFood", "AKA14VE" );
		InitialUpgrade( "GFood", "AKA15VE" );
		InitialUpgrade( "GFood", "AKA33VE" );
		InitialUpgrade( "GFood", "AKA34VE" );
	}
	else if ( difficulty == 2 )
	{
		InitialUpgrade( "GPi", "AKA16PI" );
		InitialUpgrade( "GPi", "AKA28PI" );
		InitialUpgrade( "GTu", "AKA16TU" );
		InitialUpgrade( "GTu", "AKA28TU" );
		InitialUpgrade( "GPi", "AKA27PI" );
		InitialUpgrade( "GTu", "AKA27TU" );
	}
	else if ( difficulty == 3 )
	{
		InitialUpgrade( "GPi", "AKA17PI" );
		InitialUpgrade( "GPi", "AKA20PI" );
		InitialUpgrade( "GTu", "AKA17TU" );
		InitialUpgrade( "GTu", "AKA20TU" );
		InitialUpgrade( "GPi", "AKA16PI" );
		InitialUpgrade( "GPi", "AKA28PI" );
		InitialUpgrade( "GTu", "AKA16TU" );
		InitialUpgrade( "GTu", "AKA28TU" );
		InitialUpgrade( "GPi", "AKA27PI" );
		InitialUpgrade( "GTu", "AKA27TU" );
	}

	RunTimer( 1, 1 );
	RunTimer( 2, 0x9dd );
	RunTimer( 3, 0x19 );
	RunTimer( 10, 0 );
	if ( difficulty != 0 )
	{
		RunTimer( 0xb, 5000 );
		RunTimer( 0xc, 9000 );
	}
	StartAI( 1, "PIEMONT.0", 1, 1, 1, difficulty );
	StartAI( 2, "TURKISH.0", 1, 1, 1, difficulty );
	SetLightSpot( &z4, 4, 1 );
	EnableMission( 0x42 );
	DisableMission( 0x43 );
	DisableMission( 0x44 );
	DisableMission( 0x45 );
	DisableMission( 0x46 );
	ShowPage( "#PAGE01" );
	ShowPage( "#PAGE02" );
	ShowPage( "#PAGE03" );
	SetTrigg( 0, 0 );

AfterInit:
	// Periodic reinforcements after the difficulty delay.
	if ( Trigg( 0xb ) && TimerDone( 0xb ) )
	{
		for ( int i = 0; i < difficulty * 10 + 10; i++ )
		{
			SpawnInto( &sp_pi2, &pikiner_evro_pi, &z23, 1 );
			SpawnInto( &sp_pi2, &mushketer_ev_pi, &z23, 1 );
			SpawnInto( &sp_tu2, &pikiner_turki, &z32, 2 );
			SpawnInto( &sp_tu2, &ianithar_tu, &z32, 2 );
		}
		SetTrigg( 0xb, 0 );
	}
	if ( Trigg( 0xc ) && TimerDone( 0xc ) )
	{
		for ( int i = 0; i < difficulty * 0xf + 0xf; i++ )
		{
			SpawnInto( &sp_pi3, &grenader_dip_pi, &z23, 1 );
			SpawnInto( &sp_pi3, &strelec_algir_pi, &z23, 1 );
			SpawnInto( &sp_pi4, &pikiner_evro_pi, &z23, 1 );
			SpawnInto( &sp_tu3, &grenader_dip_tu, &z32, 2 );
			SpawnInto( &sp_tu3, &strelec_algir_tu, &z32, 2 );
			SpawnInto( &sp_tu4, &pikiner_turki, &z32, 2 );
		}
		SetTrigg( 0xc, 0 );
	}

	// Every Venice building pays the player gold.
	if ( GetTotalAmount0( &g_ve ) != 0 )
	{
		AddResource( 0, 1, GetTotalAmount0( &g_ve ) / 2 + 1 );
	}

	// Fregat spawn waves (timer 3) until the target count is reached.
	if ( Trigg( 0x5b ) && TimerDone( 3 ) )
	{
		if ( GetTotalAmount0( &fregat_pool ) < difficulty + 1 )
		{
			CreateObject0( &spawn, &alone, &fregat_pi, 3, &z10, 0x40 );
			RemoveGroup( &spawn, &fregat_pool );
			SendKill200_0( &fregat_pool, &z11 );
			SendKill200_2( &fregat_pool, &z14 );
			RunTimer( 3, 0xfa );
		}
		else
		{
			RunTimer( 3, 0xdac );
		}
	}
	// Shebeka spawn waves (timer 2).
	if ( Trigg( 0x5c ) && TimerDone( 2 ) )
	{
		if ( GetTotalAmount0( &shebeka_pool ) < difficulty + 1 )
		{
			CreateObject0( &spawn, &alone, &shebeka_tu, 4, &z12, 0x40 );
			RemoveGroup( &spawn, &shebeka_pool );
			SendKill200_0( &shebeka_pool, &z13 );
			SendKill200_2( &shebeka_pool, &z15 );
			RunTimer( 2, 0xfa );
		}
		else
		{
			RunTimer( 2, 0xdac );
		}
	}

	// Player units reaching zone Z5: sacrifice the captured garrison to
	// soften the prison guards (the player's own selection is restored).
	if ( Trigg( 3 ) )
	{
		if ( GetUnitsAmount0( &z5, 0 ) > 0 )
		{
			SaveSelectedUnits( 0, &sel_player, 0 );
			ClearSelection( 0 );
			SelectUnitsInZone( &z5, 0, 0 );
			SaveSelectedUnits( 0, &sel_z5, 0 );
			ClearSelection( 0 );
			SelectUnits( &gal_fleet, 0 );
			SelAttackGroup( 4, &sel_z5 );
			ClearSelection( 4 );
			SelectUnits1( 0, &pushka1, 0 );
			SelectUnits1( 0, &pushka2, 1 );
			SelectUnits1( 0, &pushka3, 1 );
			SelDie( 0 );
			ClearSelection( 0 );
			SelectUnits( &sel_player, 0 );
		}
	}

	// The artillery groups bombard their zones.
	if ( GetTotalAmount0( &pushka1 ) != 0 )
	{
		AttackZoneByArtillery( &pushka1, &z1, 0 );
	}
	if ( GetTotalAmount0( &pushka2 ) != 0 )
	{
		AttackZoneByArtillery( &pushka2, &z2, 0 );
	}
	if ( GetTotalAmount0( &pushka3 ) != 0 )
	{
		AttackZoneByArtillery( &pushka3, &z3, 0 );
	}

	// Prison guards lost during the raid = defeat.
	if ( Trigg( 2 ) && Trigg( 0x5a ) )
	{
		if ( GetTotalAmount0( &g_prison ) == 0 )
		{
			ShowPage( "#PAGE10T" );
			LooseGame();
			SetTrigg( 0x5a, 0 );
		}
	}

	// Player captured the prison guards -> start the rescue countdown.
	if ( Trigg( 1 ) && GroupOwner( &g_prison ) == 0 )
	{
		RunTimer( 1, 100 );
		SetTrigg( 1, 0 );
		SetTrigg( 2, 1 );
	}

	// Rescue completed: the guards join the player as a fresh officer.
	if ( Trigg( 2 ) && TimerDone( 1 ) )
	{
		if ( GroupOwner( &g_prison ) == 0 )
		{
			AddResource( 0, 1, ( 4 - difficulty ) * 0x8ca );
			AddResource( 0, 4, ( 4 - difficulty ) * 0x5dc );
			AddResource( 0, 5, ( 4 - difficulty ) * 0x5dc );
			AddResource( 0, 3, ( 4 - difficulty ) * 500 );
			SaveSelectedUnits( 0, &sel_player, 0 );
			ClearSelection( 0 );
			ClearSelection( 4 );
			SelectUnits( &g_prison, 0 );
			SelChangeNation( 4, 0 );
			ClearSelection( 0 );
			SelectUnits( &sel_player, 0 );
			EnableUnit( 0, &oficer_new, 1 );
			EnableUnit( 4, &oficer_new, 1 );
			ProduceUnitFast( &g_prison, &oficer_new, &oficer_pool, 0 );
			RunTimer( 1, 200 );
			SetTrigg( 2, 0 );
			ClearLightSpot( 1 );
			DisableMission( 0x42 );
			EnableMission( 0x43 );
			EnableMission( 0x44 );
			ShowPage( "#PAGE1" );
			ShowPageParam( "#PAGE2", ( 4 - difficulty ) * 0x8ca,
				( 4 - difficulty ) * 0x5dc, ( 4 - difficulty ) * 0x5dc,
				( 4 - difficulty ) * 500 );
		}
		else
		{
			SetTrigg( 1, 1 );
		}
	}

	// Z4-zone AI garrison replenishment (timer 4), difficulty-gated.
	if ( Trigg( 4 ) )
	{
		if ( GetUnitsAmount0( &z4, 0 ) > 0 )
		{
			if ( 0 < difficulty )
			{
				for ( int i = 0; i < difficulty * 5 + 5; i++ )
				{
					ProduceUnitFast( &g_bar, &pikiner_turki, &prisoners, 0 );
					ProduceUnitFast( &g_prison, &pikiner_turki, &prisoners, 0 );
				}
			}
			RunTimer( 4, 0x96 );
			SetTrigg( 4, 0 );
		}
	}

	// Keep the guards flowing into the prison.
	if ( Trigg( 5 ) && TimerDone( 4 ) )
	{
		if ( GetTotalAmount0( &prisoners ) == 0 )
		{
			SetTrigg( 5, 0 );
		}
		else
		{
			SendKill2( &prisoners, &z4a, 0x40 );
			RunTimer( 4, 1000 );
		}
	}

	// An officer reached zone Z1: unlock the buildings, hand the
	// upgrade set to the AI, advance the objectives.
	if ( Trigg( 3 ) )
	{
		if ( GetUnitsAmount1( &z1, &oficer_pool ) > 0 )
		{
			EnableUnit( 0, &kazarma_evro, 1 );
			EnableUnit( 0, &konushnia_swesair, 1 );
			if ( GetTotalAmount0( &g_prison ) == 0 )
			{
				EnableUnit( 0, &krestian_turki, 1 );
			}
			InitialUpgrade( "GBAll", "MAINVE" );
			InitialUpgrade( "GBAll", "AKA06VE" );
			InitialUpgrade( "GBAll", "AKA29VE" );
			InitialUpgrade( "GUAll", "MAINVE" );
			InitialUpgrade( "GUAll", "AKA06VE" );
			InitialUpgrade( "GUAll", "AKA29VE" );
			DisableMission( 0x44 );
			EnableMission( 0x45 );
			EnableMission( 0x46 );
			ShowPage( "#PAGE3" );
			ShowPage( "#PAGE4" );
			SetTrigg( 3, 0 );
		}
	}

	// After the rescue: no more officers left = defeat.
	if ( Trigg( 0x5a ) )
	{
		if ( !Trigg( 2 ) && TimerDone( 1 ) )
		{
			if ( GetTotalAmount0( &oficer_pool ) == 0 )
			{
				ShowPage( "#PAGE10G" );
				LooseGame();
				SetTrigg( 0x5a, 0 );
			}
		}
	}

	// The PIEMOUNT ally fell.
	if ( Trigg( 0x5b ) && NationIsErased( 1 ) )
	{
		ShowPage( "#PAGE11" );
		SetTrigg( 0x5b, 0 );
	}
	// The TURKEY enemy fell: peasants unlocked.
	if ( Trigg( 0x5c ) && NationIsErased( 2 ) )
	{
		EnableUnit( 0, &krestian_turki, 1 );
		ShowPage( "#PAGE12" );
		SetTrigg( 0x5c, 0 );
	}

	// Defeat: the player is erased.
	if ( Trigg( 0x5a ) && NationIsErased( 0 ) )
	{
		for ( byte t = 1; t < 0x14; t++ )
		{
			if ( !TimerDone( t ) )
			{
				RunTimer( t, 1 );
			}
		}
		ShowPage( "#PAGE10" );
		LooseGame();
		SetTrigg( 0x5a, 0 );
	}

	// Victory: both AI nations are gone.
	if ( !Trigg( 0x5b ) && !Trigg( 0x5c ) )
	{
		for ( byte t = 1; t < 0x1e; t++ )
		{
			if ( !TimerDone( t ) )
			{
				FreeTimer( t );
			}
		}
		ShowPage( "#PAGE20" );
		ShowVictory();
	}
}

// Original DllMain(DLL_PROCESS_ATTACH) -> OnInit(); dlopen runs this at
// the same point in the mission load sequence.
__attribute__((constructor)) static void MissionOnLoad()
{
	OnInit();
}
