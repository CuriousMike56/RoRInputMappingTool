add_subdirectory(main)


set(ROR_BUILD_CONFIGURATOR "TRUE" CACHE BOOL "build the configurator")
IF(ROR_BUILD_CONFIGURATOR)
  add_subdirectory(configurator)
  #add_subdirectory(modmanager)
ENDIF(ROR_BUILD_CONFIGURATOR)

set(ROR_BUILD_INSTALLER "FALSE" CACHE BOOL "build the installer")
IF(ROR_BUILD_INSTALLER)
  add_subdirectory(installer)
ENDIF(ROR_BUILD_INSTALLER)

#set(ROR_BUILD_EDITOR "FALSE" CACHE BOOL "build the editor")
#IF(ROR_BUILD_EDITOR)
#  add_subdirectory(editor)
#ENDIF(ROR_BUILD_EDITOR)

IF(ROR_BUILD_VIEWER)
  add_subdirectory(viewer)
ENDIF(ROR_BUILD_VIEWER)
