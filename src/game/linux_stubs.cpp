// Linux port stubs.
//
// Temporary no-op definitions for functionality that is not ported yet:
//  - interface.cpp regions still guarded by _WIN32 (multiplayer, chat,
//    clans, random-map dialogs)
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

#ifndef _WIN32

//-------------- global variables --------------
// CUR_TOOL_MODE/DRAWLOCK/HEIGHTSTR: real definitions now compiled in
// interface.cpp - stubs deleted.
word COMPSTART[8];
char CurrentMap[64];
char DEFPLNAMES[8][64];
DWORD EBPos1 = 0;
int PLNAT[8];
EXBUFFER EBufs[MaxPL];
EXBUFFER EBufs1[MaxPL];
int ExitNI = 0;
// FogMode: defined in fog.cpp (ported)
extern const int kCtrlStickyTime = 700;
bool LockFog = false;
char MapScenaryDLL[200];
// Editor UI state (InteriorType/MI_Mode/OptHidden/ToolsHidden/TP_Made/
// NeedToPopUp) and TexMap brush parameters (TM_*/TexMapMod): real
// definitions now compiled in interface.cpp - stubs deleted.
int OrderSound = 0;
int PREVGLOBALTIME = 0;
word PrevRpos = 0;
int RM_LandType = 0;
int RM_Resstart = 0;
int RM_Restot = 0;
char SaveFileName[128];
int SeqErrorsCount = 0;
byte SYNBAD[8];
// Sound volumes: DirectSound-style slider positions (0..100). SDL_AppInit
// zeroes these before loading mode.dat (same as the original), so these
// inits only matter for the very first run before a mode.dat exists.
int WarSound = 0;
int WorkSound = 0;
DWORD RealTime = 0;

// network / internet objects (mplayer.cpp, ir_main.cpp are not ported)
PingSumm PSUMM;
PLAYERSBACKUP PBACK;
RETRANS RETSYS;
typedef void tpStartDownloadInternetFile( char* Name, char* Server, char* DestName );
typedef void tpSendRecBuffer( byte* Data, int size, bool Final );
tpStartDownloadInternetFile* StartDownloadInternetFile = NULL;
tpSendRecBuffer* SendRecBuffer = NULL;

PingSumm::PingSumm() { NPL = 0; PSET = NULL; }
PingSumm::~PingSumm() {}
void PingSumm::ClearPingInfo() {}
PLAYERSBACKUP::PLAYERSBACKUP() { NBDATA = 0; memset(BSTR, 0, sizeof(BSTR)); }
PLAYERSBACKUP::~PLAYERSBACKUP() {}
void PLAYERSBACKUP::Clear() {}
RETRANS::RETRANS() { TOT = NULL; NRET = 0; MaxRET = 0; }
RETRANS::~RETRANS() {}
void RETRANS::Clear() {}

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
void LoadSaveFile() {}
void SetBrokenState() {}
void ShowChat() {}
void ShowCentralMessage4( char* Message, int GPIDX, int DX ) { (void)Message; (void)GPIDX; (void)DX; }
void UpdateCurGame() {}

//-------------- map editor / random map --------------
// DecodeOptionsFromNumber: real implementation in newcode/mapoptionsencoder.cpp
// (now compiled) - stub deleted. Without it every additional game option
// chosen in the random-map UI decoded to 0 (default).
// ReadBMP24/CreateNEWMAP: real implementations now compiled (bmptool.cpp and
// the editor UI in interface.cpp) - stubs deleted.
// ReadClanData: real implementation is still inside a Win32 region.
void ReadClanData() {}

//-------------- network (mplayer.cpp, ir_main.cpp, unsyncro extras) --------------
// Execute-buffer reset from mplayer.cpp (real implementation: pure state, no network)
int LastRandDif = 0;
void SetupEBufs()
{
	SeqErrorsCount = 0;
	LastRandDif = 0;
	PrevRpos = 0;
	for ( int i = 0; i < MaxPL; i++ )
	{
		EBufs[i].RealTime = 0xFFFFFFFF;
		EBufs1[i].RealTime = 0xFFFFFFFF;
		EBufs[i].Enabled = true;
		EBufs1[i].Enabled = true;
		EBufs[i].Size = 0;
		EBufs1[i].Size = 0;
	}
}

int  GetPing( DWORD pid ) { (void)pid; return 0; }
// GetPName: real definition in interface.cpp (un-guarded)
void LOOSEIGAME( char* Nick ) { (void)Nick; }
void WINIGAME( char* Nick ) { (void)Nick; }
void GetGameID( char* s ) { if (s) s[0] = 0; }
void SendVictoryState( int ID, byte State ) { (void)ID; (void)State; }
void SyncroDoctor() {}
void ReceiveAll() {}
bool CheckPlayerToExit() { return false; }
bool RejectThisPlayer = false;

// ProcessSingleMission: real implementation now in interface.cpp (single
// missions + campaigns un-guarded) - stub deleted.

// lobby helpers used by the shared MPL_WaitingGame event loop
void SendChat( char* str, bool toall ) { (void)str; (void)toall; }
int  GetMaxRealPing() { return 0; }
bool SendToAllPlayers( DWORD Size, LPVOID lpData ) { (void)Size; (void)lpData; return false; }
void CreateDiffStr( char* str ) { if (str) str[0] = 0; }
int  ProcessInternetConnection( bool Active ) { (void)Active; return 0; }
// EncodeOptionsInNumber: real implementation in newcode/mapoptionsencoder.cpp
// (now compiled) - stub deleted.
void DeepDeletePeer( DWORD ID ) { (void)ID; }

// network globals (mplayer.cpp)
char CHATSTRING[256] = "";
DWORD CHATDPID = 0;
int CurrentMaxPing[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
int MinPing0 = 0;
DWORD GMGUID = 0;
bool DoNewInet = false;
int PPTIME = 0;
CDPID ServerDPID = 0;
int CurrentStartTime[8];
int NextStartTime[8];

// CheckFNSend: tiny port of interface.cpp's version (which lives in a
// _WIN32-guarded chat region). F1..F9 in GameKeyCheck call it to check
// whether the key was a multiplayer taunt; with chat disabled it always
// returns false so the F-keys open their local menus.
extern bool EnterChatMode;   // ddex1.cpp
int NGSPlayers = 0;
bool CheckFNSend( int idx )
{
	if ( !EnterChatMode )
	{
		return false;
	}
	if ( idx < NGSPlayers )
	{
		EnterChatMode = 0;
		return true;
	}
	return false;
}

// CD-audio/music: real implementations now in devicecd.cpp (SDL3 audio
// stream + stb_vorbis "Tracks/Track_XX.ogg"); stubs deleted.
// MidiSound stays here: it is only the options-slider state variable
// (interface.cpp's definition is inside a _WIN32 region).
int MidiSound = 0;
// IChat library callback (DLLIMPORT in interface.cpp); no-op without the DLL
void GoHomeAnyway() {}
// Multiplayer lobby screens; single-player play never reaches them.
bool processMultiplayer() { return false; }
bool processBattleMultiplayer() { return false; }
// Chat key dispatcher (interface.cpp, inside the Win32 multiplayer
// region; chat is a stubbed feature anyway - see PortingNotes).
void ProcessChatKeys() {}
// Terrain-editor error prompt (interface.cpp, unguarded editor UI). The
// real dialog body lives in ir_main.cpp (not compiled on Linux): draw the
// text via ShowCentralMessage4 (no-op stub for now) and wait for a
// keypress. printf keeps the message visible on the console meanwhile.
extern bool KeyPressed;
bool ProcessMessages();
void WaitWithError( char* ID, int GPID )
{
	char* str = GetTextByID( ID );
	printf( "WaitWithError: %s\n", str );
	ShowCentralMessage4( str, GPID, 90 );
	FlipPages();
	KeyPressed = 0;
	do
	{
		ProcessMessages();
	} while ( !KeyPressed );
}

// network functions (mplayer.cpp)
void AnalyseMessages() {}
void SendPings() {}
void SETPLAYERDATA( DWORD ID, void* Data, int size, bool change ) { (void)ID; (void)Data; (void)size; (void)change; }
void SETPLAYERNAME( char* name, bool change ) { (void)name; (void)change; }

#endif // !_WIN32
