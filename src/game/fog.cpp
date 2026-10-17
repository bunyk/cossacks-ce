#include "ddini.h"
#include "resfile.h"
#include "fastdraw.h"
#include "mgraph.h"
#include "mouse.h"
#include "menu.h"
#include "mapdiscr.h"
#include "mode.h"
#include "3dgraph.h"
#include "antibug.h"

//Offset for fog coordinates calculation
const int kFogOffset = 3;

int FMSX;
int FMSX2;

word* fmap;
word* fmap1;

#define MaxShad 4528

byte fog[8192 + 1024];
byte wfog[8192];
byte yfog1[8192];
byte rfog[8192];

byte Optional1[8192];
byte Optional2[8192];
byte Optional3[8192];

byte darkfog[40960];
byte yfog[8192];
byte trans4[65536];
byte trans8[65536];
byte AlphaR[65536];
byte AlphaW[65536];
byte refl[3072];
byte WaterCost[65536];
byte GraySet[256];
byte Bright[8192];

void ClearFog()
{
	memset( fmap, 0, LX_fmap*LX_fmap * 2 );
	memset( fmap1, 0, LX_fmap*LX_fmap * 2 );
}

void LoadOptionalTable( int n, char* Name )
{
	ResFile F = RReset( Name );
	switch ( n )
	{
	case 3:
		RBlockRead( F, Optional1, 8192 );
		break;
	case 4:
		RBlockRead( F, Optional2, 8192 );
		break;
	case 5:
		RBlockRead( F, Optional3, 8192 );
		break;
	};
	RClose( F );
}

int mul3( int );

// Fog mode: 0 = off, 1 = on (options / PrepareToGame set it)
int FogMode;

// Fog diffusion pass (the "new fog"): out = (sum4 >> 2) - (sum4 >> 10) -
// a 5-point blur with decay; sums wrap to 16 bits like the asm dx register.
// Applied to the active sub-rect (cols/rows 1..mlx-1/mly-1 of the
// (msx/4+6) x (msy/4+6) region). The three Windows asm variants below are
// IDENTICAL except for the row stride constant FMSX2_C (in BYTES), which
// is 2*FMSX_C with FMSX_C = 134 << (ADDSH-1) - i.e. the fmap row stride
// in words. This shared translation covers all three (S = row stride in
// WORDS; the asm starts at byte offset 2+FMSX2_C = word index S+1).
static void FogBlurPass( int S )
{
	const word* src = fmap1;
	word* dst = fmap;
	int idx = S + 1;
	int rows = ( ( msy >> 2 ) + kFogOffset + kFogOffset ) - 1;
	int cols = ( ( msx >> 2 ) + kFogOffset + kFogOffset ) - 1;
	for ( int y = 0; y < rows; y++ )
	{
		for ( int x = 0; x < cols; x++ )
		{
			word sum = (word)( src[idx - S] + src[idx + S] + src[idx - 1] + src[idx + 1] );
			dst[idx] = (word)( ( sum >> 2 ) - ( sum >> 10 ) );
			idx++;
		}
		idx += S - cols;
	}
}

#define FMSX_C 134
#define FMSX2_C (2*FMSX_C)

void ProcessFog1_1()
{
	memcpy( fmap1, fmap, LX_fmap*LX_fmap * 2 );
	FogBlurPass( 134 );
}

#undef FMSX_C
#undef FMSX2_C

#define FMSX_C (134<<1)
#define FMSX2_C (2*FMSX_C)

void ProcessFog1_2()
{
	memcpy( fmap1, fmap, LX_fmap*LX_fmap * 2 );
	FogBlurPass( 134 << 1 );
}


#undef FMSX_C
#undef FMSX2_C

#define FMSX_C (134<<2)
#define FMSX2_C (2*FMSX_C)

void ProcessFog1_3()
{
	memcpy( fmap1, fmap, LX_fmap*LX_fmap * 2 );
	FogBlurPass( 134 << 2 );
}



void ProcessFog1()
{
	switch ( ADDSH )
	{
	case 1:
		ProcessFog1_1();
		break;
	case 2:
		ProcessFog1_2();
		break;
	case 3:
		ProcessFog1_3();
		break;
	}
}

int FOGOFFS[1024];
void LoadBlobs();

DLLEXPORT int CurPalette = 0;
extern int CurrentCursorGP;
byte graysc[256];

DLLEXPORT void LoadFog( int set )
{
	CurrentCursorGP = set + 5;
	CurPalette = set;
	char cc[128];
	sprintf( cc, "%d\\agew_1d.grd", set );
	ResFile fx = RReset( cc );
	RBlockRead( fx, &fog[1024], 8192 );
	memcpy( &fog[0], &fog[1024], 256 );
	memcpy( &fog[256], &fog[1024], 256 );
	memcpy( &fog[512], &fog[1024], 256 );
	memcpy( &fog[512 + 256], &fog[1024], 256 );
	RClose( fx );
	fx = RReset( "0\\gray.set" );
	RBlockRead( fx, &graysc, 256 );
	RClose( fx );
	sprintf( cc, "%d\\orange.grd", set );
	fx = RReset( cc );
	RBlockRead( fx, &Optional1, 8192 );
	RClose( fx );
	sprintf( cc, "%d\\agew_1w.grd", set );
	fx = RReset( cc );
	RBlockRead( fx, &wfog, 8192 );
	RClose( fx );
	sprintf( cc, "%d\\agew_1br.grd", set );
	fx = RReset( cc );
	RBlockRead( fx, &Bright, 8192 );
	RClose( fx );
	fx = RReset( "ageb.grd" );
	RBlockRead( fx, &rfog, 8192 );
	RClose( fx );
	sprintf( cc, "%d\\agew_1r.grd", set );
	fx = RReset( cc );
	RBlockRead( fx, &yfog, 8192 );
	RClose( fx );
	fx = RReset( "0\\agew_1dr.grd" );
	RBlockRead( fx, &darkfog, 40960 );
	RClose( fx );
	sprintf( cc, "%d\\agew_tr.grd", set );
	fx = RReset( cc );
	RBlockRead( fx, &trans8, 65536 );
	RClose( fx );
	sprintf( cc, "%d\\agew_tr4.grd", set );
	fx = RReset( cc );
	RBlockRead( fx, &trans4, 65536 );
	RClose( fx );
	fx = RReset( "wcost.grd" );
	RBlockRead( fx, &WaterCost, 65536 );
	RClose( fx );
	fx = RReset( "refl.grd" );
	RBlockRead( fx, &refl, 3072 );
	RClose( fx );
	fx = RReset( "AlphaYR.grd" );
	RBlockRead( fx, &AlphaR, 65536 );
	RClose( fx );
	fx = RReset( "AlphaW.grd" );
	RBlockRead( fx, &AlphaW, 65536 );
	RClose( fx );
	fx = RReset( "agew_1gc.pal" );
	RBlockRead( fx, &GraySet, 256 );
	RClose( fx );
	for ( int i = 0; i < 1024; i++ )
	{
		FOGOFFS[i] = i*FMSX;
	}
	//Blob/wake data (shiptrace.cpp) - part of the same water visual set:
	//Wave1.pix frames + the wave.grd blend table used to melt foam wakes
	//into the water palette. See the comment above the Blob class there.
	LoadBlobs();
}

void SetupFog()
{
	for ( int i = 0; i < 1024; i++ )
	{
		FOGOFFS[i] = i * FMSX;
	}
}



#define zmin 0
#define zmax 32

// Renders one 32x16-pixel fog block with bilinearly interpolated fog
// values (z1..z4 = corners, 16.16 after the <<16 below; high = dark).
// Per pixel: dst = darkfog[((fog >> 8) & 0xFF00) + dst] - the same
// level-pixel fold as the terrain rasterizers. All four corners <= 63:
// fully visible, nothing to draw; all >= 96: fill with color 0.
void ShowSuperFluentFog32_160_16( int x, int y, int z1x, int z2x, int z3x, int z4x )
{
	if ( MiniMode )
	{
		return;
	}

	int z1 = z1x * 65536;
	int	z2 = z2x * 65536;
	int z3 = z3x * 65536;
	int	z4 = z4x * 65536;

	int scrof = int( ScreenPtr ) + x + y * SCRSizeX;
	int adds = SCRSizeX - 32;

	if ( z1x <= 63 && z2x <= 63 && z3x <= 63 && z4x <= 63 )
	{
		return;
	}

	if ( z1x >= 96 && z2x >= 96 && z3x >= 96 && z4x >= 96 )
	{
		// Full fog: fill the 32x16 block with color 0
		byte* dst = ScreenPtr + x + y * SCRSizeX;
		for ( int row = 0; row < 16; row++ )
		{
			memset( dst, 0, 32 );
			dst += SCRSizeX;
		}
		return;
	}
	else
	{
		int a, b, p;
		int c = ( z3 - z1 ) >> 4;
		int d = ( z1 + z4 - z3 - z2 ) >> 9;
		// Bilinear corner interpolation: per-row fog start p (= z1 + row*c),
		// per-pixel increment b (starts (z2-z1)/32, gains row*d). The index
		// into darkfog folds the pixel byte into AL exactly like the asm:
		// darkfog[((fog >> 8) & 0xFF00) + pixel].
		byte* dst = ScreenPtr + x + y * SCRSizeX;
		b = ( z2 - z1 ) >> 5;
		p = z1;
		for ( int row = 0; row < 16; row++ )
		{
			int fog = p;
			for ( int i = 0; i < 32; i++ )
			{
				dst[i] = darkfog[( ( fog >> 8 ) & 0xFF00 ) + dst[i]];
				fog += b;
			}
			dst += SCRSizeX;
			b += d;
			p += c;
		}
	}
}


#define shf 300
#define fmin 1500
byte fden[8192];

void makeFden()
{
	for ( int i = 0; i < 8192; i++ )
	{
		if ( i > shf )
			fden[i] = 158;
		else
			fden[i] = 5 + div( i * 153, shf ).quot;
	}
}
int GetF( word k )
{
	if ( k >= fmin )
	{
		return 5;
	}

	return fden[fmin - k];
}

void TurnFogOn()
{
	FogMode = 1;
}

void TurnFogOff()
{
	FogMode = 0;
}



void FogSpot( int x, int y );

int GetHeight( int x, int y );

void UnitLight( OneObject* OB )
{
	if ( !OB->Ready )
	{
		return;
	}

	int xx = OB->RealX >> 4;
	int yy = OB->RealY >> 4;
	int rr = 200;
	int rr1 = rr - ( rr >> 2 );
	yy -= ( GetHeight( xx, yy ) << 1 );
	NewMonster* NM = OB->newMons;
	switch ( NM->VisionType )
	{
	case 0:
		//Default unit line of view
		FogSpot( xx, yy );
		break;
	case 1:
		FogSpot( xx + 128, yy );
		FogSpot( xx - 128, yy );
		FogSpot( xx, yy + 128 );
		FogSpot( xx, yy - 128 );
		break;
	case 2:
		FogSpot( xx + 128, yy + 128 );
		FogSpot( xx - 128, yy + 128 );
		FogSpot( xx + 128, yy - 128 );
		FogSpot( xx - 128, yy - 128 );
		break;
	case 3:
		FogSpot( xx + 2 * 128, yy );
		FogSpot( xx - 2 * 128, yy );
		FogSpot( xx, yy - 2 * 128 );
		FogSpot( xx, yy + 2 * 128 );
		FogSpot( xx + 128, yy + 128 );
		FogSpot( xx - 128, yy + 128 );
		FogSpot( xx + 128, yy - 128 );
		FogSpot( xx - 128, yy - 128 );
		break;
	case 4:
		FogSpot( xx + 3 * 128, yy );
		FogSpot( xx - 3 * 128, yy );
		FogSpot( xx, yy + 3 * 128 );
		FogSpot( xx, yy - 3 * 128 );
		FogSpot( xx + 2 * 128, yy + 2 * 128 );
		FogSpot( xx - 2 * 128, yy + 2 * 128 );
		FogSpot( xx + 2 * 128, yy - 2 * 128 );
		FogSpot( xx - 2 * 128, yy - 2 * 128 );
		break;
	case 5:
		FogSpot( xx + 4 * 128, yy );
		FogSpot( xx - 4 * 128, yy );
		FogSpot( xx, yy + 4 * 128 );
		FogSpot( xx, yy - 4 * 128 );
		FogSpot( xx + 3 * 128, yy + 3 * 128 );
		FogSpot( xx - 3 * 128, yy + 3 * 128 );
		FogSpot( xx + 3 * 128, yy - 3 * 128 );
		FogSpot( xx - 3 * 128, yy - 3 * 128 );
		break;
	case 6:
		FogSpot( xx + 5 * 128, yy );
		FogSpot( xx - 5 * 128, yy );
		FogSpot( xx, yy + 5 * 128 );
		FogSpot( xx, yy - 5 * 128 );
		FogSpot( xx + 4 * 128, yy + 4 * 128 );
		FogSpot( xx - 4 * 128, yy + 4 * 128 );
		FogSpot( xx + 4 * 128, yy - 4 * 128 );
		FogSpot( xx - 4 * 128, yy - 4 * 128 );
		break;
	case 7:
		FogSpot( xx + 6 * 128, yy );
		FogSpot( xx - 6 * 128, yy );
		FogSpot( xx, yy + 6 * 128 );
		FogSpot( xx, yy - 6 * 128 );
		FogSpot( xx + 4 * 128, yy + 4 );
		FogSpot( xx - 4 * 128, yy + 4 * 128 );
		FogSpot( xx + 4 * 128, yy - 4 * 128 );
		FogSpot( xx - 4 * 128, yy - 4 * 128 );
		break;
	case 8:
		//Ukrainian peasant line of view
		FogSpot( xx, yy );//BUGFIX: Make peasants visible immediately at game start
		FogSpot( xx + 7 * 128, yy );
		FogSpot( xx - 7 * 128, yy );
		FogSpot( xx, yy + 7 * 128 );
		FogSpot( xx, yy - 7 * 128 );
		FogSpot( xx + 5 * 128, yy + 3 );
		FogSpot( xx - 5 * 128, yy + 5 * 128 );
		FogSpot( xx + 5 * 128, yy - 5 * 128 );
		FogSpot( xx - 5 * 128, yy - 5 * 128 );
		break;
	}
}

//New fog of war
word WFSC[128 * 128];
int maxWFX;
int maxWFY;

void SetWF( int x, int y, word w )
{
	if ( x<0 || y<0 || x>maxWFX || y>maxWFY )
	{
		return;
	}

	WFSC[x + ( y * 128 )] = w;
}

int maxFX, maxFY;

void CreateFogImage()
{
	maxWFX = smaplx;
	maxWFY = smaply;
	maxFX = ( msx / 4 ) + kFogOffset;
	maxFY = ( msy / 4 ) + kFogOffset;

	int mx0 = mapx / 4;
	int my0 = mapy / 4;
	int mx1 = mapx + smaplx;

	if ( mx1 % 4 )
	{
		mx1 = ( mx1 / 4 ) + 1;
	}
	else
	{
		mx1 /= 4;
	}

	int my1 = mapy + smaply;

	if ( my1 % 4 )
	{
		my1 = ( my1 / 4 ) + 1;
	}
	else
	{
		my1 /= 4;
	}

	word* FMAP = (word*) fmap;
	int fogof0 = ( my0 + kFogOffset ) * FMSX + mx0 + kFogOffset;

	for ( int myy = my0; myy <= my1; myy++ )
	{
		int fogof = fogof0;

		for ( int mxx = mx0; mxx <= mx1; mxx++ )
		{
			int mcx = ( mxx * 4 ) - mapx;
			int mcy = ( myy * 4 ) - mapy;

			int F[5][5];

			F[0][0] = FMAP[fogof];
			F[0][4] = FMAP[fogof + 1];
			F[4][0] = FMAP[fogof + FMSX];
			F[4][4] = FMAP[fogof + FMSX + 1];

			F[2][2] = ( F[0][0] + F[4][4] + F[0][4] + F[4][0] ) >> 2;
			F[0][2] = ( F[0][0] + F[0][4] ) / 2;
			F[2][0] = ( F[0][0] + F[4][0] ) / 2;
			F[2][4] = ( F[0][4] + F[4][4] ) / 2;
			F[4][2] = ( F[4][0] + F[4][4] ) / 2;

			F[0][1] = ( F[0][0] + F[0][2] ) / 2;
			F[0][3] = ( F[0][2] + F[0][4] ) / 2;
			F[1][0] = ( F[0][0] + F[2][0] ) / 2;
			F[1][1] = ( F[0][0] + F[2][0] + F[0][2] + F[2][2] ) / 4;
			F[1][2] = ( F[0][2] + F[2][2] ) / 2;
			F[1][3] = ( F[0][2] + F[0][4] + F[2][2] + F[2][4] ) / 4;
			F[1][4] = ( F[0][4] + F[2][4] ) / 2;

			F[2][1] = ( F[2][0] + F[2][2] ) / 2;
			F[2][3] = ( F[2][2] + F[2][4] ) / 2;

			F[3][0] = ( F[2][0] + F[4][0] ) / 2;
			F[3][1] = ( F[2][0] + F[2][2] + F[4][0] + F[4][2] ) / 4;
			F[3][2] = ( F[2][2] + F[4][2] ) / 2;
			F[3][3] = ( F[2][2] + F[2][4] + F[4][2] + F[4][4] ) / 4;
			F[3][4] = ( F[2][4] + F[4][4] ) / 2;

			F[4][1] = ( F[4][0] + F[4][2] ) / 2;
			F[4][3] = ( F[4][2] + F[4][4] ) / 2;

			for ( int i = 0; i < 4; i++ )
			{
				for ( int j = 0; j < 4; j++ )
				{
					int xx = mcx + i;
					int yy = mcy + j;

					SetWF( xx, yy, F[j][i] );
					SetWF( xx + 1, yy, F[j + 1][i] );
					SetWF( xx, yy + 1, F[j][i + 1] );
					SetWF( xx + 1, yy + 1, F[j + 1][i + 1] );
				}
			}
			fogof++;
		}
		fogof0 += FMSX;
	}
}

void DrawFog()
{
	for ( int y = 0; y < smaply; y++ )
	{
		for ( int x = 0; x < smaplx; x++ )
		{
			int xx = ( x * 32 ) + smapx;
			int yy = ( y * 16 ) + smapy;
			int sof = x + ( y * 128 );

			ShowSuperFluentFog32_160_16(
				xx, yy,
				GetF( WFSC[sof] ),
				GetF( WFSC[sof + 1] ),
				GetF( WFSC[sof + 128] ),
				GetF( WFSC[sof + 129] )
			);
		}
	}
}

void FogSpot( int x, int y )
{
	int fx = ( ( x + 64 ) / 128 ) + kFogOffset;
	int fy = ( ( y + 64 ) / 128 ) + kFogOffset;

	if ( fx < 1 )
	{
		fx = 1;
	}

	if ( fy < 1 )
	{
		fy = 1;
	}

	if ( fx > maxFX )
	{
		fx = maxFX;
	}

	if ( fy > maxFY )
	{
		fy = maxFY;
	}

	fmap[FOGOFFS[fy] + fx] = 8000;
}

//Checks if the given coordinates are concealed through fog of war
//Return value < 900 means 'fogged', everything above - 'visible'
//Beware: y coordinate must be multiplied by 2?
//Example: if (GetFog(x, y << 1) < 900 && FogMode) //true if fogged
word GetFog( int x, int y )
{
	int fx = ( ( x + 64 ) / 128 ) + kFogOffset;
	int fy = ( ( y + 64 ) / 128 ) + kFogOffset;

	if ( fx < 1 )
	{
		fx = 1;
	}

	if ( fy < 1 )
	{
		fy = 1;
	}

	if ( fx > maxFX )
	{
		fx = maxFX;
	}

	if ( fy > maxFY )
	{
		fy = maxFY;
	}

	return fmap[FOGOFFS[fy] + fx];
}

extern int MiniLx;
extern int MiniLy;
extern int MiniX, MiniY;

// Blacks out fogged cells on the minimap: fmap <= 1300 (the asm's unsigned
// "ja" skips visible cells) paints the pixel AND the one one row below.
// 64-BIT FIX: the original computed byte offsets via int(ScreenPtr)/
// int(fmap); the translation uses real pointers (fp is a WORD pointer, so
// the asm's byte strides DDDX/F_add halve).
void DrawMiniFog()
{
	byte* scr = ScreenPtr + minix + miniy * ScrWidth;
	word* fp = fmap + ( ( ( MiniX >> 1 ) << ( ADDSH - 1 ) ) + kFogOffset + 1 + FMSX * ( ( ( MiniY >> 1 ) << ( ADDSH - 1 ) ) + kFogOffset + 1 ) );

	int MMSX = ( MiniLx / 2 );
	int MMSY = ( MiniLy / 2 );
	int addscr = ScrWidth + ScrWidth - MMSX - MMSX;
	int F_add = ( FMSX2 << ( ADDSH - 1 ) ) - ( MMSX << ADDSH );
	int DDDX = 1 << ADDSH;

	for ( int y = 0; y < MMSY; y++ )
	{
		for ( int x = 0; x < MMSX; x++ )
		{
			if ( *fp <= 1300 )
			{
				*(word*)scr = 0;
				*(word*)( scr + ScrWidth ) = 0;
			}
			fp += DDDX >> 1;
			scr += 2;
		}
		fp += F_add >> 1;
		scr += addscr;
	}
}
