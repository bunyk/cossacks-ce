#include "ddini.h"
#include "resfile.h"
#include "fastdraw.h"
#include "mgraph.h"
#include "mouse.h"
#include "menu.h"
#include "mapdiscr.h"
#include "mode.h"
#include "3dgraph.h"

// Fog of war module. Two layers:
//  - fmap: a low-res grid of fog cells (one word per 128x128 map pixels),
//    LX_fmap x LX_fmap cells with a kFogOffset-cell border. 0 = unexplored,
//    FogSpot() writes kFogRevealValue into visited cells, FogBlurPass()
//    blurs+decays the grid every frame. GetFog() answers "is this point
//    visible to the player" for sounds/hints; DrawMiniFog() paints it.
//  - The byte tables below: precomputed 8-bit lookup tables loaded from
//    .grd/.pal files in LoadFog(). The renderer never computes a shade, it
//    only indexes these tables, so their sizes are fixed by the palette:
//      256   = one palette row (a remap of all 256 color indices)
//      8192  = 32 shade levels x 256 = a ".grd" gradient table
//      40960 = 160 darkness levels x 256 = darkfog (fog overlay + terrain)
//      65536 = 256 x 256 = a full 2D blend LUT indexed [a*256 + b]
//      3072  = 12 water shades x 256 = refl (water reflections)
const int kPaletteRowSize = 256;
const int kShadeLevels = 32;
const int kGrdTableSize = kShadeLevels * kPaletteRowSize;       // 8192
const int kFogGuardBytes = 4 * kPaletteRowSize;                 // 1024, see fog[]
const int kDarkFogLevels = 160;
const int kDarkFogTableSize = kDarkFogLevels * kPaletteRowSize; // 40960
const int kBlendTableSize = kPaletteRowSize * kPaletteRowSize;  // 65536
const int kReflLevels = 12;
const int kReflTableSize = kReflLevels * kPaletteRowSize;       // 3072

// Fog cell geometry and value thresholds (fmap words):
const int kFogCellSize = 128;        // one fog cell covers 128x128 map pixels
const int kFogRevealValue = 8000;    // value FogSpot() writes into a visited cell
const int kFogVisibleThreshold = 900;  // GetFog() < this means "fogged"
const int kMiniFogThreshold = 1300;    // DrawMiniFog() <= this means "fogged"

//Offset for fog coordinates calculation
const int kFogOffset = 3;

// fmap row stride in words / bytes: LX_fmap = FMSX = 134 << (ADDSH-1),
// set together with the fmap allocation in loadsave.cpp (ADDSH = map size,
// 1..3). FMSX2 is the same stride in bytes (2 bytes per word).
int FMSX;
int FMSX2;

// Low-res fog grid (LX_fmap x LX_fmap words) + scratch copy for the
// diffusion pass: FogBlurPass() reads fmap1 and writes fmap.
word* fmap;
word* fmap1;

// 32-level x 256-entry shade tables used to recolor sprites for special
// effects (pulsing/glow/gradient) - see AVBlendTable in zbuffer.cpp and
// the encoding switch in gp_draw.cpp. fog[] carries kFogGuardBytes extra
// bytes in front: they duplicate the first 4 levels of the loaded block so
// that base pointers like fog+1024 (AV_DARK) stay valid for negative level
// offsets down to -4 levels.
byte fog[kGrdTableSize + kFogGuardBytes]; // dark shade table (agew_1d.grd)
byte wfog[kGrdTableSize];  // white shade table (agew_1w.grd)
byte yfog1[kGrdTableSize]; // legacy second yellow table, extern in ctables.h, never loaded
byte rfog[kGrdTableSize];  // blue shade table (ageb.grd)

// Extra 32-level palette-encoded shade tables (orange.grd etc.) for
// multi-palette sprite layers; selected by LoadOptionalTable(n = 3..5).
byte Optional1[kGrdTableSize];
byte Optional2[kGrdTableSize];
byte Optional3[kGrdTableSize];

// 160-level darkness remap: darkfog[level*256 + pixel] -> darkened pixel.
// Used by the fog overlay (ShowSuperFluentFog32_160_16) and by the terrain
// rasterizers (3dgraph.cpp, which start at offset 16384 = level 64).
byte darkfog[kDarkFogTableSize];
byte yfog[kGrdTableSize];  // red shade table (agew_1r.grd)
// Full 256x256 blend LUTs indexed [a*256 + b]:
byte trans4[kBlendTableSize];  // transparency blend, 4-bit mask sprites
byte trans8[kBlendTableSize];  // transparency blend, 8-bit mask sprites
byte AlphaR[kBlendTableSize];  // alpha blend, red-tinted sprites (AlphaYR.grd)
byte AlphaW[kBlendTableSize];  // alpha blend, white sprites (AlphaW.grd)
byte refl[kReflTableSize];     // 12-level water reflection remap (refl.grd)
byte WaterCost[kBlendTableSize]; // animated-water shore blend/cost table
                                   // (wcost.grd), indexed [dstPixel*256 + band|color]
                                   // by realwater.cpp
byte GraySet[kPaletteRowSize]; // grayscale palette (agew_1gc.pal), interface themes
byte Bright[kGrdTableSize];    // bright/white shade table (agew_1br.grd)

// Resets both fog grids to "unexplored" (all zero) - called when a game is
// loaded/started (loadsave.cpp).
void ClearFog()
{
	memset( fmap, 0, LX_fmap*LX_fmap * 2 );
	memset( fmap1, 0, LX_fmap*LX_fmap * 2 );
}

// Loads one of the Optional1..3 tables (n = 3..5) from an arbitrary file.
void LoadOptionalTable( int n, char* Name )
{
	ResFile F = RReset( Name );
	switch ( n )
	{
	case 3:
		RBlockRead( F, Optional1, kGrdTableSize );
		break;
	case 4:
		RBlockRead( F, Optional2, kGrdTableSize );
		break;
	case 5:
		RBlockRead( F, Optional3, kGrdTableSize );
		break;
	};
	RClose( F );
}

// Fog mode: 0 = off, 1 = on (options / PrepareToGame set it)
int FogMode;

// One diffusion step of the fog grid (the "new fog"), for the current map
// size: copy fmap -> fmap1, then blur+decay fmap in place. Per cell:
// out = (sum of 4 neighbours >> 2) - (sum >> 10) - a 5-point blur with
// decay; sums wrap to 16 bits like the asm dx register. Applied to the
// active sub-rect (cols/rows 1..mlx-1/mly-1 of the (msx/4+6) x (msy/4+6)
// region). S = fmap row stride in WORDS (= LX_fmap = 134 << (ADDSH-1));
// the pass starts at word index S+1, i.e. byte offset 2+2*S like the
// original asm.
void FogBlurPass() {
	memcpy( fmap1, fmap, LX_fmap*LX_fmap * 2 );
	int S = 134 << ( ADDSH - 1 );
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

void LoadBlobs();

DLLEXPORT int CurPalette = 0;
extern int CurrentCursorGP;
byte graysc[kPaletteRowSize]; // grayscale palette (0\gray.set), disabled icons

// Loads every precomputed shading table the renderer needs. That is why so
// many files: each one is a LUT the software renderer indexes instead of
// computing (see the size constants at the top of this file). Files named
// "%d\..." exist once per palette set (set = 0/1/2, chosen by the game
// options) so each set can recolor the same effects; the rest are global.
DLLEXPORT void LoadFog( int set )
{
	CurrentCursorGP = set + 5;
	CurPalette = set;
	char cc[128];
	sprintf( cc, "%d\\agew_1d.grd", set );
	ResFile fx = RReset( cc );
	RBlockRead( fx, &fog[kFogGuardBytes], kGrdTableSize );
	// Duplicate the first 4 levels of the block into the guard area in
	// front of it (see the comment on fog[] above).
	memcpy( &fog[0], &fog[kFogGuardBytes], kPaletteRowSize );
	memcpy( &fog[256], &fog[kFogGuardBytes], kPaletteRowSize );
	memcpy( &fog[512], &fog[kFogGuardBytes], kPaletteRowSize );
	memcpy( &fog[512 + 256], &fog[kFogGuardBytes], kPaletteRowSize );
	RClose( fx );
	fx = RReset( "0\\gray.set" );
	RBlockRead( fx, &graysc, kPaletteRowSize );
	RClose( fx );
	sprintf( cc, "%d\\orange.grd", set );
	fx = RReset( cc );
	RBlockRead( fx, &Optional1, kGrdTableSize );
	RClose( fx );
	sprintf( cc, "%d\\agew_1w.grd", set );
	fx = RReset( cc );
	RBlockRead( fx, &wfog, kGrdTableSize );
	RClose( fx );
	sprintf( cc, "%d\\agew_1br.grd", set );
	fx = RReset( cc );
	RBlockRead( fx, &Bright, kGrdTableSize );
	RClose( fx );
	fx = RReset( "ageb.grd" );
	RBlockRead( fx, &rfog, kGrdTableSize );
	RClose( fx );
	sprintf( cc, "%d\\agew_1r.grd", set );
	fx = RReset( cc );
	RBlockRead( fx, &yfog, kGrdTableSize );
	RClose( fx );
	fx = RReset( "0\\agew_1dr.grd" );
	RBlockRead( fx, &darkfog, kDarkFogTableSize );
	RClose( fx );
	sprintf( cc, "%d\\agew_tr.grd", set );
	fx = RReset( cc );
	RBlockRead( fx, &trans8, kBlendTableSize );
	RClose( fx );
	sprintf( cc, "%d\\agew_tr4.grd", set );
	fx = RReset( cc );
	RBlockRead( fx, &trans4, kBlendTableSize );
	RClose( fx );
	fx = RReset( "wcost.grd" );
	RBlockRead( fx, &WaterCost, kBlendTableSize );
	RClose( fx );
	fx = RReset( "refl.grd" );
	RBlockRead( fx, &refl, kReflTableSize );
	RClose( fx );
	fx = RReset( "AlphaYR.grd" );
	RBlockRead( fx, &AlphaR, kBlendTableSize );
	RClose( fx );
	fx = RReset( "AlphaW.grd" );
	RBlockRead( fx, &AlphaW, kBlendTableSize );
	RClose( fx );
	fx = RReset( "agew_1gc.pal" );
	RBlockRead( fx, &GraySet, kPaletteRowSize );
	RClose( fx );
	//Blob/wake data (shiptrace.cpp) - part of the same water visual set:
	//Wave1.pix frames + the wave.grd blend table used to melt foam wakes
	//into the water palette. See the comment above the Blob class there.
	LoadBlobs();
}


// Darkness levels (as returned by GetF) at which a fog block is treated as
// fully visible (nothing to draw) or fully fogged (painted solid black).
const int kFogLevelVisible = 63;
const int kFogLevelBlack = 96;

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

	const int kFixShift = 65536; // 16.16 fixed point: level << 16
	int z1 = z1x * kFixShift;
	int	z2 = z2x * kFixShift;
	int z3 = z3x * kFixShift;
	int	z4 = z4x * kFixShift;

	if ( z1x <= kFogLevelVisible && z2x <= kFogLevelVisible && z3x <= kFogLevelVisible && z4x <= kFogLevelVisible )
	{
		return;
	}

	if ( z1x >= kFogLevelBlack && z2x >= kFogLevelBlack && z3x >= kFogLevelBlack && z4x >= kFogLevelBlack )
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
		int b, p;
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


// Knee and scale of the fog-word -> darkness-level curve below:
// shf = knee distance below fmin where the curve saturates at the max
// darkness, fmin = fog value from which a cell counts as fully visible.
#define shf 300
#define fmin 1500
// fden[i] = darkness level for a fog word of (fmin - i): 5 at i = 0
// (freshly revealed), growing to 158 (max darkness) beyond i = shf.
// 8192 entries cover every possible (fmin - k) for k >= 0... k is a word,
// so k < fmin gives i < 1500, well inside the table.
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
// Maps a raw fmap word to a darkness level 5..158 (5 = fully visible,
// 158 = pitch black) used as the level index into darkfog.
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

// Reveals the fog around a unit. Called for every ready own unit each frame
// (newmon.cpp) and for scenario light spots (ScenaryLights in
// activescenary.cpp). NM->VisionType (0..8, from the unit's GRE.MD entry)
// selects the vision pattern: how many FogSpot() calls and how far they
// reach - type 0 is a single spot at the unit, types 1..8 add spots on a
// cross/diagonal pattern out to 2..7 fog cells (of 128 map pixels each);
// type 8 is the Ukrainian peasant's wide 7-cell vision.
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
// WFSC: per-screen-pixel fog values for the currently visible area
// (smaplx x smaply pixels, max 128x128), upsampled from fmap by
// CreateFogImage(); DrawFog reads the 4 corners of each 32x16 block from it.
word WFSC[128 * 128];
int maxWFX; // WFSC bounds in screen pixels (= smaplx/smaply)
int maxWFY;

void SetWF( int x, int y, word w )
{
	if ( x<0 || y<0 || x>maxWFX || y>maxWFY )
	{
		return;
	}

	WFSC[x + ( y * 128 )] = w;
}

int maxFX, maxFY; // fog grid bounds in fog cells: (msx/4)+kFogOffset etc.,
                  // set by CreateFogImage, used to clamp FogSpot/GetFog

// Upsamples the low-res fmap into WFSC for the visible screen rectangle:
// each fog cell covers 4x4 screen pixels, expanded through a 5x5 control
// grid with bilinear (halving) interpolation, so the fog edges come out
// smooth instead of blocky.
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

// Paints the fog overlay over the visible screen area, one 32x16-pixel
// block per iteration; the 4 corner darkness levels come from WFSC.
// Called each rendered frame after terrain and units are drawn (mapa.cpp),
// so fogged enemy units are simply covered by the dark overlay.
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

// Marks the fog cell containing map point (x, y) as revealed. The +64 is
// rounding to the nearest cell (half of kFogCellSize); result clamped to
// the valid grid [1..maxFX]x[1..maxFY].
void FogSpot( int x, int y )
{
	int fx = ( ( x + kFogCellSize / 2 ) / kFogCellSize ) + kFogOffset;
	int fy = ( ( y + kFogCellSize / 2 ) / kFogCellSize ) + kFogOffset;

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

	fmap[fy*FMSX + fx] = kFogRevealValue;
}

//Checks if the given coordinates are concealed through fog of war
//Return value < kFogVisibleThreshold means 'fogged', everything above - 'visible'
//Beware: y coordinate must be multiplied by 2?
//Example: if (GetFog(x, y << 1) < kFogVisibleThreshold && FogMode) //true if fogged
word GetFog( int x, int y )
{
	int fx = ( ( x + kFogCellSize / 2 ) / kFogCellSize ) + kFogOffset;
	int fy = ( ( y + kFogCellSize / 2 ) / kFogCellSize ) + kFogOffset;

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

	return fmap[fy*FMSX + fx];
}

extern int MiniLx;
extern int MiniLy;
extern int MiniX, MiniY;

// Blacks out fogged cells on the minimap where fmap <= kMiniFogThreshold
void DrawMiniFog()
{
	byte* scr = ScreenPtr + minix + miniy * ScrWidth;
	word* fp = fmap + ( ( ( MiniX >> 1 ) << ( ADDSH - 1 ) ) + kFogOffset + 1 + FMSX * ( ( ( MiniY >> 1 ) << ( ADDSH - 1 ) ) + kFogOffset + 1 ) );

	int MMSX = ( MiniLx / 2 );
	int MMSY = ( MiniLy / 2 );
	int addscr = ScrWidth + ScrWidth - MMSX - MMSX; // distance from right side of minimap to the left side in next row
	int F_add = ( FMSX2 << ( ADDSH - 1 ) ) - ( MMSX << ADDSH );
	int DDDX = 1 << ADDSH;

	for ( int y = 0; y < MMSY; y++ )
	{
		for ( int x = 0; x < MMSX; x++ )
		{
			if ( *fp <= kMiniFogThreshold )
			{
				*(word*)scr = 0; // zero 2 bytes of screen
				*(word*)( scr + ScrWidth ) = 0; // and 2 bytes under them
			}
			fp += DDDX >> 1;
			scr += 2;
		}
		fp += F_add >> 1;
		scr += addscr;
	}
}
