project(RoR_Configurator)

#setup libraries
macro(setup_lib name)
   if(ROR_USE_${name})
      include_directories(${${name}_INCLUDE_DIRS})
      link_directories   (${${name}_LIBRARY_DIRS})
      add_definitions("-DUSE_${name}")
      set(optional_libs ${optional_libs} ${${name}_LIBRARIES})
      message(STATUS "${name} Enabled:      \tYES")
      message(STATUS "${name}_INCLUDE_DIRS: \t${${name}_INCLUDE_DIRS}")
      #if(${${name}_LIBRARY_DIRS})
      #   message(STATUS "${name}_LIBRARY_DIRS: \t${${name}_LIBRARY_DIRS}")
      #endif(${${name}_LIBRARY_DIRS})
      message(STATUS "${name}_LIBRARIES:    \t${${name}_LIBRARIES}")
   else()
      message(STATUS "${name} Enabled:      \tNO")
   endif(ROR_USE_${name})
endmacro(setup_lib)

IF(WIN32)
   include_directories(${BOOST_INCLUDE_DIRS})
   link_directories   (${BOOST_LIBRARY_DIRS})
   include_directories(${PThread_INCLUDE_DIRS})
   link_directories   (${PThread_LIBRARY_DIRS})
   include_directories(${WXWIDGETS_SHARED_INCLUDE_DIRS})
   link_directories   (${WXWIDGETS_SHARED_LIBRARY_DIRS})

   set(OS_LIBS "${PThread_LIBRARIES} dinput8.lib dxguid.lib")
   set(OS_SOURCE "icon.rc")

   # disable some annoying VS warnings:
   # warning C4244: 'initializing' : conversion from 'const float' to 'int', possible loss of data
   # warning C4305: 'initializing' : truncation from 'double' to 'const float'
   add_definitions("/wd4305 /wd4244")

   
   #now: delay loading for opencl
   if(ROR_USE_OPENCL)
      set(CMAKE_EXE_LINKER_FLAGS_DEBUG          "${CMAKE_EXE_LINKER_FLAGS_DEBUG}          /DELAYLOAD:OpenCL.dll")
      set(CMAKE_EXE_LINKER_FLAGS_RELEASE        "${CMAKE_EXE_LINKER_FLAGS_RELEASE}        /DELAYLOAD:OpenCL.dll")
      set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL     "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL}     /DELAYLOAD:OpenCL.dll")
      set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /DELAYLOAD:OpenCL.dll")
      set(OS_LIBS ${OS_LIBS} Delayimp.lib)
   endif(ROR_USE_OPENCL)

ELSEIF(UNIX)
   find_package(PkgConfig)
   PKG_CHECK_MODULES  (GTK gtk+-2.0 REQUIRED)
   INCLUDE_DIRECTORIES(${GTK_INCLUDE_DIRS})

   PKG_CHECK_MODULES  (Ogre OGRE REQUIRED)
   PKG_CHECK_MODULES  (Ois OIS REQUIRED)

   # find wxwidgets
   SET(wxWidgets_USE_UNICODE ON)
   FIND_PACKAGE(wxWidgets REQUIRED COMPONENTS base core html net adv)
   include_directories(${wxWidgets_INCLUDE_DIRS})
   add_definitions("-D${wxWidgets_DEFINITIONS}" ${wxWidgets_CXX_FLAGS})
   link_directories (${wxWidgets_LIB_DIR})

   set(OS_LIBS "${wxWidgets_LIBRARIES}")

   setup_lib(CRASHRPT)
ENDIF(WIN32)

setup_lib(OPENCL)

include_directories(${Ogre_INCLUDE_DIRS})
link_directories   (${Ogre_LIBRARY_DIRS})
include_directories(${Ois_INCLUDE_DIRS})
link_directories   (${Ois_LIBRARY_DIRS})
include_directories(${RoR_SOURCE_DIR}/source/protocol)

add_definitions("-DNOOGRE -D_UNICODE")
include_directories(../main)

FILE(GLOB sources "*.cpp")
FILE(GLOB headers "*.h")
add_executable(rorconfig WIN32 ${sources} ${headers} ${OS_SOURCE} ../main/InputEngine.cpp)

TARGET_LINK_LIBRARIES(rorconfig ${WXWIDGETS_SHARED_LIBRARIES} ${Ogre_LIBRARIES} ${Ois_LIBRARIES} ${OS_LIBS} ${optional_libs})



