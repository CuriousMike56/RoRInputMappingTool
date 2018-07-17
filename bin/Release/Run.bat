@ECHO OFF
:sub_setup
TITLE Rigs of Rods Input Mapping Tool
echo -- Rigs of Rods Input Mapping Tool --
echo Copying Rigs of Rods 0.4 files to config folder
xcopy  "%userprofile%\Documents\Rigs of Rods 0.4\config\input.map" "config" /s /e /y
echo Starting tool..(Do not close this window!)
start "" /wait RoRInputMappingTool.exe