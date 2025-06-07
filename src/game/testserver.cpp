#include "ddini.h"
#include "resfile.h"
#include "fastdraw.h"
#include "mgraph.h"
#include "mouse.h"
#include "menu.h"
#include "mapdiscr.h"
#include "multipl.h"
#include "fog.h"
#include "walls.h"
#include "nature.h"
#include <time.h>
#include "nucl.h"

#include "megapolis.h"
#include "dialogs.h"
#include "fonts.h"
#include "gsound.h"
#include "3dgraph.h"
#include "3dmaped.h"
#include "mapsprites.h"
#include <assert.h>
#include <math.h>
#include "newmon.h"
#include "icontool.h"
#include "gp_draw.h"
#include "3drandmap.h"
#include "activescenary.h"
#include "drawform.h"
#include "conststr.h"
#include <Process.h>
#include "recorder.h"
#include "gsinc.h"
#include "topograf.h"

#include "strategyresearch.h"
#include "safety.h"
#include "einfoclass.h"
#include "mode.h"

#pragma pack(1)

#include "ir.h"
#include "bmptool.h"

typedef void fnInitSXP();
typedef void fnRunSXP( int, char*, int x, int y, int x1, int y1 );
typedef void fnProcessSXP( int, DialogsSystem* );
typedef void fnSXP_Operation( int );
typedef char* fnGetAccessKey( int );
typedef void fnSetAccessKey( int, char* );
typedef void tpSXP_SetVar( int Index, char* Name, char* value );
typedef char* tpSXP_GetVar( int Index, char* Name );
typedef void tpOpenRef( int Index, char* home );
typedef void tpResizeSXP( int Index, int x, int y, int x1, int y1 );
typedef void tpStartDownloadInternetFile( char* Name, char* Server, char* DestName );
typedef void tpProcessDownloadInternetFiles();
typedef void tpSendRecBuffer( byte* Data, int size, bool Final );
extern bool KeyPressed;

HMODULE H_Exp = nullptr;
HMODULE H_ExpOld = nullptr;
fnInitSXP* InitSXP = nullptr;
fnRunSXP* RunSXP = nullptr;
fnProcessSXP* ProcessSXP = nullptr;
fnGetAccessKey* GetAccessKey = nullptr;
fnSetAccessKey* SetAccessKey = nullptr;
fnSXP_Operation* SXP_StepBack = nullptr;
fnSXP_Operation* SXP_StepForw = nullptr;
fnSXP_Operation* SXP_Refresh = nullptr;
tpSXP_SetVar* SXP_SetVar = nullptr;
tpSXP_GetVar* SXP_GetVar = nullptr;
tpOpenRef* OpenRef = nullptr;
tpResizeSXP* ResizeSXP = nullptr;
tpStartDownloadInternetFile* StartDownloadInternetFile = nullptr;
tpProcessDownloadInternetFiles* ProcessDownloadInternetFiles = nullptr;
tpSendRecBuffer* SendRecBuffer = nullptr;


DLLEXPORT void StartInternetExplorer()
{
	//attempt to use advanced version
	ResFile F = RReset( "Internet\\Cash\\IntExplorerNew.dll" );
	if (F != INVALID_HANDLE_VALUE)
	{
		int sz = RFileSize( F );
		void* buf = malloc( sz );
		RBlockRead( F, buf, sz );
		RClose( F );
		F = RRewrite( "IntExplorer.dll" );
		if (F != INVALID_HANDLE_VALUE)
		{
			RBlockWrite( F, buf, sz );
			RClose( F );
		}
		free( buf );
		DeleteFile( "Internet\\Cash\\IntExplorerNew.dll" );
	}

	H_Exp = LoadLibrary( "IntExplorer.dll" );

	if (H_Exp)
	{
		InitSXP = (fnInitSXP*) GetProcAddress( H_Exp, "InitSXP" );

		if (InitSXP)
		{
			InitSXP();
		}

		RunSXP = (fnRunSXP*) GetProcAddress( H_Exp, "RunSXP" );
		ProcessSXP = (fnProcessSXP*) GetProcAddress( H_Exp, "ProcessSXP" );
		GetAccessKey = (fnGetAccessKey*) GetProcAddress( H_Exp, "?GetAccessKey@@YAPADH@Z" );
		SetAccessKey = (fnSetAccessKey*) GetProcAddress( H_Exp, "?SetAccessKey@@YAXHPAD@Z" );
		SXP_StepBack = (fnSXP_Operation*) GetProcAddress( H_Exp, "SXP_StepBack" );
		SXP_StepForw = (fnSXP_Operation*) GetProcAddress( H_Exp, "SXP_StepForw" );
		SXP_Refresh = (fnSXP_Operation*) GetProcAddress( H_Exp, "SXP_Refresh" );
		SXP_SetVar = (tpSXP_SetVar*) GetProcAddress( H_Exp, "SXP_SetVar" );
		SXP_GetVar = (tpSXP_GetVar*) GetProcAddress( H_Exp, "SXP_GetVar" );
		OpenRef = (tpOpenRef*) GetProcAddress( H_Exp, "OpenRef" );
		ResizeSXP = (tpResizeSXP*) GetProcAddress( H_Exp, "ResizeSXP" );
		StartDownloadInternetFile = (tpStartDownloadInternetFile*) GetProcAddress( H_Exp, "?StartDownloadInternetFile@@YAXPAD00@Z" );
		ProcessDownloadInternetFiles = (tpProcessDownloadInternetFiles*) GetProcAddress( H_Exp, "?ProcessDownloadInternetFiles@@YAXXZ" );
		SendRecBuffer = (tpSendRecBuffer*) GetProcAddress( H_Exp, "?SendRecBuffer@@YAXPAEH_N@Z" );
	}
}

DLLEXPORT void FinishInternetExplorer()
{
	if (H_Exp)
	{
		FreeLibrary( H_Exp );

		H_Exp = nullptr;
		InitSXP = nullptr;
		RunSXP = nullptr;
		ProcessSXP = nullptr;

		if (H_ExpOld)
		{
			FreeLibrary( H_ExpOld );
		}
	}
}

DLLEXPORT void ExplorerBack( int Index )
{
	if (SXP_StepBack)
	{
		SXP_StepBack( Index );
	}
}

DLLEXPORTvoid ExplorerForw( int Index )
{
	if (SXP_StepBack)
	{
		SXP_StepForw( Index );
	}
}

DLLEXPORTvoid ExplorerRefresh( int Index )
{
	if (SXP_Refresh)
	{
		SXP_Refresh( Index );
	}
}

DLLEXPORT void RunExplorer( int Index, char* ref, int x, int y, int x1, int y1 )
{
	if (RunSXP)
	{
		RunSXP( Index, ref, x, y, x1, y1 );
	}
}

DLLEXPORTvoid ProcessExplorer( int Index )
{
	if (ProcessSXP)
	{
		ProcessSXP( Index, nullptr );
	}
}

DLLEXPORT void ProcessExplorerDSS( int Index, DialogsSystem* DSS )
{
	if (ProcessSXP)
	{
		ProcessSXP( Index, DSS );
	}
}

DLLEXPORT void ExplorerSetVar( int Index, char* Name, char* value )
{
	if (SXP_SetVar)
	{
		SXP_SetVar( Index, Name, value );
	}
}

DLLEXPORT char* ExplorerGetVar( int Index, char* Name )
{
	if (SXP_SetVar)
	{
		return SXP_GetVar( Index, Name );
	}
	else
	{
		return nullptr;
	}
}

DLLEXPORT void ExplorerOpenRef( int Index, char* ref )
{
	if (OpenRef)
	{
		OpenRef( Index, ref );
	}
}

DLLEXPORT void ExplorerResize( int Index, int x, int y, int x1, int y1 )
{
	if (ResizeSXP)
	{
		ResizeSXP( Index, x, y, x1, y1 );
	}
}

bool ProcessNewInternetLogin();

typedef void fnVoid();
typedef void fnVoidLPB( byte* );
typedef void fnVoidLPC( char* );
typedef void tpSaveAllDipData( byte** ptr, int* size );
typedef void tpPerformDipCommand( char* Data, int size );
typedef void tpLoadAllDipData( byte* ptr, int size );
typedef void tpStartDownloadInternetFile( char* Name, char* Server, char* DestName );
typedef void tpProcessDownloadInternetFiles();
typedef void tpSendRecBuffer( byte* Data, int size, bool Final );
