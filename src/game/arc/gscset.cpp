// GSCset.cpp: implementation of the CGSCset class.

#include <stdio.h>
#include <cstring>

#include "gsctypes.h"
#include "gscarch.h"
#include "gscset.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGSCset::CGSCset()
{

}

CGSCset::~CGSCset()
{

}

LPGSCfile CGSCset::gOpenFile( LPCSTR lpcsFileName, bool onlyInArchive )
{
	printf("gOpenFile: %s\n", lpcsFileName);
	LPGSCfile		gFile = NULL;
	bool			inArchive = true;

#ifdef _WIN32
	if (!onlyInArchive)
	{
		HANDLE			hFindFile;
		WIN32_FIND_DATA	FindData;
		hFindFile = FindFirstFile( lpcsFileName, &FindData );
		if (hFindFile != INVALID_HANDLE_VALUE)
		{
			inArchive = false;
		}
		FindClose( hFindFile );
	}
#endif _WIN32

	if (inArchive)
	{
		LPGSCArchList	pArchList = m_ArchList;

		while (pArchList)
		{
			gFile = pArchList->m_Arch->GetFileHandle( lpcsFileName );
			if (gFile)
			{
				gFile->m_Arch = pArchList->m_Arch;
				gFile->m_Flags = 1;
				gFile->m_Position = 0;
				return gFile;
			}
			pArchList = pArchList->m_NextArch;
		}
	}
#ifdef _WIN32
	else
	{
		gFile = new TGSCfile;
		gFile->m_Flags = 0;
		gFile->m_Position = 0;
		gFile->m_Arch = NULL;

		gFile->m_FileHandle = DWORD( CreateFile( lpcsFileName,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			0 ) );

		if (gFile->m_FileHandle == (DWORD) INVALID_HANDLE_VALUE)  // На всякий случай
		{
			delete gFile;
			return NULL;
		}
	}
#endif

	return gFile;
}

VOID CGSCset::gCloseFile( LPGSCfile gFile )
{
#ifdef _WIN32
	if (nullptr == gFile)//BUGFIX: Exception when closing game via Alt+F4
	{
		return;
	}

	if (gFile->m_Flags)
	{
		gFile->m_Arch->CloseFileHandle( gFile );
	}
	else
	{
		CloseHandle( HANDLE( gFile->m_FileHandle ) );
	}
#endif
}

VOID CGSCset::gReadFile( LPGSCfile gFile, LPBYTE lpbBuffer, DWORD dwSize )
{
	DWORD	dwBytesRead = 0;

	if (gFile->m_Flags)
	{
		gFile->m_Arch->ReadFile( gFile, lpbBuffer, dwSize );
	}
	else
	{

#ifdef _WIN32
		ReadFile( HANDLE( gFile->m_FileHandle ), lpbBuffer, dwSize, &dwBytesRead, NULL );
#else
		printf("Need to implement ReadFile for non-Windows platforms\n");
#endif
	};
}

VOID CGSCset::gSeekFile( LPGSCfile gFile, DWORD dwPosition )
{
#ifdef _WIN32
	if (gFile->m_Flags)
	{
		gFile->m_Position = dwPosition;
	}
	else
	{
		SetFilePointer( HANDLE( gFile->m_FileHandle ), dwPosition, NULL, FILE_BEGIN );
	}
#endif
}

DWORD CGSCset::gFilePos( LPGSCfile gFile )
{
#ifdef _WIN32
	if (gFile->m_Flags)
	{
		return gFile->m_Position;
	}
	else
	{
		return SetFilePointer( HANDLE( gFile->m_FileHandle ), 0, NULL, FILE_BEGIN );
	};
#endif
}

VOID AddItem( LPGSCFileList pFileList, LPCSTR lpcsFileName )
{
	LPGSCFileList	pList = pFileList, pItem;
	BOOL	Exists = FALSE;

	while (pList)
	{
		if (!strcmp( pList->m_FileName, lpcsFileName ))
		{
			Exists = TRUE;
			break;
		};
		pList = pList->m_NextEntry;
	};

	if (!Exists)
	{
		pList = pFileList;

		while (pList->m_NextEntry)
			pList = pList->m_NextEntry;

		pItem = new TGSCFileList;
		pItem->m_NextEntry = NULL;
		strcpy( pItem->m_FileName, lpcsFileName );

		pList->m_NextEntry = pItem;

	};
}

LPGSCFindInfo CGSCset::gFindFirst( LPCSTR lpcsMask )
{
	LPGSCFindInfo lpFindInfo = NULL;
#ifdef _WIN32
	WIN32_FIND_DATA	FindData;
	HANDLE	hFindFile;
	LPGSCFileList	lpFileList = NULL;

	CHAR	sUpMask[64];
	ZeroMemory( sUpMask, 64 );
	strcpy( sUpMask, lpcsMask );
	_strupr( sUpMask );

	hFindFile = FindFirstFile( sUpMask, &FindData );

	if (hFindFile != INVALID_HANDLE_VALUE)
	{
		lpFindInfo = new TGSCFindInfo;
		lpFindInfo->m_Pos = NULL;
		lpFindInfo->m_FileList = NULL;

		lpFileList = new TGSCFileList;
		lpFindInfo->m_FileList = lpFileList;
		lpFindInfo->m_Pos = lpFileList;
		lpFileList->m_NextEntry = NULL;
		strcpy( lpFileList->m_FileName, FindData.cFileName );
		_strupr( FindData.cFileName );

		while (FindNextFile( hFindFile, &FindData ))
		{
			lpFileList->m_NextEntry = new TGSCFileList;
			lpFileList = lpFileList->m_NextEntry;
			lpFileList->m_NextEntry = NULL;
			strcpy( lpFileList->m_FileName, FindData.cFileName );
			_strupr( FindData.cFileName );
		};
	};

	FindClose( hFindFile );

	LPGSCArchList	pArchList = m_ArchList;
	LPGSCFindData	pFindData;

	while (pArchList)
	{
		pFindData = pArchList->m_Arch->FindFile( sUpMask );

		if (pFindData)
		{
			if (!lpFindInfo)
			{
				lpFindInfo = new TGSCFindInfo;
				lpFindInfo->m_Pos = NULL;
				lpFindInfo->m_FileList = NULL;

				lpFileList = new TGSCFileList;
				lpFindInfo->m_FileList = lpFileList;
				lpFindInfo->m_Pos = lpFileList;
				lpFileList->m_NextEntry = NULL;
				strcpy( lpFileList->m_FileName, pFindData->m_FileName );
			}
			else
				AddItem( lpFindInfo->m_FileList, pFindData->m_FileName );

			while (pArchList->m_Arch->NextFile( pFindData ))
			{
				AddItem( lpFindInfo->m_FileList, pFindData->m_FileName );
			};
		};

		pArchList = pArchList->m_NextArch;
	};

#endif
	if (lpFindInfo)
	{
		strcpy( lpFindInfo->m_FileName, lpFindInfo->m_Pos->m_FileName );
		return lpFindInfo;
	}
	else
		return NULL;
}

BOOL CGSCset::gFindNext( LPGSCFindInfo gFindInfo )
{
	gFindInfo->m_Pos = gFindInfo->m_Pos->m_NextEntry;

	if (gFindInfo->m_Pos)
	{
		strcpy( gFindInfo->m_FileName, gFindInfo->m_Pos->m_FileName );
		return TRUE;
	}
	else
	{
		LPGSCFileList	lpFileList1, lpFileList2;

		lpFileList2 = gFindInfo->m_FileList;

		while (lpFileList2)
		{
			lpFileList1 = lpFileList2;
			lpFileList2 = lpFileList1->m_NextEntry;

			delete lpFileList1;
		}

		delete gFindInfo;

		return FALSE;
	}

}
void GSC_OpenError();

//Loads specified *.gs1 and *.gsc archives
//Priority order
//1) mods02.gs1, mods01.gs1, mods.gs1
//2) cossacks_revamp.gs1
//3) patch02.gs1, patch01.gs1
//4) A*.gsc

// BUT why I see only override.gsc and resources.gsc being searched?

#ifdef _WIN32
BOOL CGSCset::gOpen()
{
	BOOL retval = TRUE;
	printf("gOpen()\n");
	HANDLE hFindFile;
	WIN32_FIND_DATA FindData;
	LPGSCArchList pArchList = nullptr;

	m_ArchList = nullptr;

	hFindFile = FindFirstFile( "override.gsc", &FindData );
	if (INVALID_HANDLE_VALUE != hFindFile)
	{
		pArchList = new TGSCArchList;
		m_ArchList = pArchList;
		pArchList->m_NextArch = nullptr;
		pArchList->m_Arch = new CGSCarch;
		pArchList->m_Arch->Open( FindData.cFileName );

		while (FindNextFile( hFindFile, &FindData ))
		{
			pArchList->m_NextArch = new TGSCArchList;
			pArchList = pArchList->m_NextArch;

			pArchList->m_Arch = new CGSCarch;
			pArchList->m_NextArch = nullptr;
			pArchList->m_Arch->Open( FindData.cFileName );
		}
	}
	FindClose( hFindFile );

	hFindFile = FindFirstFile( "resources.gsc", &FindData );
	if (INVALID_HANDLE_VALUE != hFindFile)
	{
		if (nullptr == m_ArchList)
		{
			pArchList = new TGSCArchList;
			m_ArchList = pArchList;
		}
		else
		{
			pArchList->m_NextArch = new TGSCArchList;
			pArchList = pArchList->m_NextArch;
		}

		pArchList->m_NextArch = nullptr;
		pArchList->m_Arch = new CGSCarch;
		pArchList->m_Arch->Open( FindData.cFileName );

		while (FindNextFile( hFindFile, &FindData ))
		{
			pArchList->m_NextArch = new TGSCArchList;
			pArchList = pArchList->m_NextArch;
			pArchList->m_Arch = new CGSCarch;
			pArchList->m_NextArch = nullptr;
			pArchList->m_Arch->Open( FindData.cFileName );
		}
	}
	else
	{
		retval = FALSE;
	}

	FindClose( hFindFile );

	return retval;
}
#else

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

BOOL CGSCset::gOpen() {
    BOOL retval = TRUE;

    m_ArchList = nullptr;
    TGSCArchList** nextPtr = &m_ArchList;

    auto loadGSCs = [&](const std::string& pattern) {
        for (const auto& entry : fs::directory_iterator(".")) {
            if (entry.is_regular_file() && entry.path().extension() == ".gsc" && entry.path().filename().string().find(pattern) != std::string::npos) {
                *nextPtr = new TGSCArchList;
                (*nextPtr)->m_Arch = new CGSCarch;
                (*nextPtr)->m_Arch->Open(entry.path().string().c_str());
                (*nextPtr)->m_NextArch = nullptr;
                nextPtr = &((*nextPtr)->m_NextArch);
            }
        }
    };

    // Load override.gsc files first
    loadGSCs("override");

    // Then load resources.gsc files
    size_t before = reinterpret_cast<size_t>(nextPtr);
    loadGSCs("resources");

    loadGSCs("all"); // Added this so the only GSC file coming with GOG version loads.

    // Check if anything was loaded in "resources"
    if (reinterpret_cast<size_t>(nextPtr) == before) {
        retval = FALSE;
    }

    return retval;
}
#endif // _WIN32

VOID CGSCset::gClose()
{
	LPGSCArchList	lpArchList1, lpArchList2;

	lpArchList2 = m_ArchList;

	while (lpArchList2)
	{
		lpArchList1 = lpArchList2;
		lpArchList2 = lpArchList1->m_NextArch;

		lpArchList1->m_Arch->Close();
		delete lpArchList1->m_Arch;

		delete lpArchList1;
	};

}

DWORD CGSCset::gFileSize( LPGSCfile gFile )
{
	if (gFile->m_Flags)//m_Flags = 1 means it's in an archive
	{
		return gFile->m_Arch->GetFileSize( gFile );
	}
	else
	{
#ifdef _WIN32	
		return GetFileSize( HANDLE( gFile->m_FileHandle ), NULL );
#endif
	};
}

LPBYTE CGSCset::gMapFile( LPGSCfile gFile )
{
	if (gFile->m_Flags)
	{
		return gFile->m_Arch->GetFileData( gFile );
	}
	else
	{
		return NULL;
	};
}

BOOL CGSCset::gWriteFile( LPGSCfile gFile, LPBYTE lpbBuffer, DWORD dwSize )
{
#ifdef _WIN32	
	DWORD dwBytesWritten;
	if (gFile->m_Flags)
	{
		return FALSE;
	}
	else
	{
		WriteFile( HANDLE( gFile->m_FileHandle ), lpbBuffer, dwSize, &dwBytesWritten, NULL );
		return TRUE;
	};
#endif
}

LPGSCfile CGSCset::gWriteOpen( LPCSTR lpcsFileName )
{
	LPGSCfile gFile;
#ifdef _WIN32
	gFile = new TGSCfile;
	gFile->m_Flags = 0;
	gFile->m_Position = 0;
	gFile->m_Arch = NULL;

	gFile->m_FileHandle = DWORD( CreateFile( lpcsFileName,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		0 ) );
	if (gFile->m_FileHandle == (DWORD) INVALID_HANDLE_VALUE)
	{
		delete gFile;
		return NULL;
	};
#endif
	return gFile;
}
