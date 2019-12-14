cmake_minimum_required (VERSION 3.0)

project(PolyVR)

message( "Configuring for system name / version: " ${CMAKE_SYSTEM_NAME} " / " ${CMAKE_SYSTEM_VERSION} )

function(FIND_BOOST)
	if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
		set(BOOST_INCLUDE_DIR "-s USE_BOOST_HEADERS=1")
		set(BOOST_LIBRARY_DIR "../lib")
		set(BOOST_LIBRARIES "-lboost_filesystem -lboost_system")
	endif()
	include_directories(${BOOST_INCLUDE_DIR})
endfunction()

function(FIND_PYTHON)
	if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
		set(PYTHON_INCLUDE_DIR "../include/Python")
		set(PYTHON_LIBRARY_DIR "../lib")
		set(PYTHON_LIBRARIES "-lpython2.7")
	endif()
	include_directories(${PYTHON_INCLUDE_DIR})
endfunction()

function(FIND_GLUT)
	if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
		set(GLUT_INCLUDE_DIR "")
		set(GLUT_LIBRARY_DIR "")
		set(GLUT_LIBRARIES "-lGL -lglut")
	endif()
	include_directories(${GLUT_INCLUDE_DIR})
endfunction()

function(FIND_OPENSG)
	if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
		set(OPENSG_INCLUDE_DIR "../include")
		set(OPENSG_LIBRARY_DIR "../lib")
		set(OPENSG_LIBRARIES "-lOSGWindowGLUT -lOSGWindowWASM -lOSGWindow -lOSGSystem -lOSGUtil -lOSGDrawable -lOSGGroup -lOSGState -lOSGBase")
	endif()
	include_directories(${OPENSG_INCLUDE_DIR})
endfunction()

FIND_BOOST()
FIND_PYTHON()
FIND_GLUT()
FIND_OPENSG()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(src)

add_executable(polyvr src/main.cpp)
