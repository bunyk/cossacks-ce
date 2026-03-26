// #include <windows.h>  // Commented out for cross-platform compatibility
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#pragma pack(1)
#include "resfile.h"
#include "gfile.h"
#define LOADSCX

extern SDL_Window* sdlWindow;

// Forward declaration - gets string by ID from message system
char* GetTextByID(char* ID);
char* HiEdMode; // Height editing mode

/**
 * Loads a string constant by ID using GetTextByID
 * Shows error message box if string is not found
 */
void LOADSC(char* ID,char** str){
	(*str)=GetTextByID(ID);
	if(!strcmp(*str,ID)){
		char cc[128];
		sprintf(cc,"Unknown string: %s (see COMMENT.TXT)",ID);
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "String not found...", cc, sdlWindow);
	};
};

// This is a clever but complex macro pattern that uses the same header file twice
// with different macro definitions to generate both variable declarations and function calls.

// FIRST PASS: Generate variable declarations
// Redefine LoadSC to create external char* variable declarations initialized to NULL
#undef LoadSC
#define LoadSC(z) char* z = NULL;
#include "conststr.h"  // This expands to: char* DELSYNC = NULL; char* STOPUPG = NULL; etc.
#undef LoadSC

// SECOND PASS: Generate function implementation
// Redefine LoadSC to create function calls that load each string variable
#define LoadSC(x) LOADSC(#x, &x);

/**
 * LoadConstStr - Loads all constant strings defined in conststr.h
 * 
 * This function uses a macro trick to iterate through all string identifiers
 * defined in conststr.h and loads each one using the LOADSC function.
 * 
 * How it works:
 * 1. conststr.h contains a list of LoadSC(identifier) calls
 * 2. When included the second time, LoadSC is defined as LOADSC(#x, &x)
 * 3. This expands to LOADSC("identifier", &identifier) for each string
 * 4. LOADSC then calls GetTextByID to load the actual string value
 * 
 * The conststr.h file contains identifiers for UI elements, messages, and other
 * text used throughout the game.
 */
void LoadConstStr(){
#include "conststr.h"  // This expands to: LOADSC("DELSYNC", &DELSYNC); LOADSC("STOPUPG", &STOPUPG); etc.
};
