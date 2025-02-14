cmake_minimum_required(VERSION 3.1.0)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ./build)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ./bin)

project(fileparser VERSION 0.0.1)
set(CMAKE_CXX_STANDARD 23)

include_directories(include)
include_directories(dependencies/glew-2.1.0/include)
include_directories(dependencies/GLFW/include)
include_directories(dependencies/simde/include)

add_library(glew STATIC IMPORTED)
add_library(glfw3 STATIC IMPORTED)

add_library(files 
    src/FileTypes/Bmp.cpp 
    src/FileTypes/Jpg.cpp
    src/BitManipulationUtil.cpp
    src/Color.cpp
    src/ShaderUtil.cpp)

add_executable(FileParser src/FileParser.cpp)

target_compile_definitions(FileParser PRIVATE GLEW_STATIC)

IF(WIN32)
    set_target_properties(glew PROPERTIES IMPORTED_LOCATION "./dependencies/glew-2.1.0/lib/Windows/Win32/glew32s.lib")
    set_target_properties(glfw3 PROPERTIES IMPORTED_LOCATION "./dependencies/GLFW/lib/Windows/glfw3.lib")
    target_link_libraries(FileParser PRIVATE opengl32 files glew glfw3)
ELSE()
    set_target_properties(glew PROPERTIES IMPORTED_LOCATION "./dependencies/glew-2.1.0/lib/Linux/libGLEW.a")
    set_target_properties(glfw3 PROPERTIES IMPORTED_LOCATION "./dependencies/GLFW/lib/Linux/libglfw3.a")
    target_link_libraries(FileParser PRIVATE GL files glew glfw3)
ENDIF()

target_compile_options(FileParser PRIVATE -Wall -Wpedantic)
