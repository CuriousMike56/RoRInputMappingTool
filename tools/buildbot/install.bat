@echo off
if "%1" == "" goto error
mkdir C:\data\ror-binary-cache\rev-%1
rem copy files to cache

copy bin\*.* C:\data\ror-binary-cache\rev-%1
symstore add /r /f C:\data\ror-binary-cache\rev-%1\*.* /s C:\data\symbols /t "RigsOfRods" /v "Build Rev-%1"

xcopy /E /Y bin\*.* C:\data\ror-streams\binary\win32\beta\
cd C:\data\ror-streams\binary\win32\beta\
rem svn ci . -m="rev-%1" --username="" --password=""

goto end

:error
echo missing argument!
exit /B 1

:end
