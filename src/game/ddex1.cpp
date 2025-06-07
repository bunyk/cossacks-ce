/*==========================================================================
 *
 *  Copyright (C) 1997-1998 Andrew(GSC). All Rights Reserved.
 *
 *  Revamped in 2017 by Эреб
 *
 ***************************************************************************/

#define NAME "CEW_KERNEL"
#define NODPLAY

#include "gfile.h"
#include "mgraph.h"
#include "mouse.h"
#include "menu.h"
#include "multipl.h"
#include "fog.h"
#include "walls.h"
#include "nature.h"
#include <time.h>
#include "nucl.h"

#include "megapolis.h"
#include "dialogs.h"
#include <assert.h>

#include "3dsurf.h"
#include "cdirsnd.h"
#include "mapsprites.h"
#include "virtscreen.h"
#include "realwater.h"
#include "transport.h"
#include "antibug.h"
#include "3dbars.h"
#include "topograf.h"
#include "strategyresearch.h"

#include "safety.h"
#include "newai.h"
#include "danger.h"
#include "gp_draw.h"
#include "sort.h"
#include "recorder.h"
#include "mplayer.h"
#include "einfoclass.h"
#include "3dmaped.h"
#include "activescenary.h"
#include "fonts.h"
#include "dialogs/initfonts.h"
#include "interface.h"
#include "menu.h"

#include "playerinfo.h"
extern PlayerInfo PINFO[8];

#define TIMER_ID 1

bool AttackMode;
bool ChoosePosition;
bool DeathMode;
bool EgoFlag;
bool FASTMODE;
bool FastMode = false;
bool FullMini = true;
bool HealthMode;
bool HelpMode;
bool InfoMode;
bool MUSTDRAW;

//Unknown mode classification
bool MiniMode;

bool PeaceMode;
bool TransMode;
bool VHMode = 0;
bool fixed;




//Game version. Must match with other clients
DLLEXPORT word dwVersion = 100;
DLLEXPORT char LobbyVersion[32] = "1.00";
DLLEXPORT char BuildVersion[32] = "V 1.00";

int CostThickness;
int ReliefBrush;
int TerrBrush;
int BlobMode;
int CoalID;
int CurGroundTexture = 0;
int Flips;
int FoodID;
int FrmDec = 2;
int GoldID;
int HiStyle;
int IronID;
int LASTRAND, LASTIND;
int LastAttackDelay = 0;
int Multip;
int NMONS;
int REALTIME;

//Multiplier, depends on MiniMode
//MiniMode ? 4 : 5
int Shifter;

//Game speed mode dependent SpeedShift variable. Used for controlling animations etc
int SpeedSh = 1;

int StoneID;
int TreeID;


static int Light = 0;

char* FormationStr = nullptr;

CDirSound CDIRSND;
City CITY[8];
HugeExplosion HE;
Menu About;
Menu LoadFile;
Menu MainMenu;
Menu Options;
Nation WEP;
Weapon Arrow;
Weapon Fire1;
Weapon FlyFire1;
Weapon Flystar;
Weapon Lasso;
Weapon Magvib;
Weapon Molnia;
Weapon MolniaVibux;
Weapon Sphere;
Weapon Vibux1;

extern bool AttGrMode;
extern bool CINFMOD;
extern bool ChangeNation;
extern bool CheapMode;
extern bool FullScreenMode;
extern bool GameInProgress;
extern bool GameNeedToDraw;
extern bool GoAndAttackMode;
extern bool LockPause;
extern bool MakeMenu;
extern bool MiniActive;
extern bool MultiTvar;
extern bool Recreate;
extern bool SetDestMode;
extern bool realLpressed;
extern bool realRpressed;

extern int FogMode;
extern int Inform;
extern int MaxAllowedComputerAI;
extern int MenuType;
extern int MidiSound;
extern int NMyUnits;
extern int NThemUnits;
extern int OrderSound;
extern int RealPause;
extern int ShowGameScreen;
extern int WarSound;
extern int WorkSound;
extern int sfVersion;

extern char SaveFileName[128];

extern bool ScanPressed[SDL_SCANCODE_KP_0 + 1];
extern byte SpecCmd;
extern word rpos;
extern BlockBars LockBars;
extern BlockBars UnLockBars;
extern CDirSound* CDS;

void InitDialogs();
void SFLB_LoadGame( char* fnm, bool LoadNation );

void CheckGP();
void ClearMaps();
void CmdChangeSpeed();
void CreateRandomHMap();
void CreateTotalLocking();
void CreateUnitsLocking();
void EraseAreas();
void GSSetup800();
void GetForces();
void Init3DMapSystem();
void InitDestn();
void InitFishMap();
void LoadMessages();
void LoadNewAimations();
void Loadtextures();
void MFix();
void OnMouseMoveRedraw();
void ProcessFishing();
void ProcessSprites();
void ProcessUFO();
void RenderAllMap();
void Reset3D();
void SaveGame( char* fnm, char* gg, int ID );
void SelectAllBuildings( byte NI );

void SetLight( int Ldx, int Ldy, int Ldz );
void TestTriangle();
void WinnerControl( bool );
void makeFden();
int processMainMenu();


int GetResID( char* );

#ifndef NODPLAY
HWND hwnd;
#endif


void ShowFon1();
void WaterCorrection();
extern bool TexMapMod;
extern bool RiverEditMode;
void ClearCurve();
extern int DrawPixMode;

void TimerProc( void )
{
	if (PlayerMenuMode == 1)
	{
		ShowFon1();
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
void LoadEconomy();
void LoadNations();
void LoadWeapon();
void LoadNation( char* fn, byte msk, byte NIndex );
void LoadAllNewMonsters();
void InitNewMonstersSystem();
void LoadWaveAnimations();
extern NewAnimation* Shar;
void DoGen();
void InitDeathList();
char* GetTextByID( char* ID );
void LoadBorders();

void SetupArrays();
extern byte* RivDir;
void Init_GP_IMG();
void ReadClanData();

//Calculates window coordinates and locks cursor inside client area
//Load ids, textures etc
bool Loading()
{
	ReadClanData();
	RivDir = NULL;

	Init_GP_IMG();
	InitDeathList();
	InitNewMonstersSystem();
	InitFonts();
	LoadBorders();
	LoadMessages();
	LoadNations();
	LoadFon();
	LoadRDS();

	GoldID = GetResID( "GOLD" );
	FoodID = GetResID( "STONE" );
	TreeID = GetResID( "WOOD" );
	CoalID = GetResID( "COAL" );
	IronID = GetResID( "IRON" );
	LoadEconomy();

	Loadtextures();
	LoadFog( 1 );
	LoadTiles();
	LoadLock();
	LoadNewAimations();
	LoadWeapon();
	InitExplosions();
	InitSprites();
	LoadAllWalls();
	LoadAllNewMonsters();
	LoadWaveAnimations();

	LoadAllNations( 0 );
	LoadAllNations( 1 );
	LoadAllNations( 2 );
	LoadAllNations( 3 );
	LoadAllNations( 4 );
	LoadAllNations( 5 );
	LoadAllNations( 6 );
	LoadAllNations( 7 );

	CITY[0].CreateCity( 0 );
	CITY[1].CreateCity( 1 );
	CITY[2].CreateCity( 2 );
	CITY[3].CreateCity( 3 );
	CITY[4].CreateCity( 4 );
	CITY[5].CreateCity( 5 );
	CITY[6].CreateCity( 6 );
	CITY[7].CreateCity( 7 );

	InitTopChange();
	LoadPalettes();
	InitPrpBar();

	SetMyNation( 0 );

	FormationStr = GetTextByID( "FORMATION" );

	return 1;
}

void SaveScreenShot( char* Name )
{
	byte PAL[1024];
	memset( PAL, 0, 1024 );
	char ccx[120];
	sprintf( ccx, "%d\\agew_1.pal", CurPalette );
	ResFile f = RReset( ccx );
	int i;
	for (i = 0; i < 256; i++)
	{
		int ofs = i << 2;
		RBlockRead( f, PAL + ofs + 2, 1 );
		RBlockRead( f, PAL + ofs + 1, 1 );
		RBlockRead( f, PAL + ofs, 1 );
	};
	RClose( f );
	f = RRewrite( Name );
	i = 0x4D42;
	RBlockWrite( f, &i, 2 );
	i = RealLx*RealLy + 1080;
	RBlockWrite( f, &i, 4 );
	i = 0;
	int j = 0x436;
	RBlockWrite( f, &i, 4 );
	RBlockWrite( f, &j, 4 );
	j = 0x28;
	RBlockWrite( f, &j, 4 );
	RBlockWrite( f, &RealLx, 4 );
	RBlockWrite( f, &RealLy, 4 );
	j = 0x080001;
	RBlockWrite( f, &j, 4 );
	RBlockWrite( f, &i, 4 );
	RBlockWrite( f, &i, 4 );
	j = 0x0B12;
	RBlockWrite( f, &j, 4 );
	RBlockWrite( f, &j, 4 );
	RBlockWrite( f, &i, 4 );
	RBlockWrite( f, &i, 4 );
	RBlockWrite( f, PAL, 1024 );
	for (int i = 0; i < RealLy; i++)
	{
		char* pos = static_cast<char*>(ScreenPtr) + (RealLy - i - 1) * SCRSizeX;
		RBlockWrite( f, pos, RealLx );
	};
	RClose( f );
};
void SaveBMP8( char* Name, int lx, int ly, byte* Data )
{
	byte PAL[1024];
	memset( PAL, 0, 1024 );
	char ccc[128];
	sprintf( ccc, "%d\\agew_1.pal", CurPalette );
	ResFile f = RReset( ccc );
	int i;
	for (i = 0; i < 256; i++)
	{
		int ofs = i << 2;
		RBlockRead( f, PAL + ofs + 2, 1 );
		RBlockRead( f, PAL + ofs + 1, 1 );
		RBlockRead( f, PAL + ofs, 1 );
	};
	RClose( f );
	f = RRewrite( Name );
	i = 0x4D42;
	RBlockWrite( f, &i, 2 );
	i = lx*ly + 1080;
	RBlockWrite( f, &i, 4 );
	i = 0;
	int j = 0x436;
	RBlockWrite( f, &i, 4 );
	RBlockWrite( f, &j, 4 );
	j = 0x28;
	RBlockWrite( f, &j, 4 );
	int LX = lx;
	int LY = ly;
	RBlockWrite( f, &LX, 4 );
	RBlockWrite( f, &LY, 4 );
	j = 0x080001;
	RBlockWrite( f, &j, 4 );
	RBlockWrite( f, &i, 4 );
	RBlockWrite( f, &i, 4 );
	j = 0x0B12;
	RBlockWrite( f, &j, 4 );
	RBlockWrite( f, &j, 4 );
	RBlockWrite( f, &i, 4 );
	RBlockWrite( f, &i, 4 );
	RBlockWrite( f, PAL, 1024 );
	for (int i = 0; i < LY; i++)
	{
		char* pos = reinterpret_cast<char*>(Data) + (ly - i - 1) * lx;
		RBlockWrite( f, pos, lx );
	};
	RClose( f );
};
void SaveMiniScreenShot( char* Name )
{
	byte PAL[1024];
	memset( PAL, 0, 1024 );
	ResFile f = RReset( "agew_1.pal" );
	int i;
	for (int i = 0; i < 256; i++)
	{
		int ofs = i << 2;
		RBlockRead( f, PAL + ofs + 2, 1 );
		RBlockRead( f, PAL + ofs + 1, 1 );
		RBlockRead( f, PAL + ofs, 1 );
	};
	RClose( f );
	f = RRewrite( Name );
	i = 0x4D42;
	RBlockWrite( f, &i, 2 );
	i = RealLx*RealLy + 1080;
	RBlockWrite( f, &i, 4 );
	i = 0;
	int j = 0x436;
	RBlockWrite( f, &i, 4 );
	RBlockWrite( f, &j, 4 );
	j = 0x28;
	RBlockWrite( f, &j, 4 );
	int LX = RealLx >> 2;
	int LY = RealLy >> 2;
	RBlockWrite( f, &LX, 4 );
	RBlockWrite( f, &LY, 4 );
	j = 0x080001;
	RBlockWrite( f, &j, 4 );
	RBlockWrite( f, &i, 4 );
	RBlockWrite( f, &i, 4 );
	j = 0x0B12;
	RBlockWrite( f, &j, 4 );
	RBlockWrite( f, &j, 4 );
	RBlockWrite( f, &i, 4 );
	RBlockWrite( f, &i, 4 );
	RBlockWrite( f, PAL, 1024 );
	for (int i = 0; i < LY; i++)
	{
		char* pos = static_cast<char*>(ScreenPtr) + (RealLy - i - 1) * SCRSizeX;
		for (int j = 0; j < LX; j++)RBlockWrite( f, pos + j * 4, 1 );
	};
	RClose( f );
};
void SaveScreen()
{
	char ccc[128];
	SDL_CreateDirectory( "Screenshots");
	int i;
	for (i = 0; i < 1000; i++)
	{
		sprintf( ccc, "Screenshots\\screen%d.bmp", i );
		ResFile f = RReset( ccc );
		if (f == INVALID_HANDLE_VALUE)
		{
			RClose( f );
			goto zzz;
		};
		RClose( f );
	};
	i = 99;
zzz:
	sprintf( ccc, "Screenshots\\screen%d.bmp", i );
	SaveScreenShot( ccc );
};
extern bool NoText;
extern bool SHOWSLIDE;
void GFieldShow();
void SaveMiniScreenShot( char* Name );
void MiniRenderAllMap()
{
	SHOWSLIDE = true;
	int nx = div( msx, smaplx ).quot;
	int ny = div( msy, smaply ).quot;
	//if(nx>3)nx=3;
	//if(ny>3)ny=3;
	char ccc[128];
	NoText = true;
	for (int y = 0; y < ny; y++)
	{
		for (int x = 0; x < nx; x++)
		{
			mapx = x*smaplx;
			mapy = y*smaply;
			GFieldShow();
			FlipPages();
			int p = x + y*nx;
			if (p < 10)sprintf( ccc, "scr00%d.bmp", p );
			else if (p < 100)sprintf( ccc, "scr0%d.bmp", p );
			else sprintf( ccc, "scr%d.bmp", p );
			SaveMiniScreenShot( ccc );
		};
	};
	NoText = false;
};

void RenderAllMap()
{
	if (GetSDLKeyState( SDL_SCANCODE_LSHIFT ))
	{
		MiniRenderAllMap();
		return;
	};
	SHOWSLIDE = true;
	int nx = div( msx, smaplx ).quot;
	int ny = div( msy, smaply ).quot;
	//if(nx>3)nx=3;
	//if(ny>3)ny=3;
	char ccc[128];
	NoText = true;
	for (int y = 0; y < ny; y++)
	{
		for (int x = 0; x < nx; x++)
		{
			mapx = x*smaplx;
			mapy = y*smaply;
			GFieldShow();
			FlipPages();
			int p = x + y*nx;
			if (p < 10)sprintf( ccc, "scr00%d.bmp", p );
			else if (p < 100)sprintf( ccc, "scr0%d.bmp", p );
			else sprintf( ccc, "scr%d.bmp", p );
			SaveScreenShot( ccc );
		}
	}
	NoText = false;
}

#define MaxQu 32
MouseStack MSTC[MaxQu];
MouseStack CURMS;
int NInStack = 0;
void AddMouseEvent( int x, int y, bool L, bool R )
{
	if (NInStack < MaxQu)
	{
		MSTC[NInStack].x = x;
		MSTC[NInStack].y = y;
		MSTC[NInStack].Lpressed = L;
		MSTC[NInStack].Rpressed = R;
		MSTC[NInStack].rLpressed = L;
		MSTC[NInStack].rRpressed = R;
		MSTC[NInStack].Control = ( GetSDLKeyState( SDL_SCANCODE_LCTRL ) ) != 0;
		MSTC[NInStack].Shift = ( GetSDLKeyState( SDL_SCANCODE_LSHIFT ) ) != 0;
		NInStack++;
	}
}

int LastUMX = 0;
int LastUMY = 0;
int LastUTime = 0;

MouseStack* ReadMEvent()
{
	if (NInStack)
	{
		CURMS = MSTC[0];
		if (NInStack > 1)
		{
			memcpy( MSTC, MSTC + 1, ( NInStack - 1 ) * sizeof(MouseStack) );
		}
		NInStack--;
		return &CURMS;
	}
	return nullptr;
}

void ClearMStack()
{
	NInStack = 0;
}

extern bool unpress;

void UnPress()
{
	for (int i = 0; i < NInStack; i++)
	{
		MSTC[i].Lpressed = 0;
		MSTC[i].Rpressed = 0;
	}
	unpress = 1;
	memset( ScanPressed, false, sizeof(ScanPressed));
}

extern int CurPalette;
int SHIFT_VAL = 0;
void HandleMouse( int x, int y );
extern bool PalDone;
SDL_Keycode KeyStack[32];
byte AsciiStack[32];
int NKeys = 0;
// This is used only in InputBox_OnKeyDown()
byte LastAsciiKey = 0;

void AddKey( SDL_Keycode Key, byte Ascii )
{
	if (32 <= NKeys)
	{//Push the stack back by one element
		memcpy( &KeyStack[0], &KeyStack[1], sizeof(KeyStack) - sizeof(SDL_Keycode));
		memcpy( AsciiStack, AsciiStack + 1, 31 );
		NKeys--;
	}
	KeyStack[NKeys] = Key;
	AsciiStack[NKeys] = Ascii;
	NKeys++;
}

// This is used only in ProcessChatKeys()
byte LastAscii = 0;
wchar_t last_unicode = 0;
SDL_Keycode ReadKey()
{//Called only for chat input and resource transfer
	if (NKeys)
	{
		SDL_Keycode c = KeyStack[0];
		LastAscii = AsciiStack[0];
		if (NKeys)
		{
			memcpy( &KeyStack[0], &KeyStack[1], sizeof(SDL_Keycode) * (NKeys - 1) );
			memcpy( AsciiStack, AsciiStack + 1, NKeys - 1 );
		}
		NKeys--;
		return c;
	}
	else
	{
		return SDLK_UNKNOWN;
	}
}

void ClearKeyStack()
{
	NKeys = 0;
}

extern bool GUARDMODE;
extern bool PATROLMODE;
extern byte NeedToPopUp;
short WheelDelta = 0;
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
bool EnterChatMode = 0;

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

void EditorKeyCheck()
{
	if (KeyPressed)
	{
		KeyPressed = false;
		SDL_Keycode wParam = LastKey;
		switch (wParam)
		{
		case SDLK_RIGHT:
			//if(DrawPixMode||DrawGroundMode)TexStDX=(TexStDX+1)&7;
			break;
		case SDLK_UP:
			//if(DrawPixMode||DrawGroundMode)TexStDY=(TexStDY-1)&7;
			break;
		case SDLK_LEFT:
			//if(DrawPixMode||DrawGroundMode)TexStDX=(TexStDX-1)&7;
			break;
		case SDLK_DOWN:
			//if(DrawPixMode||DrawGroundMode)TexStDY=(TexStDY+1)&7;
			break;
		case SDLK_E:
			FastMode = !FastMode;
			break;
		case SDLK_0:
			//if(DrawPixMode||DrawGroundMode)STBRR(wParam);
			break;
		case SDLK_1:
			//if(DrawPixMode||DrawGroundMode)STBRR(wParam);else
			if (GetSDLKeyState( SDL_SCANCODE_LCTRL ))CostThickness = 1;
			else ReliefBrush = 1;
			break;
		case SDLK_2:
			//if(DrawPixMode||DrawGroundMode)STBRR(wParam);else
			if (GetSDLKeyState( SDL_SCANCODE_LCTRL ))CostThickness = 2;
			else ReliefBrush = 2;
			break;
		case SDLK_3:
			//if(DrawPixMode||DrawGroundMode)STBRR(wParam);else
			if (GetSDLKeyState( SDL_SCANCODE_LCTRL ))CostThickness = 3;
			else ReliefBrush = 3;
			break;
		case SDLK_4:
			//if(DrawPixMode||DrawGroundMode)STBRR(wParam);else
			if (GetSDLKeyState( SDL_SCANCODE_LCTRL ))CostThickness = 4;
			else ReliefBrush = 4;
			break;
		case SDLK_5:
			//if(DrawPixMode||DrawGroundMode)STBRR(wParam);else
			if (GetSDLKeyState( SDL_SCANCODE_LCTRL ))CostThickness = 5;
			else ReliefBrush = 5;
			break;
		case SDLK_6:
			//if(DrawPixMode||DrawGroundMode)STBRR(wParam);else
			if (GetSDLKeyState( SDL_SCANCODE_LCTRL ))CostThickness = 6;
			else ReliefBrush = 9;
			break;
		case SDLK_7:
			//if(DrawPixMode||DrawGroundMode)STBRR(wParam);else
			if (GetSDLKeyState( SDL_SCANCODE_LCTRL ))CostThickness = 7;
			else ReliefBrush = 20;
			break;
		case SDLK_8:
			//if(DrawPixMode||DrawGroundMode)STBRR(wParam);else
			if (GetSDLKeyState( SDL_SCANCODE_LCTRL ))CostThickness = 8;
			else ReliefBrush = 50;
			break;
		case SDLK_9:
			//if(DrawPixMode||DrawGroundMode)STBRR(wParam);else
			if (GetSDLKeyState( SDL_SCANCODE_LCTRL ))CostThickness = 9;
			else ReliefBrush = 100;
			break;
		case SDLK_H:
			//FullScreenMode=!FullScreenMode;
			//GameNeedToDraw=true;
			//GSSetup800();
			RenderAllMap();
			break;
		case SDLK_V:
			if (GetSDLKeyState( SDL_SCANCODE_LCTRL ))
			{
				ClearModes();
				NeedToPopUp = 20;
			}
			else
			{
				ClearModes();
				NeedToPopUp = 22;
				//SetFractalTexture();
				/*
				switch(Light){
				case 0:SetLight(-5,10,20);
					break;
				case 1:SetLight(-5,12,17);
					break;
				case 2:SetLight(-5,15,15);
					break;
				case 3:SetLight(-5,17,12);
					break;
				case 4:SetLight(-5,20,10);
					break;
				case 5:SetLight(-5,17,12);
					break;
				case 6:SetLight(-5,15,15);
					break;
				case 7:SetLight(-5,12,17);
				};
				Light++;
				if(Light>7)Light=0;
				*/
			};
			break;
		case SDLK_RETURN:
			if (!( GetSDLKeyState( SDL_SCANCODE_LCTRL ) ))
			{
				//MakeMenu=true;
				//MenuType=3;
				EnterChatMode = 1;
				ClearKeyStack();
			}
			else KeyPressed = true;
			break;
		case SDLK_J:
			//RSCRSizeX++;
			//ShowStatistics();
			break;
		case SDLK_PAUSE:
			if (!LockPause)SpecCmd = 137;
			//PauseMode=!PauseMode;
			break;
		case SDLK_ESCAPE:
			AttGrMode = 0;
			if (CheckCurve())
			{
				ClearCurve();
			}
			else
			{
				ClearModes();
				BuildMode = false;
				//BuildWall=false;
				GetCoord = false;
				mousePointerType = 0;
				curdx = 0;
				curdy = 0;
				GUARDMODE = 0;
				PATROLMODE = 0;
				PauseMode = false;
				SetDestMode = false;
				GoAndAttackMode = false;
				NeedToPopUp = 1;
			};
			//if(PlayerMenuMode==2)
			//MainMenu.ShowModal();
			//Options.ShowModal();
			break;
		case SDLK_F12:
			MenuType = 1;
			MakeMenu = true;
			break;
		case SDLK_TILDE:
			HealthMode = !HealthMode;
			break;
		case SDLK_F1:
			HelpMode = !HelpMode;
			break;
		case SDLK_F2:
			NeedToPopUp = 6;
			break;
		case SDLK_F3:
			NeedToPopUp = 15;
			break;
		case SDLK_F4:
			NeedToPopUp = 4;
			break;
		case SDLK_F5:
			if (WaterEditMode)
			{
				WaterEditMode = 1;
				NeedToPopUp = 8;
			}
			else
			{
				NeedToPopUp = 5;
			};
			break;
		case SDLK_U:
			if (GetSDLKeyState( SDL_SCANCODE_LCTRL ))CINFMOD = !CINFMOD;
			else if (Inform != 2)Inform = 2; else Inform = 0;
			MiniActive = 0;
			Recreate = 1;
			//CINFMOD=0;
			break;
		case SDLK_F:
			//SVSC.Zero();
			if (GetSDLKeyState( SDL_SCANCODE_LCTRL ))
			{
				TestGenMap();
			}
			else FogMode = !FogMode;
			//HeightEditMode=false;
			//MEditMode=true;
			//EditMedia=5;
			break;
		case SDLK_F6:
			if (WaterEditMode)
			{
				WaterEditMode = 2;
				NeedToPopUp = 9;
			}
			else
			{
				ClearModes();
				MakeMenu = true;
				MenuType = 6;
			};
			break;
		case SDLK_F7:
			if (WaterEditMode)
			{
				WaterEditMode = 3;
				NeedToPopUp = 10;
			}
			else Reset3D();
			break;
		case SDLK_F8:
			if (WaterEditMode)
			{
				WaterEditMode = 4;
				NeedToPopUp = 11;
			}
			else
			{
				NeedToPopUp = 3;
			};
			break;
		case SDLK_N:
			if (GetSDLKeyState( SDL_SCANCODE_LCTRL ))
			{
				AutoSMSSet();
			}
			else
			{
				NeedToPopUp = 7;
				//HeightEditMode=3;
				//MEditMode=false;
				//EditMedia=5;
			};
			break;
		case SDLK_DELETE:
			if (!DelCurrentAZone())SpecCmd = 200;
			break;

		/*
		case SDLK_D:
			if (!( GetSDLKeyState( SDL_SCANCODE_LCTRL ) ))
			{
				if (( GetSDLKeyState( SDL_SCANCODE_LSHIFT ) ))//&& PlayGameMode)
				{
					switch (HISPEED)
					{
					case 0:
						HISPEED = 1;
						break;
					case 1:
						HISPEED = 2;
						break;
					case 2:
						HISPEED = 3;
						break;
					default:
						HISPEED = 0;
						break;
					};
				};

			}
			else
			{
				//CmdChangeSpeed();//BUGFIX: real time speed changing
			}
			break;
		*/

		case SDLK_A:
			if (GetSDLKeyState( SDL_SCANCODE_LCTRL ))SpecCmd = 1;
			else if (NSL[MyNation])GoAndAttackMode = 1;
			break;
		case SDLK_S:
			//ClearModes();
			if (GetSDLKeyState( SDL_SCANCODE_LCTRL ))
			{
				//EnterRandomParams();
			}
			else
			{
				switch (LockMode)
				{
				case 0:
					NeedToPopUp = 12;
					break;
				case 1:
					NeedToPopUp = 13;
					break;
				case 2:
					NeedToPopUp = 14;
					break;
				case 3:
					NeedToPopUp = 1;
					break;
				};
			};
			break;
		case SDLK_W:
			if (GetSDLKeyState( SDL_SCANCODE_LCTRL ))ProcessSaveInSquares();
			else PeaceMode = !PeaceMode;
			break;
			/*
		case 'N':
			switch(RSCRSizeX){
				case 800:RSCRSizeX=1024;
						break;
				case 1024:RSCRSizeX=800+32;
						break;
				case 1280:RSCRSizeX=1600;
					break;
				case 1600:RSCRSizeX=800;
					break;
				default:
					RSCRSizeX=800;
			};*/
			/*if(RSCRSizeX!=1024)RSCRSizeX=1024;
			else RSCRSizeX=800;*/
			//break;
		case SDLK_C:
			//CINFMOD=!CINFMOD;
			if (GetSDLKeyState( SDL_SCANCODE_LCTRL ))
			{
				SpecCmd = 217;
			}
			else
			{
				WaterEditMode = 1;
				NeedToPopUp = 8;
			};
			break;
		case SDLK_X:
			//if(GetSDLKeyState(SDL_SCANCODE_LCTRL))SpecCmd=5;
			//else SpecCmd=6;
			if (GetSDLKeyState( SDL_SCANCODE_LCTRL ))
			{
				EraseAreas();
				rando();//!!
				CreateTotalLocking();
				ResearchIslands();
			}
			else
			{
				ClearModes();
				DrawPixMode = 1;
				NeedToPopUp = 21;
			};
			break;
		case SDLK_Q:
			LockGrid += 2;//++;
			if (LockGrid > 3)LockGrid = 0;
			MiniActive = 0;
			Recreate = 1;
			break;
		case SDLK_B:
			if (GetSDLKeyState( SDL_SCANCODE_LCTRL ))SpecCmd = 9;
			else SpecCmd = 10;
			//ClearMaps();
			//CreateUnitsLocking();
			break;
		case SDLK_Z:
			//if(DrawPixMode){
			//	PerformPixUndo();
			//};
			//if(GetSDLKeyState(SDL_SCANCODE_LCTRL))SpecCmd=11;
			//else SpecCmd=12;

			break;
			//case SDLK_F11:
			//	WaterCorrection();
			//	break;
		case SDLK_F9:
			MEditMode = false;
			HeightEditMode = false;
			LockMode = 0;
			Creator = 4096 + 255;
			NeedToPopUp = 1;
			break;
		case SDLK_KP_1:
			SetMyNation( 0 );
			MEditMode = false;
			HeightEditMode = false;
			PlayerMask = 1;
			break;
		case SDLK_KP_2:
			SetMyNation( 1 );
			MEditMode = false;
			HeightEditMode = false;
			PlayerMask = 2;
			break;
		case SDLK_KP_3:
			SetMyNation( 2 );
			MEditMode = false;
			HeightEditMode = false;
			PlayerMask = 4;
			break;
		case SDLK_KP_4:
			SetMyNation( 3 );
			MEditMode = false;
			HeightEditMode = false;
			PlayerMask = 8;
			break;
		case SDLK_KP_5:
			SetMyNation( 4 );
			MEditMode = false;
			HeightEditMode = false;
			PlayerMask = 16;
			break;
		case SDLK_KP_6:
			SetMyNation( 5 );
			MEditMode = false;
			HeightEditMode = false;
			PlayerMask = 32;
			break;
		case SDLK_KP_7:
			SetMyNation( 6 );
			MEditMode = false;
			HeightEditMode = false;
			PlayerMask = 64;
			break;
		case SDLK_KP_8:
			SetMyNation( 7 );
			MEditMode = false;
			HeightEditMode = false; PlayerMask = 128;
			break;
		case SDLK_I:
			if (GetSDLKeyState( SDL_SCANCODE_LCTRL ))InfoMode = !InfoMode;
			else if (Inform != 1)Inform = 1; else Inform = 0;
			MiniActive = 0;
			Recreate = 1;
			//InfoMode=1;
			break;
		case SDLK_O:
			if (GetSDLKeyState( SDL_SCANCODE_LCTRL ))
			{
				OptHidden = !OptHidden;
			}
			else
			{
				TransMode = !TransMode;
				MiniActive = 0;
				Recreate = 1;
			};
			break;
			//WaterCorrection();
			//if(MsPerFrame)MsPerFrame--;
		case SDLK_P:
			//MsPerFrame++;
			//if(GetSDLKeyState(SDL_SCANCODE_LCTRL))RotatePhiI();
			//RotatePhi();
			NeedToPopUp = 2;
			break;
		case SDLK_R:
			if (GetSDLKeyState( SDL_SCANCODE_LCTRL ))
			{
				//ProcessMapOptions();
			}
			else
			{
				MEditMode = false;
				HeightEditMode = false;
				MakeMenu = true;
				MenuType = 31;
			};
			break;
		case SDLK_L:
			/*
			if(!MiniMode)SetMiniMode();
			else ClearMiniMode();
			MEditMode=false;
			*/
			//ReverseLMode();
			break;
		case SDLK_T:
			//HeightEditMode=false;
			//ChoosePosition=true;
			if (GetSDLKeyState( SDL_SCANCODE_LCTRL ))
			{
				ToolsHidden = !ToolsHidden;
			};
			break;
		case SDLK_G:
			//if(GetSDLKeyState(SDL_SCANCODE_LSHIFT))CreateMapShot();
			if (GetSDLKeyState( SDL_SCANCODE_LCTRL ))SelectNextGridMode();
			else SaveScreen();
			//SVSC.Grids=!SVSC.Grids;
			//SVSC.RefreshScreen();
			break;
		case SDLK_KP_PLUS:
			if (HeightEditMode)HiStyle = 1;
			break;
		case SDLK_KP_MINUS:
			if (HeightEditMode)HiStyle = 2;
			break;
		case SDLK_KP_MULTIPLY:
			if (HeightEditMode)HiStyle = 3;
			break;
		case SDLK_KP_DIVIDE:
			if (HeightEditMode)HiStyle = 4;
			break;
		case SDLK_M://NUM 0 // why?
			//if(HeightEditMode)HiStyle=5;
			if (GetSDLKeyState( SDL_SCANCODE_LCTRL ))
			{
				//AddHill();
				GenerateRandomRoad( 5 );
			}
			else FullMini = !FullMini;
			MiniActive = 0;
			Recreate = 1;
			break;
		case SDLK_PAGEUP:
			if (HeightEditMode)HiStyle = 7;
			break;
		case SDLK_PAGEDOWN:
			if (HeightEditMode)HiStyle = 8;
			break;
		case SDLK_HOME:
			if (HeightEditMode)HiStyle = 9;
			break;
		default:
			if (SDLK_0 <= wParam && wParam <= SDLK_9)
			{
				if (GetSDLKeyState( SDL_SCANCODE_LSHIFT ))
				{
					int v = wParam - SDLK_0;
					SHIFT_VAL = SHIFT_VAL * 10 + v;
				}
				else
				{
					if (GetSDLTickCount() - LastCTRLPressTime < kCtrlStickyTime)
					{
						CmdMemSelection( MyNation, wParam - SDLK_0 );
					}
					else CmdRememSelection( MyNation, wParam - SDLK_0 );
					//if(GetSDLKeyState(SDL_SCANCODE_LCTRL))
					//	CmdMemSelection(MyNation,wParam-SDLK_0);
					//else CmdRememSelection(MyNation,wParam-SDLK_0);
				};
			};
		};
	};
};
void SERROR();
void SERROR1();
void SERROR2();

extern int PlayMode;
void StopPlayCD();
void PlayRandomTrack();
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
bool NOPAUSE = 1;
void EnumPopulation();
extern bool TutOver;
void HandleShar( Nation* NT );
void AddRandomBlobs();
void ProcessMapAutosave();

void ProcessGuard();
void DecreaseVeruVPobedu();


extern word NPlayers;
void CmdSaveNetworkGame( byte NI, int ID, char* Name );
extern char SaveFileName[128];
void ProcessNature();


int SUBTIME = 0;
void ProcessScreen();
void GSYSDRAW();
extern int StepX;
extern int StepY;
int TAverage = 50;

void WaitToTime( int Time )
{
	int dt0 = Time - GetSDLTickCount();
	bool DoDraw = dt0 > ( TAverage >> 2 );
	do
	{
		if (DoDraw)
		{
			int T0 = GetSDLTickCount();
			if (T0 - Time < 0)
			{
				int tt = T0;
				ProcessScreen();
				GSYSDRAW();
				int dt = GetSDLTickCount() - tt;
				TAverage = ( TAverage + TAverage + TAverage + dt ) >> 2;
				SUBTIME += GetSDLTickCount() - T0;
			}
		}
		ProcessMessages();
	} while ( Time - GetSDLTickCount() > 0);
	//SUBTIME=0;
}

extern bool PreNoPause;
void StopPlayCD();
void ProcessUpdate();
extern byte CaptState;
extern byte SaveState;
void WritePitchTicks();
void ShowCentralText0( char* sss );


void InitWaves();

extern byte MI_Mode;
extern int RES[8][8];
void PrepareToEdit()
{
	ClearMStack();
	MI_Mode = 1;
	ReliefBrush = 3;
	TerrBrush = 2;
	EditMedia = 0;
	HeightEditMode = false;
	MEditMode = false;
	EditMapMode = true;
	FogMode = 0;
	HelpMode = true;
	ChoosePosition = false;
	CheapMode = false;
	NMyUnits = 1;
	NThemUnits = 1;
	AutoTime = GetSDLTickCount() + 180000;
	ObjTimer.~TimeReq();
	InitWaves();
	PeaceMode = false;
	LockMode = 0;
	SaveFileName[0] = 0;
	LockBars.Clear();
	UnLockBars.Clear();
	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			RES[i][j] = 50000;
		}
	}
}


extern char CurrentMap[64];
extern int TIMECHANGE[8];
extern int AddTime;
extern int NeedAddTime;
extern byte XVIIIState;
extern char RECFILE[128];

void PerformNewUpgrade( Nation* NT, int UIndex, OneObject* OB );

bool IsGameActive();

void PrepareToGame()
{
	if (!PlayGameMode)
	{
		if (NPlayers > 1 && ( IsGameActive() || use_gsc_network_protocol ) && !RecordMode)
		{
			RecordMode = true;
			sprintf( RECFILE, "Autorecord\\%s", CurrentMap );
		}
	}

	RecordMode = true;//BUGFIX: always turn on recording
	if (RecordMode && !PlayGameMode)
	{
		RGAME.StartRecord( CurrentMap );
	}

	MI_Mode = 1;
	memset( TIMECHANGE, 0, sizeof TIMECHANGE );
	AddTime = 0;
	NeedAddTime = 0;

	ClearMStack();

	ReliefBrush = 3;
	TerrBrush = 2;
	EditMedia = 0;
	HeightEditMode = false;
	MEditMode = false;
	EditMapMode = false;
	FogMode = 1;
	HelpMode = false;
	ChoosePosition = false;
	CheapMode = false;
	NMyUnits = 1;
	NThemUnits = 1;

	AutoTime = GetSDLTickCount();
	ObjTimer.~TimeReq();

	InitWaves();

	PeaceMode = false;
	LockMode = 0;
	SaveFileName[0] = 0;

	/*
	//BUGFIX: Do not default to fast mode when starting a game
	FrmDec = 2;
	SpeedSh = 1;
	exFMode = 1;
	*/

	CurrentStepTime = 80;
}

lpCHAR FLIST[4096];
int NFILES = 0;

void CreateRadio();
extern int ScrollSpeed;
void UnLoading();


#ifdef _WIN32
//Delete random generated *.m3d map files
void EraseRND()
{
	char** RNDF = nullptr;
	int NRND = 0;
	int MaxRND = 0;
	DWORD* RndData = nullptr;
	word* Ridx = nullptr;

	WIN32_FIND_DATA FD;
	HANDLE HF = FindFirstFile( "RN? *.m3d", &FD );
	if (HF != INVALID_HANDLE_VALUE)
	{
		bool r = true;
		do
		{
			if (NRND >= MaxRND)
			{
				MaxRND += 300;
				RNDF = (char**) realloc( RNDF, 4 * MaxRND );
				RndData = (DWORD*) realloc( RndData, 2 * MaxRND );
				Ridx = (word*) realloc( Ridx, 2 * MaxRND );
			}
			Ridx[NRND] = NRND;
			RNDF[NRND] = new char[strlen( FD.cFileName ) + 1];
			strcpy( RNDF[NRND], FD.cFileName );
			RndData[NRND] = FD.ftCreationTime.dwHighDateTime;
			NRND++;
			r = FindNextFile( HF, &FD ) != 0;
		} while (r);
		if (NRND > 3)
		{
			SortClass SORT;
			SORT.CheckSize( NRND );
			memcpy( SORT.Parms, RndData, 4 * NRND );
			memcpy( SORT.Uids, Ridx, 2 * NRND );
			SORT.NUids = NRND;
			SORT.Sort();
			memcpy( Ridx, SORT.Uids, 2 * NRND );
			SORT.Copy( Ridx );
			for (int i = 0; i < NRND - 3; i++)
			{
				DeleteFile( RNDF[Ridx[i]] );
			}
		}
		if (NRND)
		{
			for (int i = 0; i < NRND; i++)
			{
				free( RNDF[i] );
			}
			free( RNDF );
			free( Ridx );
			free( RndData );
		}
	}
}
#else 
void EraseRND()
{
	printf( "TODO: EraseRND() not implemented\n" );
}
#endif

void PlayCDTrack( int Id );
void PlayRandomTrack();
extern int PlayMode;

#ifndef NODPLAY
//Create "Cossacks.reg" with Microsoft DirectPlay key
void CreateReg()
{
	char path[300];
	char path1[350];
	GetCurrentDirectory( 300, path );
	int ps1 = 0;
	int ps = 0;
	char c;
	do
	{
		c = path[ps];
		if (c == '\\')
		{
			path1[ps1] = '\\';
			path1[ps1 + 1] = '\\';
			ps1 += 2;
		}
		else
		{
			path1[ps1] = c;
			ps1++;
		};
		ps++;
	} while (c);
	GFILE* f = Gopen( "Cossacks.reg", "w" );
	Gprintf( f, "REGEDIT4\n[HKEY_LOCAL_MACHINE\\Software\\Microsoft\\DirectPlay]\n[HKEY_LOCAL_MACHINE\\Software\\Microsoft\\DirectPlay\\Applications]\n[HKEY_LOCAL_MACHINE\\Software\\Microsoft\\DirectPlay\\Applications\\Cossacks]\n\"CommandLine\"=\"\"\n\"CurrentDirectory\"=\"%s\"\n\"File\"=\"dmcr.exe\"\n\"Guid\"=\"{5BFDB060-06A4-11d0-9C4F-00A0C705475f}\"\n\"Path\"=\"%s\"\n", path1, path1 );
	Gclose( f );
}
#endif

typedef bool tpShowDialog( int NModes, int* Sizex, int* Sizey, int* Current );
tpShowDialog* lpShowDialog;

int ROLL = 1;
void NRFUNC()
{
	ROLL = 0;
}
