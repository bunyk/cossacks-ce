// gscbrowser - a small SDL3 utility to browse GSC game resource archives.
//
// Usage:
//   gscbrowser [archive.gsc]        (default: all.gsc)
//
// UI:
//   Left pane  - filterable list of files stored in the archive
//   Right pane - preview of the selected file:
//                  .bmp -> decoded image (8bpp paletted / 24bpp)
//                  .wav -> format info
//                  text-like content -> text dump
//                  everything else -> hex dump
//
// Keys:
//   Up/Down/PgUp/PgDn/Home/End - navigate
//   letters/backspace         - edit the filter box
//   TAB                       - animate the selected .gp page (cycle its
//                                pictures through GPS.ShowGP; +/- adjusts
//                                the frame time, default 100 ms; press
//                                TAB again to stop)
//   Esc                       - quit

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../src/game/arc/gscarch.h"
#include "../src/game/gp_draw.h"
#include "../src/game/fog.h"
#include "../src/game/ctables.h"
#include "../src/game/gfile.h"
#include "../src/game/resfile.h"
#include "../src/game/fastdraw.h"

// GSC_OpenError() in gscarch.cpp references this global
SDL_Window* sdlWindow = nullptr;

// ---------------------------------------------------------------- tool-side
// Globals that the linked game renderer (gp_draw/fastdraw/fog/resfile)
// references but which live in game files too heavy to link into a tool
// (ddini.cpp, newmon.cpp, ...). Values only matter where noted.
int CurrentCursorGP = 0;            // ddini.cpp: cursor sprite id, unused here
void* offScreenPtr = nullptr;       // ddini.cpp: only referenced, not dereferenced
int RealLx = 0, RealLy = 0;         // ddini.cpp: logical screen size
int SCRSizeX = 0, SCRSizeY = 0;    // ddini.cpp: offscreen buffer size
int RSCRSizeX = 0, RSCRSizeY = 0;   // ddini.cpp: real screen size
int COPYSizeX = 0;                  // ddini.cpp
int LX_fmap = 0;                    // fog.cpp: fmap grid dimension (fog stubs unused)
word RLCNSpr[1024 + 512];           // newmon.cpp: RLC sprite counters (read-only here)
int tmtmt = 0;                      // main.cpp: frame counter (InitXShift refs it, unused here)
short TSin[257], TCos[257];         // weapon.cpp: sine tables (read-only here)
void ErrM( char* s ) { fprintf( stderr, "ErrM: %s\n", s ); }  // newmon/gtext versions not linked

// bmptool.cpp's memory wrappers (guarded on Linux) - plain malloc/free
void* _ExMalloc( int Size ) { return malloc( Size ); }
void _ExFree( void* ptr ) { free( ptr ); }

// NLine: skip to end of line (defined in activescenary.cpp in the game -
// too heavy to link; same semantics via Ggetch)
void NLine( GFILE* f );
void NLine( GFILE* f )
{
	int c;
	do { c = Ggetch( f ); } while ( c != 0x0A && c != EOF );
}

// normstr: normalize a string to uppercase (used by gtext/exconst parsing)
void normstr( char* str )
{
	for ( char* p = str; *p; p++ ) *p = (char) toupper( (unsigned char) *p );
}

// ---------------------------------------------------------------- font
// Public-domain 8x8 bitmap font (classic font8x8_basic, chars 0x20..0x7E).
// Each byte is one pixel row, bit 0 = leftmost pixel.
static const uint8_t kFont[96][8] = {
	{ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 }, // ' '
	{ 0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00 }, // '!'
	{ 0x36,0x36,0x00,0x00,0x00,0x00,0x00,0x00 }, // '"'
	{ 0x36,0x36,0x7F,0x36,0x36,0x7F,0x36,0x36 }, // '#'
	{ 0x0C,0x3E,0x03,0x1E,0x30,0x1F,0x0C,0x00 }, // '$'
	{ 0x00,0x63,0x33,0x18,0x0C,0x66,0x63,0x00 }, // '%'
	{ 0x1C,0x36,0x1C,0x6E,0x3B,0x33,0x6E,0x00 }, // '&'
	{ 0x06,0x06,0x03,0x00,0x00,0x00,0x00,0x00 }, // '\''
	{ 0x18,0x0C,0x06,0x06,0x06,0x0C,0x18,0x00 }, // '('
	{ 0x06,0x0C,0x18,0x18,0x18,0x0C,0x06,0x00 }, // ')'
	{ 0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00 }, // '*'
	{ 0x00,0x0C,0x0C,0x3F,0x0C,0x0C,0x00,0x00 }, // '+'
	{ 0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x06 }, // ','
	{ 0x00,0x00,0x00,0x3F,0x00,0x00,0x00,0x00 }, // '-'
	{ 0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x00 }, // '.'
	{ 0x60,0x30,0x18,0x0C,0x06,0x03,0x01,0x00 }, // '/'
	{ 0x3E,0x63,0x73,0x7B,0x6F,0x67,0x3E,0x00 }, // '0'
	{ 0x0C,0x0E,0x0C,0x0C,0x0C,0x0C,0x3F,0x00 }, // '1'
	{ 0x1E,0x33,0x30,0x1C,0x06,0x33,0x3F,0x00 }, // '2'
	{ 0x1E,0x33,0x30,0x1C,0x30,0x33,0x1E,0x00 }, // '3'
	{ 0x38,0x3C,0x36,0x33,0x7F,0x30,0x78,0x00 }, // '4'
	{ 0x3F,0x03,0x1F,0x30,0x30,0x33,0x1E,0x00 }, // '5'
	{ 0x1C,0x06,0x03,0x1F,0x33,0x33,0x1E,0x00 }, // '6'
	{ 0x3F,0x33,0x30,0x18,0x0C,0x0C,0x0C,0x00 }, // '7'
	{ 0x1E,0x33,0x33,0x1E,0x33,0x33,0x1E,0x00 }, // '8'
	{ 0x1E,0x33,0x33,0x3E,0x30,0x18,0x0E,0x00 }, // '9'
	{ 0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x00 }, // ':'
	{ 0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x06 }, // ';'
	{ 0x18,0x0C,0x06,0x03,0x06,0x0C,0x18,0x00 }, // '<'
	{ 0x00,0x00,0x3F,0x00,0x00,0x3F,0x00,0x00 }, // '='
	{ 0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0x00 }, // '>'
	{ 0x1E,0x33,0x30,0x18,0x0C,0x00,0x0C,0x00 }, // '?'
	{ 0x3E,0x63,0x73,0x7B,0x7B,0x6F,0x3E,0x00 }, // '@'
	{ 0x0C,0x1E,0x33,0x33,0x3F,0x33,0x33,0x00 }, // 'A'
	{ 0x1F,0x33,0x33,0x1F,0x33,0x33,0x1F,0x00 }, // 'B'
	{ 0x3C,0x66,0x03,0x03,0x03,0x66,0x3C,0x00 }, // 'C'
	{ 0x1F,0x33,0x33,0x33,0x33,0x33,0x1F,0x00 }, // 'D'
	{ 0x3F,0x03,0x03,0x1F,0x03,0x03,0x3F,0x00 }, // 'E'
	{ 0x3F,0x03,0x03,0x1F,0x03,0x03,0x03,0x00 }, // 'F'
	{ 0x3C,0x66,0x03,0x03,0x73,0x66,0x7C,0x00 }, // 'G'
	{ 0x33,0x33,0x33,0x3F,0x33,0x33,0x33,0x00 }, // 'H'
	{ 0x1E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00 }, // 'I'
	{ 0x78,0x30,0x30,0x30,0x33,0x33,0x1E,0x00 }, // 'J'
	{ 0x33,0x33,0x36,0x3C,0x36,0x33,0x33,0x00 }, // 'K'
	{ 0x03,0x03,0x03,0x03,0x03,0x03,0x3F,0x00 }, // 'L'
	{ 0x63,0x77,0x7F,0x7F,0x6B,0x63,0x63,0x00 }, // 'M'
	{ 0x63,0x67,0x6F,0x6B,0x73,0x63,0x63,0x00 }, // 'N'
	{ 0x1C,0x36,0x63,0x63,0x63,0x36,0x1C,0x00 }, // 'O'
	{ 0x1F,0x33,0x33,0x1F,0x03,0x03,0x03,0x00 }, // 'P'
	{ 0x1E,0x33,0x33,0x33,0x3B,0x1E,0x38,0x00 }, // 'Q'
	{ 0x1F,0x33,0x33,0x1F,0x36,0x33,0x33,0x00 }, // 'R'
	{ 0x3E,0x63,0x03,0x1E,0x30,0x63,0x3E,0x00 }, // 'S'
	{ 0x3F,0x2D,0x0C,0x0C,0x0C,0x0C,0x1E,0x00 }, // 'T'
	{ 0x33,0x33,0x33,0x33,0x33,0x33,0x3F,0x00 }, // 'U'
	{ 0x33,0x33,0x33,0x33,0x33,0x1E,0x0C,0x00 }, // 'V'
	{ 0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00 }, // 'W'
	{ 0x63,0x63,0x36,0x1C,0x1C,0x36,0x63,0x00 }, // 'X'
	{ 0x33,0x33,0x33,0x1E,0x0C,0x0C,0x1E,0x00 }, // 'Y'
	{ 0x7F,0x63,0x31,0x18,0x4C,0x66,0x7F,0x00 }, // 'Z'
	{ 0x1E,0x06,0x06,0x06,0x06,0x06,0x1E,0x00 }, // '['
	{ 0x03,0x06,0x0C,0x18,0x30,0x60,0x40,0x00 }, // '\\'
	{ 0x1E,0x18,0x18,0x18,0x18,0x18,0x1E,0x00 }, // ']'
	{ 0x08,0x1C,0x36,0x63,0x00,0x00,0x00,0x00 }, // '^'
	{ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF }, // '_'
	{ 0x0C,0x0C,0x18,0x00,0x00,0x00,0x00,0x00 }, // '`'
	{ 0x00,0x00,0x1E,0x30,0x3E,0x33,0x6E,0x00 }, // 'a'
	{ 0x07,0x06,0x06,0x1E,0x36,0x36,0x1F,0x00 }, // 'b'
	{ 0x00,0x00,0x1E,0x33,0x03,0x33,0x1E,0x00 }, // 'c'
	{ 0x38,0x30,0x30,0x3E,0x33,0x33,0x6E,0x00 }, // 'd'
	{ 0x00,0x00,0x1E,0x33,0x3F,0x03,0x1E,0x00 }, // 'e'
	{ 0x1C,0x36,0x06,0x0F,0x06,0x06,0x0F,0x00 }, // 'f'
	{ 0x00,0x00,0x6E,0x33,0x33,0x3E,0x30,0x1F }, // 'g'
	{ 0x07,0x06,0x36,0x3E,0x66,0x66,0x67,0x00 }, // 'h'
	{ 0x0C,0x00,0x0E,0x0C,0x0C,0x0C,0x1E,0x00 }, // 'i'
	{ 0x30,0x00,0x30,0x30,0x30,0x33,0x1E,0x00 }, // 'j'
	{ 0x07,0x06,0x66,0x36,0x1E,0x36,0x67,0x00 }, // 'k'
	{ 0x0E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00 }, // 'l'
	{ 0x00,0x00,0x33,0x7F,0x7F,0x6B,0x63,0x00 }, // 'm'
	{ 0x00,0x00,0x1F,0x33,0x33,0x33,0x33,0x00 }, // 'n'
	{ 0x00,0x00,0x1E,0x33,0x33,0x33,0x1E,0x00 }, // 'o'
	{ 0x00,0x00,0x3B,0x36,0x36,0x3E,0x30,0x0F }, // 'p'
	{ 0x00,0x00,0x6E,0x33,0x33,0x3E,0x30,0x1F }, // 'q'
	{ 0x00,0x00,0x3B,0x6E,0x66,0x06,0x0F,0x00 }, // 'r'
	{ 0x00,0x00,0x3E,0x03,0x1E,0x30,0x1F,0x00 }, // 's'
	{ 0x08,0x0C,0x3E,0x0C,0x0C,0x2D,0x1E,0x00 }, // 't'
	{ 0x00,0x00,0x33,0x33,0x33,0x33,0x6E,0x00 }, // 'u'
	{ 0x00,0x00,0x33,0x33,0x33,0x1E,0x0C,0x00 }, // 'v'
	{ 0x00,0x00,0x63,0x6B,0x7F,0x77,0x63,0x00 }, // 'w'
	{ 0x00,0x00,0x63,0x36,0x1C,0x36,0x63,0x00 }, // 'x'
	{ 0x00,0x00,0x33,0x33,0x33,0x3E,0x30,0x1F }, // 'y'
	{ 0x00,0x00,0x3F,0x19,0x0C,0x26,0x3F,0x00 }, // 'z'
	{ 0x38,0x0C,0x0C,0x07,0x0C,0x0C,0x38,0x00 }, // '{'
	{ 0x00,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x00 }, // '|'
	{ 0x07,0x0C,0x0C,0x38,0x0C,0x0C,0x07,0x00 }, // '}'
	{ 0x6E,0x3B,0x00,0x00,0x00,0x00,0x00,0x00 }, // '~'
};

static const uint32_t COLOR_BG     = 0xFF101018;
static const uint32_t COLOR_PANE   = 0xFF181824;
static const uint32_t COLOR_TEXT   = 0xFFB8B8C0;
static const uint32_t COLOR_SEL    = 0xFF203050;
static const uint32_t COLOR_BRIGHT = 0xFFFFFFFF;
static const uint32_t COLOR_DIM    = 0xFF606078;

// ---------------------------------------------------------------- framebuffer
static const int W = 1280, H = 800;
static std::vector<uint32_t> pixels( W * H );

static void PutPixel( int x, int y, uint32_t c )
{
	if ( x >= 0 && x < W && y >= 0 && y < H ) pixels[y * W + x] = c;
}

static void FillRect( int x, int y, int w, int h, uint32_t c )
{
	for ( int j = y; j < y + h; j++ )
		for ( int i = x; i < x + w; i++ )
			PutPixel( i, j, c );
}

static void DrawChar( int x, int y, char ch, uint32_t c )
{
	if ( ch < 0x20 || ch > 0x7E ) ch = '?';
	const uint8_t* glyph = kFont[(int) ch - 0x20];
	for ( int j = 0; j < 8; j++ )
	{
		uint8_t bits = glyph[j];
		for ( int i = 0; i < 8; i++ )
			if ( bits & ( 1 << i ) ) PutPixel( x + i, y + j, c );
	}
}

static void DrawText( int x, int y, const std::string& s, uint32_t c )
{
	for ( char ch : s )
	{
		DrawChar( x, y, ch, c );
		x += 8;
	}
}

// ---------------------------------------------------------------- archive data
struct Entry
{
	std::string name;
	uint32_t size = 0;
	int index = 0;
	uint8_t flags = 0;
};

static std::vector<Entry> entries;          // all, sorted by name
static std::vector<int> visible;            // indices into entries after filter

// ---------------------------------------------------------------- preview
struct BmpImage
{
	bool ok = false;
	int w = 0, h = 0;
	std::vector<uint32_t> rgba;              // top-down
};

static std::vector<std::string> previewLines;
static int previewScroll = 0;
static BmpImage previewBmp;

// Game palette (RGB, loaded from "<set>\agew_1.pal" inside the archive)
static uint32_t gamePal[256];
static bool gamePalLoaded = false;
static std::string gamePalName;

// ---------------------------------------------------------------- game renderer
// The tool links the REAL game sprite renderer (gp_draw/fastdraw/fog/resfile).
// We give it a small scratch "screen" and drive GPS.Show* directly, so the
// variant previews below are produced by the exact code paths the game uses.
static const int kScrW = 512, kScrH = 512;
static uint8_t scratch[kScrW * kScrH];
static bool gameRendererReady = false;

static bool InitGameRenderer()
{
	// Registers override/resources/all .gsc archives into the game file system
	// (GSFILES) so RReset/LoadGP find the .gp and .grd files.
	if ( !FilesInit() )
	{
		fprintf( stderr, "game renderer: FilesInit() found no archives in CWD\n" );
		return false;
	}
	// Loads the real fog/gradient/transform tables (trans4/trans8/yfog/wfog/
	// Bright/Optional1-3) the Show* variants blend through.
	LoadFog( 2 );

	// Scratch screen for the sprite blits
	ScreenPtr = scratch;
	ScrWidth = kScrW;
	WindX = 0;
	WindY = 0;
	WindX1 = kScrW - 1;
	WindY1 = kScrH - 1;
	gameRendererReady = true;
	return true;
}

// Blit one sprite into the scratch buffer through a game Show* method,
// then copy the drawn region (bounding box of non-zero pixels) into a
// BmpImage cell with the game palette.
static bool RenderGpVariant( int fi, int spr, int variant, BmpImage& out, int cell );
static bool RenderGpVariant( int fi, int spr, int variant, BmpImage& out, int cell )
{
	// Varied checker background instead of black: blended layers (shadows,
	// transparency swatches) are invisible against a black backdrop, which
	// hid blend bugs in previews. Two terrain-ish palette indices.
	for ( int y = 0; y < kScrH; y++ )
		for ( int x = 0; x < kScrW; x++ )
			scratch[y * kScrW + x] = ( ( ( x >> 3 ) + ( y >> 3 ) ) & 1 ) ? 48 : 68;
	int cx = kScrW / 2, cy = kScrH / 2;
	switch ( variant )
	{
	case 0: GPS.ShowGP( cx, cy, fi, spr, 0 ); break;             // normal
	case 1: GPS.ShowGPRedN( cx, cy, fi, spr, 0, 2 ); break;     // red tint
	case 2: GPS.ShowGPDarkN( cx, cy, fi, spr, 0, 2 ); break;     // dark tint
	case 3: GPS.ShowGPTransparent( cx, cy, fi, spr, 0 ); break; // semi-transparent
	default: return false;
	}

	// bounding box of drawn pixels (background = 0)
	int x0 = kScrW, y0 = kScrH, x1 = 0, y1 = 0;
	for ( int y = 0; y < kScrH; y++ )
	{
		const uint8_t* row = scratch + y * kScrW;
		for ( int x = 0; x < kScrW; x++ )
			if ( row[x] )
			{
				if ( x < x0 ) x0 = x;
				if ( x > x1 ) x1 = x;
				if ( y < y0 ) y0 = y;
				if ( y > y1 ) y1 = y;
			}
	}
	if ( x0 > x1 || y0 > y1 ) return false;   // nothing drawn

	int bw = x1 - x0 + 1, bh = y1 - y0 + 1;
	float sc = std::min( (float) cell / bw, (float) cell / bh );
	if ( sc > 4.0f ) sc = 4.0f;
	int dw = std::max( 1, (int) ( bw * sc ) );
	int dh = std::max( 1, (int) ( bh * sc ) );
	int ox = ( cell - dw ) / 2;
	int oy = ( cell - dh ) / 2;

	out.ok = true;
	out.w = cell;
	out.h = cell;
	out.rgba.assign( (size_t) cell * cell, 0xFF242430 );
	for ( int y = 0; y < dh; y++ )
	{
		int sy = y0 + std::min( bh - 1, (int) ( y / sc ) );
		for ( int x = 0; x < dw; x++ )
		{
			int sx = x0 + std::min( bw - 1, (int) ( x / sc ) );
			uint8_t idx = scratch[sy * kScrW + sx];
			uint32_t c = gamePalLoaded ? gamePal[idx] : 0xFF808080;
			out.rgba[(size_t) ( oy + y ) * cell + ox + x] = c;
		}
	}
	return true;
}

// Render the variant strip for the first picture of a .gp page through the
// game's own drawing methods. Returns false if the game renderer is not
// available or the page could not be loaded.
static const int kVariantCell = 132;
static bool HasExt( const std::string& name, const char* ext );
static bool RenderGpVariantStrip( const Entry& e, int spr, BmpImage& strip )
{
	if ( !gameRendererReady ) return false;
	std::string name = e.name;
	if ( HasExt( name, ".gp" ) ) name.resize( name.size() - 3 );  // strip ".gp"

	int fi = GPS.PreLoadGPImage( const_cast<char*>( name.c_str() ) );
	if ( fi < 0 || GPS.GPNFrames[fi] <= 0 ) return false;
	if ( spr >= GPS.GPNFrames[fi] ) spr = 0;

	const int nVariants = 4;
	strip.ok = true;
	strip.w = kVariantCell * nVariants;
	strip.h = kVariantCell;
	strip.rgba.assign( (size_t) strip.w * strip.h, 0xFF242430 );
	for ( int v = 0; v < nVariants; v++ )
	{
		BmpImage cell;
		if ( !RenderGpVariant( fi, spr, v, cell, kVariantCell ) ) continue;
		int ox = v * kVariantCell;
		for ( int y = 0; y < cell.h; y++ )
			for ( int x = 0; x < cell.w; x++ )
				strip.rgba[(size_t) y * strip.w + ox + x] = cell.rgba[(size_t) y * cell.w + x];
	}
	GPS.UnLoadGP( fi );
	return true;
}

// ---------------------------------------------------------------- animation
// The game animates .gp pages by cycling the picture index at 40 ms/frame
// through GPS.ShowGP (see Video_OnDraw in dialogs.cpp - the anm_* buttons).
// TAB toggles the same playback here, through the real game renderer.
static bool animEnabled = false;
static int animFrame = 0;
static Uint64 animLastTick = 0;
static int animFrameMs = 100;         // adjustable with +/- (game buttons use 40)
static int animFi = -1;
static std::string animPage;

static void UnloadAnimPage()
{
	if ( animFi >= 0 && animFi < GPS.NGP ) GPS.UnLoadGP( animFi );
	animFi = -1;
	animPage.clear();
}

// Render the next frame of the selected .gp entry into previewBmp/previewLines.
static void BuildAnimatedFrame( const Entry& e )
{
	std::string name = e.name;
	if ( HasExt( name, ".gp" ) ) name.resize( name.size() - 3 );

	if ( animPage != name )
	{
		UnloadAnimPage();
		animFi = GPS.PreLoadGPImage( const_cast<char*>( name.c_str() ) );
		if ( animFi < 0 || GPS.GPNFrames[animFi] <= 0 )
		{
			animFi = -1;
			return;
		}
		animPage = name;
		animFrame = 0;
	}

	int np = GPS.GPNFrames[animFi];
	int frame = animFrame % np;
	animFrame++;

	previewLines.clear();
	previewScroll = 0;
	char info[256];
	snprintf( info, sizeof info, "%s  (%u bytes)", e.name.c_str(), e.size );
	previewLines.push_back( info );
	snprintf( info, sizeof info, "ANIMATION frame %d/%d  (%d ms/frame, +/- adjusts; game buttons run 40) - TAB stops",
		frame + 1, np, animFrameMs );
	previewLines.push_back( info );

	previewBmp = BmpImage();
	RenderGpVariant( animFi, frame, 0, previewBmp, 400 );
}

static void LoadGamePalette( CGSCarch& arch )
{
	const char* candidates[] = { "0\\agew_1.pal", "2\\agew_1.pal" };
	for ( const char* name : candidates )
	{
		LPGSCfile f = arch.GetFileHandle( const_cast<char*>( name ) );
		if ( !f ) continue;
		uint8_t buf[768];
		arch.ReadFile( f, buf, sizeof buf );
		arch.CloseFileHandle( f );
		for ( int i = 0; i < 256; i++ )
			gamePal[i] = 0xFF000000 | ( buf[i * 3] << 16 ) | ( buf[i * 3 + 1] << 8 ) | buf[i * 3 + 2];
		gamePalLoaded = true;
		gamePalName = name;
		return;
	}
}

static bool HasExt( const std::string& name, const char* ext )
{
	size_t n = strlen( ext );
	return name.size() >= n && strcasecmp( name.c_str() + name.size() - n, ext ) == 0;
}

static bool LooksLikeText( const uint8_t* data, uint32_t size )
{
	if ( !size ) return false;
	uint32_t printable = 0, checked = std::min<uint32_t>( size, 4096 );
	for ( uint32_t i = 0; i < checked; i++ )
		if ( data[i] == '\n' || data[i] == '\r' || data[i] == '\t' ||
			( data[i] >= 0x20 && data[i] <= 0x7E ) ) printable++;
	return printable >= ( checked * 9 ) / 10;
}

static uint32_t RdU32( const uint8_t* p ) { return p[0] | ( p[1] << 8 ) | ( (uint32_t) p[2] << 16 ) | ( (uint32_t) p[3] << 24 ); }
static uint16_t RdU16( const uint8_t* p ) { return (uint16_t) ( p[0] | ( p[1] << 8 ) ); }

static BmpImage TryDecodeBmp( const std::vector<uint8_t>& d )
{
	BmpImage bad;
	if ( d.size() < 54 || d[0] != 'B' || d[1] != 'M' ) return bad;
	uint32_t dataOfs = RdU32( &d[10] );
	uint32_t hdrSize = RdU32( &d[14] );
	if ( hdrSize < 40 || d.size() < dataOfs ) return bad;
	int w = (int) (int32_t) RdU32( &d[18] );
	int h = (int) (int32_t) RdU32( &d[22] );
	uint16_t bpp = RdU16( &d[28] );
	uint32_t comp = RdU32( &d[30] );
	bool bottomUp = h > 0;
	h = h < 0 ? -h : h;
	if ( w <= 0 || h <= 0 || comp != 0 || ( bpp != 8 && bpp != 24 ) ) return bad;

	BmpImage img;
	img.ok = true;
	img.w = w;
	img.h = h;
	img.rgba.resize( (size_t) w * h );
	uint32_t rowSize = ( (uint32_t) w * bpp / 8 + 3 ) & ~3u;
	for ( int y = 0; y < h; y++ )
	{
		const uint8_t* row = d.data() + dataOfs + (size_t) y * rowSize;
		int dy = bottomUp ? ( h - 1 - y ) : y;
		for ( int x = 0; x < w; x++ )
		{
			uint32_t c;
			if ( bpp == 24 )
			{
				c = 0xFF000000 | ( row[3 * x + 2] << 16 ) | ( row[3 * x + 1] << 8 ) | row[3 * x];
			}
			else
			{
				uint32_t pi = row[x];
				const uint8_t* pal = d.data() + 14 + hdrSize + pi * 4;
				c = 0xFF000000 | ( pal[2] << 16 ) | ( pal[1] << 8 ) | pal[0];
			}
			img.rgba[(size_t) dy * w + x] = c;
		}
	}
	return img;
}

// Preview pane: 8px font, pane starts at listW+12 and ends 12px from the right edge.
static const int kWrapWidth = ( W - 540 - 12 - 12 ) / 8;

// Append one logical line, word-wrapped to kWrapWidth columns.
// Breaks at the last space before the limit; words longer than the limit
// are broken hard.
static void PushWrapped( std::vector<std::string>& out, const std::string& line )
{
	size_t start = 0;
	while ( start < line.size() )
	{
		if ( line.size() - start <= (size_t) kWrapWidth )
		{
			out.push_back( line.substr( start ) );
			return;
		}
		size_t brk = line.find_last_of( ' ', start + kWrapWidth );
		if ( brk == std::string::npos || brk <= start )
			brk = start + kWrapWidth;      // hard break inside a long word
		// strip trailing spaces of the wrapped chunk
		size_t end = brk;
		while ( end > start && line[end - 1] == ' ' ) end--;
		out.push_back( line.substr( start, end - start ) );
		start = brk + 1;
		while ( start < line.size() && line[start] == ' ' ) start++;  // skip spaces after the break
	}
}

static void BuildTextPreview( const std::vector<uint8_t>& d )
{
	std::string line;
	uint32_t limit = std::min<uint32_t>( d.size(), 512 * 1024 );
	for ( uint32_t i = 0; i < limit; i++ )
	{
		uint8_t c = d[i];
		if ( c == '\n' )
		{
			PushWrapped( previewLines, line );
			line.clear();
			if ( previewLines.size() > 20000 ) break;
		}
		else if ( c == '\r' || c == '\t' ) { continue; }
		else if ( c >= 0x20 && c <= 0x7E ) line += (char) c;
		else line += '.';
	}
	if ( !line.empty() ) PushWrapped( previewLines, line );
}

static void BuildHexPreview( const std::vector<uint8_t>& d, uint32_t showBytes );

// GP decode (defined further below)
static BmpImage TryDecodeGp( const std::vector<uint8_t>& d, int& spriteCount, int& errors );
static void BuildHexPreview( const std::vector<uint8_t>& d, uint32_t showBytes )
{
	uint32_t limit = std::min<uint32_t>( d.size(), showBytes );
	char buf[160];
	for ( uint32_t off = 0; off < limit; off += 16 )
	{
		int p = snprintf( buf, sizeof buf, "%08X  ", off );
		for ( int i = 0; i < 16; i++ )
		{
			if ( off + i < limit )
				p += snprintf( buf + p, sizeof buf - p, "%02X ", d[off + i] );
			else
				p += snprintf( buf + p, sizeof buf - p, "   " );
		}
		p += snprintf( buf + p, sizeof buf - p, " " );
		for ( int i = 0; i < 16 && off + i < limit; i++ )
		{
			uint8_t c = d[off + i];
			buf[p++] = ( c >= 0x20 && c <= 0x7E ) ? (char) c : '.';
		}
		buf[p] = 0;
		previewLines.push_back( buf );
	}
}

static void BuildPreview( CGSCarch& arch, const Entry& e )
{
	previewLines.clear();
	previewScroll = 0;
	previewBmp = BmpImage();

	std::vector<uint8_t> data( e.size ? e.size : 1 );
	LPGSCfile f = arch.GetFileHandle( const_cast<char*>( e.name.c_str() ) );
	if ( !f )
	{
		previewLines.push_back( "<cannot open file handle>" );
		return;
	}
	if ( e.size ) arch.ReadFile( f, data.data(), e.size );
	arch.CloseFileHandle( f );

	char hdr[128];
	snprintf( hdr, sizeof hdr, "%s  (%u bytes, flags %u)", e.name.c_str(), e.size, e.flags );
	previewLines.push_back( hdr );
	previewLines.push_back( "" );

	if ( HasExt( e.name, ".bmp" ) )
	{
		previewBmp = TryDecodeBmp( data );
		if ( previewBmp.ok )
		{
			char info[128];
			snprintf( info, sizeof info, "BMP %dx%d", previewBmp.w, previewBmp.h );
			previewLines.push_back( info );
			previewLines.push_back( "" );
			if ( !LooksLikeText( data.data(), std::min<uint32_t>( e.size, 4096 ) ) )
				BuildHexPreview( data, 64 );
		}
		else
		{
			previewLines.push_back( "Not a decodable BMP (unsupported variant)" );
			BuildHexPreview( data, 512 );
		}
	}
	else if ( HasExt( e.name, ".gp" ) )
	{
		int spriteCount = 0, errors = 0;
		BmpImage sheet = TryDecodeGp( data, spriteCount, errors );
		char info[160];
		if ( sheet.ok )
		{
			snprintf( info, sizeof info, "GP sprite page: %d picture(s)%s%s",
				spriteCount, errors ? ", decode errors on some" : "",
				gamePalLoaded ? "" : " (no palette found)" );
		}
		else
		{
			snprintf( info, sizeof info, "GP: could not decode (%d pictures)", spriteCount );
		}
		previewLines.push_back( info );

		// Variant strip: the first picture rendered by the REAL game code
		// (GPS.ShowGP / ShowGPRedN / ShowGPDarkN / ShowGPTransparent)
		BmpImage strip;
		if ( RenderGpVariantStrip( e, 0, strip ) )
		{
			previewLines.push_back( "variants below (game code): normal | red N=2 | dark N=2 | transparent" );
			if ( sheet.ok )
			{
				// strip on top, then the full contact sheet
				BmpImage both;
				both.ok = true;
				both.w = std::max( strip.w, sheet.w );
				both.h = strip.h + 8 + sheet.h;
				both.rgba.assign( (size_t) both.w * both.h, 0xFF242430 );
				for ( int y = 0; y < strip.h; y++ )
					for ( int x = 0; x < strip.w; x++ )
						both.rgba[(size_t) y * both.w + x] = strip.rgba[(size_t) y * strip.w + x];
				int oy = strip.h + 8;
				for ( int y = 0; y < sheet.h; y++ )
					for ( int x = 0; x < sheet.w; x++ )
						both.rgba[(size_t) ( oy + y ) * both.w + x] = sheet.rgba[(size_t) y * sheet.w + x];
				previewBmp = both;
			}
			else
			{
				previewBmp = strip;
			}
		}
		else
		{
			previewBmp = sheet;
		}
		previewLines.push_back( "" );
		BuildHexPreview( data, 64 );
	}
	else if ( HasExt( e.name, ".grd" ) || HasExt( e.name, ".set" ) )
	{
		// GRD files are byte-mapping tables (fog/gradient/transform LUTs).
		// Sizes used by the game are all multiples of 256, so render them
		// as 256-wide palettized images with the game palette.
		if ( gamePalLoaded && e.size && ( e.size % 256 ) == 0 && e.size <= 1024 * 1024 )
		{
			previewBmp.ok = true;
			previewBmp.w = 256;
			previewBmp.h = (int) ( e.size / 256 );
			previewBmp.rgba.resize( e.size );
			for ( uint32_t i = 0; i < e.size; i++ )
				previewBmp.rgba[i] = gamePal[data[i]];
			char info[160];
			snprintf( info, sizeof info, "GRD table 256 x %d rows, rendered with palette %s",
				previewBmp.h, gamePalName.c_str() );
			previewLines.push_back( info );
			previewLines.push_back( "" );
			BuildHexPreview( data, 64 );
		}
		else
		{
			if ( !gamePalLoaded ) previewLines.push_back( "No palette found in archive, showing hex" );
			if ( e.size % 256 ) previewLines.push_back( "Table size is not a multiple of 256, showing hex" );
			BuildHexPreview( data, 512 );
		}
	}
	else if ( HasExt( e.name, ".pal" ) && e.size >= 768 )
	{
		// Raw RGB palette: show as a 16x16 grid
		previewBmp.ok = true;
		previewBmp.w = 16;
		previewBmp.h = 16;
		previewBmp.rgba.resize( 256 );
		for ( int i = 0; i < 256; i++ )
		{
			uint8_t r = data[i * 3], g = data[i * 3 + 1], b = data[i * 3 + 2];
			previewBmp.rgba[i] = 0xFF000000 | ( r << 16 ) | ( g << 8 ) | b;
		}
		previewLines.push_back( "Palette (16x16 grid)" );
		previewLines.push_back( "" );
	}
	else if ( HasExt( e.name, ".wav" ) && e.size >= 44 && !memcmp( data.data(), "RIFF", 4 ) )
	{
		uint16_t chans = RdU16( &data[22] );
		uint32_t rate = RdU32( &data[24] );
		uint16_t bits = RdU16( &data[34] );
		char info[128];
		snprintf( info, sizeof info, "WAV: %u Hz, %u channel(s), %u bit", rate, chans, bits );
		previewLines.push_back( info );
		BuildHexPreview( data, 64 );
	}
	else if ( LooksLikeText( data.data(), e.size ) )
	{
		BuildTextPreview( data );
	}
	else
	{
		BuildHexPreview( data, 8 * 1024 );
	}
}

// ---------------------------------------------------------------- GP sprites
#pragma pack(push, 1)
struct GpGlobalHdr                        // fixed part of GP_GlobalHeader (gp_draw.h, pack 1);
{                                          // the LGPH picture-offset table that follows is
	int32_t Sign;                          // NPictures DWORDs long, NOT a fixed 256 entries
	int16_t NPictures;
	int16_t Reserved;
	int32_t VocOffset;
	int16_t VocLength;
};
struct GpHdr                              // matches GP_Header (gp_draw.h, pack 1)
{
	int32_t NextPict;
	int16_t dx, dy, Lx, Ly;
	uint32_t Pack;
	uint8_t Options;
	uint32_t CData;
	int16_t NLines;
};
#pragma pack(pop)
static_assert( sizeof( GpGlobalHdr ) == 14, "GpGlobalHdr layout mismatch" );
static_assert( sizeof( GpHdr ) == 23, "GpHdr layout mismatch" );

// C ports of the game unpackers (gp_draw.cpp). Bounded variants for safe browsing.
static void StdUnpackT( uint8_t* dst, int dstCap, const uint8_t* src, int srcCap,
	const uint8_t* voc, int vocSize )
{
	int written = 0;
	while ( written < dstCap && srcCap > 0 )
	{
		uint8_t bits = *src++; srcCap--;
		for ( int b = 0; b < 8 && written < dstCap && srcCap > 0; b++, bits <<= 1 )
		{
			if ( bits & 0x80 )
			{
				if ( srcCap < 2 ) return;
				unsigned word = src[0] | ( (unsigned) src[1] << 8 );
				src += 2; srcCap -= 2;
				int count = ( word >> 12 ) + 3;
				int ofs = word & 0xFFF;
				if ( ofs >= vocSize ) return;
				while ( count-- > 0 && written < dstCap && ofs < vocSize )
					dst[written++] = voc[ofs++];
			}
			else
			{
				dst[written++] = *src++; srcCap--;
			}
		}
	}
}

// 2-bit national mask: each source byte -> 4 indices (0..3)
static void NatUnpackT( uint8_t* dst, int dstCap, const uint8_t* src, int srcCap )
{
	int written = 0;
	while ( written + 4 <= dstCap && srcCap > 0 )
	{
		uint8_t b = *src++; srcCap--;
		dst[written++] = b & 3;
		dst[written++] = ( b >> 2 ) & 3;
		dst[written++] = ( b >> 4 ) & 3;
		dst[written++] = ( b >> 6 ) & 3;
	}
}

struct GpSprite
{
	bool ok = false;
	bool natMask = false;
	int w = 0, h = 0;
	std::vector<uint8_t> pix;              // palette indices
};

// Decode one GP picture (header at hdrOfs, file base = d).
// Pixel stream: type 0 = StdUnpack (dictionary), type 1 = NatUnpack,
// other types = raw bytes at header+CDOffs. The per-line mask right
// behind the header places runs of pixels into the image (see
// GP_ShowMaskedPict in gp_draw.cpp).
static GpSprite DecodeGpSprite( const std::vector<uint8_t>& d, size_t hdrOfs )
{
	GpSprite bad;
	if ( hdrOfs + sizeof( GpHdr ) > d.size() ) return bad;
	const GpHdr* p = (const GpHdr*) ( d.data() + hdrOfs );
	int Lx = p->Lx, Ly = p->NLines;
	if ( Lx <= 0 || Lx > 4096 || Ly <= 0 || Ly > 4096 ) return bad;

	uint8_t opt = p->Options;
	int unpackLen = (int) ( p->CData >> 14 );
	int cdOffs = ( p->CData & 16383 ) + ( ( opt & 64 ) ? 16384 : 0 ) + ( ( opt & 128 ) ? 32768 : 0 );
	int type = opt & 63;
	if ( type == 43 ) unpackLen += 262144;
	if ( type == 44 ) unpackLen += 262144 * 2;
	if ( unpackLen < 0 || unpackLen > 4 * 1024 * 1024 ) return bad;

	const uint8_t* pix = nullptr;
	size_t pixCap = 0;
	std::vector<uint8_t> unpacked;
	size_t dataOfs = hdrOfs + (size_t) cdOffs;

	if ( type == 0 )
	{
		int vocSize = 0;
		const uint8_t* voc = nullptr;
		if ( d.size() >= 12 )
		{
			uint32_t vocOfs = RdU32( d.data() + 8 );
			if ( vocOfs > 0 && (size_t) vocOfs < d.size() )
			{
				voc = d.data() + vocOfs;
				vocSize = std::min<int>( 4096, (int) ( d.size() - vocOfs ) );
			}
		}
		unpacked.resize( unpackLen );
		if ( dataOfs < d.size() )
			StdUnpackT( unpacked.data(), unpackLen, d.data() + dataOfs,
				(int) ( d.size() - dataOfs ), voc ? voc : d.data(), vocSize );
		pix = unpacked.data();
		pixCap = unpacked.size();
	}
	else if ( type == 1 )
	{
		unpacked.resize( unpackLen );
		if ( dataOfs < d.size() )
			NatUnpackT( unpacked.data(), unpackLen, d.data() + dataOfs,
				(int) ( d.size() - dataOfs ) );
		pix = unpacked.data();
		pixCap = unpacked.size();
	}
	else
	{
		if ( dataOfs >= d.size() ) return bad;
		pix = d.data() + dataOfs;
		pixCap = d.size() - dataOfs;
	}

	GpSprite s;
	s.ok = true;
	s.natMask = ( type == 1 );
	s.w = Lx;
	s.h = Ly;
	s.pix.assign( (size_t) Lx * Ly, 255 );        // 255 = transparent

	size_t maskOfs = hdrOfs + sizeof( GpHdr );
	size_t pixPos = 0;
	for ( int row = 0; row < Ly; row++ )
	{
		if ( maskOfs >= d.size() ) break;
		uint8_t m = d[maskOfs++];
		if ( !m ) continue;
		int col = 0;
		if ( !( m & 0x80 ) )
		{
			int nSeg = m & 0x7F;
			for ( int sgi = 0; sgi < nSeg && maskOfs + 1 < d.size(); sgi++ )
			{
				int skip = d[maskOfs++];
				int count = d[maskOfs++];
				col += skip;
				for ( int i = 0; i < count && pixPos < pixCap; i++, pixPos++ )
				{
					if ( col >= 0 && col < Lx && row < Ly )
						s.pix[(size_t) row * Lx + col] = pix[pixPos];
					col++;
				}
			}
		}
		else
		{
			int spaceMask = ( m & 0x40 ) ? 16 : 0;
			int pixMask = ( m & 0x20 ) ? 16 : 0;
			int nSeg = m & 0x1F;
			for ( int sgi = 0; sgi < nSeg && maskOfs < d.size(); sgi++ )
			{
				uint8_t packed = d[maskOfs++];
				int skip = ( packed & 0x0F ) | spaceMask;
				int count = ( packed >> 4 ) | pixMask;
				col += skip;
				for ( int i = 0; i < count && pixPos < pixCap; i++, pixPos++ )
				{
					if ( col >= 0 && col < Lx && row < Ly )
						s.pix[(size_t) row * Lx + col] = pix[pixPos];
					col++;
				}
			}
		}
	}
	return s;
}

// Build a contact sheet of all pictures in a .gp file.
// Layout: Sign(4) NPictures(2) Reserved(2) VocOffset(4) VocLength(2),
// then NPictures picture-offset DWORDs (variable-length table).
static BmpImage TryDecodeGp( const std::vector<uint8_t>& d, int& spriteCount, int& errors )
{
	BmpImage bad;
	if ( d.size() < 14 ) return bad;
	int np = RdU16( d.data() + 4 );
	if ( np <= 0 || np > 4096 ) return bad;
	if ( 14 + (size_t) np * 4 > d.size() ) return bad;

	spriteCount = 0;
	errors = 0;
	std::vector<GpSprite> sprites;
	for ( int n = 0; n < np; n++ )
	{
		size_t ofs = RdU32( d.data() + 14 + 4 * n );
		GpSprite s = ( ofs < d.size() ) ? DecodeGpSprite( d, ofs ) : GpSprite();
		if ( s.ok ) sprites.push_back( s ), spriteCount++;
		else errors++;
	}
	if ( sprites.empty() ) return bad;

	const int cell = 68;                     // sprite thumbnail 64px + 4px gap
	const int maxShown = 512;                // cap the sheet for very large pages
	if ( (int) sprites.size() > maxShown ) sprites.resize( maxShown );
	int cols = 16;
	int rows = (int) ( ( sprites.size() + cols - 1 ) / cols );

	BmpImage img;
	img.ok = true;
	img.w = cols * cell;
	img.h = rows * cell;
	img.rgba.assign( (size_t) img.w * img.h, 0xFF242430 );   // sheet background

	for ( size_t i = 0; i < sprites.size(); i++ )
	{
		const GpSprite& s = sprites[i];
		int cx = (int) ( ( i % cols ) * cell + 2 );
		int cy = (int) ( ( i / cols ) * cell + 2 );
		// fit the sprite into a 64x64 cell (fractional nearest-neighbor;
		// tiny sprites are upscaled at most 4x)
		float sc = std::min( 64.0f / s.w, 64.0f / s.h );
		if ( sc > 4.0f ) sc = 4.0f;
		int dw = std::max( 1, (int) ( s.w * sc ) );
		int dh = std::max( 1, (int) ( s.h * sc ) );
		int ox = cx + ( 64 - dw ) / 2;
		int oy = cy + ( 64 - dh ) / 2;
		for ( int y = 0; y < dh; y++ )
		{
			int sy = std::min( s.h - 1, (int) ( y / sc ) );
			for ( int x = 0; x < dw; x++ )
			{
				int sx = std::min( s.w - 1, (int) ( x / sc ) );
				uint8_t idx = s.pix[(size_t) sy * s.w + sx];
				uint32_t c;
				if ( idx == 255 ) c = 0xFF181820;      // transparent
				else if ( s.natMask ) c = idx == 0 ? 0xFF101010 :
					( idx == 1 ? 0xFFD8D8D8 : ( idx == 2 ? 0xFF909090 : 0xFF404040 ) );
				else if ( !gamePalLoaded ) c = 0xFF808080;
				else c = gamePal[idx];
				img.rgba[(size_t) ( oy + y ) * img.w + ox + x] = c;
			}
		}
	}
	return img;
}

// ---------------------------------------------------------------- main
int main( int argc, char** argv )
{
	const char* archPath = argc > 1 ? argv[1] : "all.gsc";
	bool dumpMode = argc > 3 && !strcmp( argv[1], "--dump" );
	bool listMode = argc > 2 && !strcmp( argv[1], "--list" );
	if ( dumpMode || listMode ) archPath = argv[2];

	CGSCarch arch;
	if ( !arch.Open( const_cast<char*>( archPath ) ) )
	{
		fprintf( stderr, "Cannot open archive %s\n", archPath );
		return 1;
	}

	int n = arch.GetEntryCount();
	for ( int i = 0; i < n; i++ )
	{
		const TGSCarchFAT* fat = arch.GetEntryFAT( i );
		Entry e;
		e.name = (const char*) fat->m_FileName;
		e.size = fat->m_Size;
		e.index = i;
		e.flags = fat->m_Flags;
		entries.push_back( e );
	}
	std::sort( entries.begin(), entries.end(),
		[]( const Entry& a, const Entry& b ) { return a.name < b.name; } );

	// Palette for rendering GRD tables (falls back to hex if absent)
	LoadGamePalette( arch );

	// Game renderer: registers archives in the game file system (GSFILES),
	// loads the real fog/transform tables, sets up the scratch screen
	InitGameRenderer();

	// Headless mode: gscbrowser --dump <archive> <file> -> print preview info, no UI
	if ( dumpMode )
	{
		const char* want = argv[3];
		for ( const Entry& e : entries )
		{
			if ( strcasecmp( e.name.c_str(), want ) != 0 ) continue;
			BuildPreview( arch, e );
			for ( const std::string& l : previewLines ) printf( "%s\n", l.c_str() );
			printf( "image: %s (%dx%d)\n", previewBmp.ok ? "yes" : "no",
				previewBmp.w, previewBmp.h );
			return 0;
		}
		fprintf( stderr, "%s not found in %s\n", want, archPath );
		return 1;
	}

	// Headless mode: gscbrowser --list <archive> -> print all entry names
	if ( argc > 2 && !strcmp( argv[1], "--list" ) )
	{
		for ( const Entry& e : entries ) printf( "%s\n", e.name.c_str() );
		return 0;
	}

	if ( !SDL_Init( SDL_INIT_VIDEO ) )
	{
		fprintf( stderr, "SDL_Init failed: %s\n", SDL_GetError() );
		return 1;
	}
	SDL_Window* win = SDL_CreateWindow( "GSC resource browser", W, H, 0 );
	sdlWindow = win;
	SDL_Renderer* ren = SDL_CreateRenderer( win, nullptr );
	SDL_Texture* tex = SDL_CreateTexture( ren, SDL_PixelFormat::SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING, W, H );

	std::string filter;
	int sel = 0, listScroll = 0;
	bool quit = false;
	bool needPreview = true;

	auto applyFilter = [&]()
	{
		visible.clear();
		std::string f;
		for ( char c : filter ) f += (char) toupper( (unsigned char) c );
		for ( int i = 0; i < (int) entries.size(); i++ )
		{
			std::string nm = entries[i].name;
			for ( char& c : nm ) if ( c == '\\' ) c = '/';
			std::string ff = f;
			for ( char& c : ff ) if ( c == '\\' ) c = '/';
			if ( nm.find( ff ) != std::string::npos ) visible.push_back( i );
		}
		if ( sel >= (int) visible.size() ) sel = 0;
		listScroll = 0;
		needPreview = true;
	};
	applyFilter();

	while ( !quit )
	{
		SDL_Event ev;
		while ( SDL_PollEvent( &ev ) )
		{
			if ( ev.type == SDL_EVENT_QUIT ) quit = true;
			if ( ev.type == SDL_EVENT_KEY_DOWN )
			{
				SDL_Keycode k = ev.key.key;
				if ( k == SDLK_ESCAPE ) quit = true;
				else if ( k == SDLK_TAB )
				{
					// toggle animation playback of the selected .gp page
					bool selIsGp = !visible.empty() && sel < (int) visible.size()
						&& HasExt( entries[visible[sel]].name, ".gp" );
					if ( selIsGp )
					{
						animEnabled = !animEnabled;
						animFrame = 0;
						animLastTick = 0;
						if ( !animEnabled )
						{
							UnloadAnimPage();
							needPreview = true;   // back to static preview
						}
					}
				}
				else if ( k == SDLK_MINUS || k == SDLK_KP_MINUS )
				{
					animFrameMs = std::min( 500, animFrameMs + 20 );
				}
				else if ( k == SDLK_PLUS || k == SDLK_EQUALS || k == SDLK_KP_PLUS )
				{
					animFrameMs = std::max( 20, animFrameMs - 20 );
				}
				else if ( k == SDLK_UP ) { if ( sel > 0 ) { sel--; needPreview = true; } }
				else if ( k == SDLK_DOWN ) { if ( sel + 1 < (int) visible.size() ) { sel++; needPreview = true; } }
				else if ( k == SDLK_PAGEUP ) { sel = std::max( 0, sel - 20 ); needPreview = true; }
				else if ( k == SDLK_PAGEDOWN ) { sel = std::min( (int) visible.size() - 1, sel + 20 ); needPreview = true; }
				else if ( k == SDLK_HOME ) { sel = 0; needPreview = true; }
				else if ( k == SDLK_END ) { sel = std::max( 0, (int) visible.size() - 1 ); needPreview = true; }
				else if ( k == SDLK_BACKSPACE )
				{
					if ( !filter.empty() ) { filter.pop_back(); applyFilter(); }
				}
				else if ( ev.key.mod & SDL_KMOD_CTRL && k == SDLK_U )
				{
					filter.clear(); applyFilter();
				}
				else if ( k >= SDLK_SPACE && k <= SDLK_Z )
				{
					char c = (char) k;
					if ( !( ev.key.mod & ( SDL_KMOD_LSHIFT | SDL_KMOD_RSHIFT ) ) )
						c = (char) tolower( (unsigned char) c );
					filter += c;
					applyFilter();
				}
			}
		}

		if ( needPreview && !visible.empty() && sel < (int) visible.size() )
		{
			const Entry& selEntry = entries[visible[sel]];
			if ( animEnabled && HasExt( selEntry.name, ".gp" ) )
			{
				// animation tick below paints the preview each frame
				needPreview = false;
				animLastTick = 0;   // draw the first frame immediately
			}
			else
			{
				BuildPreview( arch, selEntry );
				needPreview = false;
			}
		}

		// animation playback (frame time adjustable with +/-, default 100 ms)
		if ( animEnabled && !visible.empty() && sel < (int) visible.size()
			&& HasExt( entries[visible[sel]].name, ".gp" ) )
		{
			Uint64 now = SDL_GetTicks();
			if ( now - animLastTick >= (Uint64) animFrameMs )
			{
				animLastTick = now;
				BuildAnimatedFrame( entries[visible[sel]] );
			}
		}

		// ------------------------------------------------ render
		std::fill( pixels.begin(), pixels.end(), COLOR_BG );
		const int listW = 540;
		const int rowH = 12;
		FillRect( 0, 0, W, 14, COLOR_PANE );
		char hdr[256];
		snprintf( hdr, sizeof hdr, "%s : %d entries   filter: %s_   (Esc quits, type to filter, Ctrl+U clears, TAB animates GP)",
			archPath, (int) visible.size(), filter.c_str() );
		DrawText( 4, 3, hdr, COLOR_BRIGHT );

		int rowsVisible = ( H - 20 ) / rowH;
		if ( sel < listScroll ) listScroll = sel;
		if ( sel >= listScroll + rowsVisible ) listScroll = sel - rowsVisible + 1;

		for ( int r = 0; r < rowsVisible && r < (int) visible.size(); r++ )
		{
			int ei = visible[listScroll + r];
			const Entry& e = entries[ei];
			char row[160];
			snprintf( row, sizeof row, "%9u  %s", e.size, e.name.c_str() );
			if ( listScroll + r == sel ) FillRect( 0, 20 + r * rowH, listW, rowH, COLOR_SEL );
			uint32_t col = HasExt( e.name, ".bmp" ) ? COLOR_BRIGHT : COLOR_TEXT;
			DrawText( 4, 21 + r * rowH, row, col );
		}

		FillRect( listW, 14, W - listW, H - 14, COLOR_PANE );
		int px = listW + 12, py = 24;
		int maxLines = ( H - py - 12 ) / rowH;
		int start = std::min( previewScroll, std::max( 0, (int) previewLines.size() - maxLines ) );
		for ( int i = start; i < (int) previewLines.size() && i - start < maxLines; i++ )
			DrawText( px, py + ( i - start ) * rowH, previewLines[i],
				i == 0 ? COLOR_BRIGHT : COLOR_TEXT );

		if ( previewBmp.ok )
		{
			int areaX = px, areaY = py + 4 * rowH;
			int areaW = W - px - 12, areaH = H - areaY - 12;
			// fractional nearest-neighbor scale: fit the pane, upscale at most 6x
			float scale = std::min( (float) areaW / previewBmp.w, (float) areaH / previewBmp.h );
			scale = std::min( scale, 6.0f );
			int dw = std::max( 1, (int) ( previewBmp.w * scale ) );
			int dh = std::max( 1, (int) ( previewBmp.h * scale ) );
			int ox = areaX + ( areaW - dw ) / 2;
			int oy = areaY + ( areaH - dh ) / 2;
			for ( int y = 0; y < dh; y++ )
			{
				int sy = (int) ( y / scale );
				if ( sy >= previewBmp.h ) sy = previewBmp.h - 1;
				const uint32_t* srcRow = &previewBmp.rgba[(size_t) sy * previewBmp.w];
				for ( int x = 0; x < dw; x++ )
				{
					int sx = (int) ( x / scale );
					if ( sx >= previewBmp.w ) sx = previewBmp.w - 1;
					PutPixel( ox + x, oy + y, srcRow[sx] );
				}
			}
		}

		SDL_UpdateTexture( tex, nullptr, pixels.data(), W * 4 );
		SDL_RenderClear( ren );
		SDL_RenderTexture( ren, tex, nullptr, nullptr );
		SDL_RenderPresent( ren );
	}

	SDL_DestroyTexture( tex );
	SDL_DestroyRenderer( ren );
	SDL_DestroyWindow( win );
	SDL_Quit();
	return 0;
}
