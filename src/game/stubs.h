
// https://stackoverflow.com/a/38415387/816449


#ifdef _WIN32

#define DLLEXPORT __declspec(dllexport)
#define DLLIMPORT __declspec(dllimport)
#define CDECL _cdecl

#else

#define DLLEXPORT // Nothing
#define DLLIMPORT // Nothing
#define CDECL // Nothing

typedef short small;
typedef short WCHAR;
typedef void * HANDLE;
typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef unsigned char byte;
typedef unsigned long DWORD;
typedef unsigned int BOOL;

#define INVALID_HANDLE_VALUE ((HANDLE)(-1))

typedef const char* LPCSTR; // stands for Long Pointer to a Constant STRing.
typedef wchar_t* LPWSTR;  // Long Pointer to Wide STRing
typedef char* LPSTR;  // Long Pointer to STRing (ANSI)
typedef void* LPVOID; // Long Pointer to a VOID
typedef unsigned int UINT;


// Handle to a DDL or something
typedef void* HINSTANCE;  

#endif
