// Linux port stubs.
//
// Temporary no-op definitions for functionality that is not ported yet:
//  - interface.cpp regions still guarded by _WIN32 (map interface, dialogs helpers, chat)
//  - path.cpp (pathfinding: heavy asm, needs porting)
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
#include "path.h"
#include "virtscreen.h"
#include "mplayer.h"
#include "ir.h"
#include "bmptool.h"
#include "megapolis.h"

#ifndef _WIN32

//-------------- global variables --------------
word COMPSTART[8];
char CurrentMap[64];
int CUR_TOOL_MODE = 0;
char DEFPLNAMES[8][64];
bool DRAWLOCK = false;
DWORD EBPos1 = 0;
int PLNAT[8];
EXBUFFER EBufs[MaxPL];
EXBUFFER EBufs1[MaxPL];
int ExitNI = 0;
int FogMode = 0;
char HEIGHTSTR[12];
int InteriorType = 0;
extern const int kCtrlStickyTime = 700;bool LockFog = false;
char MapScenaryDLL[200];
byte MI_Mode = 0;
bool MultiTvar = false;
byte NeedToPopUp = 0;
word NTextures = 0;
bool OptHidden = false;
int OrderSound = 0;
int PREVGLOBALTIME = 0;
word PrevRpos = 0;
int RM_LandType = 0;
int RM_Resstart = 0;
int RM_Restot = 0;
char SaveFileName[128];
int SeqErrorsCount = 0;
byte SYNBAD[8];
word TexList[128];
bool TexMapMod = false;
int TM_Height0 = 0;
int TM_Height1 = 0;
int TM_Tan0 = 0;
int TM_Tan1 = 0;
bool ToolsHidden = false;
bool TP_Made = false;
int WarSound = 0;
int WorkSound = 0;
DWORD RealTime = 0;
char RECFILE[128];
bool ChangeNation = false;

static byte WaterStorage[1 << 20];
byte* WaterBright = WaterStorage;
byte* WaterDeep = WaterStorage;

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

//-------------- fog.cpp (asm regions not ported) --------------
void DrawFog() {}
void DrawMiniFog() {}
void FogSpot( int x, int y ) { (void)x; (void)y; }
word GetFog( int x, int y ) { (void)x; (void)y; return 0; }
void SetupFog() {}
void ProcessFog1() {}
void TurnFogOff() {}
void CreateFogImage() {}
void SpotByUnit( int x, int y, int r, byte dir ) { (void)x; (void)y; (void)r; (void)dir; }
void SetupSpot() {}

//-------------- virtscr.cpp --------------

//-------------- water / rivers (realwater.cpp, river.cpp asm) --------------
void SetWaterSpot( int x, int y, int r ) { (void)x; (void)y; (void)r; }
void EraseWaterSpot( int x, int y, int r ) { (void)x; (void)y; (void)r; }
void CreateWaterLocking( int x, int y, int x1, int y1 ) { (void)x; (void)y; (void)x1; (void)y1; }
void InitBlobs() {}
void TestBlob() {}
void SetBrightSpot( int x, int y, int Brightness, bool dir ) { (void)x; (void)y; (void)Brightness; (void)dir; }

//-------------- interface.cpp guarded regions --------------
bool AskMissionQuestion( char* Bmp, char* Text ) { (void)Bmp; (void)Text; return false; }
word CheckCoorInGP( int x, int y ) { (void)x; (void)y; return 0; }
bool CheckFlagsNeed() { return false; }
bool CheckGameTime() { return false; }
void Clean_GP_IMG() {}
void CreateTrace( OneObject* OB ) { (void)OB; }
void DontMakeRaiting() {}
int  EnterHi( int* val, int Type ) { (void)val; (void)Type; return 0; }
bool EnterStr( char* str, char* Message ) { (void)str; (void)Message; return false; }
void ExplorerOpenRef( int Index, char* ref ) { (void)Index; (void)ref; }
int  GetCurGamePtr( byte** Ptr ) { if (Ptr) *Ptr = NULL; return 0; }
int  GetQueueMultiplier() { return 0; }
word GetTexture() { return 0; }
int  GetTreeItem() { return 0; }
void Init_GP_IMG() {}
bool IsGameActive() { return false; }
void LoadAIFromDLL( byte Nat, char* Name ) { (void)Nat; (void)Name; }
void LoadMapInterface() {}
void LoadPalettes() {}
void LoadSaveFile() {}
void ProcessMapInterface() {}
int  ProcessComplexQuestion( int Nx, char* Bmp1, byte or1, char* Text1, char* Bmp2, byte or2, char* Text2, char* Quest ) { (void)Nx; (void)Bmp1; (void)or1; (void)Text1; (void)Bmp2; (void)or2; (void)Text2; (void)Quest; return 0; }
void ProcessMissionText( char* Bmp, char* Text ) { (void)Bmp; (void)Text; }
int  ProcessMultilineQuestion( int Nx, char* Bmp1, byte or1, char* Text1, char* Quest ) { (void)Nx; (void)Bmp1; (void)or1; (void)Text1; (void)Quest; return 0; }
void RegisterVisibleGP( word Index, int FileIndex, int SprIndex, int x, int y ) { (void)Index; (void)FileIndex; (void)SprIndex; (void)x; (void)y; }
void SetBrokenState() {}
void ShowChat() {}
int  ShowHistryItem( char* Bmp, char* Text ) { (void)Bmp; (void)Text; return 0; }
void ShowCentralMessage4( char* Message, int GPIDX, int DX ) { (void)Message; (void)GPIDX; (void)DX; }
void TestGP() {}
void UnitLight( OneObject* OB ) { (void)OB; }
void UpdateCurGame() {}

//-------------- map editor / random map --------------
void CreateNEWMAP() {}
int  DecodeOptionsFromNumber( const int number, int* result ) { (void)number; (void)result; return 0; }
int  DetermineNationAI( byte Nat ) { (void)Nat; return 0; }
bool ReadBMP24( char* Name, BMPformat* BM, byte** data ) { (void)Name; (void)BM; (void)data; return false; }
void ReadClanData() {}

//-------------- dialogs / GP drawing helpers --------------

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
char* GetPName( int i ) { (void)i; return (char*)""; }
void LOOSEIGAME( char* Nick ) { (void)Nick; }
void WINIGAME( char* Nick ) { (void)Nick; }
void GetGameID( char* s ) { if (s) s[0] = 0; }
void SendVictoryState( int ID, byte State ) { (void)ID; (void)State; }
void SyncroDoctor() {}
void ReceiveAll() {}
bool CheckPlayerToExit() { return false; }
bool RejectThisPlayer = false;

// Starting an actual mission is not ported yet (StartIGame / ProcessSingleMission
// live in the still-Windows-only game-start block of interface.cpp ~9487-12419).
// Returning false keeps the player in the campaign mission list.
bool ProcessSingleMission( int n, int Diff ) { (void)n; (void)Diff; return false; }

// lobby helpers used by the shared MPL_WaitingGame event loop
void SendChat( char* str, bool toall ) { (void)str; (void)toall; }
int  GetMaxRealPing() { return 0; }
bool SendToAllPlayers( DWORD Size, LPVOID lpData ) { (void)Size; (void)lpData; return false; }
void CreateDiffStr( char* str ) { if (str) str[0] = 0; }
int  ProcessInternetConnection( bool Active ) { (void)Active; return 0; }
int  EncodeOptionsInNumber( int* selected_opt_values ) { (void)selected_opt_values; return 0; }
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

// network functions (mplayer.cpp)
void AnalyseMessages() {}
void SendPings() {}
void SETPLAYERDATA( DWORD ID, void* Data, int size, bool change ) { (void)ID; (void)Data; (void)size; (void)change; }
void SETPLAYERNAME( char* name, bool change ) { (void)name; (void)change; }

#endif // !_WIN32
