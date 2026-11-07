#include "ddini.h"
#include "resfile.h"
#include "fastdraw.h"
#include "mgraph.h"
#include "mouse.h"
#include "menu.h"
#include "mapdiscr.h"
#include "multipl.h"
#include "fog.h"
#include "walls.h"
#include "nature.h"
#include "newupgrade.h"
#include "conststr.h"

extern const int kMinorMessageDisplayTime;

Upgrade::Upgrade()
{
	NUpgrades = 0;
};
void PerformUpgradeLink( OneObject* OBJ );

extern int tmtmt;

// TEMP AIDIAG: prints why an AI-proposed research was rejected; AI research
// starts are rare, so the success path prints every time.
#define AIDIAG_UPG( reason )  do { \
	if ( Nat && Nat->AI_Enabled && Nat->NNUM < 8 && AIDiagUpgGate( Nat->NNUM ) ) \
		printf( "AIDIAG nation[%d] t=%d: upgrade %d rejected: %s\n", Nat->NNUM, tmtmt, NewU, reason ); \
} while ( 0 )
static int LastUpgDiagT[8];
static bool AIDiagUpgGate( int NI )
{
	if ( NI < 0 )return false;
	if ( tmtmt - LastUpgDiagT[NI] < 250 )return false;
	LastUpgDiagT[NI] = tmtmt;
	return true;
}

void OneObject::PerformUpgrade( word NewU, word MakerID )
{
	if (CheckOrderAbility())
	{
		AIDIAG_UPG("order ability");
		return;
	}

	if (UnlimitedMotion)
	{
		AIDIAG_UPG("unlimited motion");
		return;
	}

	if (Nat->NUpgrades <= NewU || !Ready)
	{
		AIDIAG_UPG("producer not ready");
		return;
	}

	word NUP = Ref.General->NUpgrades;
	word* UPL = Ref.General->Upg;
	bool canU = false;

	for (int pp = 0; pp < NUP; pp++)
	{
		if (UPL[pp] == NewU)
		{
			canU = true;
		}
	}

	if (!canU)
	{
		AIDIAG_UPG("wrong producer building");
		return;
	}

	NewUpgrade* NU = Nat->UPGRADE[NewU];

	if (!NU->Enabled)
	{
		AIDIAG_UPG("upgrade not enabled");
		return;
	}

	for (int i = 0; i < 8; i++)
	{
		if (XRESRC( NNUM, i ) < NU->Cost[i])
		{
			AIDIAG_UPG("cannot afford");
			return;
		}
	}

	if (NU->Individual)
	{
		if (SingleUpgLevel != NU->Level)
		{
			return;
		}
	}

	int Use = newMons->Usage;
	for (int i = 0; i < 8; i++)
	{
		AddXRESRC( NNUM, i, -NU->Cost[i] );
		Nat->ControlProduce( NU->Branch, i, -NU->Cost[i] );
		if (Use == MineID)
		{
			Nat->ResOnMines[i] += NU->Cost[i];
		}
		Nat->ResOnUpgrade[i] += NU->Cost[i];
	}

	Order1* Or1 = GetOrdBlock();
	Or1->PrioryLevel = 96;
	Or1->NextOrder = LocalOrder;
	Or1->OrderType = 75;//Upgrade
	Or1->OrderTime = 0;
	Or1->DoLink = &PerformUpgradeLink;
	Or1->info.PUpgrade.NewUpgrade = NewU;
	Or1->info.PUpgrade.Stage = 1;
	Or1->info.PUpgrade.NStages = NU->NStages;
	Or1->info.PUpgrade.OldUpgrade = MakerID;

	Order1* LOR = LocalOrder;
	LocalOrder = Or1;

	if (!( NU->Individual || NU->StageUp ))
	{
		NU->Enabled = false;
		NU->PermanentEnabled = false;
		NU->IsDoing = true;
	}
	Ustage = 0;
	NUstages = NU->NStages;
	Ready = false;
	if ( Nat && Nat->AI_Enabled && Nat->NNUM < 8 )// TEMP AIDIAG
	{
		printf( "AIDIAG nation[%d] t=%d: upgrade %d started at building (usage=%d)\n", Nat->NNUM, tmtmt, NewU, newMons->Usage );
	}
}

void CreateTimedHint( char* s, int time );
extern int tmtmt;
extern int StartTmtmt;
void PerformUpgradeLink( OneObject* OBJ )
{
	word OI = OBJ->LocalOrder->info.PUpgrade.NewUpgrade;
	Nation* NT = OBJ->Nat;
	byte NI = NT->NNUM;
	NewUpgrade* NUP = NT->UPGRADE[OI];
	OBJ->LocalOrder->info.PUpgrade.Stage += FrmDec;
	OBJ->Ustage = OBJ->LocalOrder->info.PUpgrade.Stage;
	OBJ->PrioryLevel = OBJ->LocalOrder->PrioryLevel;

	if (OBJ->LocalOrder->info.PUpgrade.Stage >= OBJ->LocalOrder->info.PUpgrade.NStages)
	{
		OBJ->Ready = true;
		OneObject* OBU = NULL;
		word MID = OBJ->LocalOrder->info.PUpgrade.OldUpgrade;
		if (MID != 0xFFFF)
		{
			OBU = Group[MID];
		};
		if (!( NUP->Individual || NUP->StageUp ))NUP->Done = true;
		PerformNewUpgrade( &NATIONS[OBJ->NNUM], OI, OBU );
		OBJ->Nat->AddUpgrade( OI, tmtmt + StartTmtmt );
		if (NI == MyNation)
		{
			char ccc[200];
			sprintf( ccc, UPGMADE, NUP->Message );
			CreateTimedHint( ccc, kMinorMessageDisplayTime );//Improvement complete: %s
			LastActionX = OBJ->RealX >> 4;
			LastActionY = OBJ->RealY >> 4;
		};
		OBJ->DeleteLastOrder();
		OBJ->SingleUpgLevel++;

	};
};
