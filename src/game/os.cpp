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
