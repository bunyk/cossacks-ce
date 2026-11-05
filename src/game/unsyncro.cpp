#include "ddini.h"
#include "resfile.h"
#include "fastdraw.h"
#include "mgraph.h"
#include "mouse.h"
#include "menu.h"
#include "mapdiscr.h"
#include "fog.h"
#include "megapolis.h"

#include <assert.h>
#include "walls.h"
#include "mode.h"
#include "gsound.h"
#include "mapsprites.h"
#include "newmon.h"

int prevrtime = 0;
FILE* FX = nullptr;
extern bool RecordMode;
extern byte PlayGameMode;
char CURLOG[32];

extern uint64_t GetSDLTickCount();

void WriteRec( char* s )
{
	if ( !( PlayGameMode || RecordMode ) )
	{
		return;
	}

	if ( GetSDLTickCount() - prevrtime > 10000 )
	{
		if ( FX )
		{
			fclose( FX );
			if ( PlayGameMode )
			{
				FX = fopen( "lox1.log", "a" );
				strcpy( CURLOG, "lox1.log" );
			}
			else
			{
				FX = fopen( "lox0.log", "a" );
				strcpy( CURLOG, "lox0.log" );
			}
		}
		prevrtime = GetSDLTickCount();
	}
	if ( !FX )
	{
		if ( PlayGameMode )
		{
			FX = fopen( "lox1.log", "a" );
			strcpy( CURLOG, "lox1.log" );
		}
		else
		{
			FX = fopen( "lox0.log", "a" );
			strcpy( CURLOG, "lox0.log" );
		}
	}
	fprintf( FX, s );
}

extern int tmtmt;
extern DWORD RealTime;

void Syncro::Copy( Syncro* Syn )
{
	memcpy( Syn->RSL, RSL, NSlots * sizeof(RandSlot) );
	Syn->NSlots = NSlots;
	NSlots = 0;
}

Syncro SYN;
Syncro SYN1;
extern short randoma[8192];
extern char LASTFILE[128];
extern int LastLine;
char* LastFile;

// ---- lockstep RNG divergence diagnostics ----
// RandNew records the source position of EVERY deterministic random read
// in a ring buffer. When a network game hits its first RandIndex fork
// (UpdateEBufs in mplayer.cpp), the ring is dumped once to stdout:
// diffing the two machines' RANDTRACE streams shows the exact call site
// where RNG consumption diverged (the reads match up to the fork point,
// then one machine has extra/unmatched entries).
const int RandTraceSize = 1 << 16;
const int RandTraceMask = RandTraceSize - 1;
struct RandTraceSlot
{
	char* File;
	int Line;
	word RPos;
};
RandTraceSlot RandTrace[RandTraceSize];
int RandTraceHead = 0;
int RandTraceDumpDone = 0;

void ResetRandTrace()
{
	memset( RandTrace, 0, sizeof RandTrace );
	RandTraceHead = 0;
	RandTraceDumpDone = 0;
}

//One-shot dump: prints the whole ring (oldest first) and arms itself
void DumpRandTrace()
{
	if (RandTraceDumpDone)
	{
		return;
	}
	RandTraceDumpDone = 1;
	int n = 0;
	for (int i = 0; i < RandTraceSize; i++)
	{
		const RandTraceSlot& S = RandTrace[( RandTraceHead + i ) & RandTraceMask];
		if (!S.File)
		{
			continue;
		}
		printf( "RANDTRACE %5d %s:%d\n", S.RPos, S.File, S.Line );
		n++;
	}
	printf( "RANDTRACE END entries=%d\n", n );
	fflush( stdout );
}

int RandNew( char* File, int Line )
{
	LastFile = File;
	LastLine = Line;
	int r = randoma[rpos];
	RandTraceSlot& TS = RandTrace[RandTraceHead];
	TS.File = File;
	TS.Line = Line;
	TS.RPos = rpos;
	RandTraceHead = ( RandTraceHead + 1 ) & RandTraceMask;
	rpos = ( rpos + 1 ) & 8191;
	if ( SYN.NSlots < maxUline )
	{
		RandSlot* RS = SYN.RSL + SYN.NSlots;
		RS->FileName = File;
		RS->Line = Line;
		RS->Param = 0;
		RS->rpos = rpos;
		RS->Type = -1;
		SYN.NSlots++;
	}

	return r;
}
