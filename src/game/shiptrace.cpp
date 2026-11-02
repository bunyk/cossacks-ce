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

void ProcessBlobs();
//Blobs are the animated water-surface effects: the foam wakes that moving
//ships leave behind (CreateTrace, called from motion.cpp for every object
//whose NewMonster has NWaves/WavePoints) and the random ripples sparkling
//on rivers (AddRandomBlobs - only placed on cells where GetRiverDir finds
//water). Each blob is a point that drifts with a velocity and a time-to-
//live (ProcessBlobs moves the BlobX/Y arrays and picks a frame), drawn as
//one frame of the Wave1.pix animation stack. wave.grd (TraceGrd) is the
//4096-byte blend table that maps (water pixel, blob pixel) to a water-
//palette shade, so foam melts into the water instead of covering it.
//(The original asm inner loops are translated to portable C++ below.)
class Blob {
public:
	short Lx;
	short Ly;
	short N;
	short H;
	byte* Data;
	Blob();
	~Blob();
	void Load(char* Name);
	void Show(int x, int y, int N);
};
Blob::Blob() {
	Data = NULL;
	Lx = 0;
	Ly = 0;
};
Blob::~Blob() {
	if (Data)free(Data);
};

void ErrM(char*);
void Blob::Load(char* Name) {
	ResFile f = RReset(Name);
	if (f == INVALID_HANDLE_VALUE) {
		char ccc[128];
		sprintf(ccc, "Could not load: %s", Name);
		ErrM(ccc);
		return;
	};
	RBlockRead(f, &Lx, 2);
	RBlockRead(f, &Ly, 2);
	RBlockRead(f, &N, 2);
	RBlockRead(f, &H, 2);
	Data = new byte[Lx*Ly*N];
	RBlockRead(f, Data, Lx*Ly*N);
	RClose(f);
};
byte TraceGrd[4096];
//Draws one blob frame at (x,y), blending it into the water through the
//TraceGrd table: out = TraceGrd[(blob<<8) | water]. Blob frames use
//palette indices 0..15 and water pixels 0..255, so the combined index
//fits the 4096-byte table. The two original asm versions (byte loop for
//BLX%4!=0, 4-pixel unrolled otherwise) were behaviorally identical, so
//this single portable loop replaces both.
void ShowBlob(int x, int y, byte* Blob, int Lx, int Ly) {
	if (x > WindX1 || y > WindY1)return;
	byte* bof = Blob;
	int BLX = Lx;
	int BLY = Ly;
	if (x < WindX) {
		if (x + Lx <= WindX)return;
		bof -= x - WindX;
		BLX += x - WindX;
		x = 0;
	};
	if (y < WindY) {
		if (y + Ly <= WindY)return;
		bof -= (y - WindY)*Lx;
		BLY += y - WindY;
		y = 0;
	};
	if (x + Lx > WindX1 + 1) {
		BLX -= x + Lx - WindX1 - 1;
	};
	if (y + Ly > WindY1 + 1) {
		BLY -= y + Ly - WindY1 - 1;
	};
	if (BLX <= 0 || BLY <= 0) {
		//assert(BLX>0&&BLY>0);
		return;
	};
	byte* dst = (byte*)ScreenPtr + x + y*ScrWidth;
	byte* src = bof;
	int scadd = ScrWidth - BLX;
	int badd = Lx - BLX;
	for (int i = 0; i < BLY; i++) {
		for (int j = 0; j < BLX; j++) {
			dst[j] = TraceGrd[(src[j] << 8) | dst[j]];
		};
		src += BLX + badd;
		dst += scadd + BLX;
	};
};
void Blob::Show(int x, int y, int m) {
	if (m >= N || m < 0)return;
	ShowBlob(x - (Lx >> 1), y - (Ly >> 1), Data + Lx*Ly*m, Lx, Ly);
};
//------------------------LOADING&TEST-----------------

Blob Blob1;
//Loaded as part of the water-and-fog visual set by LoadFog (fog.cpp):
//fog.cpp owns all fog/water shading tables (fog palettes, darkfog, refl,
//water cost/alpha tables), and the blob/wake layer belongs to the same
//water rendering stack - both blend through the shared water palette
//tables. Hence the call at the end of LoadFog. Fog-of-war dims what is
//unexplored; blobs decorate the water that fog has already revealed.
void LoadBlobs()
{
	ResFile f = RReset("wave.grd");
	RBlockRead(f, TraceGrd, 4096);
	RClose(f);
	Blob1.Load("Wave1.pix");
}

extern int tmtmt;
void TestBlob() {
	ProcessBlobs();
};
//-------------------Traces system--------------------
#define MaxBlob 1024
int BlobX[MaxBlob];
int BlobY[MaxBlob];
int BlobVx[MaxBlob];
int BlobVy[MaxBlob];
byte BlobTime[MaxBlob];
byte BlobVisible[MaxBlob];
byte BlobOpt[MaxBlob];
int NBlobs;
int CurBlob;
void ProcessBlobs() {
	if (!NBlobs)return;
	int MinX = (mapx << 9) - 32 * 16;
	int MinY = (mapy << 9) - 32 * 16;
	int MaxX = ((mapx + smaplx) << 9) + 32 * 16;
	int MaxY = ((mapy + smaply) << 9) + 32 * 16;
	memset(BlobVisible, 0, MaxBlob);
	//Move every live blob (BlobTime!=0) by its velocity, age it by one
	//tick and mark the ones inside the (x: [MinX,MaxX], y: [MinY,MaxY])
	//camera window as visible.
	for (int i = 0; i < NBlobs; i++)
	{
		if (!BlobTime[i])continue;
		BlobX[i] += BlobVx[i];
		BlobY[i] += BlobVy[i];
		BlobTime[i]--;
		if (BlobX[i] >= MinX && BlobX[i] <= MaxX
			&& BlobY[i] >= MinY && BlobY[i] <= MaxY)
		{
			BlobVisible[i] = 1;
		}
	}

	MinX = mapx * 32;
	MinY = mapy * 16;

	for (int i = 0; i < NBlobs; i++)
	{
		if (BlobVisible[i])
		{
			int p = BlobTime[i];
			byte op = BlobOpt[i];
			if (p)
			{
				int n = Blob1.N;

				if (p < n)
				{
					p = n - p;
				}
				else
				{
					p -= n;
				}

				if (p < 0)
					p = 0;

				if (p >= n)
					p = n - 1;

				if (op)
				{
					p = 20 + (i % 18);
				}

				Blob1.Show((BlobX[i] >> 4) - MinX, (BlobY[i] >> 5) - MinY, p);
			}
		}
		else
		{
			BlobTime[i] = 0;
		}
	}
}

short BDVX[256];
short BDVY[256];
extern short TSin[257];
extern short TCos[257];

void InitBlobs()
{
	for (int i = 0; i < 256; i++)
	{
		BDVX[i] = (TCos[i]) >> 4;
		BDVY[i] = (TSin[i]) >> 4;
	}
	NBlobs = 0;
	CurBlob = 0;
}

void AddBlob(int x, int y, byte Dir, bool dir2)
{
	int Cur;
	if (NBlobs < MaxBlob)
	{
		Cur = NBlobs;
		NBlobs++;
		CurBlob++;
		if (NBlobs == MaxBlob)
		{
			CurBlob = 0;
		}
	}
	else
	{
		//All MaxBlob slots are in use: linear-search from CurBlob up to
		//MaxBlob for a dead slot (BlobTime==0). Cur stays -1 if none is
		//free and the blob is dropped.
		Cur = -1;
		int idx = CurBlob;
		for (int cnt = MaxBlob - CurBlob; cnt > 0; cnt--)
		{
			if (!BlobTime[idx])
			{
				Cur = idx;
				idx++;
				break;
			}
			idx++;
		}
		CurBlob = idx;
	}

	if (CurBlob >= MaxBlob)
	{
		CurBlob = 0;
	}

	if (Cur == -1)
	{
		return;
	}

	BlobX[Cur] = x;
	BlobY[Cur] = y;
	BlobVx[Cur] = BDVX[Dir];
	BlobVy[Cur] = BDVY[Dir];

	if (dir2)
	{
		BlobTime[Cur] = (Blob1.N) << 1;
	}
	else
	{
		BlobTime[Cur] = (Blob1.N);
	}

	BlobOpt[Cur] = dir2;
}

void AddBlob(int x, int y, byte Dir)
{
	AddBlob(x, y, Dir, 0);
}

int srando();
extern int RealLx;
extern int RealLy;
int GetRiverDir(int x, int y);

void AddRandomBlobs()
{
	int x0 = (mapx << 5);
	int y0 = (mapy << 5);
	for (int i = 0; i < 80; i++)
	{
		int x = x0 + ((srando()*RealLx) >> 15);
		int y = y0 + ((srando()*RealLy) >> 14);
		int dir = GetRiverDir(x, y);
		if (dir != -1)
		{
			int ofst = (x >> 5) + (MAPSX >> 1)*(y >> 5);
			AddBlob(x << 4, y << 4, dir, 1);
		}
	}
}

extern int tmtmt;

void AddTrace(int x, int y, byte Dir)
{
	AddBlob(x, y, Dir + 64);
	AddBlob(x, y, Dir - 64);
}

int srando();

void CreateTrace(OneObject* OB)
{
	if (tmtmt & 3)
	{
		return;
	}

	NewMonster* NM = OB->newMons;
	byte ANGL = OB->RealDir - 64;

	int dz = NM->WaveDZ;
	int SIN = TSin[ANGL];
	int COS = TCos[ANGL];

	if (NM->NWaves)
	{
		int N = NM->NWaves;
		for (int i = 0; i < N; i++)
		{
			int x = NM->WavePoints[i + i];
			int y = NM->WavePoints[i + i + 1];
			int xx = (x*COS - y*SIN) >> 4;
			int yy = ((y*COS + x*SIN) >> 4) + (dz << 5);
			xx += srando() >> 6;
			yy += srando() >> 7;
			AddTrace(OB->RealX + xx, OB->RealY + yy, OB->RealDir);
		}
	}
}
