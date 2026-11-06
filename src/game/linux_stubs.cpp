// Linux port stubs: no-op stand-ins for code that is not compiled on Linux.
// The originals live in:
//  - ir_main.cpp (GSC online-game stats/mods hooks: CheckGameTime,
//    IsGameActive, DontMakeRaiting/StopRaiting, LOOSE*/WINIGAME, ...)
//  - _WIN32-guarded regions of interface.cpp (sound sliders, random-map
//    presets, replay filenames, ProcessInternetConnection/IChat lobby)
//  - the IChat / IntExplorer lobby DLLs (GoHomeAnyway, GetGameID,
//    SendVictoryState, StartDownloadInternetFile, SendRecBuffer)
// Each stub keeps the exact signature from the headers so the mangled names match.
// TODO: port the pieces above and delete their stubs.

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
// These mirror the originals that sit in _WIN32-guarded regions of
// interface.cpp.
char CurrentMap[64];      // replay map name (recorder.cpp)
char DEFPLNAMES[8][64];  // default player names (loadsave.cpp)
char MapScenaryDLL[200]; // scenario DLL path (savenewmap.cpp)
int RM_LandType = 0;     // random-map presets (3drandmap.cpp, savenewmap.cpp)
int RM_Resstart = 0;
int RM_Restot = 0;
// Sound volumes: DirectSound-style slider positions (0..100). SDL_AppInit
// zeroes these before loading mode.dat (same as the original), so these
// inits only matter for the very first run before a mode.dat exists.
int WarSound = 0;
int WorkSound = 0;
int OrderSound = 0;
// MidiSound is only the options-slider state variable (interface.cpp's
// definition is inside a _WIN32 region).
int MidiSound = 0;

// Map-record streaming hooks exported by the IntExplorer exchange DLL
// (not ported). They stay NULL, which disables the upload path in
// recorder.cpp (code checks them before calling).
typedef void tpStartDownloadInternetFile( char* Name, char* Server, char* DestName );
typedef void tpSendRecBuffer( byte* Data, int size, bool Final );
tpStartDownloadInternetFile* StartDownloadInternetFile = NULL;
tpSendRecBuffer* SendRecBuffer = NULL;

//-------------- ir_main.cpp hooks (GSC online-game stats, not ported) --------------
bool CheckGameTime() { return false; }
void DontMakeRaiting() {}
void ExplorerOpenRef( int Index, char* ref ) { (void)Index; (void)ref; }
int  GetCurGamePtr( byte** Ptr ) { if (Ptr) *Ptr = NULL; return 0; }
bool IsGameActive() { return false; }
void SetBrokenState() {}

void ShowCentralMessage4( char* Message, int GPIDX, int DX ) { (void)Message; (void)GPIDX; (void)DX; }
void UpdateCurGame() {}

// StopRaiting: ir_main.cpp stats-reporting hook, called by mplayer.cpp
void StopRaiting() {}

// LOOSEANDEXITFAST: ir_main.cpp - records a defeat in the GSC internet-game
// stats object (CURIGAME) on leave; the GSC session flow never runs without
// ir_main, so this is a no-op
void LOOSEANDEXITFAST() {}

void LOOSEIGAME( char* Nick ) { (void)Nick; }
void WINIGAME( char* Nick ) { (void)Nick; }
void GetGameID( char* s ) { if (s) s[0] = 0; }
void SendVictoryState( int ID, byte State ) { (void)ID; (void)State; }

// Lobby helper used by the shared MPL_WaitingGame event loop; the real one
// calls into the IChat lobby DLL (Windows-only)
int  ProcessInternetConnection( bool Active ) { (void)Active; return 0; }

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

#endif // !_WIN32
