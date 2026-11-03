#include <stdio.h>
#include <boost/coroutine2/all.hpp>
#include <boost/context/fixedsize_stack.hpp>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

// There is no main() function, instead we have
// SDL_AppInit, SDL_AppEvent, SDL_AppIterate and SDL_AppQuit callbacks.
// https://wiki.libsdl.org/SDL3/README-main-functions
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#include "ddini.h"
#include "fastdraw.h"
#include "mgraph.h"
#include "gsound.h"
#include "cdirsnd.h"
#include "recorder.h"
#include "resfile.h"
#include "mapdiscr.h"
#include "walls.h"
#include "nature.h"
#include "megapolis.h"
#include "dialogs.h"
#include "mapsprites.h"
#include "safety.h"
#include "strategyresearch.h"
#include "einfoclass.h"
#include "mouse.h"
#include "gp_draw.h"
#include "dialogs/initfonts.h"
#include "fonts.h"
#include "interface.h"
#include "fog.h"
#include "interface.h"
#include "nucl.h"
#include "menu.h"
#include "topograf.h"
#include "3dmaped.h"
#include "mplayer.h"

//Game version. Must match with other clients
DLLEXPORT word dwVersion = 100;
DLLEXPORT char LobbyVersion[32] = "1.00";
DLLEXPORT char BuildVersion[32] = "V 1.01";

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

int StoneID;
int TreeID;


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


extern bool RUNMAPEDITOR;
extern bool RUNUSERMISSION;
extern char USERMISSPATH[128];
bool window_mode;
bool borderless = false;

bool EditMapMode;
extern bool InGame;
extern bool InEditor;
byte PlayGameMode = 0;
extern word PlayerMenuMode;
extern bool GameExit; // owned by interface.cpp

extern int LastCTRLPressTime; // in interface.cpp

int DrawGroundMode = 0;
int DrawPixMode = 0;
int HeightEditMode;
bool MEditMode;
int WaterEditMode;
extern bool BuildMode; // owned by mapa.cpp WHY? 
extern bool TexPieceMode; // owned by 3drandmap.cpp

//Multiplier, depends on MiniMode
//MiniMode ? 4 : 5
int Shifter;


extern bool GetCoord; // owned by selprop.cpp

int screen_width;
int screen_height;
double screen_ratio;

extern int ModeLX[32];
extern int ModeLY[32];
extern int NModes;

//Last used display resolutions for both modes
int exRealLx, exRealLy;
int ex_other_RealLx, ex_other_RealLy;//Necessary for saving settings
									 //
extern int RealLx;
extern int RealLy;

int MaxSizeX;
int MaxSizeY;
bool PalDone;

SDL_Window* sdlWindow;
extern int CurPalette; // Defined in fog.cpp

bool RetryVideo = 0;

//Game speed mode
//0: Slow mode
//1: Fast mode
int exFMode = 1;

//Game speed mode dependent SpeedShift variable. Used for controlling animations etc
int SpeedSh = 1;

//Minimal delay between two PostDrawGameProcess() returns, in ms
const unsigned int kPostDrawInterval = 16;//~60 Hz (live multiplayer only)

//The original game tick: 80ms per sim tick (12.5 ticks/s) - the step
//multiplayer locks to (CurrentStepTime) and the pace the original game
//ran at. Single player and replays are paced to this (see the frame
//limiter at the end of PostDrawGameProcess).
const unsigned int kSimTickInterval = 80;

//Timespan in ms after last LastCTRLPressTime which allows setting unit control groups
const int kCtrlStickyTime = 50;

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

int xxx;

extern byte SpecCmd; // owned by mapa.cpp

byte EditMedia;
byte LockGrid;
byte LockMode;
byte PauseMode = 0;
byte PlayerMask;
byte Quality;
word Creator;
static word MsPerFrame = 40;

int GLOBALTIME = 0;
int PGLOBALTIME = 0;
int REALTIME;
int PitchTicks = 0;
int MaxPingTime = 0;
int RealPause = 0;
int RealStTime = 0;
int RealGameLength = 0;
int CurrentStepTime = 80;
int NeedCurrentTime = 0;

bool NOPAUSE = 1;

extern int PeaceTimeLeft;
extern int PeaceTimeStage;
void CmdChangePeaceTimeStage( int Stage );

void OnMouseMoveRedraw();

short WheelDelta = 0;


extern char CurrentMap[64];
extern byte XVIIIState;
extern char RECFILE[128];

//Timer Callback
int cadr;
int tima;
int tmtim;

//Main internal counter for intervals
int tmtmt;

int HISPEED = 0;
bool SHOWSLIDE = true;
int AutoTime;

bool ProcessMessages();

extern int mousePointerType; // owned by mouse_x.cpp
extern int curdx;
extern int curdy;
extern bool realLpressed;
extern bool realRpressed;
bool fixed;

extern uint64_t GetSDLTickCount();

boost::coroutines2::coroutine<void>::pull_type* AllGameCoroutine = nullptr;

//fonts
RLCTable RCross;
RLCTable mRCross;

void GameKeyCheck();

int NInStack = 0;
int LastUMX = 0;
int LastUMY = 0;
int LastUTime = 0;

#define MaxQu 32
MouseStack MSTC[MaxQu];
extern bool unpress;


extern int CurPalette;
int SHIFT_VAL = 0;
void HandleMouse( int x, int y );
extern bool PalDone;
SDL_Keycode KeyStack[32];
byte AsciiStack[32];
int NKeys = 0;
// This is used only in InputBox_OnKeyDown()
byte LastAsciiKey = 0;

// Defined here below
void AddMouseEvent( int x, int y, bool L, bool R );
void AddKey( SDL_Keycode Key, byte Ascii );


void LoadEconomy();
void LoadNations();
void LoadWeapon();
void LoadNation( char* fn, byte msk, byte NIndex );
void LoadAllNewMonsters();
void InitNewMonstersSystem();
void LoadWaveAnimations();
extern NewAnimation* Balloon;
void DoGen();
void InitDeathList();
char* GetTextByID( char* ID );
void LoadBorders();

void SetupArrays();
extern byte* RivDir;
void Init_GP_IMG();

void LoadMessages();
void Loadtextures();

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
void LoadNewAimations();
void MFix();
void ProcessFishing();
void ProcessSprites();
void ProcessUFO();
void RenderAllMap();
void Reset3D();

int GetResID( char* );

extern word NPlayers;
void CmdSaveNetworkGame( byte NI, int ID, char* Name );
extern char SaveFileName[128];
void ProcessNature();

//For parallel processable tasks
#define maxTask 32
typedef void EventHandPro( void* );
struct EventsTag
{
	EventHandPro* Pro;
	int	Type;
	int	Handle;
	bool Blocking;
	void* Param;
};
EventsTag Events[maxTask];
int RegisterEventHandler( EventHandPro* pro, int Type, void* param )
{
	int i;
	for (i = 0; Events[i].Pro != nullptr && i < maxTask; i++);
	if (i >= maxTask)
	{
		return -1;
	}

	Events[i].Pro = pro;
	Events[i].Type = Type;
	Events[i].Handle = i;
	Events[i].Blocking = false;
	Events[i].Param = param;
	return i;
}

void CloseEventHandler( int i )
{
	memset( &Events[i], 0, sizeof Events[i] );
}

BOOL doInit();
DLLEXPORT bool KeyPressed;
DLLEXPORT SDL_Keycode LastKey;


//Pick and enter the game data directory. The original game runs from its
//data directory: every resource path is relative to the current directory
//(loose files, .gsc archive discovery, settings and saves). To keep the
//source checkout clean, the data can live in a subdirectory instead:
//pass --gamedata <dir> (or --gamedata=<dir>), or just create ./gamedata
//where you run the game from - it is picked up automatically. We chdir()
//there before any file is opened, so reads, writes and archive discovery
//all follow. Idempotent: only the first call acts.
static void EnterGameDataDir( int argc, char* argv[] )
{
	static bool Done = false;
	if (Done)return;
	Done = true;

	const char* GameDataDir = nullptr;
	for (int i = 1; i < argc; i++)
	{
		if (strcmp( argv[i], "--gamedata" ) == 0 && i + 1 < argc)
		{
			GameDataDir = argv[++i];
		}
		else if (strncmp( argv[i], "--gamedata=", 11 ) == 0)
		{
			GameDataDir = argv[i] + 11;
		}
	}
	if (!GameDataDir && access( "gamedata", F_OK ) == 0)
	{
		GameDataDir = "gamedata";
	}
	if (GameDataDir)
	{
		if (chdir( GameDataDir ) == 0)
		{
			char CWD[1024];
			if (getcwd( CWD, sizeof CWD ))printf( "Game data directory: %s\n", CWD );
		}
		else
		{
			printf( "Could not enter game data directory '%s': %s\n",
				GameDataDir, strerror( errno ) );
		}
	}
}

#ifndef _WIN32
//MissPack MISSLIST (activescenary.cpp) and other global objects open
//resource files from their constructors. Those constructors run during
//static initialization - BEFORE SDL_AppInit, so before its chdir() - and
//with the data files moved out of the current directory they would find
//nothing (worse: FilesInit() would then cache an empty archive list for
//the whole run). So enter the data directory even earlier, from a
//prioritized constructor: it runs before the unprioritized static
//constructors of the other translation units. argc/argv do not exist yet
//at this point, so read the command line from /proc/self/cmdline
//(arguments are NUL-separated there).
__attribute__( ( constructor( 101 ) ) )
static void EnterGameDataDirEarly()
{
	char CmdLine[4096];
	FILE* ProcFile = fopen( "/proc/self/cmdline", "rb" );
	if (!ProcFile)return;
	size_t N = fread( CmdLine, 1, sizeof CmdLine - 1, ProcFile );
	fclose( ProcFile );
	if (!N)return;
	CmdLine[N] = 0;

	char* Args[256];
	int Argc = 0;
	char* p = CmdLine;
	char* End = CmdLine + N;
	while (p < End && Argc < 255)
	{
		Args[Argc++] = p;
		while (p < End && *p)p++;
		p++;//skip the NUL separator
	}
	Args[Argc] = nullptr;

	EnterGameDataDir( Argc, Args );
}
#endif //_WIN32

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv)
{
	//Game data directory: normally already entered by the early static
	//constructor above; this is a no-op then. Kept here for the case where
	//the early lookup could not run (no /proc).
	EnterGameDataDir( argc, argv );

	for (int i = 0; i < argc; i++)
	{
		if (strcmp(argv[i], "/MAPEDITOR") == 0)
		{
			RUNMAPEDITOR = 1;
			strcpy(USERMISSPATH, argv[i+1]);
		}
		else if (strcmp(argv[i], "/MISSION") == 0)
		{
			RUNUSERMISSION = 1;
			strcpy(USERMISSPATH, argv[i+1]);
		}

		if (strcmp(argv[i], "/window") == 0)
		{
			window_mode = true;
		}
		else
		{
			window_mode = false;
		}

		if (strcmp(argv[i], "/borderless") == 0)
		{
			window_mode = true;
			borderless = true;
		}
	}

	InitSDL();

	//Init DirectDraw and find possible resolutions
	EnumModesOnly();

	//Scan the data directory for .gsc archives (CGSCset::gOpen)
	if (!FilesInit())
	{
		FilesExit();
		SDL_Event e;
		e.type = SDL_EVENT_QUIT;
		e.quit.timestamp = SDL_GetTicks();
		SDL_PushEvent(&e);
	}
	printf("Files initialized.\n");

	void EraseRND();
	EraseRND();

	//Pointer to the DirectDraw screen buffer
	ScreenPtr = nullptr;

	// Declarations for globals that live in guarded decl blocks elsewhere
	// (ChangeNation/MultiTvar - linux_stubs.cpp/loadsave.cpp, sfVersion - interface.cpp,
	// CreateRadio - topograf.cpp)
	extern bool ChangeNation;
	extern bool MultiTvar;
	extern int sfVersion;
	void CreateRadio();

	ChangeNation = false;
	MultiTvar = false;
	MEditMode = false;
	WaterEditMode = false;

	Shifter = 5;
	Multip = 0;
	AutoTime = 0;
	BlobMode = 0;
	CostThickness = 4;
	EditMedia = 0;
	CreateRadio();
	SpecCmd = 0;
	sfVersion = 285;
	Quality = 2;

	RealLx = 1024;
	RealLy = 768;
	exRealLx = 1024;
	exRealLy = 768;

	// Declarations for globals that live in files not included here
	// (previously only visible to the _WIN32-only settings block).
	extern int WarSound;
	extern int WorkSound;
	extern int OrderSound;
	extern int MidiSound;
	extern int ScrollSpeed;
	extern int PlayMode;
	extern char RECFILE[128];
	extern bool RecordMode;
	void InitObjs3();

	// Zero the volumes like the original (the mode.dat load below then
	// restores the saved values; on a very first run they stay 0 until
	// the user moves a slider - same behavior as Windows).
	WarSound = 0;
	WorkSound = 0;
	OrderSound = 0;
	MidiSound = 0;


	//Zero 3D Bars variables (?)
	InitObjs3();

	// Load settings. Portable (Gopen); was _WIN32-only, so volumes,
	// resolution, speed mode and the music preference never persisted
	// across sessions on Linux.
	GFILE* fff = Gopen("mode.dat", "rt");
	ScrollSpeed = 5;
	if (fff)
	{
		//Distinguish between last window adn fullscreen resolutions
		int ex_window_x, ex_window_y, ex_x, ex_y;
		int dummy;
		//7th value was FPSTime
		Gscanf(fff, "%d%d%d%d%d%d%d%d%d%d%d%d",
			&ex_window_x, &ex_window_y, &ex_x, &ex_y,
			&WarSound, &OrderSound, &OrderSound, &MidiSound,
			&dummy, &ScrollSpeed, &exFMode, &PlayMode);
		Gclose(fff);

		//Set last 'global resolution' according to current mode
		if (window_mode)
		{
			exRealLx = ex_window_x;
			exRealLy = ex_window_y;
			ex_other_RealLx = ex_x;
			ex_other_RealLy = ex_y;
		}
		else
		{
			exRealLx = ex_x;
			exRealLy = ex_y;
			ex_other_RealLx = ex_window_x;
			ex_other_RealLy = ex_window_y;
		}
	}
	GFILE* rec_settings_file = Gopen("rec.dat", "rt");
	if (rec_settings_file)
	{
		Gscanf(rec_settings_file, "%d%s", &RecordMode, &RECFILE);
		Gclose(rec_settings_file);
	}

	// Apply the saved music volume to the device: the options dialogs
	// initialize their music slider from GetCDVolume() (the DEVICE volume),
	// so without this the slider would show the device default (100)
	// instead of the persisted MidiSound on first open.
	void SetCDVolume( int Vol );
	SetCDVolume( MidiSound );

	//Look if loaded values match possible screen resolutions
	bool ExMode = 0;
	for (int i = 0; i < NModes; i++)
	{
		if (ModeLX[i] == exRealLx && ModeLY[i] == exRealLy)
		{
			ExMode = 1;
		}
	}

	if (!ExMode)
	{//Loaded resolution not possible, reset do default
		exRealLx = 1024;
		exRealLy = 768;
	}

	//Save native display resolution for future use
	SDL_DisplayID displayId = SDL_GetPrimaryDisplay();
	SDL_Rect displayRect;
	SDL_GetDisplayBounds(displayId, &displayRect);
	screen_width = displayRect.w;
	screen_height = displayRect.h;

	//Calculate native resolution aspect ratio
	double scale = 0.01;
	screen_ratio = (double)screen_width / screen_height;
	screen_ratio = (int)(screen_ratio / scale) * scale;
	printf("Screen resolution: %dx%d, ratio: %.2f\n", screen_width, screen_height, screen_ratio);

	WindX = 0;
	WindY = 0;
	WindX1 = 1023;
	WindY1 = 767;
	WindLx = 1024;
	WindLy = 768;

	// Declarations for globals/functions defined in other files, previously
	// visible here only through Windows-only decl blocks
	extern bool HealthMode;
	extern bool InfoMode;
	extern bool DeathMode;
	extern bool AttackMode;
	extern bool TransMode;
	extern bool MUSTDRAW;
	void InitFishMap();
	void SetupGates();
	void makeFden();
	void InitXShift();

	tima = 0;
	PlayerMask = 1;
	Flips = 0;
	tmtim = 0;

	HealthMode = false;
	InfoMode = true;
	DeathMode = false;
	AttackMode = false;

	//Zero FishMap pointer
	InitFishMap();

	//Init Gates[32] array
	SetupGates();

	LockGrid = false;

#ifdef _WIN32
	FILE* Fx = fopen("cew.dll", "r");
	if (!Fx)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error...", "CEW.DLL not found. Unable to run Cossacks.", nullptr);
		return SDL_APP_FAILURE;
	}
	else
	{
		fclose(Fx);
	}
#endif // _WIN32 (cew.dll protection check)

	//Init buffers for national units?
	SetupNatList();

	//Something about fog?
	makeFden();

	PlayerMenuMode = 1;

	Creator = 4096 + 255;
	xxx = 0;
	cadr = 0;

	//MouseZones?
	InitZones();

	//Order execution buffer position = 0?
	InitEBuf();

	TransMode = false;
	MUSTDRAW = false;

	//Calculate XShift for... mirroring and water and stuff?
	InitXShift();

	//Read players.txt, load dialog resources
	SFLB_InitDialogs();

	//Water colors and buffers (realwater.cpp ported)
	extern void InitWater();
	InitWater();

#ifndef _WIN32
	SFLB_InitDialogs();
#endif

	//Load fonts(?)
	LoadRLC("xrcross.rlc", &RCross);

	memset(Events, 0, sizeof Events);

	//Register winapi window class, init DirectDraw, sounds and cursor
	if (!doInit())
	{
		return SDL_APP_FAILURE;
	}

	//Load specific palette and fog resources (alphas etc)
	LoadFog(2);
	LoadPalette("2\\agew_1.pal");

	// Music autostart (PlayMode defaults to 2 = random music, ddex1.cpp).
	// On Windows this call sits in the _WIN32 block below; nothing ever
	// started music on Linux unless the user toggled the options combo.
	extern int PlayMode;
	bool PlayRandomTrack();
	if (PlayMode)
	{
		PlayRandomTrack();
	}

	#ifdef _WIN32
	//Init DirectPlay and DPInfo structure
	SetupMultiplayer();

	//Init variables
	InitMultiDialogs();

	//UI color masking?
	SetupHint();

	//Main internal counter for intervals
	tmtmt = 0;

	REALTIME = 0;
	KeyPressed = false;

	OnMouseMoveRedraw();

	if (PlayMode)
	{
		PlayRandomTrack();
	}

	StartInternetExplorer();

	#endif // _WIN32

	// FIXME: need to be called only when focused on textboxes
	// Fine on desktop, but not on mobile
	SDL_StartTextInput(sdlWindow);

	AllGameCoroutine = new boost::coroutines2::coroutine<void>::pull_type(
		boost::context::fixedsize_stack(8 * 1024 * 1024), AllGame);

	return SDL_APP_CONTINUE;
}

// Mouse grab confines the mouse cursor to the window, when InGame or InEditor is true.
void ClipCursorToWindowArea()
{
	if (!window_mode)
	{//Just in case
		return;
	}

	SDL_SetWindowMouseGrab(sdlWindow, InGame || InEditor);
}

extern bool ScanPressed[SDL_SCANCODE_KP_0 + 1];

// Resizable-window support: current window output size (window coordinates)
// and the scale from window space to the game's logical RealLx x RealLy
// canvas. Updated on resize; mouse events are converted so clicks land
// where the (stretched) picture shows them.
// Letterbox rect shared with the renderer (defined in ddini.cpp, computed
// by ComputeLetterbox(): the RealLx x RealLy canvas fitted into the window
// with the aspect preserved). Mouse events map through it.
struct SDL_Rect;
extern "C" {
extern SDL_Rect g_letterbox;
extern "C" void ComputeLetterbox();
}
static int g_wndW = 0, g_wndH = 0;
static void UpdateWindowSize()
{
	if (sdlWindow)
	{
		SDL_GetWindowSize(sdlWindow, &g_wndW, &g_wndH);
		ComputeLetterbox();
	}
}
static void ScaleMouseToLogical(int* x, int* y)
{
	if (g_wndW > 0 && g_wndH > 0 && (g_wndW != RealLx || g_wndH != RealLy))
	{
		ComputeLetterbox();
		if (g_letterbox.w > 0 && g_letterbox.h > 0)
		{
			*x = ( ( *x - g_letterbox.x ) * RealLx ) / g_letterbox.w;
			*y = ( ( *y - g_letterbox.y ) * RealLy ) / g_letterbox.h;
			if (*x < 0) *x = 0;
			if (*y < 0) *y = 0;
			if (*x >= RealLx) *x = RealLx - 1;
			if (*y >= RealLy) *y = RealLy - 1;
		}
	}
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
	static BYTE phase = 0;

	switch (event->type)
	{
		// TODO: no idea where this is posted to the message queue
		//case 0xABCD:
		//{
		//	GFILE* F = Gopen("UserMissions\\start.dat", "r");
		//	if (F)
		//	{
		//		ReadWinString(F, USERMISSPATH, 120);
		//		Gclose(F);
		//		if (lParam == 1)
		//		{
		//			RUNMAPEDITOR = 1;
		//		}
		//		if (lParam == 0)
		//		{
		//			RUNUSERMISSION = 1;
		//		}
		//	}
		//}
		//break;

	case SDL_EVENT_MOUSE_WHEEL:
		printf("SDL_EVENT_MOUSE_WHEEL: x=%d, y=%d\n", event->wheel.x, event->wheel.y);
		// TODO: value might need some tweaking
		WheelDelta = static_cast<short>(round(event->wheel.y));
		break;

	case SDL_EVENT_MOUSE_BUTTON_DOWN:
	{
		int bx = (int)event->button.x, by = (int)event->button.y;
		ScaleMouseToLogical(&bx, &by);
		SDL_MouseButtonFlags mouseFlags = SDL_GetMouseState(nullptr, nullptr);
		if (event->button.button == SDL_BUTTON_LEFT)
		{
			mouseFlags |= SDL_BUTTON_LMASK;
			Lpressed = true;
			realLpressed = true;
			fixed = false;
			SetMPtr(bx, by, mouseFlags);
			AddMouseEvent(mouseX, mouseY, Lpressed, Rpressed);
		}
		else if (event->button.button == SDL_BUTTON_RIGHT)
		{
			mouseFlags |= SDL_BUTTON_RMASK;
			Rpressed = true;
			realRpressed = true;
			fixed = false;
			if (ScreenPtr)
			{
				SetMPtr(bx, by, mouseFlags);
			}
			AddMouseEvent(mouseX, mouseY, Lpressed, Rpressed);
		}
		break;
	}

	case SDL_EVENT_MOUSE_BUTTON_UP:
	{
		int bx = (int)event->button.x, by = (int)event->button.y;
		ScaleMouseToLogical(&bx, &by);
		SDL_MouseButtonFlags mouseFlags = SDL_GetMouseState(nullptr, nullptr);
		if (event->button.button == SDL_BUTTON_LEFT)
		{
			mouseFlags &= ~SDL_BUTTON_LMASK;
			Lpressed = false;
			realLpressed = false;

			SetMPtr(bx, by, mouseFlags);

			AddMouseEvent(mouseX, mouseY, Lpressed, Rpressed);

			//Double click
			if (!BuildMode//BUGFIX: Prevent unit selection while placing buildings
				&& (abs(mouseX - LastUMX) + abs(mouseY - LastUMY)) < 16
				&& GetSDLTickCount() - LastUTime < 600)
			{
				//Select all units of selected type on screen
				SpecCmd = 241;
			}

			LastUMX = mouseX;
			LastUMY = mouseY;
			LastUTime = GetSDLTickCount();
		}
		else if (event->button.button == SDL_BUTTON_RIGHT)
		{
			//mouseFlags &= ~SDL_BUTTON_LMASK;
			mouseFlags &= !SDL_BUTTON_RMASK;
			Rpressed = false;
			realRpressed = false;
			if (ScreenPtr)
			{
				SetMPtr(bx, by, mouseFlags);
			}
			AddMouseEvent(mouseX, mouseY, Lpressed, Rpressed);
		}
		break;
	}

	case SDL_EVENT_MOUSE_MOTION:
	{
		int mx = (int)event->motion.x, my = (int)event->motion.y;
		ScaleMouseToLogical(&mx, &my);
		if (ScreenPtr)
		{
			if (mx != mouseX || my != mouseY)
			{
				SDL_MouseButtonFlags mouseFlags = SDL_GetMouseState(nullptr, nullptr);
				SetMPtr(mx, my, mouseFlags);
				OnMouseMoveRedraw();
			}
		}
		break;
	}

	case SDL_EVENT_WINDOW_MOVED:
	case SDL_EVENT_WINDOW_RESIZED:
		//Adjust cursor zone after window was moved
		UpdateWindowSize();
		ClipCursorToWindowArea();
		break;

	case SDL_EVENT_WINDOW_RESTORED:
		//Restore cursor zone after window was minimized
		ClipCursorToWindowArea();
		break;

	case SDL_EVENT_WINDOW_FOCUS_GAINED:
		//Restore cursor zone after alt-tab
		ClipCursorToWindowArea();

		// Alternative to WM_ACTIVATEAPP
		bActive = true;
		if (primarySurface)
		{
			CreateDDObjects(sdlWindow);
			LockSurface();
			UnlockSurface();
			LoadFog(CurPalette);
			char cc[64];
			sprintf(cc, "%d\\agew_1.pal", CurPalette);
			PalDone = 0;
			LoadPalette(cc);
		}
		break;

	// No SDL equivalent
	//case WM_SETCURSOR:
	//	SetCursor(NULL);
	//	return TRUE;

	case SDL_EVENT_KEY_DOWN:
	{
		printf("SDL_EVENT_KEY_DOWN: key=%d, scancode=%d\n", event->key.key, event->key.scancode);
		SDL_Keycode keycode = event->key.key;
		SDL_Scancode scancode = event->key.scancode;

		// TODO: This is fine, but we don't need all keys, only the ones from ScanKeys
		if (scancode < SDL_SCANCODE_KP_0 + 1)
		{
			ScanPressed[scancode] = true;
		}

		LastKey = keycode;
		KeyPressed = true;

		if (LastKey == SDLK_F11)
		{
			#ifdef _WIN32
			SaveScreen();
			#endif
		}

		/*
		//Can't see where it was supposed to work. Cut it out.
		if (( !GameInProgress ) && LastKey == SDLK_R &&
			(GetSDLKeyState( SDL_SCANCODE_LCTRL )))
		{
			//RecordMode = !RecordMode;//BUGFIX: remove switching record mode in real time
		}
		*/

		{
			LastAsciiKey = 0;
			AddKey(keycode, LastAsciiKey);
		}
		break;
	}
	case SDL_EVENT_TEXT_INPUT:
	{
		const char* text = event->text.text;

		Uint32 decoded = SDL_StepUTF8(&text, nullptr);

		// FIXME: this will work fine on desktop but will need to be adjusted for mobile
		// Since mobile yields the whole string at a time (TODO: check if this is true)

		//UTF code is in cyrillic range
		//Adjust ascii code to match sprite index in mainfont.gp file
		//Sprites 192 to 255 ('А' to 'я')
		//(taken from russian cossacks version ALL.GSC)
		if (1040 <= decoded && decoded <= 1103)
		{
			decoded = decoded - 848;
		}

		if (decoded < 256)
		{
			LastAsciiKey = (char)decoded;
			// Overwrite the latest key event with the ascii code
			// Here we assume that SDL_EVENT_KEY_DOWN was already handled
			AsciiStack[NKeys - 1] = LastAsciiKey;
		}

		break;
	}

#ifdef _WIN32
	case SDL_EVENT_QUIT:
		//Leave game and assign defeat
		IAmLeft();
		LOOSEANDEXITFAST();
		break;
#else
	case SDL_EVENT_QUIT:
		//Leave game and assign defeat (Linux)
		GameExit = true;
		return SDL_APP_SUCCESS;
#endif
	}

	return SDL_APP_CONTINUE;
}


bool GetSDLKeyState(SDL_Scancode scancode, bool leftright = true)
{
	int numkeys;
	const bool* keyboardState = SDL_GetKeyboardState(&numkeys);
	if (scancode >= numkeys)
	{
		return false;
	}
	if (leftright)
	{
		switch (scancode)
		{
		case SDL_SCANCODE_LSHIFT:
		case SDL_SCANCODE_RSHIFT:
			return keyboardState[SDL_SCANCODE_LSHIFT] || keyboardState[SDL_SCANCODE_RSHIFT];
		case SDL_SCANCODE_LCTRL:
		case SDL_SCANCODE_RCTRL:
			return keyboardState[SDL_SCANCODE_LCTRL] || keyboardState[SDL_SCANCODE_RCTRL];
		case SDL_SCANCODE_LALT:
		case SDL_SCANCODE_RALT:
			return keyboardState[SDL_SCANCODE_LALT] || keyboardState[SDL_SCANCODE_RALT];
		}
	}
	return keyboardState[scancode];
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
	// Using coroutines would be perfect for this code,
	// since each menu is a separate function with render loop.

	if (!*AllGameCoroutine)
	{
		printf("AllGameCoroutine is empty, quitting...\n");
		return SDL_APP_SUCCESS;
	}
	(*AllGameCoroutine)();
	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
	printf("SDL_AppQuit: %d\n", result);
	SDL_StopTextInput(sdlWindow);

	delete AllGameCoroutine;
	AllGameCoroutine = nullptr;

	#ifdef _WIN32
	ClearScreen();
	//Zero variables and pointers
	UnLoading();
	CloseExplosions();
	ShutdownMultiplayer(1);
	#endif // _WIN32 (cleanup: network/shutdown specifics not ported)

	//Save settings before closing (portable, see the load in SDL_AppInit)
	extern int WarSound;
	extern int OrderSound;
	extern int MidiSound;
	extern int ScrollSpeed;
	extern int PlayMode;
	extern char RECFILE[128];
	extern bool RecordMode;
	void StopPlayCD();
	//Distinguish between last window adn fullscreen resolutions
	int ex_window_x, ex_window_y, ex_x, ex_y;

	//Set last 'global resolution' according to current mode
	if (window_mode)
	{
		ex_window_x = exRealLx;
		ex_window_y = exRealLy;
		ex_x = ex_other_RealLx;
		ex_y = ex_other_RealLy;
	}
	else
	{
		ex_x = exRealLx;
		ex_y = exRealLy;
		ex_window_x = ex_other_RealLx;
		ex_window_y = ex_other_RealLy;
	}

	//Save settings before closing
	GFILE* fff = Gopen("mode.dat", "wt");
	if (fff)
	{
		//7th value was FPSTime
		Gprintf(fff, "%d %d %d %d %d %d %d %d %d %d %d %d",
			ex_window_x, ex_window_y, ex_x, ex_y,
			WarSound, OrderSound, OrderSound,
			MidiSound, 0, ScrollSpeed, exFMode, PlayMode);
		Gclose(fff);
	}
	GFILE* rec_settings_file = Gopen("rec.dat", "wt");
	if (rec_settings_file)
	{
		Gprintf(rec_settings_file, "%d %s", RecordMode, RECFILE);
		Gclose(rec_settings_file);
	}

	#ifdef _WIN32
	FilesExit();
	FinishInternetExplorer();
	#endif // _WIN32 (FilesExit is called below unguarded too? keep parity)

	StopPlayCD();
	FreeDDObjects();
	// PostQuitMessage(0); TODO: find cross-platform equivalent
	exit(0);
}


bool InitScreen()
{
	PalDone = false;
	CreateDDObjects( sdlWindow );
	PalDone = false;
	LoadPalette( "agew_1.pal" );
	if (!SDLError)
	{
		LockSurface();

		UnlockSurface();

		if (!RealScreenPtr)
		{
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Loading error[1]", "Unable to initialise SDL. It is possible that hardware acceleration is turned off.", sdlWindow);
			exit( 0 );
		}

		return true;
	}
	else
	{
		PlayEffect( 0, 0, 0 );
	}
	return false;
}



void ResizeAndCenterWindow()
{
	if (!window_mode)
	{//Just in case
		return;
	}

	// The window is user-resizable (SDL_WINDOW_RESIZABLE) and the logical
	// canvas is stretched/letterboxed into it by FlipPages - so the window
	// SIZE is no longer tied to the logical resolution here. This only
	// centers the current window on the screen. (Startup creates the
	// window at RealLx x RealLy in doInit.)
	int width = 0, height = 0;
	SDL_GetWindowSize( sdlWindow, &width, &height );
	if (width <= 0) width = RealLx;
	if (height <= 0) height = RealLy;

	int x = screen_width / 2 - width / 2;
	int y = screen_height / 2 - height / 2;

	if (x < 0)
	{
		x = 0;
	}
	if (y < 0)
	{
		y = 0;
	}

	SDL_SetWindowPosition( sdlWindow, x, y );

	ClipCursorToWindowArea();

	SDL_WarpMouseInWindow( sdlWindow, width / 2, height / 2 );
}

// Declarations for globals/functions previously visible here only through
// Windows-only decl blocks (definitions: ddex1.cpp, multi.cpp, recorder.cpp, interface.cpp)
extern bool NOPAUSE;
extern bool PreNoPause;
extern word NPlayers;
extern int WasInGold[8], WasInCoal[8], WasInIron[8];
extern int NInGold[8], NInCoal[8], NInIron[8];
extern City CITY[8];
extern bool TutOver;
extern TimeReq ObjTimer;
extern CDirSound* CDS;
void ReadPichTicks();
void WritePitchTicks();
void ProcessMapAutosave();
void AddRandomBlobs();
void CmdSetSpeed( byte );
void EnumPopulation();
void ProcessCostPoints();
void ProcessGuard();
void ProcessEconomy();
void ProcessDeathList();
void HealWalls();
void InitDestn();
void GameKeyCheck();
void CreateStrategyInfo();
void ProcessFishing();
void ControlGates();
void HandleMines();
void ResearchCurrentIsland( byte Nat );
void ResearchBestPortToFish( byte Nat );
void WinnerControl( bool GenerateStat );
void ProcessSprites();
void HandleTransport();
void HandleMission();
void ProcessNewMonsters();
void ProcessExpl();
void HandleBalloon( Nation* NT );
void ProcessDynamicalTopology();
void InitXShift();
void WaitToTime( int t );
void ProcessNature();

void PostDrawGameProcess()
{
	// Some networking logic
	RGAME.TryToFlushNetworkStream( 0 );
	if (PlayGameMode == 0 && NPlayers < 2)
	{
		PitchTicks = 0;
	}

	if (PlayGameMode == 0)
	{
		if (MaxPingTime)
		{
			WaitToTime( NeedCurrentTime );
		}
		else
		{
			//Live multiplayer is paced through WaitToTime/NeedCurrentTime;
			//single player keeps the original free-run loop and replays
			//are paced by the frame limiter (kSimTickInterval).
			PitchTicks = 0;
		}
	}

	if (PlayGameMode)
	{
		ReadPichTicks();
		if (PitchTicks)
		{
			MaxPingTime = 1;
		}
		else
		{
			MaxPingTime = 0;
		}
	}
	else
	{
		WritePitchTicks();
	}
	ProcessNature();

	GLOBALTIME++;

	if (RealStTime == 0)
	{
		RealGameLength = 0;
	}

	NeedCurrentTime += CurrentStepTime;

	if (GLOBALTIME - PGLOBALTIME > PitchTicks)
	{
		//The step decay is multiplayer lag recovery (paired with
		//DoItSlow's regrow in multi.cpp). Nothing regrows it in single
		//player or in replays, where it would erode the 80ms pace within
		//minutes - so only decay in live multiplayer games.
		if ( MaxPingTime && !PlayGameMode )CurrentStepTime -= CurrentStepTime >> 5;
		RealGameLength = GetSDLTickCount() - RealStTime;

#ifdef _WIN32
		HandleMultiplayer();
#endif // _WIN32 (mplayer.cpp not ported)

		SYN.Copy( &SYN1 );

		PreNoPause = 0;
		ExecuteBuffer();

		if (PreNoPause)
		{
			NOPAUSE = 0;
		}

		PGLOBALTIME = GLOBALTIME;
		RealStTime = GetSDLTickCount();

		if (PlayGameMode)
		{

			ReadPichTicks();
			if (PitchTicks)
			{
				MaxPingTime = 1;
			}
		}
		else
		{
			if (NPlayers > 1 && MaxPingTime)
			{
				if (CurrentStepTime)
				{
					PitchTicks = 4 + ( ( MaxPingTime ) / CurrentStepTime );
				}
				else
				{
					PitchTicks = 0;
				}
			}
			else
			{
				PitchTicks = 0;
			}
			WritePitchTicks();
		}
	}

	if (!HISPEED)
	{
		SHOWSLIDE = true;
	}
	else
	{
		SHOWSLIDE = !div( tmtmt, HISPEED + 1 ).rem;
	}

	int difTime = GetSDLTickCount() - AutoTime;

	#ifdef _WIN32
	ProcessUpdate(); // Some multiplayer updates

	// Autosave logic
	int MaxDT = 60000;

	switch (SaveState)
	{
	case 1:
		MaxDT = 60000 * 2;
		break;
	case 2:
		MaxDT = 60000 * 4;
		break;
	case 3:
		MaxDT = 60000 * 6;
		break;
	case 4:
		MaxDT = 60000 * 8;
		break;
	case 5:
		MaxDT = 60000 * 10;
		break;
	case 6:
		MaxDT = 60000 * 2000;
		break;
	}

	if (difTime > MaxDT && !( PlayGameMode || SaveState == 6 ))
	{
		if (NPlayers > 1)
		{
			for (int i = 0; i < NPlayers; i++)
			{
				if (EBufs[i].Enabled)
				{
					if (PINFO[i].PlayerID == MyDPID)
					{
						int NP = 0;
						for (int j = 0; j < NPlayers; j++)
						{
							if (EBufs[j].Enabled)
							{
								NP++;
							}
						}
						char cc1[128];
						sprintf( cc1, "NetAutoSave %d players", NP );
						CmdSaveNetworkGame( MyNation, 0, cc1 );
					}
					i = 100;
				}
			}
			//SaveGame("AUTO.sav",SaveFileName,0);
		}
		else
		{
			if (!EditMapMode)
			{
				if (NATIONS[MyNation].VictState != 1 && !SCENINF.LooseGame)
				{
					ShowCentralText0( GetTextByID( "Autosaving" ) );
					FlipPages();
					SaveGame( "AUTO.sav", "auto.sav", 0 );
				}
			}
		}
		AutoTime = GetSDLTickCount();
	}
	#endif // _WIN32

	static int PrevCheckTime = 0;
	if (!PrevCheckTime)
	{
		PrevCheckTime = GetSDLTickCount();
	}

	if (GetSDLTickCount() - PrevCheckTime > 90000)
	{
		PrevCheckTime = GetSDLTickCount();
		if (PeaceTimeLeft / 60 < PeaceTimeStage)
		{
			CmdChangePeaceTimeStage( PeaceTimeLeft / 60 );
		}
	}

	/* Multiplayer save functions
	if(NPlayers > 1 && MyDPID == ServerDPID && SaveTime - GetSDLTickCount() > 60000*5)
	{
		CmdSaveNetworkGame(MyNation, GetSDLTickCount(), "NETWORK SAVE");
		SaveTime = GetSDLTickCount();
	}
	*/

	//Time of the last PostDrawGameProcess() return
	static unsigned long prev_postdraw_time = 0;
	if (0 == prev_postdraw_time)
	{
		prev_postdraw_time = GetSDLTickCount();
	}

	//Tick pacing: replays (PlayGameMode!=0) are paced here to the
	//original 80ms sim tick (kSimTickInterval, 12.5 ticks/s) - the pace
	//the recording was made at (recordings carry no timing of their own:
	//single-player streams store all-zero PitchTicks bytes, so an
	//original recording replayed at the free-run loop rate zips through
	//several times too fast). Waiting here keeps the render path
	//untouched - one render per tick, no render-inside-wait judder.
	//Live multiplayer is paced by WaitToTime above and single player
	//keeps the free-run loop (the fast feel this port has always had);
	//both get only the render cap.
	//KNOWN BUG (docs/TODO.txt, "Ticks tick too fast in scenarios"): at
	//the free-run rate the sim runs ~5x the 2001 pace (60+ ticks/s vs
	//12.5), so scenario timers (8<<SpeedSh per mission tick, HandleMission
	//on every 8th sim tick) expire ~5x too fast - Single2's escape window
	//was ~2.5s instead of ~13s. Pacing single player to 80ms here fixes
	//the timers but drops the render to 12.5fps (one frame per sim tick),
	//which is unbearable - reverted. The real fix needs the render rate
	//decoupled from the sim tick (original: 12.5 ticks/s sim, smooth
	//scrolling via DirectDraw overlays), and/or tying into the game speed
	//slider (exFMode/SpeedSh).
	unsigned int tick_interval = PlayGameMode ? kSimTickInterval : kPostDrawInterval;
	unsigned long time_since_last_call = 0;
	do
	{
		ProcessMessages();
		if (PauseMode)
		{
			GameKeyCheck();
		}
		time_since_last_call = GetSDLTickCount() - prev_postdraw_time;
	} while (PauseMode || time_since_last_call < tick_interval);

	prev_postdraw_time = GetSDLTickCount();
}

void PreDrawGameProcess()
{
	//Autosave in map editor every 5 min
	ProcessMapAutosave();

	//Something about ship traces?
	AddRandomBlobs();

	//DirectX related sound procedures
	CDS->ProcessSoundSystem();

	if (NOPAUSE)
	{
		//Refresh "peasants in mines" amounts
		for (int w = 0; w < 8; w++)
		{
			WasInGold[w] = NInGold[w];
			WasInCoal[w] = NInCoal[w];
			WasInIron[w] = NInIron[w];
			NInGold[w] = 0;
			NInCoal[w] = 0;
			NInIron[w] = 0;
		}
	}

	for (int g = 0; g < 8; g++)
	{
		if (CITY[g].Account < 0)
		{
			CITY[g].Account = 0;
		}
	}

	//Check if fast/slow mode was changed
	if (exFMode != SpeedSh)
	{
		CmdSetSpeed( exFMode );
	}

	//Calculate population values for all players
	if (( tmtmt % 256 ) == 32)
	{
		EnumPopulation();
	}

	//???
	ProcessCostPoints();

	//Auto-attack or guard logic?
	ProcessGuard();

	if (NOPAUSE)
	{
		//Refresh market exchange rates
		ProcessEconomy();

		//Take a guess...
		ProcessDeathList();
	}

	int tt = tmtmt % 256;

	//Remove dead wall cells
	HealWalls();

	if (LastAttackDelay)
	{
		LastAttackDelay--;
	}

	for (int i = 0; i < 8; i++)
	{
		memset( NATIONS[i].SoundMask, 0, 2048 );
	}

	//NDestn = 0
	InitDestn();

	//Many diffirent key checks for various game modes
	GameKeyCheck();

	//Take a guess...
	ProcessMessages();

	if (2 == tmtmt % 41)
	{
		//Crawl the map and enumerate objects
		CreateStrategyInfo();
	}

	//Take a guess...
	ProcessFishing();

	//Open and close gates, check for squashed units
	ControlGates();

	//Order the acquisition of resources from mines
	HandleMines();

	//Calculate XShift for... mirroring and water and stuff?
	InitXShift();

	//Long live compiler optimization!
	int tmtmt_div_256 = tmtmt / 256;
	int tmtmt_mod_256 = tmtmt % 256;
	if (1 == tmtmt_mod_256)
	{
		//Each player is processed every 256 * 8 internal tick
		int nation_byte = tmtmt_div_256 % 8;
		//Some island AI logic
		ResearchCurrentIsland( nation_byte );
	}

	int tmtmt_div_128 = tmtmt / 128;
	int tmtmt_mod_128 = tmtmt % 128;
	if (7 == tmtmt_mod_128)
	{
		//Each player is processed every 256 * 8 internal tick
		int nation_byte = tmtmt_div_128 % 8;

		//Take a guess...
		ResearchBestPortToFish( nation_byte );
	}

	if (SHOWSLIDE)
	{
		//Count peasants and city centers?
		WinnerControl( false );
	}

	if (tima != time( nullptr ))
	{
		Flips = tmtim;
		tmtim = 0;
		tima = time( nullptr );
	}

	if (0 == tmtmt % 64)
	{
		LASTRAND = rando();
		LASTIND = rpos;
	}

	if (NOPAUSE)
	{
		tmtim++;

		tmtmt++;

		REALTIME += FrmDec;

		for (int g = 0; g < 8; g++)
		{
			if (CITY[g].AutoEraseTime)
			{
				CITY[g].AutoEraseTime--;
				if (!CITY[g].AutoEraseTime)
				{
					int SCORES[8];
					for (int i = 0; i < 8; i++)
					{
						SCORES[i] = CITY[i].Account;
					}

					for (int i = 0; i < MAXOBJECT; i++)
					{
						OneObject* OB = Group[i];
						if (OB && ( !OB->Sdoxlo ) && ( OB->NNUM == g ))
						{
							//erasing
							OB->delay = 6000;

							if (OB->LockType)
							{
								OB->RealDir = 32;
							}

							OB->Die();
							OB = Group[i];

							if (OB)
							{
								OB->Sdoxlo = 2500;
							}
						}
					}

					for (int i = 0; i < 8; i++)
					{
						CITY[i].Account = SCORES[i];
					}
				}
			}
		}
	}

	if (NOPAUSE)
	{
		ProcessSprites();
	}

	NMONS = 0;

	//Transport ships logic
	HandleTransport();

	int tmtmt_mod_8 = tmtmt % 8;
	//Process production queues, upgrades, farm growing etc
	if (0 == tmtmt_mod_8)
	{
		//For yourself - always
		CITY[0].ProcessCreation();
	}
	if (NOPAUSE)
	{
		//Every other player is processed every 8th internal tick
		if (tmtmt_mod_8 == 1)
		{
			CITY[1].ProcessCreation();
		}
		if (tmtmt_mod_8 == 2)
		{
			CITY[2].ProcessCreation();
		}
		if (tmtmt_mod_8 == 3)
		{
			CITY[3].ProcessCreation();
		}
		if (tmtmt_mod_8 == 4)
		{
			CITY[4].ProcessCreation();
		}
		if (tmtmt_mod_8 == 5)
		{
			CITY[5].ProcessCreation();
		}
		if (tmtmt_mod_8 == 6)
		{
			CITY[6].ProcessCreation();
		}

		if (tmtmt_mod_8 == 7 || TutOver)
		{
			HandleMission();
			TutOver = 0;
		}

		int xt = ( tmtmt % 256 );
		GNFO.Process();

		for (int i = 0; i < 8; i++)
		{
			Nation* NT = NATIONS + i;
			NT->Harch += NT->NGidot*ResPerUnit;
			int mult = 2000 >> SpeedSh;
			int DHarch = NT->Harch / mult;
			if (DHarch)
			{
				if (XRESRC( i, EatenRes ) > DHarch)
				{
					AddXRESRC( i, EatenRes, -DHarch );
					NATIONS[i].ResOnLife[EatenRes] += DHarch;
					NT->AddResource( EatenRes, -DHarch );
					NT->Harch -= mult*DHarch;
					if (!NT->Harch)
					{
						NT->Harch = 1;
					}
				}
				else
				{
					SetXRESRC( i, EatenRes, 0 );
					NT->Harch = 0;
				}

			}
			if (!NT->Harch)
			{
				if (XRESRC( i, FoodID ))NT->Harch = 1;
				NATIONS[i].ResOnLife[FoodID]++;
			}
			mult = 2000000 >> SpeedSh;
			for (int j = 0; j < 8; j++)
			{
				int R = NT->ResRem[j];
				R += NT->ResSpeed[j] * 100;
				div_t dd = div( R, mult );
				R = dd.rem;
				AddXRESRC( i, j, -dd.quot );
				NATIONS[i].ResOnLife[j] += dd.quot;
				if (XRESRC( i, j ) < 0)
				{
					NATIONS[i].ResOnLife[j] += XRESRC( i, j );
					SetXRESRC( i, j, 0 );
				}
				NT->AddResource( j, -dd.quot );
				NT->ResRem[j] = R;
				if (j == GoldID)
				{
					if (XRESRC( i, j ) < 2)
					{
						NT->GoldBunt = true;
					}
					else
					{
						NT->GoldBunt = false;
					}
				}
			}
		}


		ProcessNewMonsters();

		ObjTimer.Handle();
	}

	//Process explosion animations
	ProcessExpl();

	for (int i = 0; i < 8; i++)
	{
		//Place observation balloon
		HandleBalloon( NATIONS + i );
	}

	//Something about area linking?
	ProcessDynamicalTopology();
}

/*
 * doInit - do work required for every instance of the application:
 *                create the window, initialize data
 */
static BOOL doInit()
{
	SDL_WindowFlags windowFlags = 0;
	if (!window_mode)
	{
		windowFlags |= SDL_WINDOW_FULLSCREEN;
	}
	else
	{
		// Resizable window: the renderer path (FlipPages -> BlitSurfaceScaled
		// -> RenderTexture) stretches the logical RealLx x RealLy 8-bit
		// canvas to whatever the window size is; mouse input is scaled back
		// in SDL_AppEvent (see ScaleMouseToLogical).
		windowFlags |= SDL_WINDOW_RESIZABLE;
	}
	if (borderless)
	{
		windowFlags |= SDL_WINDOW_BORDERLESS;
	}

	sdlWindow = SDL_CreateWindow(
		"Cossacks",
		window_mode ? RealLx : screen_width,
		window_mode ? RealLy : screen_height,
		windowFlags
	);
	if (!sdlWindow)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Loading error", "Unable to create SDL window", nullptr);
		return false;
	}
	SDL_HideCursor();
	if (window_mode)
	{
		ResizeAndCenterWindow();
	}
	UpdateWindowSize();   // baseline for the mouse coordinate scaling
	
	// TODO: this was mapped from winapi, not really needed
	SDL_ShowWindow( sdlWindow );
	SDL_UpdateWindowSurface( sdlWindow );

	// Sound system (CDirSound CDIRSND - ddex1.cpp, CDS - gamesound.cpp)
	extern CDirSound CDIRSND;
	extern CDirSound* CDS;

	CDIRSND.CreateDirSound();

	CDS = &CDIRSND;

	LoadSounds( "SoundList.txt" );
	ResFile F = RReset( "version.dat" );
	if (F != INVALID_HANDLE_VALUE)
	{
		word B = 0;
		RBlockRead( F, &B, 2 );
		RClose( F );
		if (B > 102)
		{
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "WARNING!", "Unable to use this testing version.", sdlWindow);
			FilesExit();
			SDL_Event e;
			e.type = SDL_EVENT_QUIT;
			e.quit.timestamp = SDL_GetTicks();
			SDL_PushEvent(&e);
			return 0;
		}
	}


	bool Loading();
	if (!Loading())
	{
		FilesExit();
		SDL_Event e;
		e.type = SDL_EVENT_QUIT;
		e.quit.timestamp = SDL_GetTicks();
		SDL_PushEvent(&e);
		return 0;
	}

	//create the main DirectDraw object
	PalDone = false;

	KeyPressed = false;

	//Fullscreen? Prepare for small not stretched menu
	if (!window_mode)
	{//Set initial window resolution to native screen resolution
		if (1920 < screen_width)
		{//Limit max resolution for menu screen to fullhd
			//Also necessary for correct offsets in stats screen
			screen_width = 1920;
			screen_height = 1080;
		}
		RealLx = screen_width;
		RealLy = screen_height;
	}

	//Create the screen object with RealLx x RealLy resolution
	CreateDDObjects( sdlWindow );

	if (!SDLError)
	{
		LockSurface();
		UnlockSurface();

		// TODO: why twice?
		LockSurface();
		UnlockSurface();

		if (!RealScreenPtr)
		{
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Loading error[2]", "Unable to initialise SDL. It is possible that hardware acceleration is turned off.", sdlWindow);
			exit( 0 );
		}

		return TRUE;
	}

	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "ERROR", "SDL Init Failed\n", sdlWindow);
	FreeDDObjects();
	// TODO: this was mapped from winapi, not needed here, could be moved to SDL_AppQuit
	SDL_DestroyWindow( sdlWindow );
	return FALSE;
}

//Decls for ClearModes: river-flow editing (river.cpp), the TexMap brush
//toggle (interface.cpp) and the curve scratchpad (3dmaped.cpp)
extern bool RiverEditMode;
extern bool TexMapMod;
void ClearCurve();

void ClearModes()
{
	DrawPixMode = 0;
	DrawGroundMode = 0;
	HeightEditMode = false;
	MEditMode = false;
	LockMode = 0;
	WaterEditMode = false;
	SetWallBuildMode( 0xFF, 0 );
	TexMapMod = false;
	RiverEditMode = 0;
	ClearCurve();
	TexPieceMode = 0;
}


//Declarations for the in-game hotkey block below; the globals themselves
//live in ddex1.cpp/mapa.cpp/interface.cpp.
extern byte SpecCmd;   // special command set by hotkeys, consumed by ProcessKey
extern bool MiniActive;
extern bool Recreate;
extern bool FullMini;
extern int  MenuType;
extern bool MakeMenu;
extern bool CheckFNSend( int idx );
extern int  Inform;
extern bool HealthMode;
extern bool InfoMode;
extern byte NeedToPopUp;
extern bool OptHidden;
extern bool ToolsHidden;
extern bool RESMODE;
extern bool TransMode;
extern bool MultiTvar;
extern bool LockPause;
extern bool EnterChatMode;
extern void SetGameDisplayModeAnyway( int Lx, int Ly );
extern bool ShowStatistics();
extern bool CheckFlagsNeed();
extern bool ChangeNation;   // hotkeys switch this nation flag
extern void ClearKeyStack();
extern void ClearMStack();

bool EnterChatMode = 0;

// Multiplayer vote keys (multi.cpp) and chat keys (linux_stubs no-op
// until the chat is ported).
void ProcessVotingKeys();
void ProcessChatKeys();

// Screenshot + full-map-render family - MOVED here from ddex1.cpp
// (the original Windows entrypoint, not part of the Linux build) for the
// map editor's G/H keys (SaveScreen/RenderAllMap); bodies unchanged.
extern int CurPalette;
extern bool NoText;     // mapa.cpp
extern bool SHOWSLIDE;  // main.cpp (declared below; defined here)
void GFieldShow();

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

// Map-editor key dispatcher - MOVED here from ddex1.cpp (the original
// Windows entrypoint file, not part of the Linux build) so the editor
// gets keyboard input; the body is unchanged. The declarations below
// come with it from ddex1.cpp (the editor tooling lives in
// 3dmaped.cpp / 3drandmap.cpp / mapa.cpp, all compiled).
void EditorKeyCheck();
void ProcessSaveInSquares();
void TestGenMap();
bool ShowStatistics();
extern bool GUARDMODE;   // mapa.cpp
extern bool PATROLMODE;  // mapa.cpp
extern bool RESMODE;
extern bool OptHidden;
extern bool ToolsHidden;
extern int PEN_RADIUS;
extern int PEN_BRIGHTNESS;
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
void SetFractalTexture();
void AutoSMSSet();
void UpdateAllPieces();

void SaveScreen();

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

//Many diffirent key checks for various game modes
void GameKeyCheck()
{
	if (PlayGameMode == 1)
	{
		if (KeyPressed)
		{
			GameExit = true;
			RetryVideo = 0;
			KeyPressed = 0;
			return;
		}
	}

	// Un-guarded 2026-11-01 for the map editor: EditorKeyCheck (and the
	// chat/voting key dispatch ahead of it) are portable (ddex1.cpp);
	// without this the editor had no keyboard input, not even ESC.
	ProcessVotingKeys();

	if (EnterChatMode)
	{
		ProcessChatKeys();
		return;
	}

	if (EditMapMode)
	{
		EditorKeyCheck();
		return;
	}

	if (KeyPressed)
	{
		KeyPressed = false;
		SDL_Keycode wParam = LastKey;

		switch (wParam)
		{
		case SDLK_ESCAPE:
			ClearModes();
			BuildMode = false;
			GetCoord = false;
			mousePointerType = 0;
			curdx = 0;
			curdy = 0;
			PauseMode = 0;
			// Un-guarded (was _WIN32-only, the usual call-site trap): the
			// mode resets AND the WaitState 1->2 advance - the latter is
			// what lets ESC skip the Victory/Defeat "press ESC" message and
			// proceed to the statistics screens (mapa.cpp CheckVictory sets
			// GameExit/ShowStat once WaitState==2).
			// Globals owned by game files not visible here (the decls sat
			// in the guarded block).
			extern bool SetDestMode;
			extern bool GoAndAttackMode;
			extern bool GUARDMODE;
			extern bool PATROLMODE;
			extern int WaitState;
			extern int ShowGameScreen;
			extern bool AttGrMode;
			SetDestMode = false;
			GoAndAttackMode = false;
			GUARDMODE = 0;
			PATROLMODE = 0;

			if (WaitState == 1)
				WaitState = 2;

			if (ShowGameScreen)
				ShowGameScreen = 2;

			AttGrMode = 0;
			break;
		case SDLK_SPACE:
			SpecCmd = 111;
			break;
		case SDLK_BACKSPACE:
			SpecCmd = 112;
			break;
		//In-game hotkeys. F1-F4 send multiplayer taunts first (CheckFNSend);
		//when chat is off they fall through to menu/panel actions. F12 opens
		//the game menu, Pause toggles the sim via the ExBuf pause command.
		case SDLK_U:
			if (Inform != 2)
			{
				Inform = 2;
			}
			else
			{
				Inform = 0;
			}
			MiniActive = 0;
			Recreate = 1;
			break;
		case SDLK_M:
			if (GetSDLKeyState( SDL_SCANCODE_LCTRL ))
			{
				SpecCmd = 114;
			}
			else
			{
				FullMini = !FullMini;
			}
			MiniActive = 0;
			Recreate = 1;
			break;
		case SDLK_F12:
			MenuType = 1;
			MakeMenu = true;
			break;
		case SDLK_F1:
			if (!CheckFNSend( 0 ))
			{
				MenuType = 4;
				MakeMenu = true;
			}
			break;
		case SDLK_F2:
			CheckFNSend( 1 );
			break;
		case SDLK_F3:
			CheckFNSend( 2 );
			break;
		case SDLK_F4:
			CheckFNSend( 3 );
			break;
		case SDLK_F5:
			CheckFNSend( 4 );
			break;
		case SDLK_F6:
			CheckFNSend( 5 );
			break;
		case SDLK_F7:
			CheckFNSend( 6 );
			break;
		case SDLK_F8:
			CheckFNSend( 7 );
			break;
		case SDLK_F9:
			if (!CheckFNSend( 8 ))
			{
				Creator = 4096 + 255;
			}
			break;
		case SDLK_TILDE:
		case SDLK_GRAVE:
			HealthMode = !HealthMode;
			printf("HealthMode: %d\n", HealthMode);
			break;
		case SDLK_DELETE:
			SpecCmd = 200;
			break;

		/*
		case SDLK_D:
			if (!( GetSDLKeyState( SDL_SCANCODE_LCTRL ) ))
			{
				if (NPlayers < 2)
				{
					if (( GetSDLKeyState( SDL_SCANCODE_LSHIFT ) ))
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
						}
					}
				}
			}
			else
			{
				//CmdChangeSpeed();//BUGFIX: real time speed changing
			}
			break;
		*/

		case SDLK_A:
			if (GetSDLKeyState( SDL_SCANCODE_LCTRL ))
				SpecCmd = 1;
			else if (NSL[MyNation])
				GoAndAttackMode = 1;
			break;
		case SDLK_S:
			if (GetSDLKeyState( SDL_SCANCODE_LCTRL ))
				SpecCmd = 201;
			break;
		case SDLK_W:
			break;
		case SDLK_J:
			if (PlayGameMode == 2)
			{
				int ExRX = RealLx;
				int ExRY = RealLy;
				if (RealLx != 1024 || RealLy != 768)
				{
					SetGameDisplayModeAnyway( 1024, 768 );
				}
				ShowStatistics();
				if (RealLx != ExRX || RealLy != ExRY)
				{
					SetGameDisplayModeAnyway( ExRX, ExRY );
				}
			}
			break;
		case SDLK_K:
			if (GetSDLKeyState( SDL_SCANCODE_LCTRL ))
			{
				RealPause -= 2;
			}
			else
			{
				RealPause += 2;
			}
			break;
		case SDLK_Q:
			LockGrid += 2;
			if (LockGrid > 3)
			{
				LockGrid = 0;
			}
			MiniActive = 0;
			Recreate = 1;
			break;
		case SDLK_B:
			if (GetSDLKeyState( SDL_SCANCODE_LCTRL ))
			{
				SpecCmd = 9;
			}
			else
			{
				SpecCmd = 10;
			}
			break;
		case SDLK_Z:
			if (GetSDLKeyState( SDL_SCANCODE_LCTRL ))
			{
				SpecCmd = 11;
			}
			else
			{
				//Select all units of the selected type on screen
				SpecCmd = 241;
			}
			break;
		case SDLK_F:
			if (GetSDLKeyState( SDL_SCANCODE_LCTRL ))
			{
				SpecCmd = 13;
			}
			else
			{
				SpecCmd = 14;
			}
			break;
		case SDLK_KP_1:
			if (MEditMode)
			{
				EditMedia = 0;
			}
			else
			{
				if (NPlayers < 2 && ChangeNation)
				{
					SetMyNation( 0 );
				}
				PlayerMask = 1;
			}
			break;
		case SDLK_KP_2:
			if (MEditMode)EditMedia = 1;
			else
			{
				if (NPlayers < 2 && ChangeNation)
				{
					SetMyNation( 1 );
				}
				PlayerMask = 2;
			}
			break;
		case SDLK_KP_3:
			if (MEditMode)
			{
				EditMedia = 2;
			}
			else
			{
				if (NPlayers < 2 && ChangeNation)
				{
					SetMyNation( 2 );
				}
				PlayerMask = 4;
			}
			break;
		case SDLK_KP_4:
			if (MEditMode)
			{
				EditMedia = 3;
			}
			else
			{
				if (NPlayers < 2 && ChangeNation)
				{
					SetMyNation( 3 );
				}
				PlayerMask = 8;
			}
			break;
		case SDLK_KP_5:
			if (MEditMode)
			{
				EditMedia = 4;
			}
			else
			{
				if (NPlayers < 2 && ChangeNation)
				{
					SetMyNation( 4 );
				}
				PlayerMask = 16;
			}
			break;
		case SDLK_KP_6:
			if (MEditMode)
			{
				BlobMode = 1;
			}
			else
			{
				if (NPlayers < 2 && ChangeNation)
				{
					SetMyNation( 5 );
				}
				PlayerMask = 32;
			}
			break;
		case SDLK_KP_7:
			if (MEditMode)
			{
				BlobMode = -1;
			}
			else
			{
				if (NPlayers < 2 && ChangeNation)
				{
					SetMyNation( 6 );
				}
				PlayerMask = 64;
			}
			break;
		case SDLK_KP_8:
			if (NPlayers < 2 && ChangeNation)
			{
				SetMyNation( 7 );
			}
			PlayerMask = 128;
			break;
		case SDLK_I:
			if (GetSDLKeyState( SDL_SCANCODE_LCTRL ))
			{
				InfoMode = !InfoMode;
			}
			else
			{
				if (Inform != 1)
				{
					Inform = 1;
				}
				else
				{
					Inform = 0;
				}
				MiniActive = 0;
				Recreate = 1;
			}
			break;

		case SDLK_CAPSLOCK:
			EgoFlag = !EgoFlag;
			break;

		case SDLK_O:
			if (GetSDLKeyState( SDL_SCANCODE_LCTRL ))
			{
				if (PlayGameMode == 2 || CheckFlagsNeed())
				{
					OptHidden = !OptHidden;
					if (!OptHidden)
					{
						Inform = 0;
					}
				};
			}
			else
			{
				TransMode = !TransMode;
				MiniActive = 0;
				Recreate = 1;
			}
			break;
		case SDLK_P:
			if (GetSDLKeyState( SDL_SCANCODE_LCTRL ))
			{
				SpecCmd = 113;
			}
			else
			{
				if (MultiTvar)
				{
					NeedToPopUp = 2;
					Inform = 0;
				}
			}
			break;
		case SDLK_PAUSE:
			if (tmtmt > 32 && !LockPause)
			{
				SpecCmd = 137;
			}
			break;
		case SDLK_RETURN:
			if (!RESMODE)
			{
				EnterChatMode = 1;
				ClearKeyStack();
			}
			break;
		default:
			if (SDLK_0 <= wParam && wParam <= SDLK_9)
			{
				if (GetSDLTickCount() - LastCTRLPressTime < kCtrlStickyTime)
				{
					CmdMemSelection( MyNation, wParam - SDLK_0 );
				}
				else
				{
					CmdRememSelection( MyNation, wParam - SDLK_0 );
				}
			}
		}
	}
}

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

void ClearKeyStack()
{
	NKeys = 0;
}

//Delete random generated *.m3d map files
void EraseRND()
{
	// Delete stale random-map files, keeping the 3 NEWEST (the Windows
	// version collects "RN? *.m3d", sorts by creation time and deletes
	// all but the last 3; here: dirent scan + mtime sort, same behavior).
	struct RNFile { char name[300]; time_t mtime; };
	RNFile files[512];
	int n = 0;

	DIR* dir = opendir( "." );
	if ( dir )
	{
		struct dirent* de;
		while ( ( de = readdir( dir ) ) && n < 512 )
		{
			const char* nm = de->d_name;
			size_t len = strlen( nm );
			// Pattern "RN? *.m3d": "RN" + one char + ' ' + ... + ".m3d"
			if ( len < 8 ) continue;
			if ( strncmp( nm, "RN", 2 ) != 0 ) continue;
			if ( nm[3] != ' ' ) continue;
			if ( strcmp( nm + len - 4, ".m3d" ) != 0 ) continue;

			struct stat st;
			if ( stat( nm, &st ) == 0 )
			{
				strncpy( files[n].name, nm, 299 );
				files[n].name[299] = 0;
				files[n].mtime = st.st_mtime;
				n++;
			}
		}
		closedir( dir );
	}

	if ( n > 3 )
	{
		// Oldest first, delete all but the 3 newest
		for ( int i = 0; i < n; i++ )
		{
			for ( int j = i + 1; j < n; j++ )
			{
				if ( files[j].mtime < files[i].mtime )
				{
					RNFile t = files[i];
					files[i] = files[j];
					files[j] = t;
				}
			}
		}
		for ( int i = 0; i < n - 3; i++ )
		{
			remove( files[i].name );
		}
	}
}

void ClearMStack()
{
	NInStack = 0;
}


int SUBTIME = 0;
void ProcessScreen();
void GSYSDRAW();
extern int StepX;
extern int StepY;
int TAverage = 50;

void WaitToTime( int Time )
{
	//All tick values fit int for sessions under 24 days (see GetSDLTickCount).
	//Keep every comparison SIGNED: the original exit condition
	//'Time - ticks > 0' evaluated in unsigned arithmetic and wrapped to a
	//huge value once the counter overshot the target - and it always
	//overshoots, each loop iteration renders for >1ms - so the wait spun
	//forever. That froze the paced game on the very first wait.
	int dt0 = Time - (int) GetSDLTickCount();
	bool DoDraw = dt0 > ( TAverage >> 2 );
	do
	{
		if (DoDraw)
		{
			int T0 = (int) GetSDLTickCount();
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
	} while ( (int) GetSDLTickCount() < Time );
	//SUBTIME=0;
}

//Calculates window coordinates and locks cursor inside client area
//Load ids, textures etc
bool Loading()
{
	RivDir = NULL;

	Init_GP_IMG();
	InitDeathList();
	InitNewMonstersSystem();
	InitFonts();
	LoadBorders();
	LoadMessages();
	LoadNations();
	LoadRDS();

	GoldID = GetResID( "GOLD" );
	FoodID = GetResID( "FOOD" );
	StoneID = GetResID( "STONE" );
	TreeID = GetResID( "WOOD" );
	CoalID = GetResID( "COAL" );
	IronID = GetResID( "IRON" );
	LoadEconomy();

	Loadtextures();
	LoadFog( 1 );
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

extern byte MI_Mode;
extern int RES[8][8];
extern int TIMECHANGE[8];
bool IsGameActive();
void InitWaves();
extern int NeedAddTime;
extern int AddTime;
extern BlockBars LockBars;
extern BlockBars UnLockBars;

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
	memset( TIMECHANGE, 0, sizeof(TIMECHANGE) );
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

MouseStack CURMS;

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
