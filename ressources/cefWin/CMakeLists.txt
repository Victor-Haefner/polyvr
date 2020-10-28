cmake_minimum_required (VERSION 3.0)

#set( CMAKE_VERBOSE_MAKEFILE on )

project(CefSubProcessWin)

message( "Configuring for system name / version: " ${CMAKE_SYSTEM_NAME} " / " ${CMAKE_SYSTEM_VERSION} )

add_definitions( -DWIN32_LEAN_AND_MEAN=1 )
add_definitions( -D__USE_MISC=1 )
add_definitions( -DHAVE_SNPRINTF )

set(INCLUDE_DIRS "D:/usr/include/CEF")
set(INCLUDE_DIRS ${INCLUDE_DIRS} "C:/usr/include/CEF")
set(LIB_DIRS "D:/usr/lib/cef")
set(LIB_DIRS ${LIB_DIRS} "C:/usr/lib/cef")
set(LINKER_FLAGS libcef_dll_wrapper shared libcef)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(INCLUDE_DIRS ${INCLUDE_DIRS} "src")
include_directories(${INCLUDE_DIRS})
link_directories(${LIB_DIRS})

add_executable(CefSubProcessWin WKSubProcess.cpp)

target_compile_options(CefSubProcessWin PRIVATE ${COMPILER_FLAGS})
target_link_libraries(CefSubProcessWin PRIVATE ${LINKER_FLAGS})







