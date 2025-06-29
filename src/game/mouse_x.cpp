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
#ifdef _WIN32
	int sofs = int( src ) + x1 + y1*SSizeX;
	int dofs = int( dest ) + bx + ( by << 5 );
	int Lx4 = Lx >> 2;
	int Lx1 = Lx & 3;
	int adds = SSizeX - Lx;
	int addd = 32 - Lx;

	__asm
	{
		push	esi
		push	edi
		mov		edx, Ly
		mov		esi, sofs
		mov		edi, dofs
		cld
		lpp1 : mov		ecx, Lx4
			   jcxz	lpp2
			   rep		movsd
			   lpp2 : mov		ecx, Lx1
					  jcxz	lpp3
					  rep		movsb
					  lpp3 : add		esi, adds
							 add		edi, addd
							 dec		edx
							 jnz		lpp1
							 pop		edi
							 pop		esi
	}
#else
	uint8_t* source = static_cast<uint8_t*>(src) + x1 + y1 * SSizeX;
	uint8_t* target = static_cast<uint8_t*>(dest) + bx + by * 32;
	int adds = SSizeX - Lx;
	int addd = 32 - Lx;

	for (int row = 0; row < Ly; ++row) {
		std::copy(source, source + Lx, target);
		source += SSizeX;
		target += 32;
	}
#endif // _WIN32
}

bool CmpMData( void* dest, void* src, int x, int y, int SSizeX, int SSizeY )
{
#ifdef _WIN32
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
	if (Lx <= 0 || Ly <= 0)return false;
	int sofs = int( src ) + x1 + y1*SSizeX;
	int dofs = int( dest ) + bx + ( by << 5 );
	int Lx4 = Lx >> 2;
	int Lx1 = Lx & 3;
	int adds = SSizeX - Lx;
	int addd = 32 - Lx;
	bool notequal = false;
	__asm
	{
		push	esi
		push	edi
		mov		edx, Ly
		mov		esi, sofs
		mov		edi, dofs
		cld
		lpp1 : mov		ecx, Lx4
			   jcxz	lpp2
			   repe	cmpsd
			   jne		noteq
			   lpp2 : mov		ecx, Lx1
					  jcxz	lpp3
					  repe	cmpsb
					  jne		noteq
					  lpp3 : add		esi, adds
							 add		edi, addd
							 dec		edx
							 jnz		lpp1
							 jmp		lpp4
							 noteq : mov		notequal, 1
									 lpp4 : pop		edi
											pop		esi
	}
	return notequal;
#else 
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
#endif // _WIN32
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

#ifdef _WIN32
	// x << 5 is equivalent to multiplying by 32.
	int src1 = int( buf ) + bx + ( by << 5 ); // source buffer address
	int srcom = int( comp ) + bx + ( by << 5 ); // comparison buffer
	int scrof = int( scrn ) + x1 + y1*SSizeX; // screen buffer address
	int addscr = SSizeX - Lx; // additional bytes to skip in the screen buffer, when moving to the next row
	int add32 = 32 - Lx; // additional bytes to skip in the 32x32 buffer

	__asm
	{
		push	esi
		push	edi
		mov		edx, Ly
		mov		esi, src1
		mov		ebx, srcom
		mov		edi, scrof
		cld
		lpp0 : mov		ecx, Lx
			   lpp1 : lodsb
					  mov		ah, [edi]
					  cmp		ah, [ebx]
					  jnz		lpp2
					  mov[edi], al
					  lpp2 : inc		edi
							 inc		ebx
							 dec		ecx
							 jnz		lpp1
							 add		edi, addscr
							 add		ebx, add32
							 add		esi, add32
							 dec		edx
							 jnz		lpp0
							 pop		edi
							 pop		esi
	}
#else
	uint8_t* src1 = reinterpret_cast<uint8_t*>(buf) + bx + by * 32; // source buffer address
	uint8_t* srcom = reinterpret_cast<uint8_t*>(comp) + bx + by * 32; // comparison buffer
	uint8_t* scrof = reinterpret_cast<uint8_t*>(scrn) + x1 + y1 * SSizeX; // screen buffer address
	int addscr = SSizeX - Lx; // additional bytes to skip in the screen buffer, when moving to the next row
	int add32 = 32 - Lx; // additional bytes to skip in the 32x32 buffer
	
	for (int row = 0; row < Ly; ++row) {
		uint8_t* src_row = src1 + row * 32;
		uint8_t* comp_row = srcom + row * 32;
		uint8_t* dst_row = scrof + (x1 + (y1 + row) * SSizeX);

		for (int col = 0; col < Lx; ++col) {
			if (src_row[col] != comp_row[col]) {
				dst_row[col] = src_row[col];
			}
		}

		scrof += addscr;
		src1 += add32;
		srcom += add32;
	}
#endif // _WIN32
}

//Sets mouse[X|Y] & real[L|R]pressed variables according to mouse state
void SetMPtr( int x, int y, SDL_MouseButtonFlags mouseFlags )
{
	if (x > RSCRSizeX - 1)
	{
		x = RSCRSizeX - 1;
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
