// TODO: this include forces IChat and IntExplorer to add SDL includes
// Need to find a way to avoid this

#ifdef _WIN32
#include <SDL3/SDL_keycode.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

typedef unsigned short word;


#define GP_USER
#define DIALOGS_USER
#define FASTDRAW_USER
#define FONTS_USER
#define GFILE_USER

#include "fastdraw.h"
#pragma pack(1)
#include "gp_draw.h"
#include "dialogs.h"
#include "fonts.h"
#include "resfile.h"
#include "gfile.h"

extern DLLIMPORT int RealLx;
extern DLLIMPORT int RealLy;
extern DLLIMPORT int SCRSizeX;
extern DLLIMPORT int SCRSizeY;
extern DLLIMPORT int RSCRSizeX;
extern DLLIMPORT int RSCRSizeY;
extern DLLIMPORT int COPYSizeX;
extern DLLIMPORT int Pitch;

extern DLLIMPORT char BuildVersion[32];

extern DLLIMPORT int FPSTime;
extern DLLIMPORT int ScrollSpeed;
extern DLLIMPORT int WarSound;
extern DLLIMPORT int WorkSound;
extern DLLIMPORT int OrderSound;
extern DLLIMPORT int MidiSound;
extern DLLIMPORT void StopPlayCD();
extern DLLIMPORT int GetCDVolume();
extern DLLIMPORT void SetCDVolume(int Vol);
extern DLLIMPORT char RECFILE[128];
extern DLLIMPORT int ModeLX[32];
extern DLLIMPORT int ModeLY[32];
extern DLLIMPORT int NModes;
extern DLLIMPORT char PlName[64];
extern DLLIMPORT bool KeyPressed;
extern DLLIMPORT int NameChoose;
extern DLLIMPORT int ItemChoose;
extern DLLIMPORT char IPADDR[128];
extern DLLIMPORT int selected_network_protocol;
extern DLLIMPORT bool TOTALEXIT;
extern DLLIMPORT SDL_Keycode LastKey;

DLLIMPORT void CBar(int x0,int y0,int Lx0,int Ly0,unsigned char c);
DLLIMPORT bool MMItemChoose(SimpleDialog* SD);
DLLIMPORT int GETV(char* Name);
DLLIMPORT char* GETS(char* Name);
DLLIMPORT void LoadFog(int set);
DLLIMPORT bool MMChooseName(SimpleDialog* SD);
DLLIMPORT bool ProcessMessages();
DLLIMPORT void StdKeys();
DLLIMPORT void SlowLoadPalette(const char* lpFileName);
DLLIMPORT void SlowUnLoadPalette(const char* lpFileName);
DLLIMPORT void SavePlayerData();
DLLIMPORT void LoadPlayerData();

DLLIMPORT uint64_t GetSDLTickCount();
#endif // _WIN32

DLLIMPORT int CurPalette;

