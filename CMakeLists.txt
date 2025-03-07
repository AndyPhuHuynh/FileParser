cmake_minimum_required(VERSION 3.1.0)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

project(FileParser VERSION 0.0.1)
set(CMAKE_CXX_STANDARD 23)

add_library(glew STATIC IMPORTED)
add_library(glfw3 STATIC IMPORTED)

file(GLOB_RECURSE INCLUDES 
    ${CMAKE_SOURCE_DIR}/include/*.cpp
    ${CMAKE_SOURCE_DIR}/include/*.h)

file(GLOB_RECURSE SOURCES 
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/src/*.h)

add_executable(FileParser ${SOURCES} ${INCLUDES})

target_include_directories(FileParser PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/dependencies/glew-2.1.0/include
    ${CMAKE_SOURCE_DIR}/dependencies/GLFW/include
    ${CMAKE_SOURCE_DIR}/dependencies/glm/include
    ${CMAKE_SOURCE_DIR}/dependencies/simde/include)

add_compile_definitions(GLEW_STATIC)

IF(WIN32)
    set_target_properties(glew PROPERTIES IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/dependencies/glew-2.1.0/lib/Windows/x64/glew32s.lib")
    set_target_properties(glfw3 PROPERTIES IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/dependencies/GLFW/lib/Windows/x64/glfw3.lib")
    target_link_libraries(FileParser PRIVATE opengl32 glew glfw3)
    target_compile_options(FileParser PRIVATE /W4)
ELSE()
    set_target_properties(glew PROPERTIES IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/dependencies/glew-2.1.0/lib/Linux/libGLEW.a")
    set_target_properties(glfw3 PROPERTIES IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/dependencies/GLFW/lib/Linux/libglfw3.a")
    target_link_libraries(FileParser PRIVATE GL glew glfw3)
    target_compile_options(FileParser PRIVATE -Wall -Wpedantic)
ENDIF()

