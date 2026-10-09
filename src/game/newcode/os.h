// OS compatibility definitions
// Mostly reimplemeting stuff from windows.h

#ifdef _WIN32

#define DLLEXPORT __declspec(dllexport)
#define DLLIMPORT __declspec(dllimport)
#define CDECL _cdecl
#include <windows.h>

#else

#define NODPLAY // Linux has no DirectPlay

#define DLLEXPORT // Nothing
#define DLLIMPORT // Nothing
#define CDECL // Nothing

// https://stackoverflow.com/a/38415387/816449
typedef short small;
typedef short WCHAR;
typedef char CHAR;
typedef void * HANDLE;
#define INVALID_HANDLE_VALUE ((HANDLE)(-1))
typedef void VOID;
typedef unsigned short WORD;
typedef unsigned short word;
typedef unsigned char BYTE;
typedef BYTE *LPBYTE;
typedef unsigned char byte;
typedef unsigned int DWORD; // DWORD was 4 bytes on 32-bit Windows
typedef int LONG;
typedef unsigned int BOOL;
#define TRUE 1
#define FALSE 0

#define _strnicmp strncasecmp
#define _stricmp strcasecmp 

// https://softwareengineering.stackexchange.com/a/194768/96231
typedef const char* LPCSTR; // stands for Long Pointer to a Constant STRing.
typedef const char* LPCTSTR; 
typedef wchar_t* LPWSTR;  // Long Pointer to Wide STRing
typedef char* LPSTR;  // Long Pointer to STRing (ANSI)
typedef void* LPVOID; // Long Pointer to a VOID
typedef unsigned int UINT;


// Handle to a DDL or something
typedef void* HINSTANCE;

// Windows DLL loading has no Linux equivalent in this port;
// LoadLibrary fails (returns NULL) so callers take their error paths.
#define LoadLibrary(Name) ((HINSTANCE)0)
#define FreeLibrary(Lib)
#define GetProcAddress(Lib, Name) ((void*)0)

// Global memory allocator shims (only fixed blocks were used)
#define GMEM_FIXED 0
#define GlobalAlloc(Flags, Size) malloc(Size)
#define GlobalFree(Ptr) free(Ptr)

BOOL DeleteFile(LPCTSTR lpFileName);
void SetLastError(DWORD dwErrCode);

#define HIBYTE(w) ((BYTE)(((w) >> 8) & 0xFF))

typedef int HRESULT;

#include <cstring>

#define ZeroMemory(Destination, Length) memset((Destination), 0, (Length))

char* _strupr(char *str);

#endif
