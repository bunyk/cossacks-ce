// Shared scaffolding for the per-nation AI modules (src/ai/<Nation>/).
// Every module's dllmain.cpp starts with:
//     #include <cstdio>
//     #include "../common/ai_common.h"
//     #include "objects.h"
// ai_common.h brings the Linux dlopen scaffolding, the full AI API
// declarations (typedefs.h, shared verbatim by all modules - the
// authoritative per-function reference), the resource/land-type
// defines and the function loader.

// Linux port: the game dlopen()s this module; game functions are resolved
// from the main executable (linked with -rdynamic) via dlsym(RTLD_DEFAULT).
// The module itself is compiled with -fvisibility=hidden so its own
// same-named globals (the function pointers in typedefs.h) are NOT
// interposed by the game's exported functions.
#include <dlfcn.h>
#include <unistd.h>
typedef void* HINSTANCE;
#define __declspec(x) __attribute__((visibility("default")))

struct GAMEOBJ;

// The per-nation module's own API-table declarations (identical file in
// every module; see the doc comments there for the semantics).
#include "typedefs.h"

#define LOAD_FUNCTION(x) x=(tp##x*)LoadFunction(#x)

// Resource ids for GetMoney/AI_Torg/SetUpgradeLock (engine order).
#define WOOD  0
#define GOLD  1
#define STONE 2
#define FOOD  3
#define IRON  4
#define COAL  5

// Map land types (GetLandType).
#define LT_LAND				0
#define LT_MEDITERRANEAN	1
#define LT_PENINSULAS		2
#define LT_ISLANDS			3
#define LT_CONTINENTS		4
#define LT_CONTINENT		5

HINSTANCE hLib = nullptr;

// Resolves one game API function by name; logs to stderr if missing.
inline void* LoadFunction(const char* Name)
{
	// The main executable exports its extern "C" AI-API functions
	// (activescenary.cpp) into the global symbol scope.
	void* fn = dlsym(RTLD_DEFAULT, Name);
	if (!fn)
	{
		fprintf(stderr, "AI module: game does not export function: %s\n", Name);
	};
	return fn;
};
