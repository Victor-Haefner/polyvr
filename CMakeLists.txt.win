cmake_minimum_required (VERSION 3.0)

project(PolyVR)

set(INCLUDE_DIRS "C:/opensg/include")
set(INCLUDE_DIRS ${INCLUDE_DIRS} "C:/freeglut")
set(INCLUDE_DIRS ${INCLUDE_DIRS} "C:/boost")
set(INCLUDE_DIRS ${INCLUDE_DIRS} "C:/lib/bullet/include")

set(LIB_DIRS "C:/opensg")
set(LIB_DIRS ${LIB_DIRS} "C:/freeglut")
set(LIB_DIRS ${LIB_DIRS} "C:/boost/lib64-msvc-14.1")
set(INCLUDE_DIRS ${INCLUDE_DIRS} "C:/lib/bullet/lib")

set(Boost_NO_BOOST_CMAKE TRUE CACHE INTERNAL "" FORCE)
set(CMAKE_CONFIGURATION_TYPES Debug Release) # Only generate Debug and Release configuration types.

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W4 -std=c++14 -showIncludes -DWIN32_LEAN_AND_MEAN -DLC_ALL=C -DLINUX -DOSG2 -D_GNU_SOURCE -DQT_CLEAN_NAMESPACE -DOSG_WITH_GLUT -DOSG_WITH_QT -DOSG_WITH_JPG -DOSG_WITH_PNG -DOSG_WITH_COLLADA -DOSG_ENABLE_OGL_VERTEXT_ATTRIB_FUNCS=ON -D_OSG_HAVE_CONFIGURED_H_ -DQT_NO_XINERAMA -DQT_NO_XRENDER -DQT_NO_XFTFREETYPE -DQT_NO_XKB -DQT_NO_SM_SUPPORT -DQT_NO_IMAGEIO_MNG -DQT_NO_IMAGEIO_JPEG -DQT_NO_STYLE_AQUA -DQT_NO_STYLE_MAC -DQT_NO_STYLE_INTERLACE -DQT_NO_STYLE_COMPACT -DOSG_BUILD_ACTIVE -DCRT_SECURE_NO_DEPRECATE -DCRT_SECURE_NO_WARNINGS -DCRT_NONSTDC_NO_DEPRECATE -DSECURE_SCL=0 -D_SCL_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_DEPRECATE -D__GCCXML__ -DCGAL_CFG_NO_CPP0X_ARRAY -DCGAL_CFG_NO_TR1_ARRAY -DCGAL_CFG_NO_VARIADIC_TEMPLATES -DNDEBUG -DCGAL_NDEBUG -D__STDC_CONSTANT_MACROS -D_GLIBCXX_USE_CXX11_ABI=1 -DBOOST_NO_COMPILER_CONFIG -DBOOST_NO_CXX14_DECLTYPE_AUTO -DBOOST_NO_CXX11_DECLTYPE_N3276 -DBOOST_NO_CXX11_HDR_ATOMIC=1 -DGL_GLEXT_PROTOTYPES -DWITH_SHADOW_VOLUME -DWITH_OPCUA -DWITH_CLUSTERING_FIX -DWITH_STEP -DCEF18 ")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W4 -std=c++14 -DWIN32_LEAN_AND_MEAN -DLC_ALL=C -DLINUX -DOSG2 -D_GNU_SOURCE -DQT_CLEAN_NAMESPACE -DOSG_WITH_GLUT -DOSG_WITH_QT -DOSG_WITH_JPG -DOSG_WITH_PNG -DOSG_WITH_COLLADA -DOSG_ENABLE_OGL_VERTEXT_ATTRIB_FUNCS=ON -D_OSG_HAVE_CONFIGURED_H_ -DQT_NO_XINERAMA -DQT_NO_XRENDER -DQT_NO_XFTFREETYPE -DQT_NO_XKB -DQT_NO_SM_SUPPORT -DQT_NO_IMAGEIO_MNG -DQT_NO_IMAGEIO_JPEG -DQT_NO_STYLE_AQUA -DQT_NO_STYLE_MAC -DQT_NO_STYLE_INTERLACE -DQT_NO_STYLE_COMPACT -DOSG_BUILD_ACTIVE -DCRT_SECURE_NO_DEPRECATE -DCRT_SECURE_NO_WARNINGS -DCRT_NONSTDC_NO_DEPRECATE -DSECURE_SCL=0 -D_SCL_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_DEPRECATE -D__GCCXML__ -DCGAL_CFG_NO_CPP0X_ARRAY -DCGAL_CFG_NO_TR1_ARRAY -DCGAL_CFG_NO_VARIADIC_TEMPLATES -DNDEBUG -DCGAL_NDEBUG -D__STDC_CONSTANT_MACROS -D_GLIBCXX_USE_CXX11_ABI=1 -DGL_GLEXT_PROTOTYPES -DWITH_SHADOW_VOLUME -DWITH_OPCUA -DWITH_CLUSTERING_FIX -DWITH_STEP -DCEF18 ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W4 -std=c++14 -DWIN32_LEAN_AND_MEAN")


set(SOURCES src/main.cpp)
set(SOURCES ${SOURCES} src/PolyVR.cpp)

include_directories(${INCLUDE_DIRS})
link_directories(${LIB_DIRS})
add_executable(PolyVR ${SOURCES})





# CEF
set_property(GLOBAL PROPERTY OS_FOLDERS ON) # Use folders in the resulting project files.
set(CEF_ROOT "c:/lib/cef") # Specify the CEF binary distribution root directory.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CEF_ROOT}/cmake")
find_package(CEF REQUIRED)
add_subdirectory(${CEF_LIBCEF_DLL_WRAPPER_PATH} libcef_dll_wrapper) # Include the libcef_dll_wrapper target.
PRINT_CEF_CONFIG() # Display CEF configuration settings.

# maybe needed for CEF, uncomment if necessary
#add_executable(${CEF_TARGET} ${CEFSIMPLE_SRCS})
#SET_EXECUTABLE_TARGET_PROPERTIES(${CEF_TARGET})  # new line
#add_dependencies(${CEF_TARGET} libcef_dll_wrapper)
#target_link_libraries(${CEF_TARGET} libcef_lib libcef_dll_wrapper ${CEF_STANDARD_LIBS})

# Bullet
set(BULLET_ROOT "c:/lib/bullet") # Specify the CEF binary distribution root directory.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${BULLET_ROOT}/cmake")
find_package(Bullet REQUIRED)







target_link_libraries(PolyVR OSGBase )
target_link_libraries(PolyVR OSGContribTrapezoidalShadowMaps )
target_link_libraries(PolyVR OSGCluster )
target_link_libraries(PolyVR OSGDrawable )
target_link_libraries(PolyVR OSGEffectGroups )
target_link_libraries(PolyVR OSGGroup )
target_link_libraries(PolyVR OSGImageFileIO )
target_link_libraries(PolyVR OSGState )
target_link_libraries(PolyVR OSGSystem )
target_link_libraries(PolyVR OSGUtil )
target_link_libraries(PolyVR OSGText )
target_link_libraries(PolyVR OSGWindow )
target_link_libraries(PolyVR OSGWindowGLUT )
#target_link_libraries(PolyVR OSGWindowX )
#target_link_libraries(PolyVR GLU )
#target_link_libraries(PolyVR GL )
#target_link_libraries(PolyVR SM )
#target_link_libraries(PolyVR ICE )
#target_link_libraries(PolyVR X11 )
#target_link_libraries(PolyVR Xext )
target_link_libraries(PolyVR freeglut )
#target_link_libraries(PolyVR Xmu )
#target_link_libraries(PolyVR Xi)
target_link_libraries(PolyVR boost_system-vc141-mt-1_65_1)