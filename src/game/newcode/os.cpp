// Here I'll put implementation of stuff from windows.h

#define SAFE // Comment this to do dangerous things like deleting files

#include <cstdio>
#include <ctype.h>

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
	// In Linux we don't have a global error code, so this is just a stub
	// In Windows this would set the last error code for GetLastError()
	(void)dwErrCode;
}


// Convert string to uppercase
char* _strupr(char *str) {
	for (char* p = str; *p; ++p) {
		*p = toupper(*p);
	}
	return str;
}
