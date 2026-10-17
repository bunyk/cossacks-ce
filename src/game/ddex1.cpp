/*==========================================================================
 *
 *  Copyright (C) 1997-1998 Andrew(GSC). All Rights Reserved.
 *
 *  Revamped in 2017 by Эреб
 *
 ***************************************************************************/

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
#include "antibug.h"
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
				MakeMenu=true;
				MenuType=3;
				EnterChatMode = 1;
				ClearKeyStack();
			}
			else KeyPressed = true;
			break;
		case SDLK_J:
			RSCRSizeX++;
			ShowStatistics();
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

int ROLL = 1;
void NRFUNC()
{
	ROLL = 0;
}
