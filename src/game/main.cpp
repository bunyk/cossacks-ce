#include <stdio.h>
#include <boost/coroutine2/all.hpp>

// There is no main function, instead we have
// SDL_AppInit, SDL_AppEvent, SDL_AppIterate and SDL_AppQuit callbacks.
// https://wiki.libsdl.org/SDL3/README-main-functions
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#include "ddini.h"
#include "interface.h"
#include "fastdraw.h"
#include "gsound.h"
#include "resfile.h"
#include "mapdiscr.h"

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
int CurPalette;

bool RetryVideo = 0;

//Game speed mode
//0: Slow mode
//1: Fast mode
int exFMode = 1;

//Minimal delay between two PostDrawGameProcess() returns, in ms
const unsigned int kPostDrawInterval = 16;//~60 Hz

//Timespan in ms after last LastCTRLPressTime which allows setting unit control groups
const int kCtrlStickyTime = 50;

int xxx;

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
int PitchTicks = 0;
int MaxPingTime = 0;
int RealPause = 0;
int RealStTime = 0;
int RealGameLength = 0;
int CurrentStepTime = 80;
int NeedCurrentTime = 0;

extern int PeaceTimeLeft;
extern int PeaceTimeStage;
void CmdChangePeaceTimeStage( int Stage );


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

extern uint64_t GetSDLTickCount();

boost::coroutines2::coroutine<void>::pull_type* AllGameCoroutine = nullptr;

//fonts
RLCTable RCross;
RLCTable mRCross;

void GameKeyCheck();

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


SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv)
{
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
	printf("SDL initialized.\n");

	//Init DirectDraw and find possible resolutions
	EnumModesOnly();

#ifndef NODPLAY
	//Create "Cossacks.reg" with Microsoft DirectPlay key
	CreateReg();
#endif

	//Load unrar.dll, call CGSCset::gOpen() to load archives
	if (!FilesInit())
	{
		FilesExit();
		SDL_Event e;
		e.type = SDL_EVENT_QUIT;
		e.quit.timestamp = SDL_GetTicks();
		SDL_PushEvent(&e);
	}
	printf("Files initialized.\n");

	#ifdef _WIN32
	//Delete random generated *.m3d map files
	EraseRND();
	#endif

	//Pointer to the DirectDraw screen buffer
	ScreenPtr = nullptr;

	#ifdef _WIN32
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
	#endif

	RealLx = 1024;
	RealLy = 768;
	exRealLx = 1024;
	exRealLy = 768;

	#ifdef _WIN32
	WarSound = 0;
	WorkSound = 0;
	OrderSound = 0;
	MidiSound = 0;

	//Zero 3D Bars variables (?)
	InitObjs3();

	//Load settings
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
	#endif // _WIN32

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

	#ifdef _WIN32
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

	//Init buffers for national units?
	SetupNatList();

	//Something about fog?
	makeFden();

	PlayerMenuMode = 1;
	#endif // _WIN32

	Creator = 4096 + 255;
	xxx = 0;
	cadr = 0;

	#ifdef _WIN32
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

	//Water colors and buffers
	InitWater();
	#endif // _WIN32

	//Load fonts(?)
	LoadRLC("xrcross.rlc", &RCross);

	memset(Events, 0, sizeof Events);

	//Register winapi window class, init DirectDraw, sounds and cursor
	if (!doInit())
	{
		return SDL_APP_FAILURE;
	}

	//Load specific palette and fog resources (alphas etc)
	#ifdef _WIN32
	LoadFog(2);
	#endif // _WIN32
	LoadPalette("2\\agew_1.pal");

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

	AllGameCoroutine = new boost::coroutines2::coroutine<void>::pull_type(AllGame);

	printf("SDL_AppInit: All game coroutine created, starting game loop.\n");

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

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
	printf("SDL_AppEvent: %d\n", event->type);
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

	#ifdef _WIN32
	case SDL_EVENT_MOUSE_WHEEL:
		// TODO: value might need some tweaking
		WheelDelta = static_cast<short>(round(event->wheel.y));
		break;

	case SDL_EVENT_MOUSE_BUTTON_DOWN:
	{
		SDL_MouseButtonFlags mouseFlags = SDL_GetMouseState(nullptr, nullptr);
		if (event->button.button == SDL_BUTTON_LEFT)
		{
			mouseFlags |= SDL_BUTTON_LMASK;
			Lpressed = true;
			realLpressed = true;
			fixed = false;
			SetMPtr(event->button.x, event->button.y, mouseFlags);
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
				SetMPtr(event->button.x, event->button.y, mouseFlags);
			}
			AddMouseEvent(mouseX, mouseY, Lpressed, Rpressed);
		}
		break;
	}

	case SDL_EVENT_MOUSE_BUTTON_UP:
	{
		SDL_MouseButtonFlags mouseFlags = SDL_GetMouseState(nullptr, nullptr);
		if (event->button.button == SDL_BUTTON_LEFT)
		{
			// !MK_LBUTTON always yields 0, probably ~MK_LBUTTON was meant
			// But keep the original code for compatibility
			//mouseFlags &= ~SDL_BUTTON_LMASK;
			mouseFlags &= !SDL_BUTTON_LMASK;
			if (fixed)
			{
				Lpressed = false;
			}
			realLpressed = false;

			SetMPtr(event->button.x, event->button.y, mouseFlags);

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
				SetMPtr(event->button.x, event->button.y, mouseFlags);
			}
			AddMouseEvent(mouseX, mouseY, Lpressed, Rpressed);
		}
		break;
	}

	case SDL_EVENT_MOUSE_MOTION:
	{
		if (ScreenPtr)
		{
			if (event->motion.x != mouseX || event->motion.y != mouseY)
			{
				SDL_MouseButtonFlags mouseFlags = SDL_GetMouseState(nullptr, nullptr);
				SetMPtr(event->motion.x, event->motion.y, mouseFlags);
				OnMouseMoveRedraw();
			}
		}
		break;
	}
	#endif

	case SDL_EVENT_WINDOW_MOVED:
	case SDL_EVENT_WINDOW_RESIZED:
		//Adjust cursor zone after window was moved
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
			#ifdef _WIN32
			LoadFog(CurPalette);
			#endif
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

#ifdef _WIN32
	case SDL_EVENT_KEY_DOWN:
	{
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
			SaveScreen();
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

	case SDL_EVENT_QUIT:
		//Leave game and assign defeat
		IAmLeft();
		LOOSEANDEXITFAST();
		break;
	#endif // _WIN32
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

	FilesExit();
	StopPlayCD();
	FinishInternetExplorer();

	FreeDDObjects();
	// PostQuitMessage(0); TODO: find cross-platform equivalent
	#endif // _WIN32
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

	int width = RealLx;
	int height = RealLy;

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
	SDL_SetWindowSize( sdlWindow, width, height );
	SDL_SyncWindow( sdlWindow );

	ClipCursorToWindowArea();

	SDL_WarpMouseInWindow( sdlWindow, width / 2, height / 2 );
}

void PostDrawGameProcess()
{
	#ifdef _WIN32
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
	#endif // _WIN32

	GLOBALTIME++;

	if (RealStTime == 0)
	{
		RealGameLength = 0;
	}

	NeedCurrentTime += CurrentStepTime;

	if (GLOBALTIME - PGLOBALTIME > PitchTicks)
	{
		CurrentStepTime -= CurrentStepTime >> 5;
		RealGameLength = GetSDLTickCount() - RealStTime;

		#ifdef _WIN32
		HandleMultiplayer();

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
		#endif
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

	unsigned long time_since_last_call = 0;
	do
	{
		ProcessMessages();
		if (PauseMode)
		{
			GameKeyCheck();
		}
		time_since_last_call = GetSDLTickCount() - prev_postdraw_time;
	} while (PauseMode || time_since_last_call < kPostDrawInterval);

	prev_postdraw_time = GetSDLTickCount();
}

void PreDrawGameProcess()
{
#ifdef _WIN32
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
		HandleShar( NATIONS + i );
	}

	//Something about area linking?
	ProcessDynamicalTopology();
#endif
}

//Register winapi window class, init DirectDraw, sounds and cursor
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
#ifndef NODPLAY
	hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(sdlWindow), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
#endif
	SDL_HideCursor();
	if (window_mode)
	{
		ResizeAndCenterWindow();
	}
	
	// TODO: this was mapped from winapi, not really needed
	SDL_ShowWindow( sdlWindow );
	SDL_UpdateWindowSurface( sdlWindow );

	#ifdef _WIN32
	CDIRSND.CreateDirSound();

	CDS = &CDIRSND;


	LoadSounds( "SoundList.txt" );
	#endif // _WIN32

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

	
	#ifdef _WIN32
	if (!Loading())
	{
		FilesExit();
		SDL_Event e;
		e.type = SDL_EVENT_QUIT;
		e.quit.timestamp = SDL_GetTicks();
		SDL_PushEvent(&e);
		return 0;
	}
	#endif // _WIN32

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

	#ifdef _WIN32
	CHKALL();
	#endif // _WIN32

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

void ClearModes()
{
	DrawPixMode = 0;
	DrawGroundMode = 0;
	HeightEditMode = false;
	MEditMode = false;
	LockMode = 0;
	WaterEditMode = false;
	#ifdef _WIN32
	SetWallBuildMode( 0xFF, 0 );
	TexMapMod = false;
	RiverEditMode = 0;
	ClearCurve();
	#endif
	TexPieceMode = 0;
}


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

	#ifdef _WIN32
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
	#endif // _WIN32

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
			#ifdef _WIN32
			SetDestMode = false;
			GoAndAttackMode = false;
			GUARDMODE = 0;
			PATROLMODE = 0;

			if (WaitState == 1)
				WaitState = 2;

			if (ShowGameScreen)
				ShowGameScreen = 2;

			AttGrMode = 0;
			#endif
			break;
#ifdef _WIN32
		case SDLK_SPACE:
			SpecCmd = 111;
			break;
		case SDLK_BACKSPACE:
			SpecCmd = 112;
			break;
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
			HealthMode = !HealthMode;
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
#endif
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
