#include "ddini.h"
#include "resfile.h"
#include "fastdraw.h"
#include "gp_draw.h"
#include <algorithm>


#define MaxMX 32
#define MsizeX 32

int CurrentCursorGP = 0;
extern int SCRSizeX;
extern int SCRSizeY;
extern int RSCRSizeX;
extern int RSCRSizeY;
extern int COPYSizeX;
bool realLpressed;
bool realRpressed;

//Current cursor image
//0: Default arrow cursor
//1: Sword
//2: Shackles
//3: Hammer
//4: Enter mine
//5: Pickaxe
//6: Axe
//7: Scythe
//8: Gathering point
//9: Artillery barrage
//10: Guard (shield)
//11: Enter transport
//12: Guard (highlighted shield)
//13: Patrol (shield and sword)
int mousePointerType; // TODO: make this enum and use constant names instead of numbers

int mouseX;
int	mouseY;
bool Lpressed;
bool Rpressed;
static char buf1[1024];
static char buf2[1024];
static char buf1o[1024];
static char buf2o[1024];
static int OldMX;
static int OldMY;
static int MX;
static int MY;

static bool LockMouse;

int curdx;
int curdy;
bool MNotRemoved;
typedef unsigned short word;
int GetF( word k );

extern int mapx;
extern int mapy;
extern int smapx;
extern int smapy;

//retreives data from the screen buffer to field 32x32
void GetMData( void* dest, void* src, int x, int y, int SSizeX, int SSizeY )
{
	if (!bActive)
		return;
	int Lx = 32;
	int Ly = 32;
	int x1 = x;
	int y1 = y;
	int bx = 0;//x-coord. on bitbap 32x32
	int by = 0;
	if (x1 < 0)
	{
		bx = -x1;
		Lx += x1;
		x1 = 0;
	}
	if (y1 < 0)
	{
		by = -y1;
		Ly += y1;
		y1 = 0;
	}
	if (x1 + 32 > SSizeX)Lx = SSizeX - x1;
	if (y1 + 32 > SSizeY)Ly = SSizeY - y1;
	if (Lx <= 0 || Ly <= 0)return;
	uint8_t* source = static_cast<uint8_t*>(src) + x1 + y1 * SSizeX;
	uint8_t* target = static_cast<uint8_t*>(dest) + bx + by * 32;
	int adds = SSizeX - Lx;
	int addd = 32 - Lx;

	for (int row = 0; row < Ly; ++row) {
		std::copy(source, source + Lx, target);
		source += SSizeX;
		target += 32;
	}
}

bool CmpMData( void* dest, void* src, int x, int y, int SSizeX, int SSizeY )
{
	uint8_t* s = static_cast<uint8_t*>(src);
	uint8_t* d = static_cast<uint8_t*>(dest);

	int Lx = 32, Ly = 32;
	int x1 = x, y1 = y;
	int bx = 0, by = 0;

	if (x1 < 0) { bx = -x1; Lx += x1; x1 = 0; }
	if (y1 < 0) { by = -y1; Ly += y1; y1 = 0; }
	if (x1 + 32 > SSizeX) Lx = SSizeX - x1;
	if (y1 + 32 > SSizeY) Ly = SSizeY - y1;
	if (Lx <= 0 || Ly <= 0) return false;

	for (int row = 0; row < Ly; ++row) {
		uint8_t* srow = s + (x1 + (y1 + row) * SSizeX);
		uint8_t* drow = d + (bx + (by + row) * 32);
		if (std::memcmp(srow, drow, Lx) != 0) return true;
	}
	return false;
}

void RestoreMData( void* scrn, void* buf, void* comp, int x, int y, int SSizeX, int SSizeY )
{
	if (!bActive) return;
	int Lx = 32; // size of area to copy
	int Ly = 32;
	int x1 = x; // coordinates of the area to copy
	int y1 = y;
	int bx = 0;//x-coord. on bitbap 32x32
	int by = 0;

	if (x1 < 0)
	{
		bx = -x1;
		Lx += x1;
		x1 = 0;
	}
	if (y1 < 0)
	{
		by = -y1;
		Ly += y1;
		y1 = 0;
	}

	if (x1 + 32 > SSizeX)
	{
		Lx = SSizeX - x1;
	}
	if (y1 + 32 > SSizeY)
	{
		Ly = SSizeY - y1;
	}

	if (Lx <= 0 || Ly <= 0)
	{
		return;
	}

	uint8_t* src1 = reinterpret_cast<uint8_t*>(buf);
	uint8_t* srcom = reinterpret_cast<uint8_t*>(comp);
	uint8_t* scrof = reinterpret_cast<uint8_t*>(scrn);

	for (int row = 0; row < Ly; ++row) {
		uint8_t* src_row = src1 + bx + (by + row) * 32;
		uint8_t* comp_row = srcom + bx + (by + row) * 32;
		uint8_t* dst_row = scrof + x1 + (y1 + row) * SSizeX;

		for (int col = 0; col < Lx; ++col) {
			// asm: restore bg only where the screen still equals the
			// post-cursor save (the cursor pixel is still there); leave
			// pixels alone that changed since the save
			if (dst_row[col] == comp_row[col]) {
				dst_row[col] = src_row[col];
			}
		}
	}
}

//Sets mouse[X|Y] & real[L|R]pressed variables according to mouse state
void SetMPtr( int x, int y, SDL_MouseButtonFlags mouseFlags )
{
	if (x > RSCRSizeX - 1)
	{
		x = RSCRSizeX - 1;
	}
	if (x < 0)
	{
		x = 0;
	}
	if (y > RSCRSizeY - 1)
	{
		y = RSCRSizeY - 1;
	}
	if (y < 0)
	{
		y = 0;
	}

	if (( x != mouseX ) | ( y != mouseY ))
	{
		mouseX = x;
		mouseY = y;
		realLpressed = ( ( mouseFlags & SDL_BUTTON_LMASK ) != 0 );
		realRpressed = ( ( mouseFlags & SDL_BUTTON_RMASK ) != 0 );
	}
}

//Redraws mouse in the offscreen buffer
//and prepares data for onscreen transferring 
void RedrawOffScreenMouse()
{
	if (!bActive)
	{
		return;
	}

	if (mousePointerType == 8)
	{
		curdx = 16;
		curdy = 17;
	}
	else
	{
		curdx = 5;
		curdy = 5;
	}

	LockMouse = true;
	MX = mouseX - curdx;
	MY = mouseY - curdy;

	RestoreMData( ScreenPtr, (void*) buf1, (void*) buf2, OldMX, OldMY, SCRSizeX, SCRSizeY );

	GetMData( (void*) buf1, ScreenPtr, MX, MY, SCRSizeX, SCRSizeY );

	GPS.ShowGP( MX, MY, CurrentCursorGP, mousePointerType, 0 );

	GetMData( (void*) buf2, ScreenPtr, MX, MY, SCRSizeX, SCRSizeY );
}

void RedrawScreenMouse()
{
	if (!bActive || window_mode)//BUGFIX: Cursor shadow trail while showing ingame menues
	{
		return;
	}

	RestoreMData( RealScreenPtr, (void*) buf1o, (void*) buf2o, OldMX, OldMY, RSCRSizeX, RSCRSizeY );
	GetMData( (void*) buf1o, RealScreenPtr, MX, MY, RSCRSizeX, RSCRSizeY );
	void* osp = ScreenPtr;
	int osx = SCRSizeX;
	int sw = ScrWidth;
	ScrWidth = RSCRSizeX;
	ScreenPtr = RealScreenPtr;
	SCRSizeX = RSCRSizeX;
	GPS.ShowGP( MX, MY, CurrentCursorGP, mousePointerType, 0 );
	SCRSizeX = osx;
	ScreenPtr = osp;
	ScrWidth = sw;
	GetMData( (void*) buf2o, RealScreenPtr, MX, MY, RSCRSizeX, RSCRSizeY );
}

void OnMouseMoveRedraw()
{
	if (LockMouse)
	{
		return;
	}

	RedrawOffScreenMouse();
	RedrawScreenMouse();
	LockMouse = false;
	OldMX = MX;
	OldMY = MY;
}


void PostRedrawMouse()
{
	bool need = true;
	if (MX == OldMX && MY == OldMY)
	{
		need = CmpMData( (void*) buf2o, RealScreenPtr, MX, MY, RSCRSizeX, RSCRSizeY );
	}

	if (need)
	{
		RestoreMData( RealScreenPtr, (void*) buf1, (void*) buf2, MX, MY, RSCRSizeX, RSCRSizeY );
		RestoreMData( RealScreenPtr, (void*) buf1o, (void*) buf2o, OldMX, OldMY, RSCRSizeX, RSCRSizeY );
		GetMData( (void*) buf1o, RealScreenPtr, MX, MY, RSCRSizeX, RSCRSizeY );
		void* osp = ScreenPtr;
		int osx = SCRSizeX;
		int sw = ScrWidth;
		ScrWidth = RSCRSizeX;
		ScreenPtr = RealScreenPtr;
		SCRSizeX = RSCRSizeX;
		GPS.ShowGP( MX, MY, CurrentCursorGP, mousePointerType, 0 );
		SCRSizeX = osx;
		ScreenPtr = osp;
		ScrWidth = sw;
		GetMData( (void*) buf2o, RealScreenPtr, MX, MY, RSCRSizeX, RSCRSizeY );
		OldMX = MX;
		OldMY = MY;
	}

	LockMouse = false;
}
