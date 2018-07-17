project(RoR_Viewer)

# find wxWidgets
include_directories(${WXWIDGETS_SHARED_INCLUDE_DIRS})
link_directories   (${WXWIDGETS_SHARED_LIBRARY_DIRS})

IF(WIN32)
 include_directories(${BOOST_INCLUDE_DIRS})
 link_directories   (${BOOST_LIBRARY_DIRS})
 include_directories(${PThread_INCLUDE_DIRS})
 link_directories   (${PThread_LIBRARY_DIRS})
 include_directories(${PThread_INCLUDE_DIRS})
 link_directories   (${PThread_LIBRARY_DIRS})

 #set(OS_LIBS "${PThread_LIBRARIES}")
 set(OS_SOURCE "icon.rc")

 # disable some annoying VS warnings:
 # warning C4244: 'initializing' : conversion from 'const float' to 'int', possible loss of data
 # warning C4305: 'initializing' : truncation from 'double' to 'const float'
 add_definitions("/wd4305 /wd4244")

ELSEIF(UNIX)
 find_package(PkgConfig)
 PKG_CHECK_MODULES  (GTK gtk+-2.0 REQUIRED)
 INCLUDE_DIRECTORIES(${GTK_INCLUDE_DIRS})

 PKG_CHECK_MODULES  (Ogre OGRE REQUIRED)
 PKG_CHECK_MODULES  (Ois OIS REQUIRED)

 # find wxwidgets
 SET(wxWidgets_USE_UNICODE ON)
 FIND_PACKAGE(wxWidgets REQUIRED COMPONENTS base core html net adv propgrid aui)
 include_directories(${wxWidgets_INCLUDE_DIRS})
 add_definitions("-D${wxWidgets_DEFINITIONS}" ${wxWidgets_CXX_FLAGS})
 link_directories (${wxWidgets_LIB_DIR})
 set(OS_LIBS "${OS_LIBS};${wxWidgets_LIBRARIES}")

 set(OS_SOURCE "")
ENDIF(WIN32)

include_directories(${Ogre_INCLUDE_DIRS})
link_directories   (${Ogre_LIBRARY_DIRS})
include_directories(${Ogre_Terrain_INCLUDE_DIRS})
link_directories   (${Ogre_Terrain_LIBRARY_DIRS})
include_directories(${Ogre_Paging_INCLUDE_DIRS})
link_directories   (${Ogre_Paging_LIBRARY_DIRS})
include_directories(${Ogre_RTShader_INCLUDE_DIRS})
link_directories   (${Ogre_RTShader_LIBRARY_DIRS})
include_directories(${Ois_INCLUDE_DIRS})
link_directories   (${Ois_LIBRARY_DIRS})

add_definitions("-D_UNICODE -DNOLANG")
include_directories(../main)

if(WIN32)
  # we have to extend the cflags to add USE_CONSOLE definition for non release modes
  set(CMAKE_CXX_FLAGS_MINSIZEREL     "${CMAKE_CXX_FLAGS_MINSIZEREL}     /D USE_CONSOLE")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /D USE_CONSOLE")
  set(CMAKE_CXX_FLAGS_DEBUG          "${CMAKE_CXX_FLAGS_DEBUG}          /D USE_CONSOLE")
endif(WIN32)

#setup libraries
macro(setup_lib name)
   if(ROR_USE_${name})
      message(STATUS "Enabled:     ${name}")
      include_directories(${${name}_INCLUDE_DIRS})
      link_directories   (${${name}_LIBRARY_DIRS})
      add_definitions("-DUSE_${name}")
      set(optional_libs ${optional_libs} ${${name}_LIBRARIES})
      message(STATUS "${name}_INCLUDE_DIRS:     ${${name}_INCLUDE_DIRS}")
      message(STATUS "${name}_LIBRARY_DIRS:     ${${name}_LIBRARY_DIRS}")
      message(STATUS "${name}_LIBRARIES:     ${${name}_LIBRARIES}")
   else()
      message(STATUS "Not enabled: ${name}")
   endif(ROR_USE_${name})
endmacro(setup_lib)

# config below
IF(WIN32)
   include_directories(${PThread_INCLUDE_DIRS})
   link_directories   (${PThread_LIBRARY_DIRS})
   set(OS_LIBS "${PThread_LIBRARIES};dinput8.lib;dxguid.lib")

   # disable some annoying VS warnings:
   # warning C4244: 'initializing' : conversion from 'const float' to 'int', possible loss of data
   # warning C4305: 'initializing' : truncation from 'double' to 'const float'
   add_definitions("/wd4305 /wd4244 -DNOMINMAX")
ELSEIF(UNIX)
   include_directories(${GTK_INCLUDE_DIRS})
   set(OS_LIBS "${OS_LIBS};X11")
ENDIF(WIN32)

# required components
include_directories(${Ogre_INCLUDE_DIRS})
link_directories   (${Ogre_LIBRARY_DIRS})
include_directories(${Ois_INCLUDE_DIRS})
link_directories   (${Ois_LIBRARY_DIRS})
link_directories   (${Ois_LIBRARY_DIRS})
include_directories(${RoR_SOURCE_DIR}/source/protocol)


# things to be used but not yet supported:
#setup_lib(OPENAL)
#setup_lib(LUA)
#setup_lib(PAGED)
#setup_lib(CAELUM)
#setup_lib(ANGELSCRIPT)
#setup_lib(HYDRAX)
#setup_lib(CRASHRPT)



FILE(GLOB viewer_sources "*.cpp")
FILE(GLOB viewer_headers "*.h")

include_directories(.)
include_directories(../main)

add_executable(RoRViewer ${viewer_sources} ${viewer_headers} ${OS_SOURCE} ${additional_sources} ${HEADER_FILES} ${SOURCE_FILES} ../main/Settings.cpp ../main/errorutils.cpp)

TARGET_LINK_LIBRARIES(RoRViewer ${WXWIDGETS_SHARED_LIBRARIES} ${Ogre_LIBRARIES} ${Ois_LIBRARIES} ${OS_LIBS} ${optional_libs})


