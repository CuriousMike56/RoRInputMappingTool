@ECHO OFF
:sub_setup
TITLE Rigs of Rods Input Mapping Tool
echo -- Rigs of Rods Input Mapping Tool --
echo Copying Documents\My Games\Rigs of Rods files to config folder
xcopy  "%userprofile%\Documents\My Games\Rigs of Rods\config\input.map" "config" /s /e /y
echo Starting tool..(Do not close this window!)
start "" /wait RoRInputMappingTool.exe