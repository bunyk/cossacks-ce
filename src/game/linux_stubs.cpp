// Linux port stubs.
//
// Temporary no-op definitions for functionality that is not ported yet:
//  - interface.cpp regions still guarded by _WIN32 (multiplayer, chat,
//    random-map dialogs)
//  - fog.cpp / virtscr.cpp asm regions
//  - realwater.cpp, shiptrace.cpp, mplayer.cpp (network), ir_main.cpp
// Each stub keeps the exact signature from the headers so the mangled names match.
// TODO: replace with real implementations while porting the files above.

#include "ddini.h"
#include "resfile.h"
#include "fastdraw.h"
#include "mgraph.h"
#include "mouse.h"
#include "menu.h"
#include "mapdiscr.h"
#include "dialogs.h"
#include "gp_draw.h"
#include "masks.h"
#include "virtscreen.h"
#include "mplayer.h"
#include "ir.h"
#include "bmptool.h"
#include "megapolis.h"
// socket/Winsock type shims for pinger.h below (its MSVC includes are
// commented out on purpose)
#include "../CommCore library/CommCore.h"
#include "pinger.h"
#include "pinger.h"

#ifndef _WIN32

//-------------- global variables --------------
// CUR_TOOL_MODE/DRAWLOCK/HEIGHTSTR: real definitions now compiled in
// interface.cpp - stubs deleted.
char CurrentMap[64];
char DEFPLNAMES[8][64];
// FogMode: defined in fog.cpp (ported)
extern const int kCtrlStickyTime = 700;
char MapScenaryDLL[200];
// Editor UI state (InteriorType/MI_Mode/OptHidden/ToolsHidden/TP_Made/
// NeedToPopUp) and TexMap brush parameters (TM_*/TexMapMod): real
// definitions now compiled in interface.cpp - stubs deleted.
int OrderSound = 0;
int RM_LandType = 0;
int RM_Resstart = 0;
int RM_Restot = 0;
// Sound volumes: DirectSound-style slider positions (0..100). SDL_AppInit
// zeroes these before loading mode.dat (same as the original), so these
// inits only matter for the very first run before a mode.dat exists.
int WarSound = 0;
int WorkSound = 0;

// network / internet pointers (ir_main.cpp is not ported; mplayer.cpp
// provides the real PSUMM/PBACK/RETSYS objects now)
typedef void tpStartDownloadInternetFile( char* Name, char* Server, char* DestName );
typedef void tpSendRecBuffer( byte* Data, int size, bool Final );
tpStartDownloadInternetFile* StartDownloadInternetFile = NULL;
tpSendRecBuffer* SendRecBuffer = NULL;


word GetNextDivCell( byte NI, int F, int Start ) { (void)NI; (void)F; (void)Start; return 0xFFFF; }
word GetNextGreCell( byte NI, int F, int Start ) { (void)NI; (void)F; (void)Start; return 0xFFFF; }
word GetNextSafeCell( byte NI, int F, int start, int Fin ) { (void)NI; (void)F; (void)start; (void)Fin; return 0xFFFF; }

//-------------- interface.cpp / gp_draw.cpp guarded regions --------------
// CheckFlagsNeed/GetTreeItem/LoadMapInterface/ProcessMapInterface: real
// definitions now compiled in interface.cpp - stubs deleted.
bool CheckGameTime() { return false; }
void DontMakeRaiting() {}
void ExplorerOpenRef( int Index, char* ref ) { (void)Index; (void)ref; }
int  GetCurGamePtr( byte** Ptr ) { if (Ptr) *Ptr = NULL; return 0; }
bool IsGameActive() { return false; }
void SetBrokenState() {}

void ShowCentralMessage4( char* Message, int GPIDX, int DX ) { (void)Message; (void)GPIDX; (void)DX; }
void UpdateCurGame() {}

//-------------- map editor / random map --------------
// DecodeOptionsFromNumber: real implementation in newcode/mapoptionsencoder.cpp
// (now compiled) - stub deleted. Without it every additional game option
// chosen in the random-map UI decoded to 0 (default).
// ReadBMP24/CreateNEWMAP: real implementations now compiled (bmptool.cpp and
// the editor UI in interface.cpp) - stubs deleted.

//-------------- network extras (ir_main.cpp is not ported) --------------
// StopRaiting: ir_main.cpp stats-reporting hook, called by mplayer.cpp
void StopRaiting() {}

// LOOSEANDEXITFAST: ir_main.cpp - records a defeat in the GSC internet-game
// stats object (CURIGAME) on leave; the GSC session flow never runs without
// ir_main, so this is a no-op
void LOOSEANDEXITFAST() {}

// GetPName: real definition in interface.cpp (un-guarded)
void LOOSEIGAME( char* Nick ) { (void)Nick; }
void WINIGAME( char* Nick ) { (void)Nick; }
void GetGameID( char* s ) { if (s) s[0] = 0; }
void SendVictoryState( int ID, byte State ) { (void)ID; (void)State; }

// ProcessSingleMission: real implementation now in interface.cpp (single
// missions + campaigns un-guarded) - stub deleted.

// lobby helpers used by the shared MPL_WaitingGame event loop
int  ProcessInternetConnection( bool Active ) { (void)Active; return 0; }
// EncodeOptionsInNumber: real implementation in newcode/mapoptionsencoder.cpp
// (now compiled) - stub deleted.

// network globals (mplayer.cpp)

//-------------- CPinger (pinger.h, Pinger.lib is Windows-only) --------------
// The pinger only measured ICMP reachability of the long-dead GameSpy
// internet lobby ("peerchat.gamespy.com") on the multiplayer waiting screen;
// a no-op keeps that screen logic running.
CPinger::CPinger() {}
CPinger::~CPinger() {}
BOOL CPinger::InitNetwork() { return TRUE; }
BOOL CPinger::DoneNetwork() { return TRUE; }
BOOL CPinger::SetTargetName( LPCSTR lpcszTargetName ) { (void)lpcszTargetName; return FALSE; }
BOOL CPinger::GetStatistic( BYTE * lpcReplies, BYTE * lpcTotal ) { if (lpcReplies) *lpcReplies = 0; if (lpcTotal) *lpcTotal = 0; return TRUE; }
BOOL CPinger::SendEcho() { return FALSE; }
BOOL CPinger::RecvEcho() { return FALSE; }

// CD-audio/music: real implementations now in devicecd.cpp (SDL3 audio
// stream + stb_vorbis "Tracks/Track_XX.ogg"); stubs deleted.
// MidiSound stays here: it is only the options-slider state variable
// (interface.cpp's definition is inside a _WIN32 region).
int MidiSound = 0;
// IChat library callback (DLLIMPORT in interface.cpp); no-op without the DLL
void GoHomeAnyway() {}
// Terrain-editor error prompt (interface.cpp, unguarded editor UI). The
// real dialog body lives in ir_main.cpp (not compiled on Linux): draw the
// text via ShowCentralMessage4 (no-op stub for now) and wait for a
// keypress. stderr keeps the message visible on the console meanwhile.
extern bool KeyPressed;
bool ProcessMessages();
void WaitWithError( char* ID, int GPID )
{
	char* str = GetTextByID( ID );
	fprintf( stderr, "WaitWithError: %s\n", str );
	ShowCentralMessage4( str, GPID, 90 );
	FlipPages();
	KeyPressed = 0;
	do
	{
		ProcessMessages();
	} while ( !KeyPressed );
}

// network functions (mplayer.cpp)

#endif // !_WIN32
