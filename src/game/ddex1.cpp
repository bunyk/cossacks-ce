/*==========================================================================
 *
 *  Copyright (C) 1997-1998 Andrew(GSC). All Rights Reserved.
 *
 *  Revamped in 2017 by Эреб
 *
 ***************************************************************************/

// This was entry point, ported stuff should be moved to main.cpp

#define NAME "CEW_KERNEL"

#include "ddini.h"
#include "gfile.h"
#include "fastdraw.h"
#include "mgraph.h"
#include "mapdiscr.h"
#include "mouse.h"
#include "common.h"
#include "menu.h"
#include "multipl.h"
#include "fog.h"
#include "walls.h"
#include <time.h>

#include "megapolis.h"
#include <assert.h>

#include "3dsurf.h"
#include "cdirsnd.h"
#include "mapsprites.h"
#include "virtscreen.h"
#include "realwater.h"
#include "transport.h"
#include "3dbars.h"
#include "strategyresearch.h"

#include "safety.h"
#include "newai.h"
#include "danger.h"
#include "gp_draw.h"
#include "sort.h"
#include "recorder.h"
#include "einfoclass.h"
#include "activescenary.h"

#include "playerinfo.h"
extern PlayerInfo PINFO[8];

#define TIMER_ID 1



extern char SaveFileName[128];

extern word rpos;
extern CDirSound* CDS;

void InitDialogs();
void SFLB_LoadGame( char* fnm, bool LoadNation );

void SaveGame( char* fnm, char* gg, int ID );
void SelectAllBuildings( byte NI );

void SetLight( int Ldx, int Ldy, int Ldz );
void TestTriangle();
void WinnerControl( bool );
void makeFden();
int processMainMenu();



void WaterCorrection();
extern bool TexMapMod;
extern bool RiverEditMode;
void ClearCurve();
extern int DrawPixMode;

#ifndef _WIN32
extern word PlayerMenuMode;
#define maxTask 32
typedef void EventHandPro( void* );
struct EventsTag
{
	EventHandPro* Pro;
	int Type;
	int Handle;
	bool Blocking;
	void* Param;
};
extern EventsTag Events[maxTask];
void InitFonts();
extern bool KeyPressed;
extern SDL_Keycode LastKey;
bool GetSDLKeyState(SDL_Scancode scancode, bool leftright = true);
extern int NInStack;
extern MouseStack MSTC[256];
extern int NKeys;
extern SDL_Keycode KeyStack[256];
extern char AsciiStack[256];
extern int ReliefBrush;
void ClearModes();
void ClearKeyStack();
extern int RealLx;
extern int RealLy;
extern byte SpecCmd;
extern bool BuildMode;
extern bool GetCoord;
extern int mousePointerType;
extern int curdx;
extern int curdy;
extern byte PauseMode;
extern byte LockMode;
extern bool MEditMode;
extern int HeightEditMode;
extern byte LockGrid;
extern int HiStyle;
extern int SHIFT_VAL;
extern int TerrBrush;
extern byte EditMedia;
extern bool EditMapMode;
extern int AutoTime;
extern int CurrentStepTime;
extern int LastCTRLPressTime;
extern const int kCtrlStickyTime;
extern uint64_t GetSDLTickCount();
extern bool ProcessMessages();
#endif

void TimerProc( void )
{
	if (PlayerMenuMode == 1)
	{
		for (int j = 0; j < maxTask; j++)
		{
			if (Events[j].Pro)
			{
				( *( Events[j].Pro ) ) ( Events[j].Param );
			}
		}
		SetRLCWindow( 0, 1, MaxSizeX, RSCRSizeY - 1, ScrWidth );
	}
	else
	{
		SetRLCWindow( 0, 1, MaxSizeY, RSCRSizeY - 1, ScrWidth );
	}
	HandleMouse( mouseX, mouseY );
	MFix();
	FlipPages();
}

//Loading...


// SaveScreenShot/SaveMiniScreenShot/SaveScreen/MiniRenderAllMap/
// RenderAllMap were MOVED to main.cpp (2026-11-01) together with
// EditorKeyCheck for the Linux map editor port.



extern bool GUARDMODE;
extern bool PATROLMODE;
extern byte NeedToPopUp;
void IAmLeft();
void LOOSEANDEXITFAST();
extern bool DoNewInet;
bool ReadWinString( GFILE* F, char* STR, int Max );

void CmdEndGame( byte NI, byte state, byte cause );


void ProcessGSaveMap();
void EditorKeyCheck();
void ProcessSaveInSquares();
void TestGenMap();
bool ShowStatistics();

char ChatString[128];
wchar_t unicode_chat_string[128];

void ProcessChatKeys();
extern int WaitState;


extern byte PlayGameMode;
bool CheckFNSend( int idx );
void ProcessVotingKeys();
extern bool RESMODE;
extern bool OptHidden;
extern word NPlayers;
bool CheckFlagsNeed();
void SetGameDisplayModeAnyway( int SizeX, int SizeY );


void CreateFastLocking();
void AddHill();
bool DelCurrentAZone();
void SelectNextGridMode();
void ProcessMapOptions();
void ResearchIslands();
void EnterRandomParams();
void GenerateRandomRoad( int idx );
bool CheckCurve();
void ClearCurve();
extern bool ToolsHidden;
void SetFractalTexture();
void AutoSMSSet();
void UpdateAllPieces();

extern int PEN_RADIUS;
extern int PEN_BRIGHTNESS;
void LoadCurPixTexture( char* Name );

// EditorKeyCheck was MOVED to main.cpp (2026-11-01) for the Linux map
// editor port - ddex1.cpp is the original Windows entrypoint and is not
// part of the Linux build.void SERROR1();
void SERROR2();

extern int PlayMode;
void StopPlayCD();
bool PlayRandomTrack();
bool First = 1;
bool ProcessMessagesEx();
void ClearRGB();

extern bool Lpressed;

void FilesExit();


void AddDestn( byte x, byte y );
void ProcessNewMonsters();
void InitXShift();
void HandleMines();
void ControlGates();
void HealWalls();
void ProcessDeathList();
void ProcessEconomy();
void HandleMission();
void CheckTops()
{
	int NT = NAreas*NAreas;
};

void ResearchCurrentIsland( byte Nat );
void ResearchBestPortToFish( byte Nat );
extern int NInGold[8];
extern int NInIron[8];
extern int NInCoal[8];
extern int WasInGold[8];
extern int WasInIron[8];
extern int WasInCoal[8];
extern bool Tutorial;
void ProcessCostPoints();
void CheckArmies( City* );
void CheckGP();

void CmdSetSpeed( byte );
void EnumPopulation();
extern bool TutOver;
void HandleBalloon( Nation* NT );
void AddRandomBlobs();
void ProcessMapAutosave();

void ProcessGuard();
void DecreaseVeruVPobedu();




extern bool PreNoPause;
void StopPlayCD();
void ProcessUpdate();
extern byte CaptState;
extern byte SaveState;
void WritePitchTicks();
void ShowCentralText0( char* sss );






void PerformNewUpgrade( Nation* NT, int UIndex, OneObject* OB );



lpCHAR FLIST[4096];
int NFILES = 0;

void CreateRadio();
extern int ScrollSpeed;
void UnLoading();


bool PlayCDTrack( int Id );
bool PlayRandomTrack();
extern int PlayMode;

typedef bool tpShowDialog( int NModes, int* Sizex, int* Sizey, int* Current );
tpShowDialog* lpShowDialog;
