// #include <windows.h>
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#pragma pack(1)
#include "resfile.h"
#include "gfile.h"
#define LOADSCX

extern SDL_Window* sdlWindow;

char* GetTextByID(char* ID);
char* HiEdMode;//"Редактирование высот."
void LOADSC(char* ID,char** str){
	(*str)=GetTextByID(ID);
	if(!strcmp(*str,ID)){
		char cc[128];
		sprintf(cc,"Unknown string: %s (see COMMENT.TXT)",ID);
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "String not found...", cc, sdlWindow);
	};
};
// FIXME: something strange's happening here
#undef LoadSC
#define LoadSC(z) char* ##z##=NULL;
#include "conststr.h"
#undef LoadSC
#define LoadSC(x) LOADSC(#x,&##x);
void LoadConstStr(){
#include "conststr.h"
};
