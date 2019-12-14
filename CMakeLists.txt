cmake_minimum_required (VERSION 3.0)

project(PolyVR)

message( "Configuring for system name / version: " ${CMAKE_SYSTEM_NAME} " / " ${CMAKE_SYSTEM_VERSION} )

#-s WASM=1 -s USE_ZLIB=1 -lz
#-s USE_BOOST_HEADERS=1
  
add_definitions( -DWASM=1 )
add_definitions( -DGLUT_glut_LIBRARY=1 )
add_definitions( -DGLUT_Xi_LIBRARY=1 )
add_definitions( -DGLUT_Xmu_LIBRARY=1 )
add_definitions( -DCMAKE_TOOLCHAIN_FILE=../emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake )
add_definitions( -DCMAKE_MODULE_PATH=../emsdk/upstream/emscripten/cmake/Modules )

function(FIND_BOOST)
	if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
		message( " -- found boost " )
		set(BOOST_INCLUDE_DIR "~/.emscripten_ports/boost_headers")
		set(BOOST_LIBRARY_DIR "../lib")
		#set(BOOST_LIBRARIES "-lboost_system -lboost_thread -lboost_program_options -lboost_serialization -lboost_filesystem -lboost_regex")
		set(BOOST_LIBRARIES "-lboost_system -lboost_program_options -lboost_serialization -lboost_filesystem")
		set(LINKER_FLAGS "${LINKER_FLAGS} ${BOOST_LIBRARIES}" PARENT_SCOPE)
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
		set(LINKER_FLAGS "${LINKER_FLAGS} ${OPENSG_LIBRARIES}" PARENT_SCOPE)
	endif()
	include_directories(${OPENSG_INCLUDE_DIR})
	link_directories(${OPENSG_LIBRARY_DIR})
endfunction()

function(SETUP_VARS)
	if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
		add_definitions( -DWITHOUT_BULLET )
		add_definitions( -DWITHOUT_GTK )
	endif()
endfunction()

FIND_BOOST()
FIND_PYTHON()
FIND_GLUT()
FIND_OPENSG()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

SETUP_VARS()
string(STRIP ${LINKER_FLAGS} LINKER_FLAGS)

include_directories(src)

add_executable(polyvr src/main.cpp)
target_sources(polyvr PRIVATE src/PolyVR.cpp)
target_sources(polyvr PRIVATE src/core/utils/VRGlobals.cpp)
target_compile_options(polyvr PRIVATE ${COMPILER_FLAGS})
target_link_libraries(polyvr PRIVATE ${LINKER_FLAGS})







