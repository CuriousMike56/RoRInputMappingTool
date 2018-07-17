@ECHO OFF
:sub_setup
TITLE Rigs of Rods Input Mapping Tool
echo -- Rigs of Rods Input Mapping Tool --
echo Restoring default input.map...
xcopy  input.map "C:\Users\%username%\Documents\Rigs of Rods Input Mapping Tool\config" /s /e /y
echo Done.
pause
exit