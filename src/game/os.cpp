// Here I'll put implementation of stuff from windows.h

#define SAFE // Comment this to do dangerous things like deleting files

#include <cstdio>
#include "os.h"

BOOL DeleteFile(LPCTSTR lpFileName)
{
	printf("Deleting file: %s\n", lpFileName);
	BOOL res = true;
	#ifndef SAFE
	res = remove(lpFileName);
	#endif
	return res;
}


void SetLastError(DWORD dwErrCode) {
	printf("Setting last error code: %lu\n", dwErrCode);
	// In Linux we don't have a global error code, so this is just a stub
	// In Windows this would set the last error code for GetLastError()
	// but here we don't have such a thing, so we just print it
}
