/*               Fast raster graphics routine
 *
 *  This module presents several useful procedures for fast output,
 * using RLC graphics format (see the description below). Before the
 * wor you must call SetRlcWindow to set properly the screen
 * capabilities.After this you mast call SetScreenPtr to set the
 * output destination. You can use LoadRLC to load the set of RLC
 * pictures into the RLCTable structure. Then you can draw the
 * picture on the screen wit the procedure ShowRLCItem
 */
#include "ddini.h"
#include "resfile.h"
#include "mode.h"
#include <stdio.h>
#include "fastdraw.h"
#include "gp_draw.h"
#include "assert.h"

extern int SCRSizeX;
extern int SCRSizeY;
extern int RSCRSizeX;
extern int RSCRSizeY;
extern int COPYSizeX;
extern int LOADED; // comes from gp_draw.cpp

DLLEXPORT void ShowCharUNICODE( int x, int y, byte* strptr, lpRLCFont lpr );
byte PAL1[256];
byte PAL2[256];
byte PAL3[256];
byte PAL4[256];
byte PAL5[256];
byte PAL6[256];
byte PAL7[256];
void ErrM( char* s );
extern void* offScreenPtr;

typedef RLCHeader* lpRLCHeader;

typedef xRLCTable* RLCTable;
typedef RLCTable* lpRLCTable;
RLCFont::RLCFont( int GP_Index )
{
	RLC = (RLCTable) GP_Index;
	FirstSymbol = 0;
	LastSymbol = 0;
	Options = 0;
	ShadowGP = -1;
};
void RLCFont::SetGPIndex( int n )
{
	RLC = (RLCTable) n;
	FirstSymbol = 0;
	LastSymbol = 0;
	Options = 0;
};
RLCFont::RLCFont()
{
	RLC = nullptr;
	FirstSymbol = 0;
	LastSymbol = 0;
	Options = 0;
	ShadowGP = -1;
};
RLCFont::~RLCFont()
{
	if (RLC&&int( RLC ) > 4096)free( RLC );
	RLC = nullptr;
};
typedef RLCFont* lpRLCFont;
//Window parameters
int WindX = 0;
int WindY = 0;
int WindLx;
int WindLy;
int WindX1;
int WindY1;
int ScrWidth;
int ScrHeight;
int BytesPerPixel = 1;
void* ScreenPtr = nullptr;
void* RealScreenPtr = nullptr;

extern int RealLx;
extern int RealLy;
void InitRLCWindows()
{
	WindLx = RealLx;
	WindLy = RealLy;
	WindX1 = RealLx - 1;
	WindY1 = RealLy - 1;
	ScrWidth = SCRSizeX;
	ScrHeight = SCRSizeY;
};

/*  Creating window
 */
bool ProcessMessages();
static int cntr;
static bool InCycle;

int ST_WX;
int ST_WY;
int ST_WLX;
int ST_WLY;
int ST_SCW;

DLLEXPORT
void SetRLCWindow( int x, int y, int lx, int ly, int slx );
void StoreWindow()
{
	ST_WX = WindX;
	ST_WY = WindY;
	ST_WLX = WindX1 - WindX + 1;
	ST_WLY = WindY1 - WindY + 1;
	ST_SCW = ScrWidth;
};
void RestoreWindow()
{
	SetRLCWindow( ST_WX, ST_WY, ST_WLX, ST_WLY, ST_SCW );
};
void SetWind( int x, int y, int Lx, int Ly )
{
	SetRLCWindow( x, y, Lx, Ly, ScrWidth );
};

DLLEXPORT void SetRLCWindow( int x, int y, int lx, int ly, int slx )
{
	WindX = x;
	WindY = y;
	WindX1 = x + lx - 1;
	WindY1 = y + ly - 1;
	ScrWidth = slx;
	if (WindX < 0)
		WindX = 0;
	if (WindY < 0)
		WindY = 0;
	if (WindX1 >= RealLx)
		WindX1 = RealLx - 1;
	if (WindY1 >= RealLy)
		WindY1 = RealLy - 1;
	WindLx = WindX1 - WindX + 1;
	WindLy = WindY1 - WindY + 1;
}

//  Setting proper value of the screen pointer
void SetScreenPtr( void )
{
	ScreenPtr = offScreenPtr;
	//RealScreenPtr = lpSurface;
}

#ifdef _WIN32
void ClearScreen();
#endif

// Linux clean-room renderer for RLC sprites:
// 4-byte header (short SizeX, SizeY), then per scanline a segment count
// byte, and per segment (skip, count) followed by count pixel bytes.
// Modes: 0 dst=src, 1 dst=pal[src], 2 dst=pal[dst] (mask-only, "fon"),
// 3 dst=pal[(src<<8)|dst] (half-transparent blend, "ht").
// mirror=true draws right-to-left (the Inv variants; x = right edge).
void ShowRLCGen( int x, int y, void* PicPtr, byte* pal, int mode, bool mirror )
{
	lpRLCHeader H = (lpRLCHeader)PicPtr;
	int PLX = H->SizeX & 65535;
	int PLY = H->SizeY & 65535;
	if (!PLY || !PLX) return;
	if (mirror)
	{
		if (y + PLY - 1 < WindY || y > WindY1) return;
		if (x < WindX || x - PLX + 1 > WindX1) return;
	}
	else
	{
		if (y + PLY - 1 < WindY || y > WindY1) return;
		if (x + PLX <= WindX || x > WindX1) return;
	}
	const byte* src = (const byte*)PicPtr + 4;
	int yy = y;
	for (int line = 0; line < PLY; line++, yy++)
	{
		int nseg = *src++;
		if (nseg && yy >= WindY && yy <= WindY1)
		{
			byte* dstrow = (byte*)ScreenPtr + (size_t)yy * ScrWidth;
			int colpos = 0;   // running column: segment skips are RELATIVE
			for (int s = 0; s < nseg; s++)
			{
				int skip = src[0];
				int cnt = src[1];
				src += 2;
				colpos += skip;   // advance from the previous segment's end
				for (int k = 0; k < cnt; k++)
				{
					int col = mirror ? (x - colpos - 1 - k) : (x + colpos + k);
					if (col >= WindX && col <= WindX1)
					{
						byte v;
						switch (mode)
						{
						case 0: v = src[k]; break;
						case 1: v = pal[src[k]]; break;
						case 2: v = pal[dstrow[col]]; break;
						default: v = pal[((unsigned)src[k] << 8) | dstrow[col]]; break;
						}
						dstrow[col] = v;
					}
				}
				colpos += cnt;
				src += cnt;
			}
		}
		else
		{
			for (int s = 0; s < nseg; s++) src += 2 + src[1];
		}
	}
}

//Showing RLC image with clipping
void ShowRLC( int x, int y, void* PicPtr ) { ShowRLCGen( x, y, PicPtr, nullptr, 0, false ); }

//Showing inverse RLC image with clipping
void ShowRLCi( int x, int y, void* PicPtr ) { ShowRLCGen( x, y, PicPtr, nullptr, 0, true ); }

void ShowRLCpal( int x, int y, void* PicPtr, byte* pal ) { ShowRLCGen( x, y, PicPtr, pal, 1, false ); }

void ShowRLCipal( int x, int y, void* PicPtr, byte* pal ) { ShowRLCGen( x, y, PicPtr, pal, 1, true ); }

void ShowRLCfonpal( int x, int y, void* PicPtr, byte* pal ) { ShowRLCGen( x, y, PicPtr, pal, 2, false ); }

void ShowRLCifonpal( int x, int y, void* PicPtr, byte* pal ) { ShowRLCGen( x, y, PicPtr, pal, 2, true ); }

//End of RLC with clipping & with palette(half-transparent fog)
void ShowRLChtpal( int x, int y, void* PicPtr, byte* pal ) { ShowRLCGen( x, y, PicPtr, pal, 3, false ); }

//Showing inverse RLC image with clipping & encodint(half-transparent fog)
void ShowRLCihtpal( int x, int y, void* PicPtr, byte* pal ) { ShowRLCGen( x, y, PicPtr, pal, 3, true ); }

void ShowRLCp1( int x, int y, void* PicPtr )
{
	ShowRLCpal( x, y, PicPtr, PAL1 );
}
void ShowRLCp2( int x, int y, void* PicPtr )
{
	ShowRLCpal( x, y, PicPtr, PAL2 );
}
void ShowRLCp3( int x, int y, void* PicPtr )
{
	ShowRLCpal( x, y, PicPtr, PAL3 );
}
void ShowRLCp4( int x, int y, void* PicPtr )
{
	ShowRLCpal( x, y, PicPtr, PAL4 );
}
void ShowRLCp5( int x, int y, void* PicPtr )
{
	ShowRLCpal( x, y, PicPtr, PAL5 );
}
void ShowRLCp6( int x, int y, void* PicPtr )
{
	ShowRLCpal( x, y, PicPtr, PAL6 );
}
void ShowRLCp7( int x, int y, void* PicPtr )
{
	ShowRLCpal( x, y, PicPtr, PAL7 );
}
void ShowRLCip1( int x, int y, void* PicPtr )
{
	ShowRLCipal( x, y, PicPtr, PAL1 );
}
void ShowRLCip2( int x, int y, void* PicPtr )
{
	ShowRLCipal( x, y, PicPtr, PAL2 );
}
void ShowRLCip3( int x, int y, void* PicPtr )
{
	ShowRLCipal( x, y, PicPtr, PAL3 );
}
void ShowRLCip4( int x, int y, void* PicPtr )
{
	ShowRLCipal( x, y, PicPtr, PAL4 );
}
void ShowRLCip5( int x, int y, void* PicPtr )
{
	ShowRLCipal( x, y, PicPtr, PAL5 );
}
void ShowRLCip6( int x, int y, void* PicPtr )
{
	ShowRLCipal( x, y, PicPtr, PAL6 );
}
void ShowRLCip7( int x, int y, void* PicPtr )
{
	ShowRLCipal( x, y, PicPtr, PAL7 );
}

extern byte fog[8192 + 1024];
extern byte wfog[8192];
extern byte yfog[8192];
extern byte rfog[8192];
extern byte trans8[65536];

void ShowRLCShadow( int x, int y, void* PicPtr )
{
	ShowRLCfonpal( x, y, PicPtr, fog + 4096 );
}
void ShowRLCiShadow( int x, int y, void* PicPtr )
{
	ShowRLCifonpal( x, y, PicPtr, fog + 4096 );
}
void ShowRLCWhite( int x, int y, void* PicPtr )
{
	ShowRLCfonpal( x, y, PicPtr, wfog + 1024 );
}
void ShowRLCDarkN( int x, int y, void* PicPtr, int N )
{
	ShowRLCpal( x, y, PicPtr, wfog + ( N << 8 ) );
};
void ShowRLCiDarkN( int x, int y, void* PicPtr, int N )
{
	ShowRLCipal( x, y, PicPtr, wfog + ( N << 8 ) );
};
void ShowRLCRedN( int x, int y, void* PicPtr, int N )
{
	ShowRLCpal( x, y, PicPtr, yfog + ( N << 8 ) );
};
void ShowRLCiRedN( int x, int y, void* PicPtr, int N )
{
	ShowRLCipal( x, y, PicPtr, yfog + ( N << 8 ) );
};

// TODO: check if this is needed.
void ShowRLCItemDarkN( int x, int y, lpRLCTable lprt, int n, int Ints )
{
	// Linux: OfsTable holds unrebased offsets (see LoadRLC)
	if (n < 4096)
	{
		ShowRLCDarkN( x, y, (void*) ( (byte*)( *lprt ) + 4 + ( *lprt )->OfsTable[n] ), Ints );
	}
	else
	{
		ShowRLCiDarkN( x, y, (void*) ( (byte*)( *lprt ) + 4 + ( *lprt )->OfsTable[n - 4096] ), Ints );
	};
};
void ShowRLCItemPal( int x, int y, lpRLCTable lprt, int n, byte* Pal )
{
	// Linux: OfsTable holds unrebased offsets (see LoadRLC)
	if (n < 4096)
	{
		ShowRLCpal( x, y, (void*) ( (byte*)( *lprt ) + 4 + ( *lprt )->OfsTable[n] ), Pal );
	}
	else
	{
		ShowRLCipal( x, y, (void*) ( (byte*)( *lprt ) + 4 + ( *lprt )->OfsTable[n - 4096] ), Pal );
	};
};
void ShowRLCItemGrad( int x, int y, lpRLCTable lprt, int n, byte* Pal )
{
	// Linux: OfsTable holds unrebased offsets (see LoadRLC)
	if (n < 4096)
	{
		ShowRLChtpal( x, y, (void*) ( (byte*)( *lprt ) + 4 + ( *lprt )->OfsTable[n] ), Pal );
	}
	else
	{
		ShowRLCihtpal( x, y, (void*) ( (byte*)( *lprt ) + 4 + ( *lprt )->OfsTable[n - 4096] ), Pal );
	};
};
void ShowRLCItemRedN( int x, int y, lpRLCTable lprt, int n, int Ints )
{
	// Linux: OfsTable holds unrebased offsets (see LoadRLC)
	if (n < 4096)
	{
		ShowRLCRedN( x, y, (void*) ( (byte*)( *lprt ) + 4 + ( *lprt )->OfsTable[n] ), Ints );
	}
	else
	{
		ShowRLCiRedN( x, y, (void*) ( (byte*)( *lprt ) + 4 + ( *lprt )->OfsTable[n - 4096] ), Ints );
	};
};
void ShowRLCWFog( int x, int y, void* PicPtr )
{
	ShowRLChtpal( x, y, PicPtr, wfog + 1024 );
}
void ShowRLCiWhite( int x, int y, void* PicPtr )
{
	ShowRLCifonpal( x, y, PicPtr, wfog + 1024 );
}
void ShowRLCiWFog( int x, int y, void* PicPtr )
{
	ShowRLCihtpal( x, y, PicPtr, wfog + 1024 );
}
void ShowRLCiTrans8( int x, int y, void* PicPtr )
{
	ShowRLCihtpal( x, y, PicPtr, trans8 );
}
void ShowRLCTrans8( int x, int y, void* PicPtr )
{
	ShowRLChtpal( x, y, PicPtr, trans8 );
}
void ShowRLCDark( int x, int y, void* PicPtr )
{
	ShowRLChtpal( x, y, PicPtr, fog + 1024 );
}
void ShowRLCiDark( int x, int y, void* PicPtr )
{
	ShowRLCihtpal( x, y, PicPtr, fog + 1024 );
}
void ShowRLCBlue( int x, int y, void* PicPtr )
{
	ShowRLChtpal( x, y, PicPtr, rfog + 2048 );
}
void ShowRLCiBlue( int x, int y, void* PicPtr )
{
	ShowRLCihtpal( x, y, PicPtr, rfog + 2048 );
}
void ShowRLCFire( int x, int y, void* PicPtr )
{
	ShowRLChtpal( x, y, PicPtr, yfog );
}
void ShowRLCiFire( int x, int y, void* PicPtr )
{
	ShowRLCihtpal( x, y, PicPtr, yfog );
}

//Load rlc file, allocate and fill provided RLCTable
bool LoadRLC( LPCSTR lpFileName, RLCTable *RLCtbl )
{
	ResFile f1 = RReset( lpFileName );
	if (INVALID_HANDLE_VALUE != f1)
	{
		DWORD fsz = RFileSize( f1 );
		LOADED += fsz;

		*RLCtbl = (RLCTable) malloc( fsz + 4 );
		( *RLCtbl )->Size = fsz + 4;

		RBlockRead( f1, &( ( *RLCtbl )->sign ), fsz );
		RClose( f1 );

		// 64-bit: OfsTable stays an int (file bytes!) - readers compute
		// (char*)table + 4 + OfsTable[n] instead (see GetRLCWidth/Height)
		return true;
	}

	char gg[128];
	sprintf( gg, "Could not load RLC file: %s", lpFileName );
	ErrM( gg );

	return false;
}

// TODO: check if this is needed.
void ShowRLCItemTrans8( int x, int y, lpRLCTable lprt, int n )
{
	if (n < 4096)
	{
		ShowRLCTrans8( x, y, (void*) ( ( *lprt )->OfsTable[n] ) );
	}
	else
	{
		ShowRLCiTrans8( x, y, (void*) ( ( *lprt )->OfsTable[n - 4096] ) );
	};
};

// TODO: check if this is needed.
void ShowRLCItemMutno( int x, int y, lpRLCTable lprt, int n )
{
	if (n < 4096)
	{
		ShowRLCWFog( x, y, (void*) ( ( *lprt )->OfsTable[n] ) );
	}
	else
	{
		ShowRLCiWFog( x, y, (void*) ( ( *lprt )->OfsTable[n - 4096] ) );
	};
};

// TODO: check if this is needed.
void ShowRLCItemDark( int x, int y, lpRLCTable lprt, int n )
{
	if (n < 4096)
	{
		ShowRLCDark( x, y, (void*) ( ( *lprt )->OfsTable[n] ) );
	}
	else
	{
		ShowRLCiDark( x, y, (void*) ( ( *lprt )->OfsTable[n - 4096] ) );
	};
};

// TODO: check if this is needed. Is used in waves commented code
void ShowRLCItemBlue( int x, int y, lpRLCTable lprt, int n )
{
	if (n < 4096)
	{
		ShowRLCBlue( x, y, (void*) ( ( *lprt )->OfsTable[n] ) );
	}
	else
	{
		ShowRLCiBlue( x, y, (void*) ( ( *lprt )->OfsTable[n - 4096] ) );
	};
};

// TODO: check if this is needed.
void ShowRLCItemFired( int x, int y, lpRLCTable lprt, int n )
{
	if (n < 4096)
	{
		ShowRLCFire( x, y, (void*) ( ( *lprt )->OfsTable[n] ) );
	}
	else
	{
		ShowRLCiFire( x, y, (void*) ( ( *lprt )->OfsTable[n - 4096] ) );
	};
};

void ShowRLCItem( int x, int y, lpRLCTable lprt, int n, byte nt )
{
	// 64-BIT FIX: font-handle-vs-pointer test (see GetRLCWidth) -
	// int(*lprt) truncated heap pointers to negative "GP file ids"
	// and crashed ShowGP.
	int GPID = (int)(uintptr_t)( *lprt );

	if ((uintptr_t)( *lprt ) < 4096)
	{
		GPS.ShowGP( x, y, GPID, n, nt );
		return;
	}

	// Linux: OfsTable holds unrebased file offsets (see LoadRLC) - the
	// picture data starts at (char*)table + 4 + OfsTable[index].
	byte* table = (byte*)( *lprt );
	bool inv = n >= 4096;
	int idx = inv ? ( n - 4096 ) : n;
	void* pic = table + 4 + ( *lprt )->OfsTable[idx];

	if ( nt == 0 || nt > 7 )
	{
		if ( inv ) ShowRLCi( x, y, pic );
		else ShowRLC( x, y, pic );
	}
	else
	{
		static byte* NATPAL[8] = { nullptr, PAL1, PAL2, PAL3, PAL4, PAL5, PAL6, PAL7 };
		if ( inv ) ShowRLCipal( x, y, pic, NATPAL[nt] );
		else ShowRLCpal( x, y, pic, NATPAL[nt] );
	}
}

int GetRLCWidth( RLCTable lpr, byte n )
{
	// 64-BIT FIX: font-handle-vs-pointer test. int(lpr) truncated 64-bit heap
	// pointers to negative values which passed "GPID < 4096" (PortingNotes
	// pattern #3). Compare the pointer width instead; handles are small ints.
	int GPID = (int)(uintptr_t)( lpr );
	if ((uintptr_t)lpr < 4096)
	{
		if (n == 32)
		{
			return GPS.GetGPWidth( GPID, 'c' );
		}
		return GPS.GetGPWidth( GPID, n );
	};
	if (n < lpr->SCount)
	{
#ifdef _WIN32
		return ( *( (lpRLCHeader) ( (void*) ( lpr->OfsTable[n] ) ) ) ).SizeX;
#else
		// OfsTable holds unrebased 32-bit offsets on 64-bit (see LoadRLC)
		return ( (lpRLCHeader) ( (char*) lpr + 4 + lpr->OfsTable[n] ) )->SizeX;
#endif
	}
	else return 0;
}

int GetCHEX( byte c )
{
	if (c >= '0'&&c <= '9')return c - '0';
	if (c >= 'a'&&c <= 'z')return c + 10 - 'a';
	if (c >= 'A'&&c <= 'Z')return c + 10 - 'A';
	return 0;
}

int GetRLCWidthUNICODE( RLCTable lpr, byte* strptr, int* L )
{
	if (strptr[0] == SIGNBYTE)//FUNNY: Funny shit, Sherlock. DEL, srsly?!
	{
		if (L)*L = 5;
		int idx = ( GetCHEX( strptr[1] ) << 12 ) +
			( GetCHEX( strptr[2] ) << 8 ) +
			( GetCHEX( strptr[3] ) << 4 ) +
			GetCHEX( strptr[4] );
		// 64-BIT FIX: font-handle-vs-pointer test (see GetRLCWidth)
		int GPID = (int)(uintptr_t)( lpr );
		if ((uintptr_t)lpr < 4096)
		{
			UNICODETABLE* UT = GPS.UNITBL[GPID];
			if (!UT)return 0;
			for (int j = 0; j < UT->NTables; j++)
			{
				if (idx >= UT->USET[j].Start)
				{
					int v = idx - UT->USET[j].Start;
					if (v < UT->USET[j].NSymbols)
					{
						v += UT->USET[j].GP_Start;
						return GPS.GetGPWidth( UT->USET[j].GPID, v );
					}
				}
			}
			return 0;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		if (L)
		{
			*L = 1;
		}
		return GetRLCWidth( lpr, strptr[0] );
	}
}

DLLEXPORT int GetRLCHeight( RLCTable lpr, byte n )
{
	// 64-BIT FIX: font-handle-vs-pointer test. int(lpr) truncated 64-bit heap
	// pointers to negative values which passed "GPID < 4096" (PortingNotes
	// pattern #3). Compare the pointer width instead; handles are small ints.
	int GPID = (int)(uintptr_t)( lpr );
	if ((uintptr_t)lpr < 4096)
	{
		return GPS.GetGPHeight( GPID, n );
	}
	if (n < lpr->SCount)
	{
#ifdef _WIN32
		return ( *( (lpRLCHeader) ( (void*) ( lpr->OfsTable[n] ) ) ) ).SizeY;
#else
		// OfsTable holds unrebased 32-bit offsets on 64-bit (see LoadRLC)
		return ( (lpRLCHeader) ( (char*) lpr + 4 + lpr->OfsTable[n] ) )->SizeY;
#endif
	}
	else
	{
		return 0;
	}
}

void RegisterRLCFont( lpRLCFont lrf, RLCTable lpr, int fir )
{
	lrf->FirstSymbol = fir;
	lrf->LastSymbol = lpr->SCount + fir - 1;
	lrf->RLC = lpr;
}
void CheckFontColor( lpRLCFont lpf )
{
	int GPID = int( lpf->RLC );
	if (GPID < 4096)GPS.ImageType[GPID] = ( GPS.ImageType[GPID] & 7 ) | lpf->Options;
};
void ShowChar( int x, int y, char c, lpRLCFont lpf )
{
	int GPID = int( lpf->RLC );
	if (GPID < 4096)
	{
		GPS.ImageType[GPID] = ( GPS.ImageType[GPID] & 7 ) | lpf->Options;
		GPS.ShowGP( x, y, GPID, byte( c ), 0 );
		return;
	};
	ShowRLCItem( x, y, &( lpf->RLC ), c, 0 );
}

DLLEXPORT void ShowCharUNICODE( int x, int y, byte* strptr, lpRLCFont lpr )
{
	if (strptr[0] == SIGNBYTE)
	{
		int idx = ( GetCHEX( strptr[1] ) << 12 ) +
			( GetCHEX( strptr[2] ) << 8 ) +
			( GetCHEX( strptr[3] ) << 4 ) +
			GetCHEX( strptr[4] );
		int GPID = int( lpr->RLC );
		if (GPID < 4096)
		{
			UNICODETABLE* UT = GPS.UNITBL[GPID];
			if (UT)
			{
				for (int j = 0; j < UT->NTables; j++)
				{
					if (idx >= UT->USET[j].Start)
					{
						int v = idx - UT->USET[j].Start;
						if (v < UT->USET[j].NSymbols)
						{
							v += UT->USET[j].GP_Start;
							GPID = UT->USET[j].GPID;
							if (lpr->ShadowGP != -1)
							{
								GPS.ImageType[lpr->ShadowGP] = 1;
								GPS.ShowGP( x + UT->USET[j].DX + lpr->ShadowDx, y + UT->USET[j].DY + lpr->ShadowDy, GPID, v, 0 );
							}
							if (UT->USET[j].UseColor)
							{
								GPS.ImageType[GPID] = ( GPS.ImageType[GPID] & 7 ) | lpr->Options;
							}
							else
							{
								GPS.ImageType[GPID] = 1;
							}
							GPS.ShowGP( x + UT->USET[j].DX, y + UT->USET[j].DY, GPID, v, 0 );
							return;
						}
					}
				}
			}
		}
	}
	else
	{
		if (lpr->ShadowGP != -1)
		{
			GPS.ImageType[lpr->ShadowGP] = 1;
			GPS.ShowGP( x + lpr->ShadowDx, y + lpr->ShadowDy, lpr->ShadowGP, strptr[0], 0 );
		}
		ShowChar( x, y, strptr[0], lpr );
	}
}

void ShowString( int x, int y, LPCSTR lps, lpRLCFont lpf )
{
	if (nullptr == lps)
	{
		return;
	}

	int GPID = int( lpf->RLC );

	if (GPID < 4096)
	{
		GPS.ImageType[GPID] = ( GPS.ImageType[GPID] & 7 ) | lpf->Options;
		byte ch;
		int i = 0;
		do
		{
			ch = lps[i];
			if (ch != 0)
			{
				ShowCharUNICODE( x, y, (byte*) ( lps + i ), lpf );
				int L = 1;
				x += GetRLCWidthUNICODE( lpf->RLC, (byte*) ( lps + i ), &L );
				i += L - 1;
			}
			i++;
		} while (ch);

		return;
	}

	byte ch;
	int i = 0;
	do
	{
		ch = lps[i];
		if (ch != 0)
		{
			ShowRLCItem( x, y, &( lpf->RLC ), lpf->FirstSymbol + ch, 0 );
			x += GetRLCWidth( lpf->RLC, lpf->FirstSymbol + ch );
		}
		i++;
	} while (ch);
}

int GetRLCStrWidth( char* str, lpRLCFont lpf )
{
	if (!str)return 0;
	int L = 0;
	int sl = strlen( str );
	for (int i = 0; i < sl; i++)
	{
		int LL = 1;
		L += GetRLCWidthUNICODE( lpf->RLC, (byte*) ( str + i ), &LL );
		i += LL - 1;
	};
	return L;
};
//Loads the PAL1..PAL7 remap tables used by ShowRLCItem's nation tints (nt=1..7).
//Was WIN32-only with a no-op stub on Linux, leaving PAL1..7 all-zero: any
//nation-tinted RLC sprite (e.g. a wheat field owned by nation 1..7) rendered
//pitch black (palette index 0) instead of its tint.
void LoadPalettes()//IMPORTANT: load color palettes for color masking
{
	ResFile f1 = RReset( "pal1.dat" );
	RBlockRead( f1, PAL1, 256 );
	RClose( f1 );
	f1 = RReset( "pal2.dat" );
	RBlockRead( f1, PAL2, 256 );
	RClose( f1 );
	f1 = RReset( "pal3.dat" );
	RBlockRead( f1, PAL3, 256 );
	RClose( f1 );
	f1 = RReset( "pal4.dat" );
	RBlockRead( f1, PAL4, 256 );
	RClose( f1 );
	f1 = RReset( "pal5.dat" );
	RBlockRead( f1, PAL5, 256 );
	RClose( f1 );
	f1 = RReset( "pal6.dat" );
	RBlockRead( f1, PAL6, 256 );
	RClose( f1 );
	f1 = RReset( "pal7.dat" );
	RBlockRead( f1, PAL7, 256 );
	RClose( f1 );
}
#ifdef _WIN32
void ShowShadString( int x, int y, LPCSTR lps, lpRLCFont lpf )
{
	if (lps == nullptr) return;
	byte	ch;
	int		i = 0;
	do
	{
		ch = lps[i];
		if (ch != 0)
		{
			ShowRLCItem( x, y, &( lpf->RLC ), lpf->FirstSymbol + ch, 0 );
			int LL = 1;
			x += GetRLCWidthUNICODE( lpf->RLC, (byte*) ( lps + i ), &LL );
			i += LL - 1;
		}
		i++;
	} while (ch - 0);
}

RLCFont::RLCFont( char* Name )
{
	int LOLD = LOADED;
	LoadRLC( Name, &RLC );
	RegisterRLCFont( this, RLC, 0 );
	LOADED = LOLD;
	Options = 0;
	ShadowGP = -1;
}
#endif // _WIN32

void RLCFont::SetRedColor()
{
	Options = 32;
	//SetStdShadow();
}

void RLCFont::SetWhiteColor()
{
	Options = 16;
	//SetStdShadow();
}

void RLCFont::SetBlackColor()
{
	Options = 0;
}

void RLCFont::SetColorTable( int n )
{
	Options = n << 4;
	//SetStdShadow();
}
/* End of Graphics routine */
