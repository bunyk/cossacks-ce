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

extern bool RUNMAPEDITOR;
extern bool RUNUSERMISSION;
extern char USERMISSPATH[128];
bool window_mode;
bool borderless = false;

bool EditMapMode;
extern bool InGame;
extern bool InEditor;

int screen_width;
int screen_height;
double screen_ratio;

//Last used display resolutions for both modes
int exRealLx, exRealLy;
int ex_other_RealLx, ex_other_RealLy;//Necessary for saving settings
									 //
extern int RealLx;
extern int RealLy;

SDL_Window* sdlWindow;

boost::coroutines2::coroutine<void>::pull_type* AllGameCoroutine = nullptr;

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

	// Init SDL window
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

	#ifdef _WIN32
	//Delete random generated *.m3d map files
	EraseRND();

	//Pointer to the DirectDraw screen buffer
	ScreenPtr = nullptr;

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

	WindX = 0;
	WindY = 0;
	WindX1 = 1023;
	WindY1 = 767;
	WindLx = 1024;
	WindLy = 768;

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

	//Water colors and buffers
	InitWater();

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

	StartExplorer();

	#endif // _WIN32

	// FIXME: need to be called only when focused on textboxes
	// Fine on desktop, but not on mobile
	SDL_StartTextInput(sdlWindow);

	AllGameCoroutine = new boost::coroutines2::coroutine<void>::pull_type(AllGame);

	return SDL_APP_CONTINUE;
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
	FinExplorer();

	finiObjects();
	// PostQuitMessage(0); TODO: find cross-platform equivalent
	#endif // _WIN32
	exit(0);
}

bool PalDone;

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

// Mouse grab confines the mouse cursor to the window, when InGame or InEditor is true.
void ClipCursorToWindowArea()
{
	if (!window_mode)
	{//Just in case
		return;
	}

	SDL_SetWindowMouseGrab(sdlWindow, InGame || InEditor);
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

