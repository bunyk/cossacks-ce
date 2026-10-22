// Unit motion/pathfinding engine.
//
// Movement is planned on two coarse grids ("motion fields", MFIELDS):
//   MFIELDS[0] = land, MFIELDS[1] = water (a unit picks its field via
//   OneObject::LockType: 0 = land, 1 = water).
// A MotionField is a bit-per-cell occupancy matrix (MapV): 1 = the cell is
// blocked. The map editor / random map generator marks terrain, buildings
// and walls into it (3dmaped.cpp, 3drandmap.cpp); moving units with
// OneObject::GLock set temporarily mark their own footprint while they
// stand or walk, so other units path around them.
//
// This is NOT an A*-style pathfinder. The algorithm (CreateFullPath +
// OneObject::CreatePrePath) is: draw a straight Bresenham line to the
// target; walk it until the first blocked cell; then wall-follow around
// the obstacle on both sides at once (right/clockwise and left/counter-
// clockwise contours) and keep whichever contour rejoins the line or
// regains direct sight of the target first; append those waypoints and
// repeat, up to four obstacle hops per move order.
//
// Coordinates everywhere are motion cells, not pixels: the map is
// MAPSX x MAPSY cells (512 << ADDSH); one cell is 256 game units, a cell
// (x,y) maps to pixel (x << 8, y << 8) and an Lx-wide footprint is centered
// with (Lx << 7). See OneObject::CreatePath for the conversion.
#include "ddini.h"
#include "resfile.h"
#include "fastdraw.h"
#include "mgraph.h"
#include "mouse.h"
#include "mapdiscr.h"
#include <assert.h>
#include "mode.h"
#include "newmon.h"
#include <math.h>
#include "path.h"
#include "topograf.h"

// Set by CreatePrePath when the planner wants the caller to throttle the
// next attempt (throttles crowd congestion); CreateFullPath converts it
// into a small random OB->PathDelay.
bool AllowPathDelay;

// DEAD CODE: MotionBrush and its two factories are implemented here and
// declared in path.h, but nothing in the game ever constructs a MotionBrush.
// They are leftovers of an older brush-based pathfinder (see the commented-
// out MotionField methods in path.h: DrawBrush/CheckBrush/FindWay - none of
// them exist anymore). The little-endian row masks (1 bit per x cell) would
// be used to stamp arbitrary shapes into MapV. No need to port or test.
MotionBrush::MotionBrush()
{
	Data = nullptr;
	Lx = 0; Ly = 0;
	Dx = 0; Dy = 0;
}

MotionBrush::~MotionBrush()
{
	if (Data)
	{
		free(Data);
		Data = nullptr;
		Lx = 0; Ly = 0;
		Dx = 0; Dy = 0;
	}
}

void MotionBrush::CreateRoundBrush(int d)
{
	Data = new int[d];
	memset(Data, 0, d << 2);
	for (int y = 0; y < d; y++)
	{
		for (int x = 0; x < d; x++)
		{
			int rr = int(sqrt((x + x - d)*(x + x - d) + (y + y - d)*(y + y - d)));
			if (rr < d + 1)
			{
				Data[y] |= 1 << x;
			}
		}
	}
	Lx = d;
	Ly = d;
	Dx = d >> 1;
	Dy = d >> 1;
}

void MotionBrush::CreateSquareBrush(int d)
{
	Data = new int[d];
	memset(Data, 0, d * 4);
	for (int y = 0; y < d; y++)
	{
		for (int x = 0; x < d; x++)
		{
			Data[y] |= 1 << x;
		}
	}
	Lx = d;
	Ly = d;
	Dx = d >> 1;
	Dy = d >> 1;
}


//-------------------------------------------------------//
//                                                       //
//                    ОБХОД ПРЕПЯТСТВИЙ                  //
//                                                       //
//-------------------------------------------------------//
// MotionField = the occupancy ("what is blocked") matrix itself.
// MapV is a bit matrix, 1 bit per motion cell, stored column-major:
//   byte index = (x << (6 + ADDSH)) + (y >> 3), bit = 1 << (y & 7)
// (the original Windows build had hand-written asm versions of the
// primitives below, specialized per map size ADDSH = 1/2/3; they were
// removed in favor of this single portable implementation).
// B* methods write occupancy, Check* methods read it.

//Zero out MapV
void MotionField::ClearMaps()
{
	memset(MapV, 0, MAPSY*BMSX);//1024*128
}

// Mark motion cell (x,y) as blocked. Out-of-bounds cells are ignored.
void MotionField::BSetPt(int x, int y)
{
	if (x >= 0 && x < MAPSX&&y >= 0 && y < MAPSY) {
		MapV[( x << ( 6 + ADDSH ) ) + ( y >> 3 )] |= 1 << ( y & 7 );
	};
}

// Mark motion cell (x,y) as free. Out-of-bounds cells are ignored.
void MotionField::BClrPt(int x, int y)
{
	if (x >= 0 && x < MAPSX&&y >= 0 && y < MAPSY) {
		MapV[( x << ( 6 + ADDSH ) ) + ( y >> 3 )] &= ~( 1 << ( y & 7 ) );
	};
}

// Mark an Lx x Lx square as blocked - used for unit footprints
// (all units occupy square Lx x Lx cells).
void MotionField::BSetBar(int x, int y, int Lx) {
	for (int ix = x; ix < x + Lx; ix++)
	{
		for (int iy = y; iy < y + Lx; iy++)
		{
			BSetPt(ix, iy);
		}
	}
}

// Mark an Lx x Lx square as free (see BSetBar).
void MotionField::BClrBar(int x, int y, int Lx) {
	for (int ix = x; ix < x + Lx; ix++)
	{
		for (int iy = y; iy < y + Lx; iy++)
		{
			BClrPt(ix, iy);
		}
	}
}

// Mark an Lx x Ly rectangle as blocked - used by the map generator/editor
// for terrain, buildings, walls and the map border (3dmaped.cpp).
void MotionField::BSetSQ(int x, int y, int Lx, int Ly) {
	for (int ix = x; ix < x + Lx; ix++)
	{
		for (int iy = y; iy < y + Ly; iy++)
		{
			BSetPt(ix, iy);
		}
	}
}

// Mark an Lx x Ly rectangle as free (see BSetSQ).
void MotionField::BClrSQ(int x, int y, int Lx, int Ly) {
	for (int ix = x; ix < x + Lx; ix++)
	{
		for (int iy = y; iy < y + Ly; iy++)
		{
			BClrPt(ix, iy);
		}
	}
}

//Checks coordinates against MotionField::MapV
//Possible return values: 0, 1, 2, 4, 8, 16, 32, 64, 128
// 0 = cell is free, nonzero = cell is blocked.
// Cells outside the map count as blocked (the world edge is a wall).
int MotionField::CheckPt(int x, int y)
{
	int retval = 0;
	if (x >= 0 && x < MAPSX&&y >= 0 && y < MAPSY) 
	{
		retval = MapV[( x << ( 6 + ADDSH ) ) + ( y >> 3 )] & ( 1 << ( y & 7 ) );
		return retval;
	}
	else
	{
		return 1;
	}
}

// 1 if any of the Lx cells of the horizontal run starting at (x,y) is
// blocked, 0 if the whole run is free.
int MotionField::CheckHLine(int x, int y, int Lx)
{
	for (int i = 0; i < Lx; i++)
	{
		if (CheckPt(x + i, y))
		{
			return 1;
		}
	}
	return 0;
}

// Same as CheckHLine for a vertical run of Lx cells downward from (x,y).
// Fast path: the whole run fits in one 32-bit word (hence Lx <= 24 and the
// little-endian byte gather in the portable branch). Outside the map or
// with Lx > 24 it conservatively reports blocked.
int MotionField::CheckVLine(int x, int y, int Lx)
{
	int retval = 0;
	if (x > 0 && y > 0 && y + Lx - 1 < MAPSY && x < MAPSX && Lx <= 24)
	{
		// one little-endian 32-bit load; (1<<Lx)-1 shifted left by (y&7),
		// no wraparound since (y&7)+Lx <= 31
		const byte* p = MapV + ( x << ( 6 + ADDSH ) ) + ( y >> 3 );
		DWORD v = p[0] | ( p[1] << 8 ) | ( p[2] << 16 ) | ( p[3] << 24 );
		retval = v & ( ( ( 1 << Lx ) - 1 ) << ( y & 7 ) );
		return retval;
	}
	else
	{
		return 1;
	}
}

// true if any cell of the Lx x Ly rectangle at (x,y) is blocked.
// This is the main "can a unit of this size stand here?" test.
bool MotionField::CheckBar(int x, int y, int Lx, int Ly)
{
	for (int ix = 0; ix < Lx; ix++)
	{
		if (CheckVLine(x + ix, y, Ly))
		{
			return true;
		}
	}
	return false;
}

extern byte NewCirc[16];// defined in newmon.cpp

// Fill NewCirc[0..7] with the blocked/free state of the eight cells
// surrounding the Lx x Ly rectangle at (x,y) - the input for the
// wall-follower in CreatePrePath. Direction numbering (clockwise from
// north, matching ndrr/nidrx below):
//   0=N, 1=NE, 2=E, 3=SE, 4=S, 5=SW, 6=W, 7=NW
// i.e. NewCirc[i] = 1 when the neighbor strip in direction i is blocked.
// The strip tests (HLine/VLine) cover the sides; for low rectangles the
// diagonals need a single CheckPt, for high ones a corner diagonal can be
// blocked along either adjoining side, hence the Ly cases.
// At the end NewCirc[8..15] is set as a mirror of NewCirc[0..7]
// (xx[2]=xx[0], xx[3]=xx[1]), so the caller can index directions as
// (8 + d) without a modulo when scanning backwards.
void MotionField::FillNewCirc(int x, int y, int Lx, int Ly)
{
	int* xx = (int*)NewCirc;
	xx[0] = 0;
	xx[1] = 0;

	if (CheckHLine(x, y - 1, Lx))
	{
		NewCirc[0] = 1;
	}
	if (CheckHLine(x, y + Ly, Lx))
	{
		NewCirc[4] = 1;
	}
	if (CheckVLine(x + Lx, y, Ly))
	{
		NewCirc[2] = 1;
	}
	if (CheckVLine(x - 1, y, Ly))
	{
		NewCirc[6] = 1;
	}

	if (Ly < 2)
	{
		if (CheckPt(x - 1, y - 1))
			NewCirc[7] = 1;

		if (CheckPt(x + Lx, y - 1))
			NewCirc[1] = 1;

		if (CheckPt(x - 1, y + Ly))
			NewCirc[5] = 1;

		if (CheckPt(x + Lx, y + Ly))
			NewCirc[3] = 1;
	}
	else
	{
		if (Ly == 2)
		{
			if (CheckHLine(x + 1, y - 1, Lx) && CheckPt(x + Lx, y))
				NewCirc[1] = 1;

			if (CheckHLine(x + 1, y + Ly, Lx) && CheckPt(x + Lx, y + 1))
				NewCirc[3] = 1;

			if (CheckHLine(x - 1, y + Ly, Lx) && CheckPt(x - 1, y + 1))
				NewCirc[5] = 1;

			if (CheckHLine(x - 1, y - 1, Lx) && CheckPt(x - 1, y))
				NewCirc[7] = 1;
		}
		else
		{
			if (CheckHLine(x + 1, y - 1, Lx) || CheckVLine(x + Lx, y, Ly - 1))
				NewCirc[1] = 1;

			if (CheckHLine(x + 1, y + Ly, Lx) || CheckVLine(x + Lx, y + 1, Ly - 1))
				NewCirc[3] = 1;

			if (CheckHLine(x - 1, y + Ly, Lx) || CheckVLine(x - 1, y + 1, Ly - 1))
				NewCirc[5] = 1;

			if (CheckHLine(x - 1, y - 1, Lx) || CheckVLine(x - 1, y, Ly - 1))
				NewCirc[7] = 1;
		}
	}
	xx[2] = xx[0];
	xx[3] = xx[1];
}

//------------------------------------------------//
//          KERNEL OF THE MOTION ENGINE           //
//            Search for the best way             //
//------------------------------------------------//
#define MaxP (4096)
extern word pxx[MaxP + 160];// scratch: Bresenham line points, newmon.cpp
extern word pyy[MaxP + 160];
extern word RVIS[MaxP + 160];// scratch: contour indices with direct sight
extern word LVIS[MaxP + 160];// of the target (CreatePrePath only)
int GetLAngle(int dx, int dy, int Angle);
int GetRAngle(int dx, int dy, int Angle);
// ndrr: step delta (dx,dy in {-1,0,1}) -> compass direction 0..7,
// indexed as ndrr[(dx+1)*3 + (dy+1)]:
//   0=N(0,-1) 1=NE(1,-1) 2=E(1,0) 3=SE(1,1)
//   4=S(0,1)  5=SW(-1,1) 6=W(-1,0) 7=NW(-1,-1)
const int ndrr[9] = { 7,6,5,0,0,4,1,2,3 };
// nidrx/nidry: direction 0..7 -> step delta (matching the list above).
const int nidrx[8] = { 0,1,1,1,0,-1,-1,-1 };
const int nidry[8] = { -1,-1,0,1,1,1,0,-1 };

MotionField MFIELDS[2];//0-Land,1-Water

// Spiral search for a free spot near a desired destination: scan the
// square ring at growing Chebyshev distance from (*xd,*yd) (R0 rings max)
// for a position where OB's footprint (Lx + 2 margin) is unblocked, taking
// the candidate closest to the unit's current cell. Adjusts *xd/*yd in
// place; returns true on success. Used to snap move targets that would
// otherwise land inside an obstacle or a crowd.
bool FindBestPosition(OneObject* OB, int* xd, int* yd, int R0)
{
	MotionField* MFI = &MFIELDS[OB->LockType];

	if (!OB)
		return false;

	if (!MFI->CheckBar((*xd) - 1, (*yd) - 1, OB->Lx + 2, OB->Lx + 2))
		return true;

	int Lx = OB->Lx + 2;
	int R1 = R0;
	int bx = *xd;
	int by = *yd;
	int bdist = 100000;
	int xxx = bx - 1;
	int yyy = by - 1;
	int LL = 2;
	int bx1, by1, dstn;
	int x = OB->x;
	int y = OB->y;
	while (R1)
	{
		for (int i = 0; i <= LL; i++)if (!MFI->CheckBar(xxx + i - 1, yyy - 1, Lx, Lx)) {
			bx1 = xxx + i;
			by1 = yyy;
			dstn = Norma(bx1 - x, by1 - y);
			if (dstn < bdist) {
				bx = bx1;
				by = by1;
				bdist = dstn;
			};
		};
		for (int i = 0; i <= LL; i++)if (!MFI->CheckBar(xxx + i - 1, yyy + LL - 1, Lx, Lx)) {
			bx1 = xxx + i;
			by1 = yyy + LL;
			dstn = Norma(bx1 - x, by1 - y);
			if (dstn < bdist) {
				bx = bx1;
				by = by1;
				bdist = dstn;
			};
		};
		for (int i = 0; i < LL - 1; i++)if (!MFI->CheckBar(xxx - 1, yyy + i, Lx, Lx)) {
			bx1 = xxx;
			by1 = yyy + i + 1;
			dstn = Norma(bx1 - x, by1 - y);
			if (dstn < bdist) {
				bx = bx1;
				by = by1;
				bdist = dstn;
			};
		};
		for (int i = 0; i < LL - 1; i++)if (!MFI->CheckBar(xxx + LL - 1, yyy + i, Lx, Lx)) {
			bx1 = xxx + LL;
			by1 = yyy + i + 1;
			dstn = Norma(bx1 - x, by1 - y);
			if (dstn < bdist) {
				bx = bx1;
				by = by1;
				bdist = dstn;
			};
		};
		if (bdist < 100000) {
			*xd = bx;
			*yd = by;
			return true;
		};
		R1--;
		LL += 2;
		xxx--;
		yyy--;
	};
	return false;
}

// Build the full path of OB to cell (x1,y1): drop any old path, snap the
// target to a reachable cell (FindBestPosition), then call CreatePrePath
// repeatedly (max 4 segments). Each CreatePrePath call walks around ONE
// obstacle and ends at a cell from which the target is directly visible,
// so the loop chains segments across several obstacles until the target
// is reached or an attempt fails.
void CreateFullPath(int x1, int y1, OneObject* OB)
{
	OB->DeletePath();
	OB->NIPoints = 0;
	int xx1 = x1;
	int yy1 = y1;

	if (!FindBestPosition(OB, &xx1, &yy1, 40))
		return;

	bool pc;
	int NN = 0;
	AllowPathDelay = false;
	do
	{
		pc = OB->CreatePrePath(xx1, yy1);

		if (pc) {
			xx1 = OB->PathX[OB->NIPoints - 1];
			yy1 = OB->PathY[OB->NIPoints - 1];
		};
		NN++;
		rando();
	} while (pc&&NN < 4);

	if (AllowPathDelay)
	{
		OB->PathDelay = (rando() & 7);
	}
}

// sssh = subsampling shift for stored waypoints of the bypass segment:
// only every (1 << sssh)-th contour point is appended to PathX/PathY
// (smoother paths store more points). CreatePrePath: 1 (every 2nd),
// CreatePrePath2: 2 (every 4th), CreatePrePath4: 3 (every 8th).
#define sssh 1
// One step of the path search (land variant): try to advance OB toward
// cell (x1,y1) and append waypoints for the FIRST obstacle on the way.
// Returns false when no new segment is needed - either the straight line
// is already clear (uu == Pps check below), the unit is already there
// (<2 cells), or planning failed (sets PathDelay, a random retry timeout
// in frames decremented in newmon.cpp, to spread crowd congestion:
// 200+ for land, 64+ for water).
//
// Algorithm (this and the two *2/*4 copies below share it):
//  1. Bresenham-line the unit's cell to (x1,y1) into the global pxx/pyy.
//  2. If the unit's own cell is blocked (InLocked, e.g. it stands inside
//     a crowd), make a one-point path to the first free cell on the line.
//  3. Otherwise walk the line to the first blocked cell and wall-follow
//     the obstacle with two contours in lockstep: R = clockwise ("right")
//     and L = counterclockwise ("left"), one step of each per iteration,
//     guided by FillNewCirc. Track per contour:
//       - Rppm/Lppm: the point of maximum angular deviation from the
//         start->target line (GetLAngle/GetRAngle) - the best exit point,
//       - RVIS/LVIS (this variant) or Rvp/Lvp (*2): contour points from
//         which the target is directly visible again,
//       - RDoing/LDoing: whether the contour has re-crossed the straight
//         line yet (Rcum/Lcum are cross products of the contour step
//         against the line direction, sign change = crossing).
//  4. Prefer the contour that re-crossed first (RightPrefer); truncate it
//     at the max-deviation point, extend to the first direct-sight point,
//     subsample by sssh and append to PathX/PathY (NeedPath = true tells
//     the movement code in newmon.cpp a fresh path exists).
// If OB->GLock is set the unit owns its occupancy bits: they are cleared
// while planning (so the unit does not path around itself) and restored
// on every exit path. The stack arrays Rpx/Rpy/Lpx/Lpy are 2 KB each.
bool OneObject::CreatePrePath(int x1, int y1)
{
	AllowPathDelay = false;
	if (LockType == 1) {
		return CreatePrePath2(x1, y1);
	};
	if (PathDelay)return false;

	AllowPathDelay = true;
	if (abs(x - x1) < 2 && abs(y - y1) < 2)return false;
	MotionField* MFI = &MFIELDS[LockType];
	if (GLock)MFI->BClrBar(x, y, Lx);
	bool InLocked = MFI->CheckBar(x, y, Lx, Lx);
	if (InLocked&&PathX) {
		if (GLock)MFI->BSetBar(x, y, Lx);
		return false;
	};
	int sdx = x1 - x;
	int	sdy = y1 - y;
	int	Cum = 0;
	int Pps = 0;
	int sx = (sdx > 0) ? 1 : -1;
	int sy = (sdy > 0) ? 1 : -1;
	int dx = abs(sdx);
	int dy = abs(sdy);
	int	Mdx = dx;
	int	Mdy = dy;
	int	Mx = x;
	int	My = y;
	int	xx1 = x;
	int yy1 = y;
	int rx = sx;
	int ry = sy;
	if (dx > dy)ry = 0;
	if (dy > dx)rx = 0;
	int Angle0 = GetLAngle(x1 - x, y1 - y, 0);
	int Angle;
	int ddx, ddy;
	int Lvp = 0;
	int Rvp = 0;
	bool LvpLast = false;
	bool RvpLast = false;
	int RvpDist = 100000;
	int LvpDist = 100000;

	CurIPoint = 0;
	NeedPath = false;

	//соединяем линией начальную и конечную точки. 
	//Оптимизация только по скорости
	//Linux translation of the asm Bresenham walker: fills pxx/pyy with the
	//point line from (Mx,My) toward the target; Pps = number of points.
	//The asm's eight direction cases reduce to sx/sy sign multipliers, and
	//the 16-bit cum counter is safe (cum < Mdx + Mdy, map-sized values).
	{
		int nn = 0;
		pxx[nn] = (word)Mx;
		pyy[nn] = (word)My;
		nn++;
		if (Mdx > Mdy) {
			for (int i = 0; i < Mdx; i++) {
				Mx += sx;
				Cum += Mdy;
				if (Cum >= Mdx) {
					Cum -= Mdx;
					My += sy;
				};
				pxx[nn] = (word)Mx;
				pyy[nn] = (word)My;
				nn++;
			};
		}
		else {
			for (int i = 0; i < Mdy; i++) {
				My += sy;
				Cum += Mdx;
				if (Cum >= Mdy) {
					Cum -= Mdy;
					Mx += sx;
				};
				pxx[nn] = (word)Mx;
				pyy[nn] = (word)My;
				nn++;
			};
		};
		Pps = nn;
	}

	Pps--;

	if (InLocked)
	{
		int uu;
		for (uu = 0; uu < Pps&&MFI->CheckBar(pxx[uu], pyy[uu], Lx, Lx); uu++);
		if (uu >= Pps)
		{
			return false;
		}

		PathX = new short;
		PathY = new short;
		PathX[0] = pxx[uu];
		PathY[0] = pyy[uu];
		NIPoints = 1;
		return true;
	}

	int RVisPos = 0;
	int LVisPos = 0;
	bool RightPrefer = true;
	int Rtx;//current point 
	int Rty;
	int Ltx;
	int Lty;
	word Rpx[MaxP];//right path
	word Rpy[MaxP];
	word Lpx[MaxP];//left path
	word Lpy[MaxP];
	int Rpp = 1;//index of current point
	bool LDoing;//=true if last point reached
	bool RDoing;
	byte Rdirc;//currend direction
	byte Ldirc;
	int Rmaxalt;//maximum alteration,right path
	int Lmaxalt;//maximum alteration,left path
	int Rcum = 0;
	int Rcum1 = 0;
	int Lcum = 0;
	int Lcum1 = 0;
	bool Rvis, Lvis;
	//Проверяем прямую проходимость
	int uu;
	for (uu = 1; uu < Pps && !MFI->CheckBar(pxx[uu], pyy[uu], Lx, Lx); uu++);
	if (uu == Pps)return false;
	//Идем, пока не упремся в стенку
	for (uu = 1; uu < Pps && !MFI->CheckBar(pxx[uu] - 1, pyy[uu] - 1, Lx + 2, Lx + 2); uu++);
	Rpp = uu;
	Rtx = pxx[Rpp];
	Rty = pyy[Rpp];
	int Rppm = uu;
	int Lppm = uu;
	// Если dx>dy,то на каждом шагу dx изменяетя строго на 1
	if (Rtx != x1 || Rty != y1) {
		//LLock[y][x]=false;
		if (Rpp > 0)Rpp -= 1;
		Rtx = pxx[Rpp];
		Rty = pyy[Rpp];
		Ltx = pxx[Rpp];
		Lty = pyy[Rpp];
		int Ppi = Rpp + 1;
		LDoing = true;
		RDoing = true;
		//Ищем, пока находимся в занятой зоне
		while (MFI->CheckBar(pxx[Ppi] - 1, pyy[Ppi] - 1, Lx + 2, Lx + 2) && Ppi < Pps)Ppi++;
		if (Ppi > Pps)LDoing = false;//Кон.точка недостижима
		int Xls = pxx[Ppi - 1];
		int Yls = pyy[Ppi - 1];
		//Уперлись...Вычисляем направление движения
		Rdirc = ndrr[(pxx[Rpp + 1] - pxx[Rpp] + 1) * 3 + pyy[Rpp + 1] - pyy[Rpp] + 1];
		Ldirc = Rdirc;
		//Выбираем начальное направление-right
		MFI->FillNewCirc(Rtx - 1, Rty - 1, Lx + 2, Lx + 2);
		int dirc1 = (Rdirc + 1) & 7;
		for (int z = 0; NewCirc[dirc1] && z < 7; dirc1++, z++);
		if (!NewCirc[6 + (dirc1 & 7)])dirc1++;
		Rdirc = dirc1 & 7;
		//-left
		dirc1 = 8 + ((Ldirc + 7) & 7);
		for (int z = 0; NewCirc[dirc1] && z < 7; dirc1--, z++);
		if (!NewCirc[(dirc1 & 7) + 2])dirc1--;
		Ldirc = dirc1 & 7;
		//Идем по правому краю до тех пор пока вновь не пересе-
		//чемся с прямой линией, соединяющей начальную и конечную
		//точки
		Rmaxalt = 0;
		Lmaxalt = 0;
		while (Rpp < MaxP - 8 && LDoing&&RDoing) {
			//пытаемся повернуть направо
			MFI->FillNewCirc(Rtx - 1, Rty - 1, Lx + 2, Lx + 2);
			int dirc1 = (Rdirc + 6) & 7;
			for (int z = 0; z < 6 && NewCirc[dirc1]; dirc1++, z++);
			Rdirc = dirc1 & 7;
			Rpp++;
			int Tdx = nidrx[Rdirc];
			int Tdy = nidry[Rdirc];
			Rcum -= sdy*Tdx;
			Rcum += sdx*Tdy;
			Rtx += Tdx;
			Rty += Tdy;
			Rpx[Rpp] = Rtx; Rpy[Rpp] = Rty;
			Angle = GetLAngle(Rtx - x, Rty - y, Angle0);
			if (Angle > Rmaxalt) {
				Rmaxalt = Angle;
				Rppm = Rpp;
			};
			//проверяем условие прямой видимости
			ddx = x1 - Rtx;
			ddy = y1 - Rty;
			if (ddx > 1)ddx = 1;
			if (ddx < -1)ddx = -1;
			if (ddy > 1)ddy = 1;
			if (ddy < -1)ddy = -1;
			int dirvi = ndrr[ddx + ddx + ddx + ddy + 4];
			int dirmo = (ndrr[Tdx + Tdx + Tdx + Tdy + 4] - dirvi) & 7;
			if ((dirmo == 1 || dirmo == 7 || dirmo == 0) && !MFI->CheckBar(Rtx + ddx + ddx - 1, Rty + ddy + ddy - 1, Lx + 2, Lx + 2)) {
				//if(!NewCirc[dirvi]){
					//direct vision
				RVIS[RVisPos] = Rpp;
				RVisPos++;
			};
			//the same, but left
			MFI->FillNewCirc(Ltx - 1, Lty - 1, Lx + 2, Lx + 2);
			dirc1 = 8 + ((Ldirc + 2) & 7);
			for (int z = 0; z < 6 && NewCirc[dirc1]; dirc1--, z++);
			Ldirc = dirc1 & 7;
			Tdx = nidrx[Ldirc];
			Tdy = nidry[Ldirc];
			Lcum += sdy*Tdx;
			Lcum -= sdx*Tdy;
			Ltx += Tdx;
			Lty += Tdy;
			Lpx[Rpp] = Ltx; Lpy[Rpp] = Lty;
			Angle = GetRAngle(Ltx - x, Lty - y, Angle0);
			if (Angle > Lmaxalt) {
				Lmaxalt = Angle;
				Lppm = Rpp;
			};
			//проверяем условие прямой видимости
			ddx = x1 - Ltx;
			ddy = y1 - Lty;
			if (ddx > 1)ddx = 1;
			if (ddx < -1)ddx = -1;
			if (ddy > 1)ddy = 1;
			if (ddy < -1)ddy = -1;
			dirvi = ndrr[ddx + ddx + ddx + ddy + 4];
			dirmo = (ndrr[Tdx + Tdx + Tdx + Tdy + 4] - dirvi) & 7;
			if ((dirmo == 1 || dirmo == 7 || dirmo == 0) && !MFI->CheckBar(Ltx + ddx + ddx - 1, Lty + ddy + ddy - 1, Lx + 2, Lx + 2)) {
				//if(!NewCirc[dirvi]){
				LVIS[LVisPos] = Rpp;
				LVisPos++;
			};
			//Вычисляем значение y на прямой линии, соотв. 
			//данному x
			if (Rcum <= 0 && Rcum1 >= 0) {
				if (!Rvis)Rvp = Rpp;
				if (dx > dy) {
					if (sx > 0) {
						if (Rtx >= Xls)RDoing = false;
					}
					else if (Rtx <= Xls)RDoing = false;
				}
				else {
					if (sy > 0) {
						if (Rty >= Yls)RDoing = false;
					}
					else if (Rty <= Yls)RDoing = false;
				};
				if (!RDoing)RightPrefer = true;
			};
			Rcum1 = Rcum;
			//the same for left
			if (Lcum <= 0 && Lcum1 >= 0) {
				if (!Lvis)Lvp = Rpp;
				if (dx > dy) {
					if (sx > 0) {
						if (Ltx >= Xls)LDoing = false;
					}
					else if (Ltx <= Xls)LDoing = false;
				}
				else {
					if (sy > 0) {
						if (Lty >= Yls)LDoing = false;
					}
					else if (Lty <= Yls)LDoing = false;
				};
				if (!LDoing)RightPrefer = false;
			};
			Lcum1 = Lcum;
		};
		//LLock[y][x]=true;
		if (Rpp < MaxP - 9)
		{
			if (RightPrefer)
			{
				Pps = Rppm;
				//search for direct vision point
				int i;
				for (i = 0; i < RVisPos&&RVIS[i] < Pps; i++);

				if (i < RVisPos)
				{
					Rvp = RVIS[i];
				}
				else
				{
					Rvp = Rpp - 1;
				}
				if (Rvp < 0)
				{
					Rvp = 0;
				}
			}
			else
			{
				Pps = Lppm;
				//Rvp=Lvp;
				//search for direct vision point
				int i;
				for (i = 0; i < LVisPos&&LVIS[i] < Pps; i++);

				if (i < LVisPos)
				{
					Rvp = LVIS[i];
				}
				else
				{
					Rvp = Rpp - 1;
				}
				if (Rvp < 0)
				{
					Rvp = 0;
				}
			}
			if (Rvp < Rpp)Rvp++;
			if (Rvp < Pps)Rvp = Pps;
			int addNIPoints = Rvp - Pps + 1;
			int maxp = addNIPoints;
			CurIPoint = 0;
			if (addNIPoints > 2) {
				addNIPoints = 2 + ((addNIPoints - 2) >> sssh);
			};
			short* OldPathX = PathX;
			short* OldPathY = PathY;
			PathX = new short[NIPoints + addNIPoints];
			PathY = new short[NIPoints + addNIPoints];
			if (OldPathX) {
				memcpy(PathX, OldPathX, NIPoints << 1);
				memcpy(PathY, OldPathY, NIPoints << 1);
				free(OldPathX);
				free(OldPathY);
			};
			if (RightPrefer) {
				if (maxp <= 2) {
					PathX[NIPoints] = Rpx[Pps + addNIPoints - 1];
					PathY[NIPoints] = Rpy[Pps + addNIPoints - 1];
					if (maxp == 2) {
						PathX[NIPoints + 1] = Rpx[Pps];
						PathY[NIPoints + 1] = Rpy[Pps];
					};
					NIPoints += addNIPoints;
				}
				else {
					int np = ((maxp - 2) >> sssh) + 1;
					for (int nn = 0; nn < np; nn++) {
						PathX[NIPoints + np - nn] = Rpx[Pps + (nn << sssh)];
						PathY[NIPoints + np - nn] = Rpy[Pps + (nn << sssh)];
					};
					PathX[NIPoints] = Rpx[Pps + maxp - 1];
					PathY[NIPoints] = Rpy[Pps + maxp - 1];
					NIPoints += addNIPoints;
				};
			}
			else {
				if (maxp <= 2) {
					PathX[NIPoints] = Lpx[Pps + addNIPoints - 1];
					PathY[NIPoints] = Lpy[Pps + addNIPoints - 1];
					if (maxp == 2) {
						PathX[NIPoints + 1] = Lpx[Pps];
						PathY[NIPoints + 1] = Lpy[Pps];
					};
					NIPoints += addNIPoints;
				}
				else {
					int np = ((maxp - 2) >> sssh) + 1;
					for (int nn = 0; nn < np; nn++) {
						PathX[NIPoints + np - nn] = Lpx[Pps + (nn << sssh)];
						PathY[NIPoints + np - nn] = Lpy[Pps + (nn << sssh)];
					};
					PathX[NIPoints] = Lpx[Pps + maxp - 1];
					PathY[NIPoints] = Lpy[Pps + maxp - 1];
					NIPoints += addNIPoints;
				};
			};
			NeedPath = true;
			if (GLock)MFI->BSetBar(x, y, Lx);
			return true;
		}
		else {
			if (GLock)MFI->BSetBar(x, y, Lx);
			PathDelay = 200 + (rando() & 15);
			return false;
		};
	};
	if (GLock)MFI->BSetBar(x, y, Lx);
	return false;
};
#undef sssh
#define sssh 2
// Water variant: CreatePrePath forwards here for LockType == 1 (ships).
// Same wall-follow algorithm as CreatePrePath, but keeps the direct-sight
// points (Rvp/Lvp) on the fly instead of in the RVIS/LVIS lists, and uses
// a shorter retry PathDelay (64+ vs 200+).
bool OneObject::CreatePrePath2(int x1, int y1) {
	if (PathDelay)return false;
	if (abs(x - x1) < 2 && abs(y - y1) < 2)return false;
	MotionField* MFI = &MFIELDS[LockType];
	if (GLock)MFI->BClrBar(x, y, Lx);
	bool InLocked = MFI->CheckBar(x, y, Lx, Lx);
	if (InLocked&&PathX) {
		if (GLock)MFI->BClrBar(x, y, Lx);
		return false;
	};
	int sdx = x1 - x;
	int	sdy = y1 - y;
	int	Cum = 0;
	int Pps = 0;
	int sx = (sdx > 0) ? 1 : -1;
	int sy = (sdy > 0) ? 1 : -1;
	int dx = abs(sdx);
	int dy = abs(sdy);
	int	Mdx = dx;
	int	Mdy = dy;
	int	Mx = x;
	int	My = y;
	int	xx1 = x;
	int yy1 = y;
	int rx = sx;
	int ry = sy;
	if (dx > dy)ry = 0;
	if (dy > dx)rx = 0;
	int Angle0 = GetLAngle(x1 - x, y1 - y, 0);
	int Angle;
	int ddx, ddy;
	int Lvp = 0;
	int Rvp = 0;
	bool LvpLast = false;
	bool RvpLast = false;
	int RvpDist = 100000;
	int LvpDist = 100000;
	//if(PathX)free(PathX);
	//if(PathY)free(PathY);
	//PathX=NULL;
	//PathY=NULL;
	//NIPoints=0;
	CurIPoint = 0;
	NeedPath = false;
	//соединяем линией начальную и конечную точки. 
	//Оптимизация только по скорости
	//Linux translation of the asm Bresenham walker: fills pxx/pyy with the
	//point line from (Mx,My) toward the target; Pps = number of points.
	//The asm's eight direction cases reduce to sx/sy sign multipliers, and
	//the 16-bit cum counter is safe (cum < Mdx + Mdy, map-sized values).
	{
		int nn = 0;
		pxx[nn] = (word)Mx;
		pyy[nn] = (word)My;
		nn++;
		if (Mdx > Mdy) {
			for (int i = 0; i < Mdx; i++) {
				Mx += sx;
				Cum += Mdy;
				if (Cum >= Mdx) {
					Cum -= Mdx;
					My += sy;
				};
				pxx[nn] = (word)Mx;
				pyy[nn] = (word)My;
				nn++;
			};
		}
		else {
			for (int i = 0; i < Mdy; i++) {
				My += sy;
				Cum += Mdx;
				if (Cum >= Mdy) {
					Cum -= Mdy;
					Mx += sx;
				};
				pxx[nn] = (word)Mx;
				pyy[nn] = (word)My;
				nn++;
			};
		};
		Pps = nn;
	}
	Pps--;
	//assert(Pps<MaxP);
	if (InLocked) {
		int uu;
		for (uu = 0; uu < Pps&&MFI->CheckBar(pxx[uu], pyy[uu], Lx, Lx); uu++);
		if (uu >= Pps)return false;
		PathX = new short;
		PathY = new short;
		PathX[0] = pxx[uu];
		PathY[0] = pyy[uu];
		NIPoints = 1;
		return true;
	};
	bool RightPrefer = true;
	int Rtx;//current point 
	int Rty;
	int Ltx;
	int Lty;
	word Rpx[MaxP];//right path
	word Rpy[MaxP];
	word Lpx[MaxP];//left path
	word Lpy[MaxP];
	int Rpp = 1;//index of current point
	bool LDoing;//=true if last point reached
	bool RDoing;
	byte Rdirc;//currend direction
	byte Ldirc;
	int Rmaxalt;//maximum alteration,right path
	int Lmaxalt;//maximum alteration,left path
	int Rcum = 0;
	int Rcum1 = 0;
	int Lcum = 0;
	int Lcum1 = 0;
	bool Rvis, Lvis;
	//Проверяем прямую проходимость
	int uu;
	for (uu = 1; uu < Pps && !MFI->CheckBar(pxx[uu], pyy[uu], Lx, Lx); uu++);
	if (uu == Pps)return false;
	//Идем, пока не упремся в стенку
	for (uu = 1; uu < Pps && !MFI->CheckBar(pxx[uu] - 1, pyy[uu] - 1, Lx + 2, Lx + 2); uu++);
	Rpp = uu;
	Rtx = pxx[Rpp];
	Rty = pyy[Rpp];
	int Rppm = uu;
	int Lppm = uu;
	// Если dx>dy,то на каждом шагу dx изменяетя строго на 1
	if (Rtx != x1 || Rty != y1) {
		//LLock[y][x]=false;
		if (Rpp > 0)Rpp -= 1;
		Rtx = pxx[Rpp];
		Rty = pyy[Rpp];
		Ltx = pxx[Rpp];
		Lty = pyy[Rpp];
		int Ppi = Rpp + 1;
		LDoing = true;
		RDoing = true;
		//Ищем, пока находимся в занятой зоне
		while (MFI->CheckBar(pxx[Ppi] - 1, pyy[Ppi] - 1, Lx + 2, Lx + 2) && Ppi < Pps)Ppi++;
		if (Ppi > Pps)LDoing = false;//Кон.точка недостижима
		int Xls = pxx[Ppi - 1];
		int Yls = pyy[Ppi - 1];
		//Уперлись...Вычисляем направление движения
		Rdirc = ndrr[(pxx[Rpp + 1] - pxx[Rpp] + 1) * 3 + pyy[Rpp + 1] - pyy[Rpp] + 1];
		Ldirc = Rdirc;
		//Выбираем начальное направление-right
		MFI->FillNewCirc(Rtx - 1, Rty - 1, Lx + 2, Lx + 2);
		int dirc1 = (Rdirc + 1) & 7;
		for (int z = 0; NewCirc[dirc1] && z < 7; dirc1++, z++);
		if (!NewCirc[6 + (dirc1 & 7)])dirc1++;
		Rdirc = dirc1 & 7;
		//-left
		dirc1 = 8 + ((Ldirc + 7) & 7);
		for (int z = 0; NewCirc[dirc1] && z < 7; dirc1--, z++);
		if (!NewCirc[(dirc1 & 7) + 2])dirc1--;
		Ldirc = dirc1 & 7;
		//Идем по правому краю до тех пор пока вновь не пересе-
		//чемся с прямой линией, соединяющей начальную и конечную
		//точки
		Rmaxalt = 0;
		Lmaxalt = 0;
		while (Rpp < MaxP - 8 && LDoing&&RDoing) {
			//пытаемся повернуть направо
			MFI->FillNewCirc(Rtx - 1, Rty - 1, Lx + 2, Lx + 2);
			int dirc1 = (Rdirc + 6) & 7;
			for (int z = 0; z < 6 && NewCirc[dirc1]; dirc1++, z++);
			Rdirc = dirc1 & 7;
			Rpp++;
			int Tdx = nidrx[Rdirc];
			int Tdy = nidry[Rdirc];
			Rcum -= sdy*Tdx;
			Rcum += sdx*Tdy;
			Rtx += Tdx;
			Rty += Tdy;
			Rpx[Rpp] = Rtx; Rpy[Rpp] = Rty;
			Angle = GetLAngle(Rtx - x, Rty - y, Angle0);
			if (Angle > Rmaxalt) {
				Rmaxalt = Angle;
				Rppm = Rpp;
			};
			//проверяем условие прямой видимости
			ddx = x1 - Rtx;
			ddy = y1 - Rty;
			if (ddx > 1)ddx = 1;
			if (ddx < -1)ddx = -1;
			if (ddy > 1)ddy = 1;
			if (ddy < -1)ddy = -1;
			int dirvi = ndrr[ddx + ddx + ddx + ddy + 4];
			if (!NewCirc[dirvi]) {
				Rvis = true;
				if (!RvpLast) {
					//int distant=Norma(x1-Rtx,y1-Rty);
					//if(RvpDist>distant){
					Rvp = Rpp;
					RvpLast = true;
					//	RvpDist=distant;
					//}else{
					//	RvpLast=true;
					//};
				};
			}
			else {
				RvpLast = false;
				Rvis = false;
			};
			//the same, but left
			MFI->FillNewCirc(Ltx - 1, Lty - 1, Lx + 2, Lx + 2);
			dirc1 = 8 + ((Ldirc + 2) & 7);
			for (int z = 0; z < 6 && NewCirc[dirc1]; dirc1--, z++);
			Ldirc = dirc1 & 7;
			Tdx = nidrx[Ldirc];
			Tdy = nidry[Ldirc];
			Lcum += sdy*Tdx;
			Lcum -= sdx*Tdy;
			Ltx += Tdx;
			Lty += Tdy;
			Lpx[Rpp] = Ltx; Lpy[Rpp] = Lty;
			Angle = GetRAngle(Ltx - x, Lty - y, Angle0);
			if (Angle > Lmaxalt) {
				Lmaxalt = Angle;
				Lppm = Rpp;
			};
			//проверяем условие прямой видимости
			ddx = x1 - Ltx;
			ddy = y1 - Lty;
			if (ddx > 1)ddx = 1;
			if (ddx < -1)ddx = -1;
			if (ddy > 1)ddy = 1;
			if (ddy < -1)ddy = -1;
			dirvi = ndrr[ddx + ddx + ddx + ddy + 4];
			if (!NewCirc[dirvi]) {
				Lvis = true;
				if (!LvpLast) {
					//int distant=Norma(x1-Ltx,y1-Lty);
					//if(LvpDist>distant){
					Lvp = Rpp;
					LvpLast = true;
					//	LvpDist=distant;
					//}else LvpLast=true;
				};
			}
			else {
				LvpLast = false;
				Lvis = false;
			};
			//Вычисляем значение y на прямой линии, соотв. 
			//данному x
			if (Rcum <= 0 && Rcum1 >= 0) {
				if (!Rvis)Rvp = Rpp;
				if (dx > dy) {
					if (sx > 0) {
						if (Rtx >= Xls)RDoing = false;
					}
					else if (Rtx <= Xls)RDoing = false;
				}
				else {
					if (sy > 0) {
						if (Rty >= Yls)RDoing = false;
					}
					else if (Rty <= Yls)RDoing = false;
				};
				if (!RDoing)RightPrefer = true;
			};
			Rcum1 = Rcum;
			//the same for left
			if (Lcum <= 0 && Lcum1 >= 0) {
				if (!Lvis)Lvp = Rpp;
				if (dx > dy) {
					if (sx > 0) {
						if (Ltx >= Xls)LDoing = false;
					}
					else if (Ltx <= Xls)LDoing = false;
				}
				else {
					if (sy > 0) {
						if (Lty >= Yls)LDoing = false;
					}
					else if (Lty <= Yls)LDoing = false;
				};
				if (!LDoing)RightPrefer = false;
			};
			Lcum1 = Lcum;
		};
		//LLock[y][x]=true;
		if (Rpp < MaxP - 9) {
			if (RightPrefer) {
				//if(Rppm+1<Rpp)Rppm+=1;else Rppm=Rpp;
				//memcpy(xx,Rpx,(Rppm+1));
				//memcpy(yy,Rpy,(Rppm+1));
				Pps = Rppm;
			}
			else {
				//if(Lppm+1<Rpp)Lppm+=1;else Lppm=Rpp;
				//memcpy(xx,Lpx,(Lppm+1));
				//memcpy(yy,Lpy,(Lppm+1));
				Pps = Lppm;
				Rvp = Lvp;
				//for(int G=Pps;G<=Rpp;G++)assert(Lpx[G]<400&&Lpx[G]>0);
			};
			if (Rvp < Rpp)Rvp++;
			if (Rvp < Pps)Rvp = Pps;
			int addNIPoints = Rvp - Pps + 1;
			int maxp = addNIPoints;
			CurIPoint = 0;
			if (addNIPoints > 2) {
				addNIPoints = 2 + ((addNIPoints - 2) >> sssh);
			};
			short* OldPathX = PathX;
			short* OldPathY = PathY;
			PathX = new short[NIPoints + addNIPoints];
			PathY = new short[NIPoints + addNIPoints];
			if (OldPathX) {
				memcpy(PathX, OldPathX, NIPoints << 1);
				memcpy(PathY, OldPathY, NIPoints << 1);
				free(OldPathX);
				free(OldPathY);
			};
			if (RightPrefer) {
				if (maxp <= 2) {
					PathX[NIPoints] = Rpx[Pps + addNIPoints - 1];
					PathY[NIPoints] = Rpy[Pps + addNIPoints - 1];
					if (maxp == 2) {
						PathX[NIPoints + 1] = Rpx[Pps];
						PathY[NIPoints + 1] = Rpy[Pps];
					};
					NIPoints += addNIPoints;
				}
				else {
					int np = ((maxp - 2) >> sssh) + 1;
					for (int nn = 0; nn < np; nn++) {
						PathX[NIPoints + np - nn] = Rpx[Pps + (nn << sssh)];
						PathY[NIPoints + np - nn] = Rpy[Pps + (nn << sssh)];
					};
					PathX[NIPoints] = Rpx[Pps + maxp - 1];
					PathY[NIPoints] = Rpy[Pps + maxp - 1];
					NIPoints += addNIPoints;
				};
			}
			else {
				if (maxp <= 2) {
					PathX[NIPoints] = Lpx[Pps + addNIPoints - 1];
					PathY[NIPoints] = Lpy[Pps + addNIPoints - 1];
					if (maxp == 2) {
						PathX[NIPoints + 1] = Lpx[Pps];
						PathY[NIPoints + 1] = Lpy[Pps];
					};
					NIPoints += addNIPoints;
				}
				else {
					int np = ((maxp - 2) >> sssh) + 1;
					for (int nn = 0; nn < np; nn++) {
						PathX[NIPoints + np - nn] = Lpx[Pps + (nn << sssh)];
						PathY[NIPoints + np - nn] = Lpy[Pps + (nn << sssh)];
					};
					PathX[NIPoints] = Lpx[Pps + maxp - 1];
					PathY[NIPoints] = Lpy[Pps + maxp - 1];
					NIPoints += addNIPoints;
				};
			};
			NeedPath = true;
			if (GLock)MFI->BSetBar(x, y, Lx);
			return true;
		}
		else {
			if (GLock)MFI->BSetBar(x, y, Lx);
			PathDelay = 64 + (rando() & 15);
			return false;
		};
	};
	if (GLock)MFI->BSetBar(x, y, Lx);
	return false;
};
#undef sssh
#define sssh 3
// DEAD CODE: CreatePrePath4 has no callers anywhere (only declared in
// mapdiscr.h). It is a copy of CreatePrePath2 with coarser waypoint
// subsampling (every 8th point, sssh = 3). Leftover experiment; safe to
// ignore when porting.
bool OneObject::CreatePrePath4(int x1, int y1) {
	if (PathDelay)return false;
	if (abs(x - x1) < 2 && abs(y - y1) < 2)return false;
	MotionField* MFI = &MFIELDS[LockType];
	if (GLock)MFI->BClrBar(x, y, Lx);
	bool InLocked = MFI->CheckBar(x, y, Lx, Lx);
	if (InLocked&&PathX) {
		if (GLock)MFI->BClrBar(x, y, Lx);
		return false;
	};
	int sdx = x1 - x;
	int	sdy = y1 - y;
	int	Cum = 0;
	int Pps = 0;
	int sx = (sdx > 0) ? 1 : -1;
	int sy = (sdy > 0) ? 1 : -1;
	int dx = abs(sdx);
	int dy = abs(sdy);
	int	Mdx = dx;
	int	Mdy = dy;
	int	Mx = x;
	int	My = y;
	int	xx1 = x;
	int yy1 = y;
	int rx = sx;
	int ry = sy;
	if (dx > dy)ry = 0;
	if (dy > dx)rx = 0;
	int Angle0 = GetLAngle(x1 - x, y1 - y, 0);
	int Angle;
	int ddx, ddy;
	int Lvp = 0;
	int Rvp = 0;
	bool LvpLast = false;
	bool RvpLast = false;
	int RvpDist = 100000;
	int LvpDist = 100000;
	//if(PathX)free(PathX);
	//if(PathY)free(PathY);
	//PathX=NULL;
	//PathY=NULL;
	//NIPoints=0;
	CurIPoint = 0;
	NeedPath = false;
	//соединяем линией начальную и конечную точки. 
	//Оптимизация только по скорости
	//Linux translation of the asm Bresenham walker: fills pxx/pyy with the
	//point line from (Mx,My) toward the target; Pps = number of points.
	//The asm's eight direction cases reduce to sx/sy sign multipliers, and
	//the 16-bit cum counter is safe (cum < Mdx + Mdy, map-sized values).
	{
		int nn = 0;
		pxx[nn] = (word)Mx;
		pyy[nn] = (word)My;
		nn++;
		if (Mdx > Mdy) {
			for (int i = 0; i < Mdx; i++) {
				Mx += sx;
				Cum += Mdy;
				if (Cum >= Mdx) {
					Cum -= Mdx;
					My += sy;
				};
				pxx[nn] = (word)Mx;
				pyy[nn] = (word)My;
				nn++;
			};
		}
		else {
			for (int i = 0; i < Mdy; i++) {
				My += sy;
				Cum += Mdx;
				if (Cum >= Mdy) {
					Cum -= Mdy;
					Mx += sx;
				};
				pxx[nn] = (word)Mx;
				pyy[nn] = (word)My;
				nn++;
			};
		};
		Pps = nn;
	}
	Pps--;
	//assert(Pps<MaxP);
	if (InLocked)
	{
		int uu;
		for (uu = 0; uu < Pps&&MFI->CheckBar(pxx[uu], pyy[uu], Lx, Lx); uu++);
		if (uu >= Pps)return false;
		PathX = new short;
		PathY = new short;
		PathX[0] = pxx[uu];
		PathY[0] = pyy[uu];
		NIPoints = 1;
		return true;
	};
	bool RightPrefer = true;
	int Rtx;//current point 
	int Rty;
	int Ltx;
	int Lty;
	word Rpx[MaxP];//right path
	word Rpy[MaxP];
	word Lpx[MaxP];//left path
	word Lpy[MaxP];
	int Rpp = 1;//index of current point
	bool LDoing;//=true if last point reached
	bool RDoing;
	byte Rdirc;//currend direction
	byte Ldirc;
	int Rmaxalt;//maximum alteration,right path
	int Lmaxalt;//maximum alteration,left path
	int Rcum = 0;
	int Rcum1 = 0;
	int Lcum = 0;
	int Lcum1 = 0;
	bool Rvis, Lvis;
	//Проверяем прямую проходимость
	int uu;
	for (uu = 1; uu < Pps && !MFI->CheckBar(pxx[uu], pyy[uu], Lx, Lx); uu++);
	if (uu == Pps)return false;
	//Идем, пока не упремся в стенку
	for (uu = 1; uu < Pps && !MFI->CheckBar(pxx[uu] - 1, pyy[uu] - 1, Lx + 2, Lx + 2); uu++);
	Rpp = uu;
	Rtx = pxx[Rpp];
	Rty = pyy[Rpp];
	int Rppm = uu;
	int Lppm = uu;
	// Если dx>dy,то на каждом шагу dx изменяетя строго на 1
	if (Rtx != x1 || Rty != y1) {
		//LLock[y][x]=false;
		if (Rpp > 0)Rpp -= 1;
		Rtx = pxx[Rpp];
		Rty = pyy[Rpp];
		Ltx = pxx[Rpp];
		Lty = pyy[Rpp];
		int Ppi = Rpp + 1;
		LDoing = true;
		RDoing = true;
		//Ищем, пока находимся в занятой зоне
		while (MFI->CheckBar(pxx[Ppi] - 1, pyy[Ppi] - 1, Lx + 2, Lx + 2) && Ppi < Pps)Ppi++;
		if (Ppi > Pps)LDoing = false;//Кон.точка недостижима
		int Xls = pxx[Ppi - 1];
		int Yls = pyy[Ppi - 1];
		//Уперлись...Вычисляем направление движения
		Rdirc = ndrr[(pxx[Rpp + 1] - pxx[Rpp] + 1) * 3 + pyy[Rpp + 1] - pyy[Rpp] + 1];
		Ldirc = Rdirc;
		//Выбираем начальное направление-right
		MFI->FillNewCirc(Rtx - 1, Rty - 1, Lx + 2, Lx + 2);
		int dirc1 = (Rdirc + 1) & 7;
		for (int z = 0; NewCirc[dirc1] && z < 7; dirc1++, z++);
		if (!NewCirc[6 + (dirc1 & 7)])dirc1++;
		Rdirc = dirc1 & 7;
		//-left
		dirc1 = 8 + ((Ldirc + 7) & 7);
		for (int z = 0; NewCirc[dirc1] && z < 7; dirc1--, z++);
		if (!NewCirc[(dirc1 & 7) + 2])dirc1--;
		Ldirc = dirc1 & 7;
		//Идем по правому краю до тех пор пока вновь не пересе-
		//чемся с прямой линией, соединяющей начальную и конечную
		//точки
		Rmaxalt = 0;
		Lmaxalt = 0;
		while (Rpp < MaxP - 8 && LDoing&&RDoing) {
			//пытаемся повернуть направо
			MFI->FillNewCirc(Rtx - 1, Rty - 1, Lx + 2, Lx + 2);
			int dirc1 = (Rdirc + 6) & 7;
			for (int z = 0; z < 6 && NewCirc[dirc1]; dirc1++, z++);
			Rdirc = dirc1 & 7;
			Rpp++;
			int Tdx = nidrx[Rdirc];
			int Tdy = nidry[Rdirc];
			Rcum -= sdy*Tdx;
			Rcum += sdx*Tdy;
			Rtx += Tdx;
			Rty += Tdy;
			Rpx[Rpp] = Rtx; Rpy[Rpp] = Rty;
			Angle = GetLAngle(Rtx - x, Rty - y, Angle0);
			if (Angle > Rmaxalt) {
				Rmaxalt = Angle;
				Rppm = Rpp;
			};
			//проверяем условие прямой видимости
			ddx = x1 - Rtx;
			ddy = y1 - Rty;
			if (ddx > 1)ddx = 1;
			if (ddx < -1)ddx = -1;
			if (ddy > 1)ddy = 1;
			if (ddy < -1)ddy = -1;
			int dirvi = ndrr[ddx + ddx + ddx + ddy + 4];
			if (!NewCirc[dirvi]) {
				Rvis = true;
				if (!RvpLast) {
					//int distant=Norma(x1-Rtx,y1-Rty);
					//if(RvpDist>distant){
					Rvp = Rpp;
					RvpLast = true;
					//	RvpDist=distant;
					//}else{
					//	RvpLast=true;
					//};
				};
			}
			else {
				RvpLast = false;
				Rvis = false;
			};
			//the same, but left
			MFI->FillNewCirc(Ltx - 1, Lty - 1, Lx + 2, Lx + 2);
			dirc1 = 8 + ((Ldirc + 2) & 7);
			for (int z = 0; z < 6 && NewCirc[dirc1]; dirc1--, z++);
			Ldirc = dirc1 & 7;
			Tdx = nidrx[Ldirc];
			Tdy = nidry[Ldirc];
			Lcum += sdy*Tdx;
			Lcum -= sdx*Tdy;
			Ltx += Tdx;
			Lty += Tdy;
			Lpx[Rpp] = Ltx; Lpy[Rpp] = Lty;
			Angle = GetRAngle(Ltx - x, Lty - y, Angle0);
			if (Angle > Lmaxalt) {
				Lmaxalt = Angle;
				Lppm = Rpp;
			};
			//проверяем условие прямой видимости
			ddx = x1 - Ltx;
			ddy = y1 - Lty;
			if (ddx > 1)ddx = 1;
			if (ddx < -1)ddx = -1;
			if (ddy > 1)ddy = 1;
			if (ddy < -1)ddy = -1;
			dirvi = ndrr[ddx + ddx + ddx + ddy + 4];
			if (!NewCirc[dirvi]) {
				Lvis = true;
				if (!LvpLast) {
					//int distant=Norma(x1-Ltx,y1-Lty);
					//if(LvpDist>distant){
					Lvp = Rpp;
					LvpLast = true;
					//	LvpDist=distant;
					//}else LvpLast=true;
				};
			}
			else {
				LvpLast = false;
				Lvis = false;
			};
			//Вычисляем значение y на прямой линии, соотв. 
			//данному x
			if (Rcum <= 0 && Rcum1 >= 0) {
				if (!Rvis)Rvp = Rpp;
				if (dx > dy) {
					if (sx > 0) {
						if (Rtx >= Xls)RDoing = false;
					}
					else if (Rtx <= Xls)RDoing = false;
				}
				else {
					if (sy > 0) {
						if (Rty >= Yls)RDoing = false;
					}
					else if (Rty <= Yls)RDoing = false;
				};
				if (!RDoing)RightPrefer = true;
			};
			Rcum1 = Rcum;
			//the same for left
			if (Lcum <= 0 && Lcum1 >= 0) {
				if (!Lvis)Lvp = Rpp;
				if (dx > dy) {
					if (sx > 0) {
						if (Ltx >= Xls)LDoing = false;
					}
					else if (Ltx <= Xls)LDoing = false;
				}
				else {
					if (sy > 0) {
						if (Lty >= Yls)LDoing = false;
					}
					else if (Lty <= Yls)LDoing = false;
				};
				if (!LDoing)RightPrefer = false;
			};
			Lcum1 = Lcum;
		};
		//LLock[y][x]=true;
		if (Rpp < MaxP - 9) {
			if (RightPrefer) {
				//if(Rppm+1<Rpp)Rppm+=1;else Rppm=Rpp;
				//memcpy(xx,Rpx,(Rppm+1));
				//memcpy(yy,Rpy,(Rppm+1));
				Pps = Rppm;
			}
			else {
				//if(Lppm+1<Rpp)Lppm+=1;else Lppm=Rpp;
				//memcpy(xx,Lpx,(Lppm+1));
				//memcpy(yy,Lpy,(Lppm+1));
				Pps = Lppm;
				Rvp = Lvp;
				//for(int G=Pps;G<=Rpp;G++)assert(Lpx[G]<400&&Lpx[G]>0);
			};
			if (Rvp < Rpp)Rvp++;
			if (Rvp < Pps)Rvp = Pps;
			int addNIPoints = Rvp - Pps + 1;
			int maxp = addNIPoints;
			CurIPoint = 0;
			if (addNIPoints > 2) {
				addNIPoints = 2 + ((addNIPoints - 2) >> sssh);
			};
			short* OldPathX = PathX;
			short* OldPathY = PathY;
			PathX = new short[NIPoints + addNIPoints];
			PathY = new short[NIPoints + addNIPoints];
			if (OldPathX) {
				memcpy(PathX, OldPathX, NIPoints << 1);
				memcpy(PathY, OldPathY, NIPoints << 1);
				free(OldPathX);
				free(OldPathY);
			};
			if (RightPrefer) {
				if (maxp <= 2) {
					PathX[NIPoints] = Rpx[Pps + addNIPoints - 1];
					PathY[NIPoints] = Rpy[Pps + addNIPoints - 1];
					if (maxp == 2) {
						PathX[NIPoints + 1] = Rpx[Pps];
						PathY[NIPoints + 1] = Rpy[Pps];
					};
					NIPoints += addNIPoints;
				}
				else {
					int np = ((maxp - 2) >> sssh) + 1;
					for (int nn = 0; nn < np; nn++) {
						PathX[NIPoints + np - nn] = Rpx[Pps + (nn << sssh)];
						PathY[NIPoints + np - nn] = Rpy[Pps + (nn << sssh)];
					};
					PathX[NIPoints] = Rpx[Pps + maxp - 1];
					PathY[NIPoints] = Rpy[Pps + maxp - 1];
					NIPoints += addNIPoints;
				};
			}
			else {
				if (maxp <= 2) {
					PathX[NIPoints] = Lpx[Pps + addNIPoints - 1];
					PathY[NIPoints] = Lpy[Pps + addNIPoints - 1];
					if (maxp == 2) {
						PathX[NIPoints + 1] = Lpx[Pps];
						PathY[NIPoints + 1] = Lpy[Pps];
					};
					NIPoints += addNIPoints;
				}
				else {
					int np = ((maxp - 2) >> sssh) + 1;
					for (int nn = 0; nn < np; nn++) {
						PathX[NIPoints + np - nn] = Lpx[Pps + (nn << sssh)];
						PathY[NIPoints + np - nn] = Lpy[Pps + (nn << sssh)];
					};
					PathX[NIPoints] = Lpx[Pps + maxp - 1];
					PathY[NIPoints] = Lpy[Pps + maxp - 1];
					NIPoints += addNIPoints;
				};
			};
			NeedPath = true;
			if (GLock)MFI->BSetBar(x, y, Lx);
			return true;
		}
		else {
			if (GLock)MFI->BSetBar(x, y, Lx);
			PathDelay = 64 + (rando() & 15);
			return false;
		};
	};
	if (GLock)MFI->BSetBar(x, y, Lx);
	return false;
};
// Push a unit that ended up standing INSIDE a blocked area (e.g. a wall
// was completed on top of it) back out: scan rings of precomputed offsets
// (Rarr, topograf.h) at growing distance from its current cell for the
// first unblocked position for its footprint and teleport it there
// (updates x/y and the pixel RealX/RealY). Called from newmon.cpp.
void CorrectLockPosition(OneObject* OB) {
	MotionField* MFI = MFIELDS + OB->LockType;
	int x0 = OB->x;
	int y0 = OB->y;
	int Lx = OB->Lx;
	for (int r = 1; r < 40; r++) {
		char* xi = Rarr[r].xi;
		char* yi = Rarr[r].yi;
		int N = Rarr[r].N;
		for (int j = 0; j < N; j++) {
			int x1 = x0 + xi[j];
			int y1 = y0 + yi[j];
			if (!MFI->CheckBar(x1, y1, Lx, Lx)) {
				OB->x = x1;
				OB->y = y1;
				OB->RealX = (x1 << 8) + (Lx << 7);
				OB->RealY = (y1 << 8) + (Lx << 7);
				return;
			};
		};
	};
};
#undef CreatePath
// Main entry point of the pathfinder, called by all movement/order code
// (newmon.cpp, transport.cpp) with a target motion cell. CPdestX/CPdestY
// cache the destination of the current path:
//  - same destination again: reuse the existing path, just consuming the
//    remaining waypoints (and re-planning via CreateFullPath only if the
//    next waypoint has meanwhile become blocked);
//  - new destination: free the old path and plan a fresh one.
// OneObject::PathX/PathY hold the waypoints in cell coordinates; the
// walker in newmon.cpp consumes them from the end (NIPoints--) and heads
// for PathX[NIPoints-1] via DestX/DestY (cell << 8 + centering Lx << 7).
// UnlimitedMotion units (projectiles/birds) skip all planning and just
// get DestX/DestY set.
void OneObject::CreatePath(int x1, int y1) {


	if (UnlimitedMotion) {
		DestX = (x1 << 8) + ((Lx) << 7);
		DestY = (y1 << 8) + ((Lx) << 7);
		CPdestX = (word)-1;
		CPdestY = (word)-1;
		return;
	};
	MotionField* MFI = &MFIELDS[LockType];
	if (CPdestX == x1&&CPdestY == y1) {
		if (PathX) {
			if (NIPoints) {
				//if(MFI->CheckBar(x,y,Lx,Lx)){
					//BClrBar(x,y,Lx);         //???CHECK???
				//	CorrectLockPosition(this);
				//};
				int cx = PathX[NIPoints - 1];
				int cy = PathY[NIPoints - 1];
				int dis = DistTo(cx, cy);
				//if(dis<2&&NIPoints>1)NIPoints--;
				//else if((!dis))NIPoints--;
				if (dis < 2)NIPoints--;
				if (NIPoints) {
					cx = PathX[NIPoints - 1];
					cy = PathY[NIPoints - 1];
					if (MFI->CheckBar(cx, cy, Lx, Lx)) {
						NIPoints = 0;
						free(PathX);
						free(PathY);
						PathX = NULL;
						PathY = NULL;

						CreateFullPath(x1, y1, this);
					}
					else {
						DestX = (cx << 8) + ((Lx) << 7);
						DestY = (cy << 8) + ((Lx) << 7);
					};
				}
				else {
					free(PathX);
					free(PathY);
					PathX = NULL;
					PathY = NULL;

					CreateFullPath(x1, y1, this);
					if (NIPoints == 1) {
						int cx1 = PathX[0];
						int cy1 = PathY[0];
						if (Norma(cx1 - x, cy1 - y) < 3) {
							free(PathX);
							free(PathY);
							PathX = NULL;
							PathY = NULL;
							NIPoints = 0;
						};
					};
				};
			}
			else {

				CreateFullPath(x1, y1, this);
			};
		}
		else {
			DestX = (x1 << 8) + ((Lx) << 7);
			DestY = (y1 << 8) + ((Lx) << 7);
		};
	}
	else {
		if (PathX) {
			free(PathX);
			free(PathY);
			NIPoints = 0;
			PathX = NULL;
			PathY = NULL;
		};

		CreateFullPath(x1, y1, this);
	};
	CPdestX = x1;
	CPdestY = y1;
};
//void OneObject::CreateSimplePath(int x,int y){
//
//};

