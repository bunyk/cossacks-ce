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
	src/game/drawforms.cpp
	src/game/weapon.cpp
	src/game/3dbars.cpp
	src/game/lines.cpp
	src/game/nature.cpp
	src/game/cdirsnd.cpp
	src/game/cwave.cpp
	src/game/gamesound.cpp
	src/game/multi.cpp
	src/game/unsyncro.cpp
	src/game/recorder.cpp
	src/game/river.cpp
	src/game/savenewmap.cpp
	src/game/gvclass.cpp
	src/game/report.cpp
	src/game/linux_stubs.cpp

	src/game/conststr.cpp

	src/game/antibug.cpp
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
	# src/game/path.cpp  # TODO: heavy asm, needs porting
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
	src/game/ddex1.cpp
	src/game/3dmaped.cpp
	src/game/3dsurf.cpp
	src/game/megapolis.cpp
	src/game/icontool.cpp
	src/game/topograf.cpp
	src/game/strategyresearch.cpp
	src/game/einfoclass.cpp
	src/game/3dgraph.cpp
)

# Link to the actual SDL3 libraries.
target_link_libraries(cossacks PRIVATE SDL3_image::SDL3_image SDL3::SDL3)


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
)
target_link_libraries(gscbrowser PRIVATE SDL3::SDL3 Boost::iostreams Boost::filesystem)
