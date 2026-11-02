#pragma warning (disable : 4035)

#include "../newcode/os.h"
#include <cstdint>


void isiDecryptMem(LPBYTE lpbBuffer, DWORD dwSize, BYTE dbKey)
{
    BYTE* ptr = lpbBuffer;
    BYTE* end = lpbBuffer + dwSize;

    while (ptr < end) {
        BYTE al = *ptr;        // mov al, [ebx]
        al = ~al;              // not al
        al ^= dbKey;           // xor al, ah  (ah was loaded with dbKey)
        *ptr = al;             // mov [ebx], al
        ++ptr;                 // inc ebx
    }
}

DWORD isiCalcHash(LPSTR lpszFileName)
{
	char	szFileName[64];

	ZeroMemory(szFileName,64);
	strcpy(szFileName,_strupr(lpszFileName));

    uint32_t hash = 0;

    for (int i = 0; i < 16; ++i) {
        // Read 4 bytes as uint32_t
        uint32_t val = *(uint32_t*)(szFileName + i * 4);

        // xchg ah, al
        uint8_t al = val & 0xFF;
        uint8_t ah = (val >> 8) & 0xFF;
        val = (val & 0xFFFF0000) | (al << 8) | ah;

        // rol eax, 16
        val = (val << 16) | (val >> 16);

        // xchg ah, al again
        al = val & 0xFF;
        ah = (val >> 8) & 0xFF;
        val = (val & 0xFFFF0000) | (al << 8) | ah;

        hash += val;
    }

    return hash;
}

// C translation of the isiMatchesMask asm glob matcher ('*' and '?',
// case-insensitive; both strings are uppercased first, like the asm does)
static BOOL MatchMaskUp( const char* file, const char* mask )
{
	if (!*mask) return !*file;
	if (*mask == '?') return MatchMaskUp( file + 1, mask + 1 );
	if (*mask == '*')
	{
		for (const char* f = file; ; f++)
		{
			if (MatchMaskUp( f, mask + 1 )) return TRUE;
			if (!*f) return FALSE;
		}
	}
	return *file == *mask && MatchMaskUp( file + 1, mask + 1 );
}

BOOL isiMatchesMask(LPSTR lpszFile, LPSTR lpszMask)
{
	char	szFile[255];
	char	szMask[255];

	// NOTE: unlike the asm version, do NOT uppercase in place - lpszFile can
	// point into the read-only mapped archive FAT (CGSCarch::FindFile).
	strcpy(szFile, lpszFile);
	strcpy(szMask, lpszMask);
	_strupr(szFile);
	_strupr(szMask);

	return MatchMaskUp( szFile, szMask );
}
