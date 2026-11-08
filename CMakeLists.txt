cmake_minimum_required(VERSION 3.16)
project(cossacks)

# set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# This assumes the SDL source is available in vendored/SDL
# and SDL_image in vendored/SDL_image
add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)
add_subdirectory(vendored/SDL_image EXCLUDE_FROM_ALL)

add_compile_options(-Wno-write-strings) # Turn off warnings about string literals for now
add_compile_options(-fpermissive) # Allow more casts for now
add_compile_options(-w) # Inhibit all warnings for now 

add_executable(cossacks
    src/game/main.cpp
    src/game/ddini.cpp
    src/game/activescenary.cpp
    src/game/interface.cpp
    src/game/bmptool.cpp
    src/game/timing.cpp
	src/game/selprop.cpp
	src/game/fastdraw.cpp
	src/game/resfile.cpp
	src/game/arc/gscset.cpp
	src/game/arc/gscarch.cpp
	src/game/gp_draw.cpp
	src/game/virtscr.cpp
	src/game/gamesound.cpp
	src/game/loadsave.cpp
	src/game/3drandmap.cpp
	src/game/multi.cpp
	src/game/mouse_x.cpp
	src/game/newcode/securedivision.cpp
	src/game/newcode/os.cpp
	src/game/newcode/queuemanagement.cpp
	src/game/newcode/mapoptionsencoder.cpp
	src/game/mapa.cpp
	src/game/select.cpp
	src/game/dialogs.cpp
	src/game/newmon.cpp
	src/game/arc/isimasks.cpp
	src/game/mouszone.cpp
	src/game/exconst.cpp
	src/game/gtext.cpp
	src/game/strecog.cpp
	src/game/fog.cpp
	src/game/masks.cpp
	src/game/drawforms.cpp
	src/game/weapon.cpp
	src/game/3dbars.cpp
	src/game/lines.cpp
	src/game/nature.cpp
	src/game/cdirsnd.cpp
	src/game/cwave.cpp
	src/game/devicecd.cpp
	src/game/newcode/stb_vorbis.c
	src/game/gamesound.cpp
	src/game/multi.cpp
	src/game/unsyncro.cpp
	src/game/recorder.cpp
	src/game/realwater.cpp
	src/game/river.cpp
	src/game/savenewmap.cpp
	src/game/gvclass.cpp
	src/game/report.cpp
	src/game/ir_main.cpp

	src/game/mplayer.cpp

	"src/CommCore library/CommCore.cpp"
	"src/CommCore library/CommInet.cpp"
	"src/CommCore library/CommInit.cpp"
	"src/CommCore library/CommPeers.cpp"
	"src/CommCore library/CommQueue.cpp"
	"src/CommCore library/CommRaw.cpp"
	"src/CommCore library/CommRecv.cpp"
	"src/CommCore library/CommServerList.cpp"

	src/game/conststr.cpp

	src/game/stringshash.cpp
	src/game/graphs.cpp
	src/game/fractal.cpp
	src/game/archtool.cpp
	src/game/nation.cpp
	src/game/mapsprites.cpp
	src/game/menu.cpp
	src/game/walls.cpp
	src/game/waves.cpp
	src/game/hint.cpp
	src/game/economy.cpp
	src/game/groups.cpp
	src/game/motion.cpp
	src/game/path.cpp
	src/game/navorots.cpp
	src/game/newai.cpp
	src/game/zbuffer.cpp
	src/game/brigade.cpp
	src/game/build.cpp
	src/game/danger.cpp
	src/game/activezone.cpp
	src/game/upgrade.cpp
	src/game/newupgrade.cpp
	src/game/nuclear.cpp
	src/game/onwater.cpp
	src/game/transport.cpp
	src/game/mine.cpp
	src/game/3dmaped.cpp
	src/game/3dsurf.cpp
	src/game/megapolis.cpp
	src/game/safenet.cpp
	src/game/icontool.cpp
	src/game/topograf.cpp
	src/game/strategyresearch.cpp
	src/game/einfoclass.cpp
	src/game/3dgraph.cpp
	src/game/shiptrace.cpp
)

# Link to the actual SDL3 libraries.
target_link_libraries(cossacks PRIVATE SDL3_image::SDL3_image SDL3::SDL3)

# Export the game's extern "C" AI-API functions (activescenary.cpp) into the
# dynamic symbol table, so AI modules loaded with dlopen can resolve them
# via dlsym(RTLD_DEFAULT) - the Linux equivalent of the AI DLLs calling
# GetProcAddress on the game executable.
target_link_options(cossacks PRIVATE "-rdynamic")

###################################################
#         Nation AI modules                       #
###################################################

# The game dlopen()s it via
# LoadAIFromDLL; on failure-free data the name comes from the text tables
# (e.g. "AI\Algeria.dll").
# The game data lives in ./gamedata (main.cpp chdir()s there at startup,
# see the --gamedata option) and dlopen()s the module as a loose file
# "AI/<Nation>.so" - dlopen cannot read files packed inside all.gsc
# (see docs/PortingNotes.md).
# So emit the modules straight into the game data dir's AI/ folder, next
# to the original Windows AI/<Nation>.dll set, instead of the build tree.
# OUTPUT_NAMEs must be byte-exact with the names in TEXT\AI.TXT (packed
# in all.gsc): the game dlopens "AI\\<name>.dll" -> "<name>.so" and Linux
# is case-sensitive (POLAND.dll, SPAIN.dll, SVEDEN.dll, HOLLAND.dll,
# SAKSINIA.dll and VENECIA.dll are uppercase there; Hungary stays
# "Vengria.dll" - verify EVERY name against that file, do not guess).
set(AI_MODULES_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/gamedata/AI")

add_library(Algeria MODULE src/ai/Algeria/dllmain.cpp)
set_target_properties(Algeria PROPERTIES PREFIX "" OUTPUT_NAME "Algeria"
    LIBRARY_OUTPUT_DIRECTORY "${AI_MODULES_OUTPUT_DIR}")
# Hidden by default: keeps the module's same-named function-pointer globals
# (typedefs.h) from being interposed by the game's exported symbols. The three
# game-facing exports (InitAI/ProcessAI/OnInit) override this explicitly.
target_compile_options(Algeria PRIVATE "-fvisibility=hidden")

add_library(Ukraine MODULE src/ai/Ukraine/dllmain.cpp)
set_target_properties(Ukraine PROPERTIES PREFIX "" OUTPUT_NAME "Ukraine"
    LIBRARY_OUTPUT_DIRECTORY "${AI_MODULES_OUTPUT_DIR}")
target_compile_options(Ukraine PRIVATE "-fvisibility=hidden")

add_library(Austria MODULE src/ai/Austria/dllmain.cpp)
set_target_properties(Austria PROPERTIES PREFIX "" OUTPUT_NAME "Austria"
    LIBRARY_OUTPUT_DIRECTORY "${AI_MODULES_OUTPUT_DIR}")
target_compile_options(Austria PRIVATE "-fvisibility=hidden")

add_library(Bavaria MODULE src/ai/Bavaria/dllmain.cpp)
set_target_properties(Bavaria PROPERTIES PREFIX "" OUTPUT_NAME "Bavaria"
    LIBRARY_OUTPUT_DIRECTORY "${AI_MODULES_OUTPUT_DIR}")
target_compile_options(Bavaria PRIVATE "-fvisibility=hidden")

add_library(England MODULE src/ai/England/dllmain.cpp)
set_target_properties(England PROPERTIES PREFIX "" OUTPUT_NAME "England"
    LIBRARY_OUTPUT_DIRECTORY "${AI_MODULES_OUTPUT_DIR}")
target_compile_options(England PRIVATE "-fvisibility=hidden")

add_library(France MODULE src/ai/France/dllmain.cpp)
set_target_properties(France PROPERTIES PREFIX "" OUTPUT_NAME "France"
    LIBRARY_OUTPUT_DIRECTORY "${AI_MODULES_OUTPUT_DIR}")
target_compile_options(France PRIVATE "-fvisibility=hidden")

add_library(Prussia MODULE src/ai/Prussia/dllmain.cpp)
set_target_properties(Prussia PROPERTIES PREFIX "" OUTPUT_NAME "German"
    LIBRARY_OUTPUT_DIRECTORY "${AI_MODULES_OUTPUT_DIR}")
target_compile_options(Prussia PRIVATE "-fvisibility=hidden")

add_library(Holland MODULE src/ai/Holland/dllmain.cpp)
set_target_properties(Holland PROPERTIES PREFIX "" OUTPUT_NAME "HOLLAND"
    LIBRARY_OUTPUT_DIRECTORY "${AI_MODULES_OUTPUT_DIR}")
target_compile_options(Holland PRIVATE "-fvisibility=hidden")

add_library(Piemont MODULE src/ai/Piemont/dllmain.cpp)
set_target_properties(Piemont PROPERTIES PREFIX "" OUTPUT_NAME "Piemont"
    LIBRARY_OUTPUT_DIRECTORY "${AI_MODULES_OUTPUT_DIR}")
target_compile_options(Piemont PRIVATE "-fvisibility=hidden")

add_library(Poland MODULE src/ai/Poland/dllmain.cpp)
set_target_properties(Poland PROPERTIES PREFIX "" OUTPUT_NAME "POLAND"
    LIBRARY_OUTPUT_DIRECTORY "${AI_MODULES_OUTPUT_DIR}")
target_compile_options(Poland PRIVATE "-fvisibility=hidden")

add_library(Portugal MODULE src/ai/Portugal/dllmain.cpp)
set_target_properties(Portugal PROPERTIES PREFIX "" OUTPUT_NAME "Portugalia"
    LIBRARY_OUTPUT_DIRECTORY "${AI_MODULES_OUTPUT_DIR}")
target_compile_options(Portugal PRIVATE "-fvisibility=hidden")

add_library(Saxony MODULE src/ai/Saxony/dllmain.cpp)
set_target_properties(Saxony PROPERTIES PREFIX "" OUTPUT_NAME "SAKSINIA"
    LIBRARY_OUTPUT_DIRECTORY "${AI_MODULES_OUTPUT_DIR}")
target_compile_options(Saxony PRIVATE "-fvisibility=hidden")

add_library(Spain MODULE src/ai/Spain/dllmain.cpp)
set_target_properties(Spain PROPERTIES PREFIX "" OUTPUT_NAME "SPAIN"
    LIBRARY_OUTPUT_DIRECTORY "${AI_MODULES_OUTPUT_DIR}")
target_compile_options(Spain PRIVATE "-fvisibility=hidden")

add_library(Sweden MODULE src/ai/Sweden/dllmain.cpp)
set_target_properties(Sweden PROPERTIES PREFIX "" OUTPUT_NAME "SVEDEN"
    LIBRARY_OUTPUT_DIRECTORY "${AI_MODULES_OUTPUT_DIR}")
target_compile_options(Sweden PRIVATE "-fvisibility=hidden")

add_library(Venice MODULE src/ai/Venice/dllmain.cpp)
set_target_properties(Venice PROPERTIES PREFIX "" OUTPUT_NAME "VENECIA"
    LIBRARY_OUTPUT_DIRECTORY "${AI_MODULES_OUTPUT_DIR}")
target_compile_options(Venice PRIVATE "-fvisibility=hidden")

add_library(Hungary MODULE src/ai/Hungary/dllmain.cpp)
set_target_properties(Hungary PROPERTIES PREFIX "" OUTPUT_NAME "Vengria"
    LIBRARY_OUTPUT_DIRECTORY "${AI_MODULES_OUTPUT_DIR}")
target_compile_options(Hungary PRIVATE "-fvisibility=hidden")

add_library(Switzerland MODULE src/ai/Switzerland/dllmain.cpp)
set_target_properties(Switzerland PROPERTIES PREFIX "" OUTPUT_NAME "Switzerland"
    LIBRARY_OUTPUT_DIRECTORY "${AI_MODULES_OUTPUT_DIR}")
target_compile_options(Switzerland PRIVATE "-fvisibility=hidden")

add_library(Turkey MODULE src/ai/Turkey/dllmain.cpp)
set_target_properties(Turkey PROPERTIES PREFIX "" OUTPUT_NAME "Turcia"
    LIBRARY_OUTPUT_DIRECTORY "${AI_MODULES_OUTPUT_DIR}")
target_compile_options(Turkey PRIVATE "-fvisibility=hidden")

add_library(Russia MODULE src/ai/Russia/dllmain.cpp)
set_target_properties(Russia PROPERTIES PREFIX "" OUTPUT_NAME "Russia"
    LIBRARY_OUTPUT_DIRECTORY "${AI_MODULES_OUTPUT_DIR}")
target_compile_options(Russia PRIVATE "-fvisibility=hidden")

# European-family AI (49152-byte DLL, handlers identical to Bavaria's).
# TEXT\AI.TXT asks for AI\Denmark.dll - mixed case, keep it exact.
add_library(Denmark MODULE src/ai/Denmark/dllmain.cpp)
set_target_properties(Denmark PROPERTIES PREFIX "" OUTPUT_NAME "Denmark"
    LIBRARY_OUTPUT_DIRECTORY "${AI_MODULES_OUTPUT_DIR}")
target_compile_options(Denmark PRIVATE "-fvisibility=hidden")


# ---------------------------------------------------------------------------
# Ported mission modules. The original Missions/<Group>/<Name>/Mission.dll
# files import the scenary API from dmcr.exe and export OnInit +
# ProcessScenary; the game LoadLibrary()s "Missions\<Group>\<Name>\Mission.dll"
# and the os.cpp shim retries it as .so (see docs/missions.md). The engine
# exports the whole scenary API via -rdynamic, so the modules call it
# directly - no dlsym scaffolding needed.
function(add_mission MISSION_PATH)
    get_filename_component(mission_name "${MISSION_PATH}" NAME)
    add_library("Mission_${mission_name}" MODULE
        "src/missions/${MISSION_PATH}/mission.cpp")
    set_target_properties("Mission_${mission_name}" PROPERTIES
        PREFIX "" OUTPUT_NAME "Mission"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/gamedata/Missions/${MISSION_PATH}")
    target_compile_options("Mission_${mission_name}" PRIVATE "-fvisibility=hidden")
endfunction()

add_mission(Tutorial/Tutorial1)
add_mission(Singles/Single59)
add_mission(Singles/Single1)
add_mission(Singles/Single2)


# And Boost
find_package(Boost REQUIRED COMPONENTS context filesystem iostreams)
target_link_libraries(cossacks PRIVATE Boost::context Boost::filesystem Boost::iostreams)

# Small SDL3 utility to browse GSC resource archives.
# Links the real game sprite renderer (gp_draw/fastdraw/fog/resfile) so GP
# previews are produced by the exact code paths the game uses on Linux.
add_executable(gscbrowser
    tools/gscbrowser.cpp
    src/game/arc/gscarch.cpp
    src/game/arc/gscset.cpp
    src/game/arc/isimasks.cpp
    src/game/newcode/os.cpp
    src/game/resfile.cpp
    src/game/gtext.cpp
    src/game/exconst.cpp
    src/game/gp_draw.cpp
    src/game/fastdraw.cpp
    src/game/fog.cpp
    src/game/newcode/securedivision.cpp
)
target_link_libraries(gscbrowser PRIVATE SDL3::SDL3 Boost::iostreams Boost::filesystem)
