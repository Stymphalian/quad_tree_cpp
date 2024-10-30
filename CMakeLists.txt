cmake_minimum_required(VERSION 3.10)
project(BASIC_CPP)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

set(SDL2_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/include/SDL2")
set(SDL2_LIB_DIR "${CMAKE_CURRENT_LIST_DIR}/lib/sdl")
include_directories(${SDL2_INCLUDE_DIRS})
link_directories(${SDL2_LIB_DIR})

add_executable(noin src/jquad.cpp src/metrics.cpp src/jint_list.cpp src/scene.cpp src/main.cpp)
target_link_libraries(noin SDL2)
add_custom_command(TARGET noin POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_if_different
                   "${CMAKE_CURRENT_LIST_DIR}/lib/sdl/SDL2.dll"
                   "$<TARGET_FILE_DIR:noin>/SDL2.dll")