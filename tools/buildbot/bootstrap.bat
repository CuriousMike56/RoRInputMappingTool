mkdir build
cd build
rem poor mans escaping of newlines to make this giant more readable ...
"C:\Program Files (x86)\CMake 2.6\bin\cmake.exe" ^
-G "Visual Studio 9 2008" ^
-DROR_DEPENDENCIES_DIR=C:\data\ror_deps_released\ror-deps-windows-32bit\dependencies ^
-DROR_BUILD_INSTALLER=ON ^
-DROR_BUILD_CONFIGURATOR=ON ^
-DROR_BUILD_EDITOR=OFF ^
-DROR_USE_ANGELSCRIPT=ON ^
-DROR_USE_CAELUM=ON ^
-DROR_USE_CRASHRPT=ON ^
-DROR_USE_HYDRAX=ON ^
-DROR_USE_LUA=ON ^
-DROR_USE_MOFILEREADER=ON ^
-DROR_USE_MYGUI=ON ^
-DROR_USE_OIS_G27=ON ^
-DROR_USE_OPENAL=ON ^
-DROR_USE_PAGED=ON ^
-DROR_USE_SOCKETW=ON ^
..