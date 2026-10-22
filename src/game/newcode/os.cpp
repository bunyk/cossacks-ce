// Here I'll put implementation of stuff from windows.h

#define SAFE // Comment this to do dangerous things like deleting files

#include <cstdio>
#include <ctype.h>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

#include <dlfcn.h>

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

//-------------- dynamic library loading (Windows LoadLibrary) --------------

// Copy a Windows-style path ("AI\Algeria.dll") to a Linux one ("AI/Algeria.dll").
static char* toLinuxLibPath(const char* Name)
{
	char* p = strdup(Name);
	for (char* q = p; *q; ++q)
		if (*q == '\\') *q = '/';
	return p;
}

// Directory the main executable lives in (Windows LoadLibrary searches it).
static void getExeDir(char* buf, size_t bufSize)
{
	buf[0] = 0;
	ssize_t n = readlink("/proc/self/exe", buf, bufSize - 1);
	if (n > 0)
	{
		buf[n] = 0;
		char* slash = strrchr(buf, '/');
		if (slash) *slash = 0;
		else buf[0] = 0;
	}
}

// Try to dlopen one candidate path; also accepts "*.dll" data names by
// retrying the same name with a ".so" extension.
static void* tryLoadLibrary(const char* path)
{
	void* lib = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
	if (!lib)
	{
		size_t len = strlen(path);
		if (len > 4 && !strcasecmp(path + len - 4, ".dll"))
		{
			char* alt = strdup(path);
			strcpy(alt + len - 4, ".so");
			lib = dlopen(alt, RTLD_NOW | RTLD_GLOBAL);
			free(alt);
		}
	}
	return lib;
}

HINSTANCE osLoadLibrary(const char* Name)
{
	if (!Name || !Name[0]) return NULL;

	char* path = toLinuxLibPath(Name);
	void* lib = NULL;

	// A path containing a slash is taken relative to the working directory
	// (the game is run from the data directory), matching Windows behavior.
	if (strchr(path, '/'))
	{
		lib = tryLoadLibrary(path);
	}
	else
	{
		// A bare name on Windows is searched in the exe directory and cwd.
		char cand[512];
		char exeDir[512];
		getExeDir(exeDir, sizeof(exeDir));

		snprintf(cand, sizeof(cand), "./%s", path);
		lib = tryLoadLibrary(cand);

		if (!lib && exeDir[0])
		{
			snprintf(cand, sizeof(cand), "%s/%s", exeDir, path);
			lib = tryLoadLibrary(cand);
		}
	}
	if (!lib)
	{
		fprintf(stderr, "LoadLibrary(%s) failed: %s\n", Name, dlerror());
	}
	free(path);
	return (HINSTANCE)lib;
}

void osFreeLibrary(HINSTANCE Lib)
{
	if (Lib) dlclose(Lib);
}

void* osGetProcAddress(HINSTANCE Lib, const char* Name)
{
	return dlsym(Lib, Name);
}
