cmake_minimum_required(VERSION 3.10)
project(BASIC_CPP)

set(CMAKE_CXX_STANDARD 17)
# Create an option to switch between a system sdl library and a vendored SDL library
# option(MYGAME_VENDORED "Use vendored libraries" ON)
# if(MYGAME_VENDORED)
#     # This assumes you have added SDL as a submodule in vendored/SDL
#     add_subdirectory(lib/sdl EXCLUDE_FROM_ALL)
# else()
#     # 1. Look for a SDL3 package,
#     # 2. look for the SDL3-shared component, and
#     # 3. fail if the shared component cannot be found.
#     find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)
# endif()

add_executable(noin src/main.cpp)

# find_package( OpenCV REQUIRED )
# include_directories( ${OpenCV_INCLUDE_DIRS} )
# target_link_libraries( noin ${OpenCV_LIBS} )

# add_executable( noin2 main2.cpp )



