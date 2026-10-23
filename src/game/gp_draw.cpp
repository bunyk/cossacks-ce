#include <stdio.h>
#include <assert.h>
#include <math.h>

#include "newcode/os.h"
#include "ddini.h"
#include "resfile.h"
#include "fastdraw.h"
#include "gp_draw.h"
#include "gsinc.h"
#include "gfile.h"
#include "ctables.h"//extern declarations of fog/trans* color tables (needed on Linux too)

#include "mode.h"
#include "lines.h"
#include "mapdiscr.h"
#include "realwater.h"
#include "activescenary.h"

bool NewGPImage;

#define INTV(x) (((int*)(x))[0])

//What are you?
extern int XShift[512];

extern word RLCNSpr[MaxGPIdx];
void ShowRLCItemPal( int x, int y, lpRLCTable lprt, int n, byte* Pal );
void ShowRLCItemGrad( int x, int y, lpRLCTable lprt, int n, byte* Pal );
word GP_L_IDXS[MaxGPIdx];

int LOADED = 0;

typedef short* lpShort;
typedef DWORD* lpDWORD;
GP_System::GP_System()
{
	CashSize = 4200000;
	PackCash = new byte[CashSize + 4];
	PackCash[CashSize] = 0x37;
	PackCash[CashSize + 1] = 0x42;
	INTV( PackCash ) = 0;
	INTV( PackCash + 4 ) = CashSize;
	CashPos = 0;
	NGP = 0;
	NGPReady = MaxGPIdx;
	GPH = new lpGP_GlobalHeader[NGPReady];
	GPNames = new LPCHAR[NGPReady];
	GPSize = new int[NGPReady];
	GPLastTime = new int[NGPReady];
	RLCImage = new RLCTable[NGPReady];
	memset( RLCImage, 0, ( sizeof(RLCImage) )*NGPReady );
	RLCShadow = new RLCTable[NGPReady];
	memset( RLCShadow, 0, ( sizeof(RLCShadow) )*NGPReady );
	GPNFrames = new word[NGPReady];
	ImageType = new byte[NGPReady];
	UNITBL = (UNICODETABLE**) malloc( NGPReady * sizeof(UNICODETABLE*) );
	memset( ImageType, 0, NGPReady );
	memset( UNITBL, 0, NGPReady << 2 );
	ImLx = new lpShort[NGPReady];
	ImLy = new lpShort[NGPReady];
	ItDX = (char**) malloc( NGPReady * sizeof(char*) );
	ItLX = (char**) malloc( NGPReady * sizeof(char*) );
	memset( ImLx, 0, 4 * NGPReady );
	memset( ImLy, 0, 4 * NGPReady );
	memset( ItDX, 0, 4 * NGPReady );
	memset( ItLX, 0, 4 * NGPReady );
	CASHREF = new lpDWORD[NGPReady];
	Mapping = new byte[NGPReady];
	memset( CASHREF, 0, NGPReady << 2 );
	memset( Mapping, 0, NGPReady );
	//PreLoadGPImage("gets2");
	memset( GP_L_IDXS, 0, sizeof(GP_L_IDXS) );
};

GP_System::~GP_System()
{
	free( PackCash );
	for (int i = 0; i < NGP; i++)
	{
		if (GPH[i] && !Mapping[i])free( GPH[i] );
		if (RLCImage[i])free( RLCImage[i] );
		if (RLCShadow[i])free( RLCShadow[i] );
		free( GPNames[i] );
		if (CASHREF[i])free( CASHREF[i] );
	};
	free( GPH );
	free( GPNames );
	free( RLCImage );
	free( RLCShadow );
	free( GPNFrames );
	free( ImageType );
	free( CASHREF );
	free( Mapping );
	free( UNITBL );
};

int  GP_System::GetGPWidth( int i, int n )
{
	if (LoadGP( i ))
	{
		if (n >= GPS.GPNFrames[i])
			return 0;
		if (ItLX[i])
			return ItLX[i][n];
		else
			return ImLx[i][n];
	}
	else return 0;
};

int  GP_System::GetGPShift( int i, int n )
{
	if (LoadGP( i ))
	{
		if (n >= GPS.GPNFrames[i])return 0;
		if (ItDX[i])return ItDX[i][n];
		else return 0;
	}
	else return 0;
}

int  GP_System::GetGPHeight( int i, int n )
{
	if (LoadGP( i ))
	{
		if (n >= GPS.GPNFrames[i])
		{
			return 0;
		}
		return ImLy[i][n];
	}
	else
	{
		return 0;
	}
}

byte* GP_System::GetCash( int Size )
{
	int UsedSize = -8;
	int NSeg = 0;
	int tpos = CashPos;
	int cas = CashSize;
	//COUNTER=div(CashPos*100,CashSize).quot;
	byte* Cash = PackCash;
	//assert(Cash[CashSize]==0x37&&Cash[CashSize+1]==0x42);
	while (tpos < cas&&UsedSize < Size)
	{
		int sz = INTV( Cash + tpos + 4 );
		UsedSize += sz;
		tpos += sz;
		NSeg++;
	};
	if (UsedSize < Size)
	{
		tpos = 0;
		UsedSize = -8;
		NSeg = 0;
		CashPos = 0;
		while (UsedSize < Size)
		{
			int sz = INTV( Cash + tpos + 4 );
			UsedSize += sz;
			tpos += sz;
			NSeg++;
		};
	};
	//free refs
	tpos = CashPos;
	for (int i = 0; i < NSeg; i++)
	{
		int sz = INTV( Cash + tpos + 4 );
		int* ptr = (int*) INTV( Cash + tpos );
		if (ptr)
		{
			ptr[0] = 0xFFFFFFFF;
			INTV( Cash + tpos ) = NULL;
		};
		tpos += sz;
	};
	if (UsedSize - Size > 32)
	{
		INTV( Cash + CashPos + Size + 8 ) = 0;
		INTV( Cash + CashPos + Size + 12 ) = UsedSize - Size;
		UsedSize = Size;
	};
	INTV( Cash + CashPos ) = 0;
	INTV( Cash + CashPos + 4 ) = UsedSize + 8;
	byte* cps = Cash + CashPos;
	CashPos += UsedSize + 8;
	return cps;
};

int GP_System::PreLoadGPImage( char* Name, bool Shadow )
{
	//search for existing name
	return PreLoadGPImage( Name );
	/*
	NewGPImage=false;
	for(int i=0;i<NGP;i++){
		if(!strcmp(Name,GPNames[i])){
			return i;
		};
	};
	UNITBL[i]=UFONTS.FindFont(Name);
	NewGPImage=true;
	//determining image type
	//1. GP
	char FName[128];
	sprintf(FName,"%s.gp",Name);
	ResFile f=RReset(FName);
	ImageType[NGP]=0;
	if(f!=INVALID_HANDLE_VALUE){
		RSeek(f,4);
		RBlockRead(f,&GPNFrames[NGP],2);
		RClose(f);
		AText(FName);
		ImageType[NGP]=1;
	}else{
		//2. RLC?
		sprintf(FName,"%s.rlc",Name);
		f=RReset(FName);
		if(f!=INVALID_HANDLE_VALUE){
			AText(FName);
			RSeek(f,4);
			RBlockRead(f,&GPNFrames[NGP],2);
			RClose(f);
			ImageType[NGP]=2;
			if(Shadow){
				sprintf(FName,"%ss.rlc",Name);
				f=RReset(FName);
				if(f!=INVALID_HANDLE_VALUE){
					RClose(f);
					ImageType[NGP]=3;
				};
			};
		}else{
			sprintf(FName,"Could not load %s.(gp|rlc)",Name);
			ErrM(FName);
		};
	};
	RLCNSpr[NGP]=GPNFrames[NGP];
	GPNames[NGP]=new char[strlen(Name)+1];
	strcpy(GPNames[NGP],Name);
	GPH[NGP]=NULL;
	GPLastTime[NGP]=0;
	GPSize[NGP]=0;
	NGP++;
	return NGP-1;
	*/
};

void GP_System::SetWhiteFont( int n )
{
	if (n >= NGP)return;
	ImageType[n] = ( ImageType[n] & 7 ) | 16;
};
void GP_System::SetRedFont( int n )
{
	if (n >= NGP)return;
	ImageType[n] = ( ImageType[n] & 7 ) | 31;
};
void GP_System::SetBlackFont( int n )
{
	if (n >= NGP)return;
	ImageType[n] = ( ImageType[n] & 7 );
};
void GP_System::SetOptionalColor( int n, int c )
{
	if (n >= NGP)return;
	ImageType[n] = ( ImageType[n] & 7 ) | ( c << 4 );
};
void GP_System::UnLoadGP( int i )
{
	if (i >= NGP)return;
	UNITBL[i] = NULL;
	switch (ImageType[i] & 7)
	{
	case 1:
		if (GPSize[i])
		{
			LOADED -= GPSize[i];
			GPSize[i] = 0;
			FreeRefs( i );
			if (!Mapping[i])free( GPH[i] );
			GPH[i] = NULL;
			Mapping[i] = 0;
			free( ImLx[i] );
			free( ImLy[i] );
			ImLx[i] = NULL;
			ImLy[i] = NULL;
			free( CASHREF[i] );
			CASHREF[i] = NULL;
			if (ItDX[i])
			{
				free( ItDX[i] );
				free( ItLX[i] );
				ItDX[i] = NULL;
				ItLX[i] = NULL;
			};
		};
		GPNames[i][0] = '*';
		break;
	case 0:
	case 2:
		assert( false );
	};
	for (int j = NGP - 1; j >= 0 && GPNames[j][0] == '*'; j--)
	{
		free( GPNames[j] );
		NGP--;
	};
};
int GP_Header::GetLx()
{
	GP_Header* GPH = this;
	GP_Header* GPS = this;
	int DIFF = -1;
	int LxMax = 0;
	do
	{
		DIFF = GPH->NextPict;
		int Lxx = GPH->dx + GPH->Lx;
		if (Lxx > LxMax)LxMax = Lxx;
		GPH = (GP_Header*)((uintptr_t)GPS + DIFF);
	} while (DIFF != -1);
	return LxMax;
};
int GP_Header::GetLy()
{
	GP_Header* GPH = this;
	GP_Header* GPS = this;
	int DIFF = -1;
	int LyMax = 0;
	do
	{
		DIFF = GPH->NextPict;
		int Lyy = GPH->dy + GPH->Ly;
		if (Lyy > LyMax)LyMax = Lyy;
		GPH = (GP_Header*)((uintptr_t)GPS + DIFF);
	} while (DIFF != -1);
	return LyMax;
};
int GP_Header::GetDx()
{
	GP_Header* GPH = this;
	GP_Header* GPS = this;
	int DIFF = -1;
	int LxMax = 10000;
	do
	{
		DIFF = GPH->NextPict;
		int Lxx = GPH->dx;
		if (Lxx < LxMax)LxMax = Lxx;
		GPH = (GP_Header*)((int)GPS + DIFF);
	} while (DIFF != -1);
	return LxMax;
};
int GP_Header::GetDy()
{
	GP_Header* GPH = this;
	GP_Header* GPS = this;
	int DIFF = -1;
	int LxMax = 10000;
	do
	{
		DIFF = GPH->NextPict;
		int Lxx = GPH->dy;
		if (Lxx < LxMax)LxMax = Lxx;
		GPH = (GP_Header*)((int)GPS + DIFF);
	} while (DIFF != -1);
	return LxMax;
}

extern bool ProtectionMode;

int GP_System::PreLoadGPImage( char* Name )
{
	NewGPImage = false;

	int fidx = -1;

	//Search for existing name
	for (int i = 0; i < NGP; i++)
	{
		if (GPNames[i][0] == '*')
		{
			fidx = i;
		}

		if (!strcmp( Name, GPNames[i] ))
		{
			//File already loaded
			return i;
		}
	}

	if (fidx == -1)
	{
		fidx = NGP;
	}

	UNITBL[fidx] = UFONTS.FindFont( Name );

	NewGPImage = true;

	ImageType[fidx] = 0;

	//Determine image type
	char FName[128];
	//GP?
	sprintf( FName, "%s.gp", Name );
	ResFile f = RReset( FName );
	if (f != INVALID_HANDLE_VALUE)
	{
		RSeek( f, 4 );
		RBlockRead( f, &GPNFrames[fidx], 2 );
		RClose( f );
		ImageType[fidx] = 1;//GP
	}
	else
	{
		//RLC?
		sprintf( FName, "%s.rlc", Name );
		f = RReset( FName );
		if (f != INVALID_HANDLE_VALUE)
		{
			RSeek( f, 4 );
			RBlockRead( f, &GPNFrames[fidx], 2 );
			RClose( f );
			ImageType[fidx] = 2;//RLC

			//RLC + Shadow?
			sprintf( FName, "%ss.rlc", Name );
			f = RReset( FName );
			if (f != INVALID_HANDLE_VALUE)
			{
				RClose( f );
				ImageType[fidx] = 3;//RLC + Shadow
			}
		}
		else
		{
			if (strstr( Name, "L_Mode\\" ))
			{
				return 0;
			}
			sprintf( FName, "Could not load %s.(gp|rlc)", Name );
			ErrM( FName );
		}
	}

	RLCNSpr[fidx] = GPNFrames[fidx];

	GPNames[fidx] = new char[strlen( Name ) + 1];
	strcpy( GPNames[fidx], Name );

	GPH[fidx] = nullptr;
	GPLastTime[fidx] = 0;
	GPSize[fidx] = 0;

	if (fidx == NGP)
	{
		NGP++;
	}

	char cc3[256];
	if (!strstr( Name, "\\" ))
	{
		sprintf( cc3, "L_Mode\\%s", Name );
		GP_L_IDXS[fidx] = PreLoadGPImage( cc3 );
	}

	return fidx;
}

#define GPX(x, y) ((GP_Header*)((uintptr_t)(x) + (x)->y))

bool GP_System::LoadGP( int i )
{
	char Name[128];
	if (i > NGP)
	{
		return false;
	}
	ResFile f;
	byte* mpptr = 0;
	switch (ImageType[i] & 7)
	{
	case 0:
		return false;
	case 1:
		if (GPH[i])return true;
		sprintf( Name, "%s.gp", GPNames[i] );
		//ProtectionMode=1;
		f = RReset( Name );
		//ProtectionMode=0;
		//assert(f!=INVALID_HANDLE_VALUE);
		if (f != INVALID_HANDLE_VALUE)
		{
			GPSize[i] = RFileSize( f );
			mpptr = GSFILES.gMapFile( (LPGSCfile) f );
			lpGP_GlobalHeader lpGPH;
			if (mpptr)
			{
				GPH[i] = (lpGP_GlobalHeader) mpptr;
				lpGPH = GPH[i];
				Mapping[i] = 1;
			}
			else
			{
				LOADED += GPSize[i];
				GPH[i] = lpGP_GlobalHeader( new byte[GPSize[i]] );
				lpGPH = GPH[i];
				RBlockRead( f, GPH[i], GPSize[i] );
				Mapping[i] = 0;
			};
			//int* lpint=(int*)(&lpGPH->LGPH);
			ImLx[i] = new short[lpGPH->NPictures];
			ImLy[i] = new short[lpGPH->NPictures];
			int np = lpGPH->NPictures;
			for (int n = 0; n < np; n++)
			{
				//lpint[n]+=int(lpGPH);         //----------!new!----------//
				ImLx[i][n] = GPX( lpGPH, LGPH[n] )->GetLx();
				ImLy[i][n] = GPX( lpGPH, LGPH[n] )->GetLy();
			};
			RClose( f );
			sprintf( Name, "%s.gpi", GPNames[i] );
			f = RReset( Name );
			if (f != INVALID_HANDLE_VALUE)
			{
				ItDX[i] = (char*) malloc( lpGPH->NPictures );
				ItLX[i] = (char*) malloc( lpGPH->NPictures );
				RBlockRead( f, ItDX[i], lpGPH->NPictures );
				RBlockRead( f, ItLX[i], lpGPH->NPictures );
				RClose( f );
			};
			//Creating cash references
			int csz = np;
			for (int n = 0; n < np; n++)
			{
				GP_Header* LGP = GPX( lpGPH, LGPH[n] );
				GP_Header* LGP0 = LGP;
				int DIFF = -1;
				int LxMax = 0;
				do
				{
					DIFF = LGP->NextPict;
					csz++;
					LGP = (GP_Header*)((uintptr_t)LGP0 + DIFF);
				} while (DIFF != -1);
			};
			CASHREF[i] = new DWORD[csz + 1];
			DWORD* CREF = CASHREF[i];
			memset( CREF, 0xFF, ( csz + 1 ) << 2 );
			csz = np;
			for (int n = 0; n < np; n++)
			{
				GP_Header* LGP = GPX( lpGPH, LGPH[n] );
				GP_Header* LGP0 = LGP;
				int DIFF = -1;
				int LxMax = 0;
				CREF[n] = csz;
				do
				{
					DIFF = LGP->NextPict;
					csz++;
					LGP = (GP_Header*)((uintptr_t)LGP0 + DIFF);
				} while (DIFF != -1);
			};
			return true;
		}
		else return false;
	case 2:
	{
		if (RLCImage[i])
		{
			return true;
		}
		sprintf( Name, "%s.rlc", GPNames[i] );
		LoadRLC( Name, RLCImage + i );
		ImLx[i] = new short[RLCImage[i]->SCount];
		ImLy[i] = new short[RLCImage[i]->SCount];
		for (int n = 0; n < RLCImage[i]->SCount; n++)
		{
			ImLx[i][n] = GetRLCWidth( RLCImage[i], n );
			ImLy[i][n] = GetRLCWidth( RLCImage[i], n );
		};
	};
	return true;
	case 3:
	{
		if (RLCImage[i])return true;
		sprintf( Name, "%s.rlc", GPNames[i] );
		LoadRLC( Name, RLCImage + i );
		ImLx[i] = new short[RLCImage[i]->SCount];
		ImLy[i] = new short[RLCImage[i]->SCount];
		for (int n = 0; n < RLCImage[i]->SCount; n++)
		{
			ImLx[i][n] = GetRLCWidth( RLCImage[i], n );
			ImLy[i][n] = GetRLCWidth( RLCImage[i], n );
		};
		sprintf( Name, "%ss.rlc", GPNames[i] );
		LoadRLC( Name, RLCShadow + i );
	};
	return true;
	default:
		return false;
	};
}

// C++ port of the GP sprite decompressors and blit routines.

static void StdUnpack( byte* Dest, byte* Src, int Len, byte* Voc )
{
    byte* dst = Dest;
    byte* src = Src;
    int rem = Len;
    while (rem > 0)
    {
        byte bits = *src++;
        for (int b = 0; b < 8 && rem > 0; b++, bits <<= 1)
        {
            if (bits & 0x80)
            {
                unsigned cx = (unsigned char)src[0] | ((unsigned char)src[1] << 8);
                src += 2;
                int count = (cx >> 12) + 3;
                byte* from = Voc + (cx & 0xFFF);
                rem -= count;
                while (count-- > 0) *dst++ = *from++;
            }
            else
            {
                *dst++ = *src++;
                rem--;
            }
        }
    }
}

static void LZUnpack( byte* Dest, byte* Src, int Len )
{
    byte* dst = Dest;
    byte* src = Src;
    int rem = Len;
    while (rem > 0)
    {
        byte bits = *src++;
        for (int b = 0; b < 8 && rem > 0; b++, bits >>= 1)
        {
            if (bits & 1)
            {
                unsigned cx = (unsigned char)src[0] | ((unsigned char)src[1] << 8);
                src += 2;
                int count = (cx >> 13) + 3;
                int negofs = (cx & 0x1FFF);
                byte* from = dst - negofs - 1;
                rem -= count;
                while (count-- > 0) *dst++ = *from++;
            }
            else
            {
                *dst++ = *src++;
                rem--;
            }
        }
    }
}

// C++ port of the Windows asm GreyUnpack.
// Each source byte encodes two 4-bit grey values:
//   lo nibble -> dest byte: (al & 0x0F) << 1
//   hi nibble -> dest byte: (al & 0xF0) >> 3
// Input: Len/2 bytes; output: Len bytes (Len must be even).
static void GreyUnpack( byte* Dest, byte* Src, int Len )
{
    Len >>= 1;
    while (Len-- > 0)
    {
        byte al = *Src++;
        *Dest++ = (al & 0x0F) << 1;
        *Dest++ = (al & 0xF0) >> 3;
    }
}

extern byte fog[8192 + 1024];
extern byte yfog[8192];
extern byte Bright[8192];
extern byte Optional1[8192];
extern byte Optional2[8192];
extern byte Optional3[8192];

// Palette-mapped blit: like GP_ShowMaskedPict but applies Encoder[src<<8 | dst] per pixel.
static void GP_ShowMaskedMultiPalPict( int x, int y, GP_Header* Pic, byte* CData, byte* Encoder )
{
    x += Pic->dx;
    y += Pic->dy;
    int Lx     = Pic->Lx;
    int NLines = Pic->NLines;

    if (y + NLines <= WindY || x + Lx <= WindX || x > WindX1 || y > WindY1)
        return;

    byte* mask = (byte*)Pic + 23;
    byte* pix  = CData;

    int top_skip = (y < WindY) ? WindY - y : 0;
    for (int i = 0; i < top_skip; i++)
    {
        byte m = *mask++;
        if (!m) continue;
        if (!(m & 0x80))
        {
            int n = m & 0x7F;
            for (int s = 0; s < n; s++) { mask++; pix += *mask++; }
        }
        else
        {
            int n = m & 0x1F;
            byte pm = (m & 0x20) ? 16 : 0;
            for (int s = 0; s < n; s++) { byte pk = *mask++; pix += ((pk >> 4) | pm); }
        }
    }

    if (y < WindY) y = WindY;
    NLines = (y + NLines - top_skip > WindY1 + 1) ? WindY1 - y + 1 : NLines - top_skip;

    int clip_left  = (x < WindX)               ? WindX - x       : 0;
    int clip_right = (x + Lx - 1 > WindX1) ? WindX1 - x + 1 : Lx;

    byte* screen_row = (byte*)ScreenPtr + y * ScrWidth + x;

    for (int row = 0; row < NLines; row++, screen_row += ScrWidth)
    {
        byte m = *mask++;
        if (!m) continue;

        byte* dst = screen_row;

        if (!(m & 0x80))
        {
            int n_segs = m & 0x7F;
            int cur_cl = clip_left;
            for (int s = 0; s < n_segs; s++)
            {
                int skip  = *mask++;
                int count = *mask++;
                dst   += skip;
                cur_cl -= skip;

                int draw_start = (cur_cl > 0) ? cur_cl : 0;
                int col        = (int)(dst - screen_row);
                int draw_end   = (col + count > clip_right) ? clip_right - col : count;
                int draw_n     = draw_end - draw_start;

                if (draw_n > 0)
                {
                    byte* d = dst + draw_start;
                    byte* s2 = pix + draw_start;
                    for (int p = 0; p < draw_n; p++)
                        d[p] = Encoder[((unsigned)s2[p] << 8) | d[p]];
                }

                pix    += count;
                dst    += count;
                cur_cl -= count;
            }
        }
        else
        {
            byte space_mask = (m & 0x40) ? 16 : 0;
            byte pix_mask   = (m & 0x20) ? 16 : 0;
            int  n_segs     = m & 0x1F;
            int  cur_cl     = clip_left;

            for (int s = 0; s < n_segs; s++)
            {
                byte packed = *mask++;
                int skip    = (packed & 0x0F) | space_mask;
                int count   = (packed >> 4)   | pix_mask;

                dst   += skip;
                cur_cl -= skip;

                int draw_start = (cur_cl > 0) ? cur_cl : 0;
                int col        = (int)(dst - screen_row);
                int draw_end   = (col + count > clip_right) ? clip_right - col : count;
                int draw_n     = draw_end - draw_start;

                if (draw_n > 0)
                {
                    byte* d = dst + draw_start;
                    byte* s2 = pix + draw_start;
                    for (int p = 0; p < draw_n; p++)
                        d[p] = Encoder[((unsigned)s2[p] << 8) | d[p]];
                }

                pix    += count;
                dst    += count;
                cur_cl -= count;
            }
        }
    }
}

// Blit one scanline segment. `dst` advances by skip+count pixels.
// Returns new dst position.
static inline byte* blit_seg( byte* dst, byte* src, int skip, int count,
                               int clip_left, int clip_right )
{
    dst += skip;
    if (count <= 0) return dst;

    int draw_start = 0;
    int draw_end   = count;

    if (clip_left > 0)
    {
        draw_start = clip_left;
        clip_left  = 0;
    }
    if (clip_right >= 0 && draw_end > clip_right)
    {
        draw_end = clip_right;
    }

    int skip_src = draw_start;
    int draw_n   = draw_end - draw_start;

    dst      += draw_start;
    src      += skip_src;

    if (draw_n > 0)
        memcpy(dst, src, draw_n);

    dst += count - draw_start;
    return dst;
}

// C++ equivalent of the Windows asm GP_ShowMaskedPict.
// Handles simple lines (bit7=0) and complex lines (bit7=1).
// Full left/right/top/bottom clipping.
static void GP_ShowMaskedPict( int x, int y, GP_Header* Pic, byte* CData, byte* /*Encoder*/ )
{
    x += Pic->dx;
    y += Pic->dy;
    int Lx     = Pic->Lx;
    int NLines = Pic->NLines;


    if (y + NLines <= WindY || x + Lx <= WindX || x > WindX1 || y > WindY1)
        return;

    // mask table starts at Pic+23 bytes
    byte* mask = (byte*)Pic + 23;
    byte* pix  = CData;         // pixel data pointer

    // skip top-clipped lines
    int top_skip = (y < WindY) ? WindY - y : 0;
    for (int i = 0; i < top_skip; i++)
    {
        byte m = *mask++;
        if (!m) continue;
        int n_segs;
        bool complex_line = (m & 0x80) != 0;
        if (!complex_line)
        {
            n_segs = m & 0x7F;
            for (int s = 0; s < n_segs; s++)
            {
                /* skip */ mask++;
                int cnt = *mask++;
                pix += cnt;
            }
        }
        else
        {
            n_segs = m & 0x1F;
            byte pix_mask = (m & 0x20) ? 16 : 0;
            for (int s = 0; s < n_segs; s++)
            {
                byte packed = *mask++;
                int cnt = (packed >> 4) | pix_mask;
                pix += cnt;
            }
        }
    }

    if (y < WindY) y = WindY;
    if (y + NLines - top_skip > WindY1 + 1)
        NLines = WindY1 - y + 1;
    else
        NLines -= top_skip;

    int clip_left  = (x < WindX)   ? WindX - x       : 0;
    int clip_right = (x + Lx - 1 > WindX1) ? WindX1 - x + 1 : Lx;

    byte* screen_row = (byte*)ScreenPtr + y * ScrWidth + x;

    for (int row = 0; row < NLines; row++, screen_row += ScrWidth)
    {
        byte m = *mask++;
        if (!m) continue;

        byte* dst = screen_row;
        bool complex_line = (m & 0x80) != 0;

        if (!complex_line)
        {
            int n_segs = m & 0x7F;
            int cur_cl = clip_left;
            for (int s = 0; s < n_segs; s++)
            {
                int skip  = *mask++;
                int count = *mask++;
                dst += skip;
                cur_cl -= skip;

                int draw_start = (cur_cl > 0) ? cur_cl : 0;
                int draw_end   = count;
                int col        = (int)(dst - screen_row);
                if (col + count > clip_right)
                    draw_end = clip_right - col;

                int skip_src = draw_start;
                int draw_n   = draw_end - draw_start;
                if (draw_n > 0)
                    memcpy(dst + draw_start, pix + skip_src, draw_n);

                pix += count;
                dst += count;
                cur_cl -= count;
            }
        }
        else
        {
            byte space_mask = (m & 0x40) ? 16 : 0;
            byte pix_mask   = (m & 0x20) ? 16 : 0;
            int  n_segs     = m & 0x1F;
            int  cur_cl     = clip_left;

            for (int s = 0; s < n_segs; s++)
            {
                byte packed = *mask++;
                int skip    = (packed & 0x0F) | space_mask;
                int count   = (packed >> 4)   | pix_mask;
                dst += skip;
                cur_cl -= skip;

                int draw_start = (cur_cl > 0) ? cur_cl : 0;
                int draw_end   = count;
                int col        = (int)(dst - screen_row);
                if (col + count > clip_right)
                    draw_end = clip_right - col;

                int skip_src = draw_start;
                int draw_n   = draw_end - draw_start;
                if (draw_n > 0)
                    memcpy(dst + draw_start, pix + skip_src, draw_n);

                pix += count;
                dst += count;
                cur_cl -= count;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Generic masked blit covering all GP drawing variants (the asm versions differ
// only in the per-pixel "variation zone" and whether they draw mirrored):
//   op 0: dst = src                  (GP_ShowMaskedPict)
//   op 1: dst = Enc[(src<<8)|dst]    (GP_ShowMaskedMultiPalPict)
//   op 2: dst = Enc[src]             (GP_ShowMaskedPalPict - NatPal color swap)
//   op 3: dst = Enc[dst]             (GP_ShowMaskedPictShadow - CData unused)
//   op 4: dst = Enc[(dst<<8)|src]    (GP_ShowMaskedMultiPalTPict)
// mirror: the *Inv variants - horizontally flipped sprite; x is the RIGHT edge,
// hot-point dx is subtracted, segments run right-to-left (see the Inv asm).
extern byte refl[3072];   // water reflection LUT, loaded from refl.grd (fog.cpp)

// Wobble shift table for the water-mirror op below. The Windows asm got it
// as a parameter; the generic draw helper does not take one, so the mirror
// wrappers stash it here right before their call.
static int* s_MirrorShift = nullptr;

static void GP_ShowMaskedGeneric( int x, int y, GP_Header* Pic, byte* CData,
    byte* Encoder, int op, bool mirror )
{
    if (mirror) x -= Pic->dx;
    else        x += Pic->dx;
    y += Pic->dy;
    int Lx     = Pic->Lx;
    int NLines = Pic->NLines;
    int OCNTR  = y;   // original y (before clipping) - used by the Overpoint dither

    if (mirror)
    {
        if (y + NLines <= WindY || x - Lx >= WindX1 || x < WindX || y > WindY1)
            return;
    }
    else
    {
        if (y + NLines <= WindY || x + Lx <= WindX || x > WindX1 || y > WindY1)
            return;
    }

    byte* mask = (byte*)Pic + 23;
    byte* pix  = CData;
    bool usePix = (op != 3);

    // skip top-clipped lines
    int top_skip = (y < WindY) ? WindY - y : 0;
    for (int i = 0; i < top_skip; i++)
    {
        byte m = *mask++;
        if (!m) continue;
        if (!(m & 0x80))
        {
            int n = m & 0x7F;
            for (int s = 0; s < n; s++)
            {
                mask++;
                if (usePix) pix += *mask;
                mask++;
            }
        }
        else
        {
            int n = m & 0x1F;
            byte pm = (m & 0x20) ? 16 : 0;
            for (int s = 0; s < n; s++)
            {
                byte pk = *mask++;
                if (usePix) pix += ((pk >> 4) | pm);
            }
        }
    }

    if (y < WindY) y = WindY;
    NLines = (y + NLines - top_skip > WindY1 + 1) ? WindY1 - y + 1 : NLines - top_skip;

    int left_edge  = mirror ? (x - Lx + 1) : x;

    // op 6 (water mirror): per-scanline horizontal wobble. The shift is a
    // running sum over the drawn rows, same as the Windows asm, which kept
    // the offset in LineStart across scanlines.
    int wobble = 0;
    for (int row = 0; row < NLines; row++)
    {
        if (op == 6 && s_MirrorShift) wobble += s_MirrorShift[row];
        int row_edge = left_edge + wobble;
        byte* screen_row = (byte*)ScreenPtr + (y + row) * ScrWidth + row_edge;
        byte m = *mask++;
        if (!m) continue;

        int col = 0;
        if (!(m & 0x80))
        {
            int n_segs = m & 0x7F;
            for (int s = 0; s < n_segs; s++)
            {
                int skip  = *mask++;
                int count = *mask++;
                col += skip;
                for (int p = 0; p < count; p++, col++)
                {
                    int out = mirror ? (Lx - 1 - col) : col;
                    int sx = row_edge + out;
                    // Consume the source pixel even when this column is
                    // clipped: the Windows asm advances the source pointer
                    // over off-screen columns too. Skipping the consumption
                    // here desyncs pix from the mask stream and smears the
                    // rest of the row.
                    byte src = (op == 3 || op == 5) ? 0 : *pix++;
                    if (sx < WindX || sx > WindX1) continue;
                    byte* d = screen_row + out;
                    if (op == 5)
                    {
                        // Overpoint dither (GP_ShowMaskedPictOverpoint): paint 0
                        // on (y+x) odd cells; reads neither CData nor Encoder
                        if (((OCNTR + sx) & 1)) *d = 0;
                        continue;
                    }
                    if (op == 6)
                    {
                        // Water mirror (GP_ShowMaskedMirrorPict/Inv): tint only
                        // pixels lying on water-colored background (0xB0..0xBB,
                        // the 12 shades the refl table covers); refl maps
                        // (water shade, sprite pixel) to the reflected color.
                        // Everything else is left untouched, so the reflection
                        // shows up only on the water surface.
                        int wi = *d - 0xB0;
                        if (wi >= 0 && wi < 12) *d = refl[(wi << 8) | src];
                        continue;
                    }
                    byte outc;
                    if (op == 3)          outc = Encoder[*d];
                    else
                    {
                        switch (op)
                        {
                        case 0:  outc = src; break;
                        case 1:  outc = Encoder[((unsigned)src << 8) | *d]; break;
                        case 2:  outc = Encoder[src]; break;
                        default: outc = Encoder[((unsigned)*d << 8) | src]; break; // op 4
                        }
                    }
                    *d = outc;
                }
            }
        }
        else
        {
            byte space_mask = (m & 0x40) ? 16 : 0;
            byte pix_mask   = (m & 0x20) ? 16 : 0;
            int  n_segs     = m & 0x1F;
            for (int s = 0; s < n_segs; s++)
            {
                byte packed = *mask++;
                int skip    = (packed & 0x0F) | space_mask;
                int count   = (packed >> 4)   | pix_mask;
                col += skip;
                for (int p = 0; p < count; p++, col++)
                {
                    int out = mirror ? (Lx - 1 - col) : col;
                    int sx = row_edge + out;
                    // Consume the source pixel even when this column is
                    // clipped: the Windows asm advances the source pointer
                    // over off-screen columns too. Skipping the consumption
                    // here desyncs pix from the mask stream and smears the
                    // rest of the row.
                    byte src = (op == 3 || op == 5) ? 0 : *pix++;
                    if (sx < WindX || sx > WindX1) continue;
                    byte* d = screen_row + out;
                    if (op == 5)
                    {
                        // Overpoint dither (GP_ShowMaskedPictOverpoint): paint 0
                        // on (y+x) odd cells; reads neither CData nor Encoder
                        if (((OCNTR + sx) & 1)) *d = 0;
                        continue;
                    }
                    if (op == 6)
                    {
                        // Water mirror (GP_ShowMaskedMirrorPict/Inv): tint only
                        // pixels lying on water-colored background (0xB0..0xBB,
                        // the 12 shades the refl table covers); refl maps
                        // (water shade, sprite pixel) to the reflected color.
                        // Everything else is left untouched, so the reflection
                        // shows up only on the water surface.
                        int wi = *d - 0xB0;
                        if (wi >= 0 && wi < 12) *d = refl[(wi << 8) | src];
                        continue;
                    }
                    byte outc;
                    if (op == 3)          outc = Encoder[*d];
                    else
                    {
                        switch (op)
                        {
                        case 0:  outc = src; break;
                        case 1:  outc = Encoder[((unsigned)src << 8) | *d]; break;
                        case 2:  outc = Encoder[src]; break;
                        default: outc = Encoder[((unsigned)*d << 8) | src]; break; // op 4
                        }
                    }
                    *d = outc;
                }
            }
        }
    }
}

// C port of the Windows asm NatUnpack: one source byte -> four 2-bit indices
static void NatUnpackT( byte* Dest, byte* Src, int Len )
{
    int n = Len / 4;
    while (n-- > 0)
    {
        byte b = *Src++;
        *Dest++ = b & 3;
        *Dest++ = (b >> 2) & 3;
        *Dest++ = (b >> 4) & 3;
        *Dest++ = (b >> 6) & 3;
    }
}

// Variant wrappers used by GP_System::Show* on Linux
static void GP_ShowMaskedPalPict( int x, int y, GP_Header* Pic, byte* CData, byte* Encoder )
{
    GP_ShowMaskedGeneric( x, y, Pic, CData, Encoder, 2, false );
}
static void GP_ShowMaskedPalPictInv( int x, int y, GP_Header* Pic, byte* CData, byte* Encoder )
{
    GP_ShowMaskedGeneric( x, y, Pic, CData, Encoder, 2, true );
}
static void GP_ShowMaskedMultiPalPictInv( int x, int y, GP_Header* Pic, byte* CData, byte* Encoder )
{
    GP_ShowMaskedGeneric( x, y, Pic, CData, Encoder, 1, true );
}
static void GP_ShowMaskedMultiPalTPict( int x, int y, GP_Header* Pic, byte* CData, byte* Encoder )
{
    GP_ShowMaskedGeneric( x, y, Pic, CData, Encoder, 4, false );
}
static void GP_ShowMaskedMultiPalTPictInv( int x, int y, GP_Header* Pic, byte* CData, byte* Encoder )
{
    GP_ShowMaskedGeneric( x, y, Pic, CData, Encoder, 4, true );
}
static void GP_ShowMaskedPictShadow( int x, int y, GP_Header* Pic, byte* CData, byte* Encoder )
{
    GP_ShowMaskedGeneric( x, y, Pic, CData, Encoder, 3, false );
}
static void GP_ShowMaskedPictShadowInv( int x, int y, GP_Header* Pic, byte* CData, byte* Encoder )
{
    GP_ShowMaskedGeneric( x, y, Pic, CData, Encoder, 3, true );
}
static void GP_ShowMaskedPictOverpoint( int x, int y, GP_Header* Pic, byte* CData, byte* Encoder )
{
    (void)CData; (void)Encoder;
    GP_ShowMaskedGeneric( x, y, Pic, nullptr, nullptr, 5, false );
}
static void GP_ShowMaskedPictOverpointInv( int x, int y, GP_Header* Pic, byte* CData, byte* Encoder )
{
    (void)CData; (void)Encoder;
    GP_ShowMaskedGeneric( x, y, Pic, nullptr, nullptr, 5, true );
}
static void GP_ShowMaskedPictInv( int x, int y, GP_Header* Pic, byte* CData, byte* Encoder )
{
    (void)Encoder;
    GP_ShowMaskedGeneric( x, y, Pic, CData, nullptr, 0, true );
}
// Water-reflection variants (Windows: GP_ShowMaskedMirrorPict/Inv asm).
// Each scanline is displaced horizontally by a running sum of the XShift
// wobble table (InitXShift recomputes it from the tmtmt tick in main.cpp,
// so reflections sway over time), and pixels tint only over water colors
// via the refl table - see the op-6 branch in GP_ShowMaskedGeneric above.
static void GP_ShowMaskedMirrorPict( int x, int y, GP_Header* Pic, byte* CData, int* WSHIFT )
{
    s_MirrorShift = WSHIFT;
    GP_ShowMaskedGeneric( x, y, Pic, CData, nullptr, 6, false );
}
static void GP_ShowMaskedMirrorPictInv( int x, int y, GP_Header* Pic, byte* CData, int* WSHIFT )
{
    s_MirrorShift = WSHIFT;
    GP_ShowMaskedGeneric( x, y, Pic, CData, nullptr, 6, true );
}



//Moved out of the _WIN32 block: pure data, needed by the Linux draw variants
extern byte Bright[8192];

//Used for color masking of interface elements. Color offset = Nation byte * 4
byte NatPal[64] =
{
	0xD0,//red
	0xD1,
	0xD2,
	0xD3,
	0xD4,//blue
	0xD5,
	0xD6,
	0xD7,
	0xD8,//cyan
	0xD9,
	0xDA,
	0xDB,
	0xDC,//purple
	0xDD,
	0xDE,
	0xDF,
	0xE0,//orange
	0xE1,
	0xE2,
	0xE3,
	0xE4,//black
	0xE5,
	0xE6,
	0xE7,
	0xE8,//white
	0xE9,
	0xEA,
	0xEB,
	0xEC,//mercs
	0xED,
	0xEE,
	0xEF
};

extern byte Optional1[8192];
extern byte Optional2[8192];
extern byte Optional3[8192];

int startTrans = 0;

void GP_System::ShowGP(//IMPORTANT: show sprite with color masking
	int x, int y, int FileIndex, int SprIndex,
	byte Nation
)
{
    if (!( FileIndex < NGP && ( SprIndex & 4095 ) < GPNFrames[FileIndex] ))
        return;

    if (( ImageType[FileIndex] & 7 ) > 1)
    {
        // RLC file — not needed for fonts
        if (!RLCImage[FileIndex])
            LoadGP( FileIndex );
        if (RLCImage[FileIndex])
            ShowRLCItem( x, y, RLCImage + FileIndex, SprIndex, Nation );
        return;
    }

    if (!GPH[FileIndex])
        LoadGP( FileIndex );

    GP_GlobalHeader* lpGH = GPH[FileIndex];
    if (!lpGH)
        return;

    int RSprIndex = SprIndex;
    if (SprIndex >= lpGH->NPictures)
        RSprIndex &= 4095;

    GP_Header* lpGP    = GPX( lpGH, LGPH[RSprIndex] );
    GP_Header* lpGPCUR = lpGP;


    DWORD* PAK = CASHREF[FileIndex];
    PAK += PAK[RSprIndex];

    if (ItDX[FileIndex])
        x -= ItDX[FileIndex][RSprIndex];

    bool inv = SprIndex >= 4096;
    int imt = ImageType[FileIndex] >> 4;
    int DIFF = -1;
    do
    {
        int UnpackLen = lpGPCUR->CData >> 14;
        int CDOffs    = lpGPCUR->CData & 16383;
        byte Optx     = lpGPCUR->Options;
        if (( Optx & 63 ) == 43) UnpackLen += 262144;
        if (( Optx & 63 ) == 44) UnpackLen += 262144 * 2;
        if (Optx & 64) CDOffs += 16384;
        if (Optx & 128) CDOffs += 32768;

        byte Opt = Optx & 63;
        switch (Opt)
        {
        case 0:
        {
            // CASHREF is 32-bit pointer storage, unusable on 64-bit Linux.
            // Decompress into a temporary buffer on each draw call.
            // Slack covers StdUnpack's final dictionary copy, which (exactly
            // like the Windows asm) may legally run up to 17 bytes past
            // UnpackLen. On Windows the overrun lands inside the big GetCash
            // ring buffer; here each sprite is its own heap chunk, so without
            // the extra room it corrupts the next chunk's header.
            byte* tmp = new byte[UnpackLen + 32];
            StdUnpack( tmp, (byte*)lpGPCUR + CDOffs, UnpackLen,
                       (byte*)lpGH + lpGH->VocOffset );
            GP_ShowMaskedPict( x, y, lpGPCUR, tmp, nullptr );
            delete[] tmp;
            break;
        }
        case 41:
            GP_ShowMaskedPict( x, y, lpGPCUR, (byte*)lpGPCUR + CDOffs, nullptr );
            break;
        case 42:
        case 43:
        case 44:
        {
            byte* tmp = new byte[UnpackLen + 10];
            LZUnpack( tmp, (byte*)lpGPCUR + CDOffs, UnpackLen );
            GP_ShowMaskedPict( x, y, lpGPCUR, tmp, nullptr );
            delete[] tmp;
            break;
        }
        case 38:
        {
            int imt = ImageType[FileIndex] >> 4;
            byte* tmp = new byte[UnpackLen];
            GreyUnpack( tmp, (byte*)lpGPCUR + CDOffs, UnpackLen );
            switch (imt)
            {
            case 1:
                GP_ShowMaskedMultiPalPict( x, y, lpGPCUR, tmp, Bright + startTrans );
                break;
            case 2:
                GP_ShowMaskedMultiPalPict( x, y, lpGPCUR, tmp, yfog + startTrans );
                break;
            case 3:
                GP_ShowMaskedMultiPalPict( x, y, lpGPCUR, tmp, Optional1 + startTrans );
                break;
            case 4:
                GP_ShowMaskedMultiPalPict( x, y, lpGPCUR, tmp, Optional2 + startTrans );
                break;
            case 5:
                GP_ShowMaskedMultiPalPict( x, y, lpGPCUR, tmp, Optional3 );
                break;
            default:
                GP_ShowMaskedMultiPalPict( x, y, lpGPCUR, tmp, fog + 1024 );
                break;
            }
            delete[] tmp;
            break;
        }
        default:
            // Ported from the Windows switch: national-mask, transparent and
            // shadow sprite types (previously missing -> sprites invisible)
            if (Opt == 1)
            {
                // National mask: 2-bit indices -> NatPal color swap
                byte* tmp = new byte[UnpackLen];
                NatUnpackT( tmp, ( (byte*) lpGPCUR ) + CDOffs, UnpackLen );
                byte* enc = (byte*) ( NatPal + ( Nation << 2 ) );
                if (inv) GP_ShowMaskedPalPictInv( x, y, lpGPCUR, tmp, enc );
                else GP_ShowMaskedPalPict( x, y, lpGPCUR, tmp, enc );
                delete[] tmp;
            }
            else if (Opt == 2)
            {
                // transparent 1/4, raw data
                if (inv) GP_ShowMaskedMultiPalTPictInv( x, y, lpGPCUR, ( (byte*) lpGPCUR ) + CDOffs, trans4 );
                else GP_ShowMaskedMultiPalTPict( x, y, lpGPCUR, ( (byte*) lpGPCUR ) + CDOffs, trans4 );
            }
            else if (Opt == 3)
            {
                // transparent 1/2, raw data
                if (inv) GP_ShowMaskedMultiPalTPictInv( x, y, lpGPCUR, ( (byte*) lpGPCUR ) + CDOffs, trans8 );
                else GP_ShowMaskedMultiPalTPict( x, y, lpGPCUR, ( (byte*) lpGPCUR ) + CDOffs, trans8 );
            }
            else if (Opt == 4)
            {
                // transparent 3/4, raw data
                if (inv) GP_ShowMaskedMultiPalPictInv( x, y, lpGPCUR, ( (byte*) lpGPCUR ) + CDOffs, trans4 );
                else GP_ShowMaskedMultiPalPict( x, y, lpGPCUR, ( (byte*) lpGPCUR ) + CDOffs, trans4 );
            }
            else if (Opt == 5)
            {
                // Shadow: re-maps what is already on screen, themed by ImageType
                byte* enc = fog + 4096;
                switch (imt)
                {
                case 1: enc = Bright + 7936; break;    // white
                case 2: enc = yfog + 7936; break;      // red
                case 3: enc = Optional1 + 7936; break;
                case 4: enc = Optional2 + 7936; break;
                case 5: enc = Optional3 + 7936; break;
                }
                if (inv) GP_ShowMaskedPictShadowInv( x, y, lpGPCUR, NULL, enc );
                else GP_ShowMaskedPictShadow( x, y, lpGPCUR, NULL, enc );
            }
            else if (Opt == 39)
            {
                // Raw grey data (rom10/romBig fonts); same encoders as case 38
                // but without GreyUnpack - the data at CDOffs is already unpacked.
                // Previously missing -> all rom10/romBig glyphs invisible.
                byte* src = ( (byte*) lpGPCUR ) + CDOffs;
                if (!inv)
                {
                    switch (imt)
                    {
                    case 1:  GP_ShowMaskedMultiPalPict( x, y, lpGPCUR, src, Bright + startTrans ); break;
                    case 2:  GP_ShowMaskedMultiPalPict( x, y, lpGPCUR, src, yfog + startTrans ); break;
                    case 3:  GP_ShowMaskedMultiPalPict( x, y, lpGPCUR, src, Optional1 + startTrans ); break;
                    case 4:  GP_ShowMaskedMultiPalPict( x, y, lpGPCUR, src, Optional2 + startTrans ); break;
                    case 5:  GP_ShowMaskedMultiPalPict( x, y, lpGPCUR, src, Optional3 ); break;
                    default: GP_ShowMaskedMultiPalPict( x, y, lpGPCUR, src, fog + 1024 ); break;
                    }
                }
                else
                {
                    switch (imt)
                    {
                    case 1:  GP_ShowMaskedMultiPalPictInv( x, y, lpGPCUR, src, fog ); break;
                    case 2:  GP_ShowMaskedMultiPalPictInv( x, y, lpGPCUR, src, yfog ); break;
                    case 3:  GP_ShowMaskedMultiPalPictInv( x, y, lpGPCUR, src, Optional1 ); break;
                    case 4:  GP_ShowMaskedMultiPalPictInv( x, y, lpGPCUR, src, Optional2 ); break;
                    case 5:  GP_ShowMaskedMultiPalPictInv( x, y, lpGPCUR, src, Optional3 ); break;
                    default: GP_ShowMaskedMultiPalPictInv( x, y, lpGPCUR, src, fog + 1024 ); break;
                    }
                }
            }
            break;
        }

        DIFF    = lpGPCUR->NextPict;
        lpGPCUR = (GP_Header*)( (byte*)lpGP + DIFF );
        PAK++;
    } while (DIFF != -1);
}

void GP_System::ShowGPLayers(//IMPORTANT: color masking for units and buildings
	int x, int y, int FileIndex, int SprIndex,
	byte Nation, int mask )
{
// same structure as the Linux ShowGPPalLayers;
// temp-buffer decompression instead of the ring cache. Differs from PalLayers
// in the case-0/41/42-44 draws (plain/mirror Pict, water mirroring via XShift)
// and the national-mask table (NatPal - same as the Windows ShowGPLayers body).
	if ( FileIndex >= NGP || ( SprIndex & 4095 ) >= GPNFrames[FileIndex] )
	{
		return;
	}
	if (( ImageType[FileIndex] & 7 ) > 1)
	{
		//RLC file (ShowRLCItem is a no-op on Linux)
		if (!RLCImage[FileIndex]) LoadGP( FileIndex );
		if (RLCImage[FileIndex]) ShowRLCItem( x, y, RLCImage + FileIndex, SprIndex, Nation );
		return;
	}
	if (!GPH[FileIndex]) LoadGP( FileIndex );
	GP_GlobalHeader* lpGH = GPH[FileIndex];
	if (!lpGH) return;
	GP_Header* lpGP = GPX( lpGH, LGPH[SprIndex & 4095] );
	GP_Header* lpGPCUR = lpGP;
	bool inv = SprIndex >= 4096;
	int imt = ImageType[FileIndex] >> 4;
	int DIFF = -1;
	do
	{
		int UnpackLen = lpGPCUR->CData >> 14;
		int CDOffs = lpGPCUR->CData & 16383;
		byte Optx = lpGPCUR->Options;
		if (( Optx & 63 ) == 43) UnpackLen += 262144;
		if (( Optx & 63 ) == 44) UnpackLen += 262144 * 2;
		if (Optx & 64) CDOffs += 16384;
		if (Optx & 128) CDOffs += 32768;
		byte Opt = Optx & 63;
		byte* src = ( (byte*) lpGPCUR ) + CDOffs;
		switch (Opt)
		{
		case 0://standart packing
			if (mask & 1)
			{
				// +32 slack: StdUnpack's final dictionary copy may legally run
				// up to 17 bytes past UnpackLen (same as the Windows asm).
				byte* tmp = new byte[UnpackLen + 32];
				StdUnpack( tmp, src, UnpackLen, ( (byte*) lpGH ) + lpGH->VocOffset );
				if (mask & 512)
				{
					if (inv) GP_ShowMaskedPictInv( x, y, lpGPCUR, tmp, NULL );
					else GP_ShowMaskedPict( x, y, lpGPCUR, tmp, NULL );
				}
				else
				{
					if (inv) GP_ShowMaskedMirrorPictInv( x, y, lpGPCUR, tmp, XShift );
					else GP_ShowMaskedMirrorPict( x, y, lpGPCUR, tmp, XShift );
				}
				delete[] tmp;
			}
			break;
		case 1://National mask
			if (mask & 2)
			{
				byte* tmp = new byte[UnpackLen];
				NatUnpackT( tmp, src, UnpackLen );
				byte* enc = (byte*) ( NatPal + ( Nation << 2 ) );
				if (inv) GP_ShowMaskedPalPictInv( x, y, lpGPCUR, tmp, enc );
				else GP_ShowMaskedPalPict( x, y, lpGPCUR, tmp, enc );
				delete[] tmp;
			}
			break;
		case 2://transparent 1/4
			if (mask & 4)
			{
				if (inv) GP_ShowMaskedMultiPalTPictInv( x, y, lpGPCUR, src, trans4 );
				else GP_ShowMaskedMultiPalTPict( x, y, lpGPCUR, src, trans4 );
			}
			break;
		case 3://transparent 1/2
			if (mask & 8)
			{
				if (inv) GP_ShowMaskedMultiPalTPictInv( x, y, lpGPCUR, src, trans8 );
				else GP_ShowMaskedMultiPalTPict( x, y, lpGPCUR, src, trans8 );
			}
			break;
		case 4://transparent 3/4
			if (mask & 16)
			{
				if (inv) GP_ShowMaskedMultiPalPictInv( x, y, lpGPCUR, src, trans4 );
				else GP_ShowMaskedMultiPalPict( x, y, lpGPCUR, src, trans4 );
			}
			else if (mask & 2048)
			{
				if (inv) GP_ShowMaskedPictInv( x, y, lpGPCUR, src, NULL );
				else GP_ShowMaskedPict( x, y, lpGPCUR, src, NULL );
			}
			break;
		case 5://Shadow / Overpoint
			if (mask & 32)
			{
				// Windows parity: mask bit 64 = soft table shadow,
				// no bit 64 = black Overpoint column dither
				if (mask & 64)
				{
					if (inv) GP_ShowMaskedPictShadowInv( x, y, lpGPCUR, NULL, fog + 4096 );
					else GP_ShowMaskedPictShadow( x, y, lpGPCUR, NULL, fog + 4096 );
				}
				else
				{
					if (inv) GP_ShowMaskedPictOverpointInv( x, y, lpGPCUR, NULL, fog + 4096 );
					else GP_ShowMaskedPictOverpoint( x, y, lpGPCUR, NULL, fog + 4096 );
				}
			}
			break;
		case 6://AlphaRY
			if (mask & 128)
			{
				if (inv) GP_ShowMaskedMultiPalPictInv( x, y, lpGPCUR, src, AlphaR );
				else GP_ShowMaskedMultiPalPict( x, y, lpGPCUR, src, AlphaR );
			}
			break;
		case 7://AlphaWB
			if (mask & 256)
			{
				if (inv) GP_ShowMaskedMultiPalPictInv( x, y, lpGPCUR, src, AlphaW );
				else GP_ShowMaskedMultiPalPict( x, y, lpGPCUR, src, AlphaW );
			}
			break;
		case 38://grayscale multipal (interface color themes)
			if (mask & 1024)
			{
				byte* tmp = new byte[UnpackLen];
				GreyUnpack( tmp, src, UnpackLen );
				byte* enc = nullptr;
				switch (imt)
				{
				case 1: enc = inv ? Bright : Bright + startTrans; break;
				case 2: enc = inv ? yfog : yfog + startTrans; break;
				case 3: enc = inv ? Optional1 : Optional1 + startTrans; break;
				case 4: enc = inv ? Optional2 : Optional2 + startTrans; break;
				case 5: enc = Optional3; break;
				default: enc = fog + 1024; break;
				}
				GP_ShowMaskedMultiPalPict( x, y, lpGPCUR, tmp, enc );
				delete[] tmp;
			}
			break;
		case 39:
			if (mask & 1024)
			{
				byte* enc = nullptr;
				switch (imt)
				{
				case 1: enc = inv ? Bright : Bright + startTrans; break;
				case 2: enc = inv ? yfog : yfog + startTrans; break;
				case 3: enc = inv ? Optional1 : Optional1 + startTrans; break;
				case 4: enc = inv ? Optional2 : Optional2 + startTrans; break;
				case 5: enc = Optional3; break;
				default: enc = fog + 1024; break;
				}
				if (inv) GP_ShowMaskedMultiPalPictInv( x, y, lpGPCUR, src, enc );
				else GP_ShowMaskedMultiPalPict( x, y, lpGPCUR, src, enc );
			}
			break;
		case 41:
			if (mask & 512)
			{
				if (inv) GP_ShowMaskedPictInv( x, y, lpGPCUR, src, NULL );
				else GP_ShowMaskedPict( x, y, lpGPCUR, src, NULL );
			}
			else
			{
				if (inv) GP_ShowMaskedMirrorPictInv( x, y, lpGPCUR, src, XShift );
				else GP_ShowMaskedMirrorPict( x, y, lpGPCUR, src, XShift );
			}
			break;
		case 42:
		case 43:
		case 44:
			if (mask & 1)
			{
				byte* tmp = new byte[UnpackLen + 10];
				LZUnpack( tmp, src, UnpackLen );
				if (mask & 512)
				{
					if (inv) GP_ShowMaskedPictInv( x, y, lpGPCUR, tmp, NULL );
					else GP_ShowMaskedPict( x, y, lpGPCUR, tmp, NULL );
				}
				else
				{
					if (inv) GP_ShowMaskedMirrorPictInv( x, y, lpGPCUR, tmp, XShift );
					else GP_ShowMaskedMirrorPict( x, y, lpGPCUR, tmp, XShift );
				}
				delete[] tmp;
			}
			break;
		};
		DIFF = lpGPCUR->NextPict;
		lpGPCUR = (GP_Header*)( (byte*) lpGP + DIFF );
	} while (DIFF != -1);
};

void GP_System::ShowGPTransparent( int x, int y, int FileIndex, int SprIndex, byte Nation )
{
	if (!( FileIndex < NGP && ( SprIndex & 4095 ) < GPNFrames[FileIndex] ))
		return;
	if (( ImageType[FileIndex] & 7 ) > 1)
	{
		if (!RLCImage[FileIndex]) LoadGP( FileIndex );
		if (RLCImage[FileIndex]) ShowRLCItem( x, y, RLCImage + FileIndex, SprIndex, Nation );
		return;
	}
	if (!GPH[FileIndex]) LoadGP( FileIndex );
	GP_GlobalHeader* lpGH = GPH[FileIndex];
	if (!lpGH) return;
	int RSprIndex = SprIndex & 4095;
	GP_Header* lpGP = GPX( lpGH, LGPH[RSprIndex] );
	GP_Header* lpGPCUR = lpGP;
	if (ItDX[FileIndex]) x -= ItDX[FileIndex][RSprIndex];
	bool inv = SprIndex >= 4096;
	int DIFF = -1;
	do
	{
		int UnpackLen = lpGPCUR->CData >> 14;
		int CDOffs = lpGPCUR->CData & 16383;
		byte Optx = lpGPCUR->Options;
		if (( Optx & 63 ) == 43) UnpackLen += 262144;
		if (( Optx & 63 ) == 44) UnpackLen += 262144 * 2;
		if (Optx & 64) CDOffs += 16384;
		if (Optx & 128) CDOffs += 32768;
		byte Opt = Optx & 63;
		switch (Opt)
		{
		case 0://standart packing
		{
			byte* tmp = new byte[UnpackLen + 32];
			StdUnpack( tmp, ( (byte*) lpGPCUR ) + CDOffs, UnpackLen,
				( (byte*) lpGH ) + lpGH->VocOffset );
			if (inv) GP_ShowMaskedMultiPalPictInv( x, y, lpGPCUR, tmp, trans8 );
			else GP_ShowMaskedMultiPalPict( x, y, lpGPCUR, tmp, trans8 );
			delete[] tmp;
			break;
		}
		case 1://National mask
		{
			byte* tmp = new byte[UnpackLen];
			NatUnpackT( tmp, ( (byte*) lpGPCUR ) + CDOffs, UnpackLen );
			byte* enc = (byte*) ( NatPal + ( Nation << 2 ) );
			if (inv) GP_ShowMaskedPalPictInv( x, y, lpGPCUR, tmp, enc );
			else GP_ShowMaskedPalPict( x, y, lpGPCUR, tmp, enc );
			delete[] tmp;
			break;
		}
		case 3://transparent 1/2
			if (inv) GP_ShowMaskedMultiPalTPictInv( x, y, lpGPCUR, ( (byte*) lpGPCUR ) + CDOffs, trans4 );
			else GP_ShowMaskedMultiPalTPict( x, y, lpGPCUR, ( (byte*) lpGPCUR ) + CDOffs, trans4 );
			break;
		case 4://transparent 3/4
			if (inv) GP_ShowMaskedMultiPalPictInv( x, y, lpGPCUR, ( (byte*) lpGPCUR ) + CDOffs, trans8 );
			else GP_ShowMaskedMultiPalPict( x, y, lpGPCUR, ( (byte*) lpGPCUR ) + CDOffs, trans8 );
			break;
		case 5://Shadow
			if (inv) GP_ShowMaskedPictShadowInv( x, y, lpGPCUR, NULL, fog + 4096 );
			else GP_ShowMaskedPictShadow( x, y, lpGPCUR, NULL, fog + 4096 );
			break;
		case 41://direct data, then falls through to the LZ-unpacked pass (as in the asm)
			if (inv) GP_ShowMaskedMultiPalPictInv( x, y, lpGPCUR, ( (byte*) lpGPCUR ) + CDOffs, trans8 );
			else GP_ShowMaskedMultiPalPict( x, y, lpGPCUR, ( (byte*) lpGPCUR ) + CDOffs, trans8 );
			// fall through
		case 42:
		case 43:
		case 44:
		{
			byte* tmp = new byte[UnpackLen + 10];
			LZUnpack( tmp, ( (byte*) lpGPCUR ) + CDOffs, UnpackLen );
			if (inv) GP_ShowMaskedMultiPalPictInv( x, y, lpGPCUR, tmp, trans8 );
			else GP_ShowMaskedMultiPalPict( x, y, lpGPCUR, tmp, trans8 );
			delete[] tmp;
			break;
		}
		}
		DIFF = lpGPCUR->NextPict;
		lpGPCUR = (GP_Header*)( (byte*) lpGP + DIFF );
	} while (DIFF != -1);
}
// Linux port of ShowGPTransparentLayers: draws only the layers selected by the
// `mask` bitmask (used for the layered fog/animation passes).
void GP_System::ShowGPTransparentLayers( int x, int y, int FileIndex, int SprIndex, byte Nation, int mask )
{
	if (!( FileIndex < NGP && ( SprIndex & 4095 ) < GPNFrames[FileIndex] ))
		return;
	if (( ImageType[FileIndex] & 7 ) > 1)
	{
		if (!RLCImage[FileIndex]) LoadGP( FileIndex );
		if (RLCImage[FileIndex]) ShowRLCItem( x, y, RLCImage + FileIndex, SprIndex, Nation );
		return;
	}
	if (!GPH[FileIndex]) LoadGP( FileIndex );
	GP_GlobalHeader* lpGH = GPH[FileIndex];
	if (!lpGH) return;
	int RSprIndex = SprIndex & 4095;
	GP_Header* lpGP = GPX( lpGH, LGPH[RSprIndex] );
	GP_Header* lpGPCUR = lpGP;
	if (ItDX[FileIndex]) x -= ItDX[FileIndex][RSprIndex];
	bool inv = SprIndex >= 4096;
	int DIFF = -1;
	do
	{
		int UnpackLen = lpGPCUR->CData >> 14;
		int CDOffs = lpGPCUR->CData & 16383;
		byte Optx = lpGPCUR->Options;
		if (( Optx & 63 ) == 43) UnpackLen += 262144;
		if (( Optx & 63 ) == 44) UnpackLen += 262144 * 2;
		if (Optx & 64) CDOffs += 16384;
		if (Optx & 128) CDOffs += 32768;
		byte Opt = Optx & 63;
		switch (Opt)
		{
		case 0://standart packing
			if (mask & 1)
			{
				byte* tmp = new byte[UnpackLen + 32];
				StdUnpack( tmp, ( (byte*) lpGPCUR ) + CDOffs, UnpackLen,
					( (byte*) lpGH ) + lpGH->VocOffset );
				if (inv) GP_ShowMaskedMultiPalPictInv( x, y, lpGPCUR, tmp, trans8 );
				else GP_ShowMaskedMultiPalPict( x, y, lpGPCUR, tmp, trans8 );
				delete[] tmp;
			}
			break;
		case 1://National mask
			if (mask & 2)
			{
				byte* tmp = new byte[UnpackLen];
				NatUnpackT( tmp, ( (byte*) lpGPCUR ) + CDOffs, UnpackLen );
				byte* enc = (byte*) ( NatPal + ( Nation << 2 ) );
				if (inv) GP_ShowMaskedPalPictInv( x, y, lpGPCUR, tmp, enc );
				else GP_ShowMaskedPalPict( x, y, lpGPCUR, tmp, enc );
				delete[] tmp;
			}
			break;
		case 3://transparent 1/2
			if (mask & 8)
			{
				if (inv) GP_ShowMaskedMultiPalTPictInv( x, y, lpGPCUR, ( (byte*) lpGPCUR ) + CDOffs, trans4 );
				else GP_ShowMaskedMultiPalTPict( x, y, lpGPCUR, ( (byte*) lpGPCUR ) + CDOffs, trans4 );
			}
			break;
		case 4://transparent 3/4
			if (mask & 16)
			{
				if (inv) GP_ShowMaskedMultiPalPictInv( x, y, lpGPCUR, ( (byte*) lpGPCUR ) + CDOffs, trans8 );
				else GP_ShowMaskedMultiPalPict( x, y, lpGPCUR, ( (byte*) lpGPCUR ) + CDOffs, trans8 );
			}
			break;
		case 5://Shadow / Overpoint
			if (mask & 32)
			{
				// Windows parity: mask bit 64 = soft table shadow,
				// no bit 64 = black Overpoint column dither
				if (mask & 64)
				{
					if (inv) GP_ShowMaskedPictShadowInv( x, y, lpGPCUR, NULL, fog + 4096 );
					else GP_ShowMaskedPictShadow( x, y, lpGPCUR, NULL, fog + 4096 );
				}
				else
				{
					if (inv) GP_ShowMaskedPictOverpointInv( x, y, lpGPCUR, NULL, fog + 4096 );
					else GP_ShowMaskedPictOverpoint( x, y, lpGPCUR, NULL, fog + 4096 );
				}
			}
			break;
		case 41://storing, direct data - falls through to the LZ-unpacked pass
			if (mask & 1)
			{
				if (inv) GP_ShowMaskedMultiPalPictInv( x, y, lpGPCUR, ( (byte*) lpGPCUR ) + CDOffs, trans8 );
				else GP_ShowMaskedMultiPalPict( x, y, lpGPCUR, ( (byte*) lpGPCUR ) + CDOffs, trans8 );
			}
			// fall through
		case 42:
		case 43:
		case 44:
			if (mask & 1)
			{
				byte* tmp = new byte[UnpackLen + 10];
				LZUnpack( tmp, ( (byte*) lpGPCUR ) + CDOffs, UnpackLen );
				if (inv) GP_ShowMaskedMultiPalPictInv( x, y, lpGPCUR, tmp, trans8 );
				else GP_ShowMaskedMultiPalPict( x, y, lpGPCUR, tmp, trans8 );
				delete[] tmp;
			}
			break;
		}
		DIFF = lpGPCUR->NextPict;
		lpGPCUR = (GP_Header*)( (byte*) lpGP + DIFF );
	} while (DIFF != -1);
}
void GP_System::FreeRefs( int FileIndex )
{
	// Linux: no-op BY DESIGN. The runtime unpack cache (PackCash/GetCash ring
	// buffer + CASHREF 32-bit pointer tables) is not used on 64-bit - the draw
	// path decompresses each sprite into a temporary buffer per call. Since no
	// cache blocks are ever allocated here, there is nothing to release.
	// If the cache is ever ported (performance work for in-game rendering),
	// port the _WIN32 body below 1:1: it walks each sprite slot and its
	// NextPict chain, zeroes the cache block headers (INTV(PACKOFS-8)=0) so
	// GetCash can reclaim them, and resets slots to NO_PACK.
	(void) FileIndex;
#ifdef _WIN32
	//	assert(FileIndex<NGP);
	if (( ImageType[FileIndex] & 7 ) > 1 || !GPH[FileIndex])
	{
		return;
	}

	GP_GlobalHeader* lpGH = GPH[FileIndex];
	int NFR = GPNFrames[FileIndex];

	for (int SprIndex = 0; SprIndex < NFR; SprIndex++)
	{
		GP_Header* lpGP = GPX( lpGH, LGPH[SprIndex & 4095] );
		GP_Header* lpGPCUR = lpGP;
		DWORD* PAK = CASHREF[FileIndex];
		PAK += PAK[SprIndex & 4095];

		int DIFF = -1;

		int UnpackLen = lpGP->CData >> 14;
		int CDOffs = lpGP->CData & 16383;

		byte Optx = lpGP->Options;
		if (Optx & 64)
			CDOffs += 16384;
		if (Optx & 128)
			CDOffs += 32768;

		byte* PACKOFS = (byte*) ( *PAK );//lpGP->Pack;

		do
		{
			if (( PACKOFS ) && PACKOFS != NO_PACK)
			{
				INTV( PACKOFS - 8 ) = 0;
			}

			//lpGPCUR->Pack=NULL;

			*PAK = 0xFFFFFFFF;
			DIFF = lpGPCUR->NextPict;

			/*
			__asm
			{
				mov		eax, lpGP
				add		eax, DIFF
				mov		lpGPCUR, eax
			}
			*/
			lpGPCUR = (GP_Header*)( (byte*) lpGP + DIFF );

			UnpackLen = lpGPCUR->CData >> 14;
			CDOffs = lpGPCUR->CData & 16383;

			byte Optx = lpGPCUR->Options;
			if (Optx & 64)
				CDOffs += 16384;
			if (Optx & 128)
				CDOffs += 32768;

			PAK++;
			PACKOFS = (byte*) ( *PAK );//lpGPCUR->Pack;
		} while (DIFF != -1);
	}
#endif // _WIN32
}

void GP_System::ShowGPRedN( int x, int y, int FileIndex, int SprIndex, byte Nation, int N )
{
	ShowGPPal( x, y, FileIndex, SprIndex, Nation, yfog + ( N * 256 ) );
}

void GP_System::ShowGPDarkN( int x, int y, int FileIndex, int SprIndex, byte Nation, int N )
{
	ShowGPPal( x, y, FileIndex, SprIndex, Nation, wfog + ( N << 8 ) );
};
void GP_System::ShowGPDark( int x, int y, int FileIndex, int SprIndex, byte Nation )
{
	//	assert(FileIndex<NGP);
	ShowGPGrad( x, y, FileIndex, SprIndex, Nation, fog + 1024 );
};
void GP_System::ShowGPFired( int x, int y, int FileIndex, int SprIndex, byte Nation )
{
	//	assert(FileIndex<NGP);
	ShowGPGrad( x, y, FileIndex, SprIndex, Nation, yfog + 1024 );
};
void GP_System::ShowGPMutno( int x, int y, int FileIndex, int SprIndex, byte Nation )
{
	ShowGPGrad( x, y, FileIndex, SprIndex, Nation, wfog + 1024 );
};
void GP_System::ShowGPGrad( int x, int y, int FileIndex, int SprIndex, byte Nation, byte* Table )
{
	//assert(FileIndex<NGP);
	if (( ImageType[FileIndex] & 7 ) > 1)
	{
		//RLC file
		if (!RLCImage[FileIndex])
		{
			LoadGP( FileIndex );
		};
		if (RLCImage[FileIndex])ShowRLCItemGrad( x, y, RLCImage + FileIndex, SprIndex, Table );

		return;
	};
	ShowGP( x, y, FileIndex, SprIndex, Nation );
};

// Linux port of ShowGPPal: palette-masked sprite (buildings in placement mode).
// All draws use GP_ShowMaskedPalPict semantics: dst = Table[src].
void GP_System::ShowGPPal( int x, int y, int FileIndex, int SprIndex, byte Nation, byte* Table )
{
	if (( SprIndex & 4095 ) >= GPNFrames[FileIndex])
		return;
	if (( ImageType[FileIndex] & 7 ) > 1)
	{
		if (!RLCImage[FileIndex]) LoadGP( FileIndex );
		if (RLCImage[FileIndex]) ShowRLCItemPal( x, y, RLCImage + FileIndex, SprIndex, Table );
		return;
	}
	if (!GPH[FileIndex]) LoadGP( FileIndex );
	GP_GlobalHeader* lpGH = GPH[FileIndex];
	if (!lpGH) return;
	GP_Header* lpGP = GPX( lpGH, LGPH[SprIndex & 4095] );
	GP_Header* lpGPCUR = lpGP;
	bool inv = SprIndex >= 4096;
	int imt = ImageType[FileIndex] >> 4;
	int DIFF = -1;
	do
	{
		int UnpackLen = lpGPCUR->CData >> 14;
		int CDOffs = lpGPCUR->CData & 16383;
		byte Optx = lpGPCUR->Options;
		if (( Optx & 63 ) == 43) UnpackLen += 262144;
		if (( Optx & 63 ) == 44) UnpackLen += 262144 * 2;
		if (Optx & 64) CDOffs += 16384;
		if (Optx & 128) CDOffs += 32768;
		byte Opt = Optx & 63;
		switch (Opt)
		{
		case 0://standart packing
		{
			byte* tmp = new byte[UnpackLen + 32];
			StdUnpack( tmp, ( (byte*) lpGPCUR ) + CDOffs, UnpackLen,
				( (byte*) lpGH ) + lpGH->VocOffset );
			if (inv) GP_ShowMaskedPalPictInv( x, y, lpGPCUR, tmp, Table );
			else GP_ShowMaskedPalPict( x, y, lpGPCUR, tmp, Table );
			delete[] tmp;
			break;
		}
		case 1://National mask
		{
			byte* tmp = new byte[UnpackLen];
			NatUnpackT( tmp, ( (byte*) lpGPCUR ) + CDOffs, UnpackLen );
			byte* enc = (byte*) ( NatPal + ( Nation << 2 ) );
			if (inv) GP_ShowMaskedPalPictInv( x, y, lpGPCUR, tmp, enc );
			else GP_ShowMaskedPalPict( x, y, lpGPCUR, tmp, enc );
			delete[] tmp;
			break;
		}
		case 3://transparent 1/2
			if (inv) GP_ShowMaskedPalPictInv( x, y, lpGPCUR, ( (byte*) lpGPCUR ) + CDOffs, Table );
			else GP_ShowMaskedPalPict( x, y, lpGPCUR, ( (byte*) lpGPCUR ) + CDOffs, Table );
			break;
		case 4://transparent 3/4
			if (inv) GP_ShowMaskedPalPictInv( x, y, lpGPCUR, ( (byte*) lpGPCUR ) + CDOffs, Table );
			else GP_ShowMaskedPalPict( x, y, lpGPCUR, ( (byte*) lpGPCUR ) + CDOffs, Table );
			break;
		case 5://Shadow
			if (inv) GP_ShowMaskedPictShadowInv( x, y, lpGPCUR, nullptr, fog + 4096 );
			else GP_ShowMaskedPictShadow( x, y, lpGPCUR, nullptr, fog + 4096 );
			break;
		case 41:
			if (inv) GP_ShowMaskedPalPictInv( x, y, lpGPCUR, ( (byte*) lpGPCUR ) + CDOffs, Table );
			else GP_ShowMaskedPalPict( x, y, lpGPCUR, ( (byte*) lpGPCUR ) + CDOffs, Table );
			break;
		case 42:
		case 43:
		case 44:
		{
			byte* tmp = new byte[UnpackLen + 10];
			LZUnpack( tmp, ( (byte*) lpGPCUR ) + CDOffs, UnpackLen );
			if (inv) GP_ShowMaskedPalPictInv( x, y, lpGPCUR, tmp, Table );
			else GP_ShowMaskedPalPict( x, y, lpGPCUR, tmp, Table );
			delete[] tmp;
			break;
		}
		case 38://grey, std-packed - themed by ImageType
		{
			byte* tmp = new byte[UnpackLen];
			GreyUnpack( tmp, ( (byte*) lpGPCUR ) + CDOffs, UnpackLen );
			byte* enc = nullptr;
			switch (imt)
			{
			case 1: enc = inv ? Bright : Bright + startTrans; break;
			case 2: enc = inv ? yfog : yfog + startTrans; break;
			case 3: enc = inv ? Optional1 : Optional1 + startTrans; break;
			case 4: enc = inv ? Optional2 : Optional2 + startTrans; break;
			case 5: enc = Optional3; break;
			default: enc = fog + 1024; break;
			}
			if (inv) GP_ShowMaskedMultiPalPictInv( x, y, lpGPCUR, tmp, enc );
			else GP_ShowMaskedMultiPalPict( x, y, lpGPCUR, tmp, enc );
			delete[] tmp;
			break;
		}
		case 39://grey, raw data (rom fonts)
		{
			byte* src = ( (byte*) lpGPCUR ) + CDOffs;
			byte* enc = nullptr;
			switch (imt)
			{
			case 1: enc = inv ? fog : Bright + startTrans; break;
			case 2: enc = inv ? yfog : yfog + startTrans; break;
			case 3: enc = inv ? Optional1 : Optional1 + startTrans; break;
			case 4: enc = inv ? Optional2 : Optional2 + startTrans; break;
			case 5: enc = Optional3; break;
			default: enc = fog + 1024; break;
			}
			if (inv) GP_ShowMaskedMultiPalPictInv( x, y, lpGPCUR, src, enc );
			else GP_ShowMaskedMultiPalPict( x, y, lpGPCUR, src, enc );
			break;
		}
		}
		DIFF = lpGPCUR->NextPict;
		lpGPCUR = (GP_Header*)( (byte*) lpGP + DIFF );
	} while (DIFF != -1);
}
// Linux port of ShowGPPalLayers: palette-masked sprite with layer selection.
void GP_System::ShowGPPalLayers( int x, int y, int FileIndex, int SprIndex, byte Nation, byte* Table, int mask )
{
	if (( SprIndex & 4095 ) >= GPNFrames[FileIndex])
		return;
	if (( ImageType[FileIndex] & 7 ) > 1)
	{
		if (!RLCImage[FileIndex]) LoadGP( FileIndex );
		if (RLCImage[FileIndex]) ShowRLCItemPal( x, y, RLCImage + FileIndex, SprIndex, Table );
		return;
	}
	if (!GPH[FileIndex]) LoadGP( FileIndex );
	GP_GlobalHeader* lpGH = GPH[FileIndex];
	if (!lpGH) return;
	GP_Header* lpGP = GPX( lpGH, LGPH[SprIndex & 4095] );
	GP_Header* lpGPCUR = lpGP;
	bool inv = SprIndex >= 4096;
	int imt = ImageType[FileIndex] >> 4;
	int DIFF = -1;
	do
	{
		int UnpackLen = lpGPCUR->CData >> 14;
		int CDOffs = lpGPCUR->CData & 16383;
		byte Optx = lpGPCUR->Options;
		if (( Optx & 63 ) == 43) UnpackLen += 262144;
		if (( Optx & 63 ) == 44) UnpackLen += 262144 * 2;
		if (Optx & 64) CDOffs += 16384;
		if (Optx & 128) CDOffs += 32768;
		byte Opt = Optx & 63;
		switch (Opt)
		{
		case 0://standart packing
			if (mask & 1)
			{
				byte* tmp = new byte[UnpackLen + 32];
				StdUnpack( tmp, ( (byte*) lpGPCUR ) + CDOffs, UnpackLen,
					( (byte*) lpGH ) + lpGH->VocOffset );
				if (inv) GP_ShowMaskedPalPictInv( x, y, lpGPCUR, tmp, Table );
				else GP_ShowMaskedPalPict( x, y, lpGPCUR, tmp, Table );
				delete[] tmp;
			}
			break;
		case 1://National mask
			if (mask & 2)
			{
				byte* tmp = new byte[UnpackLen];
				NatUnpackT( tmp, ( (byte*) lpGPCUR ) + CDOffs, UnpackLen );
				byte* enc = (byte*) ( NatPal + ( Nation << 2 ) );
				if (inv) GP_ShowMaskedPalPictInv( x, y, lpGPCUR, tmp, enc );
				else GP_ShowMaskedPalPict( x, y, lpGPCUR, tmp, enc );
				delete[] tmp;
			}
			break;
		case 2://transparent 1/4
			if (mask & 4)
			{
				if (inv) GP_ShowMaskedMultiPalTPictInv( x, y, lpGPCUR, ( (byte*) lpGPCUR ) + CDOffs, trans4 );
				else GP_ShowMaskedMultiPalTPict( x, y, lpGPCUR, ( (byte*) lpGPCUR ) + CDOffs, trans4 );
			}
			break;
		case 3://transparent 1/2
			if (mask & 8)
			{
				if (inv) GP_ShowMaskedMultiPalTPictInv( x, y, lpGPCUR, ( (byte*) lpGPCUR ) + CDOffs, trans8 );
				else GP_ShowMaskedMultiPalTPict( x, y, lpGPCUR, ( (byte*) lpGPCUR ) + CDOffs, trans8 );
			}
			break;
		case 4://transparent 3/4
			if (mask & 16)
			{
				if (inv) GP_ShowMaskedMultiPalPictInv( x, y, lpGPCUR, ( (byte*) lpGPCUR ) + CDOffs, trans4 );
				else GP_ShowMaskedMultiPalPict( x, y, lpGPCUR, ( (byte*) lpGPCUR ) + CDOffs, trans4 );
			}
			break;
		case 5://Shadow / Overpoint
			if (mask & 32)
			{
				// Windows parity: mask bit 64 = soft table shadow,
				// no bit 64 = black Overpoint column dither (the
				// branches here were swapped like the f3aa479 bug)
				if (mask & 64)
				{
					if (inv) GP_ShowMaskedPictShadowInv( x, y, lpGPCUR, nullptr, fog + 4096 );
					else GP_ShowMaskedPictShadow( x, y, lpGPCUR, nullptr, fog + 4096 );
				}
				else
				{
					if (inv) GP_ShowMaskedPictOverpointInv( x, y, lpGPCUR, nullptr, fog + 4096 );
					else GP_ShowMaskedPictOverpoint( x, y, lpGPCUR, nullptr, fog + 4096 );
				}
			}
			break;
		case 41:
			if (inv) GP_ShowMaskedPalPictInv( x, y, lpGPCUR, ( (byte*) lpGPCUR ) + CDOffs, Table );
			else GP_ShowMaskedPalPict( x, y, lpGPCUR, ( (byte*) lpGPCUR ) + CDOffs, Table );
			break;
		case 42:
		case 43:
		case 44:
			if (mask & 1)
			{
				byte* tmp = new byte[UnpackLen + 10];
				LZUnpack( tmp, ( (byte*) lpGPCUR ) + CDOffs, UnpackLen );
				if (inv) GP_ShowMaskedPalPictInv( x, y, lpGPCUR, tmp, Table );
				else GP_ShowMaskedPalPict( x, y, lpGPCUR, tmp, Table );
				delete[] tmp;
			}
			break;
		case 38://grayscale multipal (interface color themes)
			if (mask & 1024)
			{
				byte* tmp = new byte[UnpackLen];
				GreyUnpack( tmp, ( (byte*) lpGPCUR ) + CDOffs, UnpackLen );
				byte* enc = nullptr;
				switch (imt)
				{
				case 1: enc = inv ? Bright : Bright + startTrans; break;
				case 2: enc = inv ? yfog : yfog + startTrans; break;
				case 3: enc = inv ? Optional1 : Optional1 + startTrans; break;
				case 4: enc = inv ? Optional2 : Optional2 + startTrans; break;
				case 5: enc = Optional3; break;
				case 6: enc = fog + 1024; break; // Optional4 not present
				default: enc = fog + 1024; break;
				}
				GP_ShowMaskedMultiPalPict( x, y, lpGPCUR, tmp, enc );
				delete[] tmp;
			}
			break;
		case 39://grey, raw data (rom fonts) - same encoders, no GreyUnpack
			if (mask & 1024)
			{
				byte* src = ( (byte*) lpGPCUR ) + CDOffs;
				byte* enc = nullptr;
				switch (imt)
				{
				case 1: enc = inv ? fog : Bright + startTrans; break;
				case 2: enc = inv ? yfog : yfog + startTrans; break;
				case 3: enc = inv ? Optional1 : Optional1 + startTrans; break;
				case 4: enc = inv ? Optional2 : Optional2 + startTrans; break;
				case 5: enc = Optional3; break;
				default: enc = fog + 1024; break;
				}
				if (inv) GP_ShowMaskedMultiPalPictInv( x, y, lpGPCUR, src, enc );
				else GP_ShowMaskedMultiPalPict( x, y, lpGPCUR, src, enc );
			}
			break;

		}
		DIFF = lpGPCUR->NextPict;
		lpGPCUR = (GP_Header*)( (byte*) lpGP + DIFF );
	} while (DIFF != -1);
}
GP_System GPS;

#ifdef _WIN32

extern int mapx;
extern int mapy;
static int npp = 0;

extern byte WaterCost[65536];

//Waves

void ShowGradPicture( int x, int y, int Lx, int Ly,
	int z1, int z2, int z3, int z4,
	byte* Bitmap, int BMLx )
{
	BMLx -= Lx;
	int a = z1 << 16;
	int b = div( ( z2 - z1 ) << 16, Lx ).quot;
	int c = div( ( z3 - z1 ) << 16, Ly ).quot;
	int d = div( ( z4 + z1 - z2 - z3 ) << 16, Lx*Ly ).quot;
	int ofst = int( ScreenPtr ) + x + y*ScrWidth;
	int addo = ScrWidth - Lx;

	__asm
	{
		push	esi
		push	edi
		pushf
		mov		esi, Bitmap
		mov		edi, ofst
		mov		ebx, a
		mov		edx, b
		lpp0 : mov		cl, byte ptr Lx
			   push	ebx
			   lpp1 : mov		eax, ebx
					  shr		eax, 13
					  mov		ah, [edi]
					  and al, 11111000b
					  or al, [esi]
					  add		ebx, edx
					  inc		esi
					  mov		al, [WaterCost + eax]
					  mov[edi], al
					  inc edi
					  dec		cl
					  jnz		lpp1
					  pop		ebx
					  add		edx, d
					  add		ebx, c
					  add		esi, BMLx
					  add		edi, addo
					  dec		Ly
					  jnz		lpp0
					  popf
					  pop		edi
					  pop		esi
	}
}

class WaterSpot
{
public:
	byte* Vertex;
	byte* Altitude;
	int Lx, Ly;

	WaterSpot();

	void WSpot( int x, int y, int r, int dr );
	void Process();
	void Draw( int x, int y );
};

WaterSpot::WaterSpot()
{
	Lx = 20;
	Ly = 20;
	int nn = Lx * Ly;

	Vertex = new byte[nn];
	Altitude = new byte[nn];

	memset( Vertex, 0, nn );
	memset( Altitude, 0, nn );

	for (int i = 0; i < nn; i++)
	{
		Altitude[i] = rand() & 7;

		if ((rand() & 0x7FFF) < 16384)//Windows 15-bit rand range
		{
			Altitude[i] |= 128;
		}
	}

	WSpot( 32 * 8, 32 * 8, 10, 256 );
}

void WaterSpot::WSpot( int xx, int yy, int r, int dr )
{
	int i = 0;
	for (int y = 0; y < Ly; y++)
	{
		for (int x = 0; x < Lx; x++)
		{
			int x1 = x * 32;
			int y1 = y * 32;
			int dd = int( sqrt( ( xx - x1 )*( xx - x1 ) + ( yy - y1 )*( yy - y1 ) ) );
			int dp = 0;

			if (dd < r)
			{
				dp = 32;
			}

			if (dd > r + dr)
			{
				dp = 0;
			}

			if (dd > r && dd < r + dr)
			{
				dp = div( ( r + dr - dd ) * 32, dr ).quot;
			}

			Vertex[i] += dp;

			if (Vertex[i] > 30)
			{
				Vertex[i] = 30;
			}

			i++;
		}
	}
}

extern int tmtmt;
void WaterSpot::Process()
{
	int nn = Lx*Ly;
	for (int i = 0; i < nn; i++)
	{
		byte alt = Altitude[i];
		if (alt & 128)
		{
			if (alt < 128 + 4)alt++;
			else alt &= 127;
		}
		else
		{
			if (alt)
			{
				if (tmtmt & 1)alt--;
			}
			else alt |= 128;
		};
		Altitude[i] = alt;
	};
};
byte ttt[4096];
void WaterSpot::Draw( int xx, int yy )
{
	int i = 0;
	for (int y = 0; y < Ly - 1; y++)
	{
		for (int x = 0; x < Lx - 1; x++)
		{
			int z1 = int( Vertex[i] ) + ( Altitude[i] & 127 ) - 5;
			if (z1 < 0)z1 = 0;
			if (z1 > 30)z1 = 30;
			int z2 = int( Vertex[i + 1] ) + ( Altitude[i + 1] & 127 ) - 3;
			if (z2 < 0)z2 = 0;
			if (z2 > 30)z2 = 30;
			int z3 = int( Vertex[i + Lx] ) + ( Altitude[i + Lx] & 127 ) - 3;
			if (z3 < 0)z3 = 0;
			if (z3 > 30)z3 = 30;
			int z4 = int( Vertex[i + Lx + 1] ) + ( Altitude[i + Lx + 1] & 127 ) - 3;
			if (z4 < 0)z4 = 0;
			if (z4 > 30)z4 = 30;
			ShowGradPicture( xx + ( x << 5 ), yy + ( y << 4 ), 32, 16, z1, z2, z3, z4, ttt, 64 );
			i++;
		};
		i++;
	};
};

#endif // _WIN32
// Declarations previously visible only through the guarded region above
extern int tmtmt;
extern short TSin[257];
extern short TCos[257];
int XShift[512];
void InitXShift()
{
	int ttt = tmtmt;
	int ii;
	int T1 = 48 + 20;
	int T2 = 84 + 20;
	int T3 = 68 + 20;
	int T4 = 56 + 20;

	for (int i = 0; i < 64; i++)
	{
		ii = i << 2;
		XShift[i] = ( ( TSin[( ( ( ii - ttt )*T1 ) / 16 ) & 255]
			+ TSin[( ( ( ii - ttt )*T2 ) / 16 ) & 255]
			+ TSin[( ( ( ii + ttt )*T3 ) / 16 ) & 255]
			+ TCos[( ( ( ii + ttt )*T4 ) / 16 ) & 255] ) * 4 ) >> 6;
	}

	memcpy( XShift + 64, XShift, 256 );
	memcpy( XShift + 128, XShift + 64, 256 );
	memcpy( XShift + 192, XShift + 128, 256 );
	memcpy( XShift + 256, XShift + 192, 256 );
	memcpy( XShift + 256 + 64, XShift + 256, 256 );
	memcpy( XShift + 256 + 128, XShift + 256 + 64, 256 );
	memcpy( XShift + 256 + 192, XShift + 256 + 128, 256 );

	XShift[0] = 0;

	for (int i = 0; i < 512; i++)
	{
		XShift[i] = ( XShift[i] * ( i + 20 ) ) >> 10;
	}

	for (int i = 511; i > 0; i--)
	{
		XShift[i] -= XShift[i - 1];
	}
}

LocalGP::LocalGP()
{
	GPID = -1;
	Uniq = false;
}

DLLEXPORT LocalGP::LocalGP( char* Name )
{
	GPID = -1;
	Load( Name );
}

DLLEXPORT LocalGP::~LocalGP()
{
	UnLoad();
}

void LocalGP::Load( char*Name )
{
	if (GPID != -1)
	{
		UnLoad();
	}
	GPID = GPS.PreLoadGPImage( Name );
	Uniq = NewGPImage;
}

void LocalGP::UnLoad()
{
	if (GPID != -1)
	{
		if (Uniq)
			GPS.UnLoadGP( GPID );
		GPID = -1;
		Uniq = 0;
	};
};
UNIFONTS UFONTS;

UNIFONTS::UNIFONTS()
{
	NFonts = 0;
	UFONTS = NULL;
};
UNIFONTS::~UNIFONTS()
{
	if (UFONTS)free( UFONTS );
	NFonts = 0;
	UFONTS = NULL;
};
void FONERR()
{
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "ERROR!", "Invalid Unicode.dat", sdlWindow);
	assert( 0 );
};

int UNI_LINEDLY1 = 0;
int UNI_LINEDY1 = 0;
int UNI_LINEDLY2 = 0;
int UNI_LINEDY2 = 0;
int UNI_HINTDY1 = 0;
int UNI_HINTDLY1 = 0;
int UNI_HINTDY2 = 0;
int UNI_HINTDLY2 = 0;

void UNIFONTS::LoadFonts()
{
	GFILE* F = Gopen( "Unicode.dat", "r" );
	if (!F)
	{
		return;
	}
	int N;
	int z = Gscanf( F, "%d", &N );
	if (1 == z)
	{
		for (int i = 0; i < N; i++)
		{
			char ccc[128];
			int z = Gscanf( F, "%s", ccc );
			GPS.PreLoadGPImage( ccc );
		}
	}
	z = Gscanf( F, "%d", &N );
	if (z == 1)
	{
		NFonts = 0;
		UFONTS = new OneUniFont[N];
		for (int i = 0; i < N; i++)
		{
			int q;
			int z = Gscanf( F, "%s%d", UFONTS[i].FontName, &q );
			if (z != 2)
			{
				FONERR();
			}
			if (q > 4)
			{
				FONERR();
			}
			UFONTS[i].UTBL.NTables = q;
			for (int p = 0; p < q; p++)
			{
				char FNM[64];
				int uc = 0;
				int z = Gscanf( F, "%d%d%d%d%d%d%s", &UFONTS[i].UTBL.USET[p].DX,
					&UFONTS[i].UTBL.USET[p].DY,
					&UFONTS[i].UTBL.USET[p].Start,
					&UFONTS[i].UTBL.USET[p].NSymbols,
					&UFONTS[i].UTBL.USET[p].GP_Start, &uc,
					FNM );
				UFONTS[i].UTBL.USET[p].UseColor = uc;
				if (z != 7)
				{
					FONERR();
				}
				UFONTS[i].UTBL.USET[p].GPID = GPS.PreLoadGPImage( FNM );
			}
		}
		NFonts = N;
		z = Gscanf( F, "%d", &N );
		int v;
		char ccc[64];
		for (int i = 0; i < N; i++)
		{
			z = Gscanf( F, "%s%d", ccc, &v );
			if (z == 2)
			{
				if (!strcmp( ccc, "UNI_LINEDLY1" ))
				{
					UNI_LINEDLY1 = v;
				}
				else
					if (!strcmp( ccc, "UNI_LINEDLY2" ))
					{
						UNI_LINEDLY2 = v;
					}
					else
						if (!strcmp( ccc, "UNI_LINEDY1" ))
						{
							UNI_LINEDY1 = v;
						}
						else
							if (!strcmp( ccc, "UNI_LINEDY2" ))
							{
								UNI_LINEDY2 = v;
							}
							else
								if (!strcmp( ccc, "UNI_HINTDY1" ))
								{
									UNI_HINTDY1 = v;
								}
								else
									if (!strcmp( ccc, "UNI_HINTDLY1" ))
									{
										UNI_HINTDLY1 = v;
									}
									else
										if (!strcmp( ccc, "UNI_HINTDY2" ))
										{
											UNI_HINTDY2 = v;
										}
										else
											if (!strcmp( ccc, "UNI_HINTDLY2" ))
											{
												UNI_HINTDLY2 = v;
											};
			};
		};
	};
	Gclose( F );
}

UNICODETABLE* UNIFONTS::FindFont( char* Name )
{
	for (int i = 0; i < NFonts; i++)
	{
		if (!_stricmp( UFONTS[i].FontName, Name ))
		{
			return &UFONTS[i].UTBL;
		}
	}
	return nullptr;
}

//Tests whether point (x, y) hits an opaque pixel of a packed GP sprite.
//Portable translation of the original inline x86 asm (line format is
//identical, see the drawing code in ShowGPTransparent for the encoder side):
//the pixel data starts at byte offset 23 (right after GP_Header), each line
//begins with a header byte; simple lines (bit 7 clear) store header pairs of
//(transparent run, opaque run) as two bytes per segment, complex lines
//(bit 7 set) pack one segment per byte - low nibble transparent run, high
//nibble opaque run, with bits 6/5 of the header extending runs past 15.
bool CheckInsideMask( GP_Header* Pic, int x, int y )
{
	x -= Pic->dx;
	y -= Pic->dy;
	if (x < 0 || y < 0)return false;
	if (x >= Pic->Lx)return false;
	if (y >= Pic->Ly)return false;
	//Sprite pixel data starts right after the 23-byte GP_Header
	const byte* p = (const byte*)Pic + 23;

	//Skip the first y lines to reach the requested row
	for (int i = 0; i < y; i++)
	{
		byte hdr = *p++;
		if (hdr & 128)
		{
			//complex line: low 5 bits = number of segment bytes
			p += hdr & 31;
		}
		else
		{
			//simple line: header = number of (space, data) byte pairs
			p += word(hdr) * 2;
		}
	}

	//Walk this line segment by segment, counting x down through each
	//transparent run followed by an opaque run; if x expires inside an
	//opaque run, the point is inside the sprite.
	byte hdr = *p;
	if (hdr & 128)
	{
		//complex line: bits 6/5 extend the respective runs to 16..31
		int spaceExt = (hdr & 64) ? 16 : 0;
		int dataExt = (hdr & 32) ? 16 : 0;
		int n = hdr & 31;
		p++;
		for (int i = 0; i < n; i++)
		{
			byte seg = *p++;
			int space = (seg & 15) | spaceExt;
			int data = (seg >> 4) | dataExt;
			if (x < space)return false;
			x -= space;
			if (x < data)return true;
			x -= data;
		}
		return false;
	}
	else
	{
		//simple line: two bytes per segment - transparent run, opaque run
		int n = hdr;
		if (!n)return false;
		p++;
		for (int i = 0; i < n; i++)
		{
			int space = *p++;
			int data = *p++;
			if (x < space)return false;
			x -= space;
			if (x < data)return true;
			x -= data;
		}
		return false;
	}
}

DLLEXPORT bool CheckGP_Inside( int FileIndex, int SprIndex, int dx, int dy )
{
	if (!( FileIndex < GPS.NGP && ( SprIndex & 4095 ) < GPS.GPNFrames[FileIndex] ))
		return false;

	int imt = GPS.ImageType[FileIndex] >> 4;

	if (( GPS.ImageType[FileIndex] & 7 ) > 1)
		return false;

	if (!GPS.GPH[FileIndex])
	{
		GPS.LoadGP( FileIndex );
	}
	GP_GlobalHeader* lpGH = GPS.GPH[FileIndex];
	int RSprIndex = SprIndex;
	if (SprIndex >= lpGH->NPictures)RSprIndex &= 4095;
	GP_Header* lpGP = GPX( lpGH, LGPH[RSprIndex] );
	GP_Header* lpGPCUR = lpGP;
	int DIFF = -1;

	byte Optx = lpGP->Options;


	do
	{
		byte Opt = lpGPCUR->Options & 63;
		switch (Opt)
		{
		case 0://standart packing
		case 1://National mask
		case 3://transparent 1/2
		case 4://transparent 3/4
		case 43:
		case 44:
		case 42:
			if (SprIndex >= 4096)
			{
				if (CheckInsideMask( lpGPCUR, -dx, dy ))
					return true;
			}
			else
			{
				if (CheckInsideMask( lpGPCUR, dx, dy ))
					return true;
			}
			break;
		}

		DIFF = lpGPCUR->NextPict;
		//NextPict is a raw byte offset to the next chained picture
		//(same byte-based advance the drawing code uses; plain pointer
		//arithmetic would scale by sizeof(GP_Header) and walk into garbage)
		lpGPCUR = (GP_Header*)( (byte*)lpGP + DIFF );


	} while (DIFF != -1);
	return false;
}

extern int mouseX;
extern int mouseY;

//Registring GP-frames
int N_GP_Reg = 0;
int Max_GP_Reg = 0;
struct GP_IMG;
GP_IMG* GP_Reg = nullptr;

struct GP_IMG
{
	word Index;
	word GPID;
	word Spr;
	short x;
	short y;
};

void Init_GP_IMG()
{
	N_GP_Reg = 0;
	Max_GP_Reg = 1024;
	GP_Reg = new GP_IMG[Max_GP_Reg];
}

void Clean_GP_IMG()
{
	N_GP_Reg = 0;
}

void RegisterVisibleGP( word Index, int FileIndex, int SprIndex, int x, int y )
{
	if (N_GP_Reg >= Max_GP_Reg)
	{
		Max_GP_Reg += 512;
		GP_Reg = (GP_IMG*) realloc( GP_Reg, Max_GP_Reg * sizeof(GP_IMG) );
	}

	GP_Reg[N_GP_Reg].GPID = FileIndex;
	GP_Reg[N_GP_Reg].Spr = SprIndex;
	GP_Reg[N_GP_Reg].x = x;
	GP_Reg[N_GP_Reg].y = y;
	GP_Reg[N_GP_Reg].Index = Index;
	N_GP_Reg++;
}

word CheckCoorInGP( int x, int y )
{
	for (int i = N_GP_Reg - 1; i >= 0; i--)
	{
		if (CheckGP_Inside( GP_Reg[i].GPID, GP_Reg[i].Spr, x - GP_Reg[i].x, y - GP_Reg[i].y ))
		{
			return GP_Reg[i].Index;
		}
	}
	return 0xFFFF;
}

void GetRealGPSize( int File, int spr, int* dx, int* dy )
{
	*dx = 0;
	*dy = 0;

	if (!GPS.GPH[File])
	{
		return;
	}

	if (spr >= GPS.GPH[File]->NPictures)
	{
		return;
	}

	lpGP_GlobalHeader lpGPH = GPS.GPH[File];
	*dx = GPX( lpGPH, LGPH[spr] )->GetDx();
	*dy = GPX( lpGPH, LGPH[spr] )->GetDy();
}
