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
    src/game/ddex1.cpp
	src/game/gp_draw.cpp
)

# Link to the actual SDL3 libraries.
target_link_libraries(cossacks PRIVATE SDL3_image::SDL3_image SDL3::SDL3)
