
#include "ddini.h"
#include "resfile.h"
#include "fastdraw.h"
#include "mgraph.h"
#include "mouse.h"
#include "menu.h"
#include "mapdiscr.h"
#include "fog.h"
#include "megapolis.h"

#include <assert.h>
#include "walls.h"
#include "mode.h"
#include "gsound.h"
#include "mapsprites.h"
#include "newmon.h"
#include "math.h"
#include "gp_draw.h"
#include "realwater.h"
#include "newupgrade.h"
#include "zbuffer.h"
#include "path.h"
#include "transport.h"
#include "3dbars.h"
#include "cdirsnd.h"
#include "newai.h"
#include "topograf.h"
#include "danger.h"
#include "strategyresearch.h"
#include "safety.h"
#include "einfoclass.h"
class BuildingAI
{
public:
	word ID;
	word SN;
	short sx;
	short sy;
	int MoreInfo;
};
class BuildingProject
{
public:
	short apprX;
	short apprY;
	byte Stage;//0-search place,1-removing units,2-search for builders
	int Time;
	int PrecX;
	int PrecY;
	int PushOutRadius;
	int Razbros;
};
class Kingdom
{
public:
	word NSklad;
	word MaxSklad;
	BuildingAI* Sklads;
	word NFarms;
	word MaxFarm;
	word OnLandPeasants[1024];
	word NPeasants;
	bool BuildProjectPresent;
	BuildingProject BPR;
};
//01-wood(trees>2)
//02-stone(stones>5)
//04-full empty
//08-empty from locking
//16-plane zone
//32-high plate
byte* InfoMap;
//01-sklad
//02-farm
//04-tower
//08-building
//16-melnica
byte* CantBuild;

int* MineList;
word NMines;
word MaxMine;
void CreateInfoMap()
{
	memset( CantBuild, 0, VAL_SPRNX*VAL_SPRNX );
	int mmax = msx >> 2;
	int mmay = msy >> 2;
	for (int y = 0; y <= mmay; y++)
	{
		for (int x = 0; x <= mmax; x++)
		{
			byte ms = 0;
			if (!CheckBar( x << 3, y << 3, 8, 8 ))ms |= 8;
			int x0 = x << 7;
			int y0 = y << 7;
			int H1 = GetHeight( x0, y0 );
			int H2 = GetHeight( x0 + 128, y0 );
			int H3 = GetHeight( x0, y0 + 128 );
			int H4 = GetHeight( x0, y0 + 128 );
			int HH = ( H1 + H2 + H3 + H4 ) >> 2;
			if (abs( H1 - HH ) < 20 && abs( H2 - HH ) < 20 && abs( H3 - HH ) < 20 && abs( H4 - HH ) < 20)ms |= 16;
			//resource checking;
			int ofst = x + y*VAL_SPRNX;
			int N = NSpri[ofst];
			int NTrees = 0;
			int NStones = 0;
			if (( ms & 8 ) && ( ms & 16 ) && !N)ms |= 4;
			int* spr = SpRefs[ofst];
			for (int i = 0; i < N; i++)
			{
				word SID = spr[i];
				OneSprite* OS = &Sprites[SID];
				SprGroup* SG = OS->SG;
				ObjCharacter* OC = &SG->ObjChar[OS->SGIndex];
				if (OC->ResType == TreeID)NTrees++;
				if (OC->ResType == StoneID)NStones++;
			};
			if (NTrees > 2)ms |= 1;
			if (NStones > 5)ms |= 2;
			if (NTrees || NStones)ms &= ~4;
			InfoMap[ofst] = ms;
		};
	};
	if (MineList)free( MineList );
	NMines = 0;
	MineList = new int[256];
	MaxMine = 256;
	for (int i = 0; i < MaxSprt; i++)
	{
		OneSprite* OS = &Sprites[i];
		if (OS->Enabled)
		{
			SprGroup* SG = OS->SG;
			ObjCharacter* OC = &SG->ObjChar[OS->SGIndex];
			if (OC->IntResType < 8)
			{
				if (NMines >= MaxMine)
				{
					MaxMine += 256;
					MineList = (int*) realloc( MineList, MaxMine << 2 );
				};
				MineList[NMines] = i;
				NMines++;
			};
		};
	};
};
//x,y-pixel coordinates
void CHKS();
void RenewInfoMap( int x, int y )
{
	x = ( x >> 7 ) - 1;
	y = ( y >> 7 ) - 1;
	for (int dx = 0; dx < 3; dx++)
	{
		for (int dy = 0; dy < 3; dy++)
		{
			int sx = x + dx;
			int sy = y + dy;
			if (sx >= 0 && sy >= 0)
			{
				int ofst = sx + sy*VAL_SPRNX;
				byte ms = InfoMap[ofst];
				ms &= 16;
				if (!CheckBar( sx << 3, sy << 3, 8, 8 ))ms |= 8;
				int N = NSpri[ofst];
				int NTrees = 0;
				int NStones = 0;
				if (( ms & 8 ) && ( ms & 16 ) && !N)ms |= 4;
				int* spr = SpRefs[ofst];
				for (int i = 0; i < N; i++)
				{
					word SID = spr[i];
					OneSprite* OS = &Sprites[SID];
					SprGroup* SG = OS->SG;
					ObjCharacter* OC = &SG->ObjChar[OS->SGIndex];
					if (OC->ResType == TreeID)NTrees++;
					if (OC->ResType == StoneID)NStones++;
				};
				if (NTrees)ms |= 1;
				if (NStones)ms |= 2;
				InfoMap[ofst] = ms;
			};
		};
	};
};
void FreeInfoMap()
{
	free( MineList );
};
//special search procedures
//1.Melnica&field

int SsMaxX;
int SsMaxY;
#define FieldSX 2
bool CheckMill( int x, int y )
{
	if (x <= FieldSX || y <= FieldSX || x >= SsMaxX - FieldSX || y >= SsMaxY - FieldSX)return false;
	int ofst = x + y*VAL_SPRNX;
	if (CantBuild[ofst] & CB_Mill)return false;
	ofst -= FieldSX + VAL_SPRNX*FieldSX;
	for (int dy = -FieldSX; dy <= FieldSX; dy++)
	{
		for (int dx = -FieldSX; dx <= FieldSX; dx++)
		{
			if (!( InfoMap[ofst] & 4 ))return false;
			ofst++;
		};
		ofst += VAL_SPRNX - FieldSX - FieldSX - 1;
	};
	return true;
};
bool CheckStoneSklad( int x, int y )
{
	if (x <= 1 || y <= 1 || x >= SsMaxX - 1 || y >= SsMaxY - 1)return false;
	int ofst = x + y*VAL_SPRNX;
	if (CantBuild[ofst] & CB_Sklad)return false;
	if (!( InfoMap[ofst] & 4 ))return false;
	if (( InfoMap[ofst - VAL_SPRNX] & 2 ) || ( InfoMap[ofst + VAL_SPRNX] & 2 ) ||
		( InfoMap[ofst + 1] & 2 ) || ( InfoMap[ofst - 1] & 2 ) ||
		( InfoMap[ofst - VAL_SPRNX - 1] & 2 ) || ( InfoMap[ofst - VAL_SPRNX + 1] & 2 ) ||
		( InfoMap[ofst + VAL_SPRNX - 1] & 2 ) || ( InfoMap[ofst - VAL_SPRNX + 1] & 2 ))return true;
	return false;
};
bool CheckWoodSklad( int x, int y )
{
	if (x <= 1 || y <= 1 || x >= SsMaxX - 1 || y >= SsMaxY - 1)return false;
	int ofst = x + y*VAL_SPRNX;
	if (CantBuild[ofst] & CB_Sklad)return false;
	if (!( InfoMap[ofst] & 4 ))return false;
	if (( InfoMap[ofst - VAL_SPRNX] & 1 ) || ( InfoMap[ofst + VAL_SPRNX] & 1 ) ||
		( InfoMap[ofst + 1] & 1 ) || ( InfoMap[ofst - 1] & 1 ) ||
		( InfoMap[ofst - VAL_SPRNX - 1] & 1 ) || ( InfoMap[ofst - VAL_SPRNX + 1] & 1 ) ||
		( InfoMap[ofst + VAL_SPRNX - 1] & 1 ) || ( InfoMap[ofst - VAL_SPRNX + 1] & 1 ))return true;
	return false;
};
bool CheckStoneWoodSklad( int x, int y )
{
	if (x <= 2 || y <= 2 || x >= SsMaxX - 2 || y >= SsMaxY - 2)return false;
	int ofst = ( x - 2 ) + ( y - 2 )*VAL_SPRNX;
	int of1 = ofst + 2 + ( 2 << SprShf );
	if (CantBuild[of1] & CB_Sklad)return false;
	if (!( InfoMap[of1] & 4 ))return false;
	bool tpresent = false;
	for (int ix = 0; ix < 5; ix++)
	{
		for (int iy = 0; iy < 5; iy++)
		{
			if (InfoMap[ofst + ix + ( iy << SprShf )] & 2)tpresent = true;
		};
	};
	if (!tpresent)return false;
	for (int ix = 0; ix < 5; ix++)
	{
		for (int iy = 0; iy < 5; iy++)
		{
			if (InfoMap[ofst + ix + ( iy << SprShf )] & 1)return true;
		};
	};
	/*
	if(!((InfoMap[ofst-VAL_SPRNX]&2)||(InfoMap[ofst+VAL_SPRNX]&2)||
	   (InfoMap[ofst+1]&2)||(InfoMap[ofst-1]&2)||
	   (InfoMap[ofst-VAL_SPRNX-1]&2)||(InfoMap[ofst-VAL_SPRNX+1]&2)||
	   (InfoMap[ofst+VAL_SPRNX-1]&2)||(InfoMap[ofst-VAL_SPRNX+1]&2)))return false;
	if((InfoMap[ofst-VAL_SPRNX]&1)||(InfoMap[ofst+VAL_SPRNX]&1)||
	   (InfoMap[ofst+1]&1)||(InfoMap[ofst-1]&1)||
	   (InfoMap[ofst-VAL_SPRNX-1]&1)||(InfoMap[ofst-VAL_SPRNX+1]&1)||
	   (InfoMap[ofst+VAL_SPRNX-1]&1)||(InfoMap[ofst-VAL_SPRNX+1]&1))return true;
	*/
	return false;
}

bool CheckFarm( int x, int y )
{
	if (x <= 1 || y <= 1 || x >= SsMaxX - 1 || y >= SsMaxY - 1)
	{
		return false;
	}
	int ofst = x + y*VAL_SPRNX;
	if (CantBuild[ofst] & CB_Farm)
	{
		return false;
	}
	bool retval = ( InfoMap[ofst] & 4 ) != 0;
	return retval;
}

bool CheckBuilding( int x, int y )
{
	if (x <= 1 || y <= 1 || x >= SsMaxX - 1 || y >= SsMaxY - 1)return false;
	int ofst = x + y*VAL_SPRNX;
	if (CantBuild[ofst] & CB_Building)return false;
	return ( InfoMap[ofst] & 4 ) && ( InfoMap[ofst + 1] & 4 ) && ( InfoMap[ofst + VAL_SPRNX] & 4 ) && ( InfoMap[ofst + VAL_SPRNX + 1] & 4 );
}

byte NPORTS;
short PORTSX[32];
short PORTSY[32];
bool CheckTower( int x, int y )
{
	for (int i = 0; i < NPORTS; i++)
	{
		if (Norma( int( PORTSX[i] ) - x, int( PORTSY[i] ) - y ) < 9)return false;
	};
	if (x <= 1 || y <= 1 || x >= SsMaxX - 1 || y >= SsMaxY - 1)return false;
	int ofst = x + y*VAL_SPRNX;
	if (CantBuild[ofst] & CB_Building)return false;
	return ( InfoMap[ofst] & 4 ) && ( InfoMap[ofst + 1] & 4 ) && ( InfoMap[ofst + VAL_SPRNX] & 4 ) && ( InfoMap[ofst + VAL_SPRNX + 1] & 4 );
};
bool CheckPort( int x, int y )
{
	if (x <= 5 || y <= 5 || x >= SsMaxX - 5 || y >= SsMaxY - 5)return false;
	for (int i = 0; i < NPORTS; i++)
	{
		if (Norma( int( PORTSX[i] ) - x, int( PORTSY[i] ) - y ) < 9)return false;
	};
	int ofst = x + y*VAL_SPRNX;
	if (CantBuild[ofst] & CB_Port)return false;
	x <<= 2;
	y <<= 2;
	int ofsw = x + 2 + ( y + 2 )*( MaxWX );
	int DY = MAPSX * 10;
	int DX = 20;
	return !( WaterDeep[ofsw - DY] < 128 && WaterDeep[ofsw + DY] < 128 && WaterDeep[ofsw - DX] < 128 && WaterDeep[ofsw + DX] < 128 &&
		WaterDeep[ofsw - DY + DX] < 128 && WaterDeep[ofsw - DY - DX] < 128 && WaterDeep[ofsw + DY - DX] < 128 && WaterDeep[ofsw + DY + DX] < 128 );
};
extern int CURRENTAINATION;
int GetTopology( int x, int y );
//Finds a spot for a building near (x,y) by scanning rings around the origin.
//All coordinates (xx1, yy1 in/out, SFN arguments) are "search cells" - the
//InfoMap/CantBuild grid indexed as x + y*VAL_SPRNX, playable size
//SsMaxX = msx>>2 = 60<<ADDSH cells. One search cell = 2 TopRef cells = 128 px:
//that is why GetTopology() is called with x<<7 (it shifts >>6) and TopRef is
//indexed as xx+xx + (yy<<(TopSH+1)).
//  SFN - predicate (CheckMill/CheckBuilding/CheckTower/...) in search cells,
//        true = cell is acceptable for the building;
//  r   - max ring radius; Rarr[1..r-1] are scanned (Rarr[i] = offsets at
//        integer radius i, applied here on the search grid, so ring i is
//        i search cells = 2i TopRef cells away).
//A ring cell is tested only if it is: inside the playable map, safe
//(GetSafeVal==0 - SafeMAP danger bitmap of CURRENTAINATION's EnemyInfo,
//nonzero = an enemy object can shoot there), in a valid topology area
//(TopRef < 0xFFFE), and its area passes the distance slack
//TDST - PrevTopDst < 24, where TDST = LinksDist[originArea*NAreas + cellArea].
//Same area as the origin counts as TDST=0 because the LinksDist diagonal is
//never written (stays 0xFFFF). PrevTopDst is the max area distance accepted so
//far, so cells in areas up to 24 farther than the best area found are still
//considered - this lets the search cross a gap (water/ridge) to a reachable
//area instead of rejecting everything beyond the origin's own area.
//Rings are scanned in increasing radius, so the first SFN hit is
//approximately the nearest acceptable cell. Returns true and stores the spot
//into *xx1/*yy1, or false if nothing within radius r-1 qualifies.
bool SearchPlace( int* xx1, int* yy1, SearchFunction* SFN, int r )
{
	SsMaxX = msx >> 2;
	SsMaxY = msy >> 2;
	int x = *xx1;
	int y = *yy1;
	if (SFN( x, y ))
	{
		//assert(y<17648);
		return true;
	};
	int MyTop = GetTopology( x << 7, y << 7 );
	if (MyTop >= 0xFFFE)return false;
	int REALMYTOP = MyTop;
	MyTop *= NAreas;
	int PrevTopDst = 0;
	for (int i = 1; i < r; i++)
	{
		char* xi = Rarr[i].xi;
		char* yi = Rarr[i].yi;
		int N = Rarr[i].N;
		for (int j = 0; j < N; j++)
		{
			int xx = x + xi[j];
			int yy = y + yi[j];
			if (xx > 0 && yy > 0 && xx < SsMaxX&&yy < SsMaxY)
			{
				if (!GNFO.EINF[CURRENTAINATION]->GetSafeVal( xx, yy ))
				{
					int top = TopRef[xx + xx + ( yy << ( TopSH + 1 ) )];
					if (top < 0xFFFE)
					{
						int TDST = LinksDist[MyTop + top];
						if (top == REALMYTOP)TDST = 0;
						if (TDST - PrevTopDst < 24)
						{
							if (TDST > PrevTopDst)PrevTopDst = TDST;
							if (SFN( xx, yy ))
							{
								*xx1 = xx;
								*yy1 = yy;
								return true;
							};
						};
					};
				};
			};
		};
	};
	return false;
};
//SearchPlace variant for the nation's first tower(s): same ring search, but
//only cells on the enemy-facing side of the base are considered.
//  xc, yc - base center (City::CenterX/CenterY): FindCenter() averages the
//           positions of all the nation's PEASANT units, result in search
//           cells (megapolis.cpp:2601-2605);
//  xe, ye - "enemy point", computed by the caller
//           (City::FindApproximateBuildingPlace, megapolis.cpp:1576-1604) as
//           the center of mass of ALL living objects (units AND buildings,
//           !Sdoxlo) with NNUM == 0 - nation index 0, i.e. the human player
//           (hardcoded since the original 2001 code; in single player the
//           human is always nation 0). Each contributes RealX>>11, RealY>>11
//           = search cells (RealX>>4 = px, >>7 more = search cell). Own
//           towers of this type are excluded (counted into NPORTS instead).
//           If no nation-0 object exists, xe=ye=0 and the half-plane below
//           degenerates to "the side away from map corner (0,0)".
//The half-plane test (xx-xc)*(xe-xc)+(yy-yc)*(ye-yc) > 0 is a dot product:
//candidate minus base center, dotted with enemy minus base center. >0 means
//the candidate lies on the enemy side of the line through (xc,yc)
//perpendicular to the base->enemy axis - i.e. towers face the enemy.
bool SearchTowerPlace( int* xx1, int* yy1, SearchFunction* SFN, int r, int xc, int yc, int xe, int ye )
{
	// NPORTS = count of the nation's existing towers of this type (filled by
	// the caller). The first tower(s) use the strict enemy-facing topology
	// search below; once 2+ towers exist, the plain SearchPlace ring is used.
	if (NPORTS > 1)return SearchPlace( xx1, yy1, SFN, r );
	SsMaxX = msx >> 2;
	SsMaxY = msy >> 2;
	int x = *xx1;
	int y = *yy1;
	if (SFN( x, y ))
	{
		//assert(y<17648);
		return true;
	};
	int MyTop = GetTopology( x << 7, y << 7 );
	if (MyTop >= 0xFFFE)return false;
	// MyArea: the base's own topology area. Same-area candidates must count as
	// distance 0 (the LinksDist diagonal is 0xFFFF, see ClearLinkInfo), as
	// SearchPlace() already does - without this, a base sitting in a large or
	// weakly-linked area can never place its first tower (NPORTS stays <= 1,
	// so the NPORTS>1 fallback to plain SearchPlace is never reached).
	int MyArea = MyTop;
	MyTop *= NAreas;
	int PrevTopDst = 0;
	// rings start at 6 (not 1 like SearchPlace) so the tower is not built
	// right on top of the town center; ring i = i search cells from (x,y)
	for (int i = 6; i < r; i++)
	{
		char* xi = Rarr[i].xi;
		char* yi = Rarr[i].yi;
		int N = Rarr[i].N;
		for (int j = 0; j < N; j++)
		{
			int xx = x + xi[j];
			int yy = y + yi[j];
			// enemy-facing half-plane: dot((cell - base center), (enemy - base center)) > 0
			if (( xx - xc )*( xe - xc ) + ( yy - yc )*( ye - yc ) > 0 && xx > 0 && yy > 0 && xx < SsMaxX&&yy < SsMaxY)
			{
				if (!GNFO.EINF[CURRENTAINATION]->GetSafeVal( xx, yy ))
				{
					int top = TopRef[xx + xx + ( yy << ( TopSH + 1 ) )];
					if (top < 0xFFFE)
					{
						int TDST = LinksDist[MyTop + top];
						if (top == MyArea)TDST = 0;
						// same distance slack as SearchPlace (<24); the original <14 made
						// first-tower placement fail outright for bases whose area links
						// start at distance >= 14 (e.g. a small area at a map edge)
						if (TDST - PrevTopDst < 24)
						{
							if (TDST > PrevTopDst)PrevTopDst = TDST;
							if (SFN( xx, yy ))
							{
								*xx1 = xx;
								*yy1 = yy;
								return true;
							};
						};
					};
				};
			};
		};
	};
	// The strict enemy-facing search above can still fail permanently (base area
	// has no cell in the enemy-facing half-plane and no linked area within 24),
	// and since it only runs while NPORTS <= 1, the nation would never build
	// even one tower. Fall back to the plain ring search that towers 2+
	// (NPORTS > 1) already use.
	return SearchPlace( xx1, yy1, SFN, r );
};
bool FindCenter( int* xx, int *yy, byte NI )
{
	int N = 0;
	int x = 0;
	int y = 0;
	for (int i = 0; i < MAXOBJECT; i++)
	{
		OneObject* OB = Group[i];
		if (OB&&OB->NNUM == NI&&OB->newMons->Peasant)
		{
			x += OB->RealX >> 4;
			y += OB->RealY >> 4;
			N++;
		};
	};
	if (N)
	{
		x = ( div( x, N ).quot ) >> 7;
		y = ( div( y, N ).quot ) >> 7;
		*xx = x;
		*yy = y;
		return true;
	}
	else return false;
};
void SetUnusable( int x, int y, byte Mask )
{
	if (x < 0 || y < 0 || x >= SsMaxX || y >= SsMaxY)return;
	CantBuild[x + y*VAL_SPRNX] |= Mask;
};
