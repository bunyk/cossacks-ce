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
//   Esc                       - quit

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../src/game/arc/gscarch.h"

// GSC_OpenError() in gscarch.cpp references this global
SDL_Window* sdlWindow = nullptr;

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

static void BuildTextPreview( const std::vector<uint8_t>& d )
{
	previewLines.clear();
	std::string line;
	uint32_t limit = std::min<uint32_t>( d.size(), 512 * 1024 );
	for ( uint32_t i = 0; i < limit; i++ )
	{
		uint8_t c = d[i];
		if ( c == '\n' )
		{
			previewLines.push_back( line );
			line.clear();
			if ( previewLines.size() > 20000 ) break;
		}
		else if ( c == '\r' || c == '\t' ) { continue; }
		else if ( c >= 0x20 && c <= 0x7E ) line += (char) c;
		else line += '.';
	}
	if ( !line.empty() ) previewLines.push_back( line );
}

static void BuildHexPreview( const std::vector<uint8_t>& d, uint32_t showBytes )
{
	previewLines.clear();
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

// ---------------------------------------------------------------- main
int main( int argc, char** argv )
{
	const char* archPath = argc > 1 ? argv[1] : "all.gsc";

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
			BuildPreview( arch, entries[visible[sel]] );
			needPreview = false;
		}

		// ------------------------------------------------ render
		std::fill( pixels.begin(), pixels.end(), COLOR_BG );
		const int listW = 540;
		const int rowH = 12;
		FillRect( 0, 0, W, 14, COLOR_PANE );
		char hdr[256];
		snprintf( hdr, sizeof hdr, "%s : %d entries   filter: %s_   (Esc quits, type to filter, Ctrl+U clears)",
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
			int scale = 1;
			while ( previewBmp.w * ( scale + 1 ) <= areaW && previewBmp.h * ( scale + 1 ) <= areaH ) scale++;
			int ox = areaX + ( areaW - previewBmp.w * scale ) / 2;
			int oy = areaY + ( areaH - previewBmp.h * scale ) / 2;
			for ( int y = 0; y < previewBmp.h; y++ )
				for ( int x = 0; x < previewBmp.w; x++ )
				{
					uint32_t c = previewBmp.rgba[(size_t) y * previewBmp.w + x];
					for ( int j = 0; j < scale; j++ )
						for ( int i = 0; i < scale; i++ )
							PutPixel( ox + x * scale + i, oy + y * scale + j, c );
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
