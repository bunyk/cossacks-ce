/*      Work with the resource files
 *
 *  You must use this module for accesss to files this
 * routine allows you to read files from disk or from
 * the resource file, you even will not recognise where
 * the given file is.
 */

#include <stdlib.h>
#include <stdio.h>
#include <cstring>

#include "arc/gscset.h"
#include "resfile.h"
#include "newcode/os.h"

bool InitDone = 0;

CGSCset GSFILES;

//Opening the resource file
bool ProtectionMode = false;

ResFile RResetEx( LPCSTR lpFileName )
{
	bool Only = GSFILES.m_ArchList&&ProtectionMode;

	FilesInit();

	LPGSCfile lpf = GSFILES.gOpenFile( lpFileName, Only );
	if (lpf)
	{
		return ResFile( lpf );
	}
	else
	{
		return INVALID_HANDLE_VALUE;
	}
}

void RCloseEx( ResFile hFile );
DLLEXPORT ResFile RReset( LPCSTR lpFileName )
{
	SetLastError( 0 );

	return RResetEx( lpFileName );
}


//Rewriting file
DLLEXPORT ResFile RRewrite( LPCSTR lpFileName )
{
	FilesInit();
	//return CreateFile(lpFileName,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,
	//	                         CREATE_ALWAYS,0,NULL);
	LPGSCfile lpf = GSFILES.gWriteOpen( lpFileName );
	if ( lpf )
	{
		return ResFile( lpf );
	}
	else
	{
		return INVALID_HANDLE_VALUE;
	}
}

//Getting size of the resource file
DLLEXPORT DWORD RFileSize( ResFile hFile )
{
	if ( hFile == INVALID_HANDLE_VALUE )
	{
		return 0;
	}
	LPGSCfile hf = LPGSCfile( hFile );
	return GSFILES.gFileSize( hf );//GetFileSize(hFile,NULL);
}

// Setting file position 
DLLEXPORT DWORD RSeek( ResFile hFile, int pos )
{
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return 0;
	}

	LPGSCfile hf = LPGSCfile( hFile );
	GSFILES.gSeekFile( hf, pos );

	return 0;
}

//Reading the file
DLLEXPORT DWORD RBlockRead( ResFile hFile, LPVOID lpBuffer, DWORD BytesToRead )
{
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return 0;
	}

	LPGSCfile hf = LPGSCfile( hFile );

	GSFILES.gReadFile( hf, LPBYTE( lpBuffer ), BytesToRead );

	return BytesToRead;
}

//Writing the file
DLLEXPORT DWORD RBlockWrite( ResFile hFile, LPVOID lpBuffer, DWORD BytesToWrite )
{
	if ( hFile == INVALID_HANDLE_VALUE )
	{
		return 0;
	}

	LPGSCfile hf = LPGSCfile( hFile );
	GSFILES.gWriteFile( hf, LPBYTE( lpBuffer ), BytesToWrite );

	return BytesToWrite;
}

DWORD IOresult( void )
{
	return 0;//GetLastError();
}

void RCloseEx( ResFile hFile )
{
	if (hFile == INVALID_HANDLE_VALUE)return;
	//CloseHandle(hFile);
	LPGSCfile hf = LPGSCfile( hFile );
	GSFILES.gCloseFile( hf );
}

DLLEXPORT void RClose( ResFile hFile )
{
	if (hFile == INVALID_HANDLE_VALUE)return;
	LPGSCfile hf = LPGSCfile( hFile );
	//CloseHandle(hFile);
	RCloseEx( hf );
}


//Scans the data directory for .gsc archives (CGSCset::gOpen)
bool FilesInit()
{
	if (InitDone)
	{
		return true;
	}

	InitDone = 1;

	bool retval = 0 != GSFILES.gOpen();

	return retval;
}

void FilesExit()
{
	GSFILES.gClose();
}
