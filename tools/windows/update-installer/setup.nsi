; HM NIS Edit Wizard helper defines
!define PRODUCT_NAME "Rigs of Rods"
!define PRODUCT_VERSION "(Update Service)"
!define PRODUCT_PUBLISHER "Pierre-Michel Ricordel"
!define PRODUCT_WEB_SITE "http://www.rigsofrods.com"

SetCompressor lzma

BrandingText "Rigs of Rods"
InstProgressFlags smooth colored
XPStyle on
ShowInstDetails show
SetDateSave on
#SetDatablockOptimize on
CRCCheck on
SilentInstall normal

; MUI 1.67 compatible ------
!include "MUI.nsh"
!include "LogicLib.nsh"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install-blue.ico"

; Welcome page
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "readme-installer.txt"
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

; Language files
!insertmacro MUI_LANGUAGE "English" ;first language is the default language
!insertmacro MUI_LANGUAGE "French"
!insertmacro MUI_LANGUAGE "German"
!insertmacro MUI_LANGUAGE "Spanish"
!insertmacro MUI_LANGUAGE "SpanishInternational"
!insertmacro MUI_LANGUAGE "SimpChinese"
!insertmacro MUI_LANGUAGE "TradChinese"
!insertmacro MUI_LANGUAGE "Japanese"
!insertmacro MUI_LANGUAGE "Korean"
!insertmacro MUI_LANGUAGE "Italian"
!insertmacro MUI_LANGUAGE "Dutch"
!insertmacro MUI_LANGUAGE "Danish"
!insertmacro MUI_LANGUAGE "Swedish"
!insertmacro MUI_LANGUAGE "Norwegian"
!insertmacro MUI_LANGUAGE "NorwegianNynorsk"
!insertmacro MUI_LANGUAGE "Finnish"
!insertmacro MUI_LANGUAGE "Greek"
!insertmacro MUI_LANGUAGE "Russian"
!insertmacro MUI_LANGUAGE "Portuguese"
!insertmacro MUI_LANGUAGE "PortugueseBR"
!insertmacro MUI_LANGUAGE "Polish"
!insertmacro MUI_LANGUAGE "Ukrainian"
!insertmacro MUI_LANGUAGE "Czech"
!insertmacro MUI_LANGUAGE "Slovak"
!insertmacro MUI_LANGUAGE "Croatian"
!insertmacro MUI_LANGUAGE "Bulgarian"
!insertmacro MUI_LANGUAGE "Hungarian"
!insertmacro MUI_LANGUAGE "Thai"
!insertmacro MUI_LANGUAGE "Romanian"
!insertmacro MUI_LANGUAGE "Latvian"
!insertmacro MUI_LANGUAGE "Macedonian"
!insertmacro MUI_LANGUAGE "Estonian"
!insertmacro MUI_LANGUAGE "Turkish"
!insertmacro MUI_LANGUAGE "Lithuanian"
!insertmacro MUI_LANGUAGE "Slovenian"
!insertmacro MUI_LANGUAGE "Serbian"
!insertmacro MUI_LANGUAGE "SerbianLatin"
!insertmacro MUI_LANGUAGE "Arabic"
!insertmacro MUI_LANGUAGE "Farsi"
!insertmacro MUI_LANGUAGE "Hebrew"
!insertmacro MUI_LANGUAGE "Indonesian"
!insertmacro MUI_LANGUAGE "Mongolian"
!insertmacro MUI_LANGUAGE "Luxembourgish"
!insertmacro MUI_LANGUAGE "Albanian"
!insertmacro MUI_LANGUAGE "Breton"
!insertmacro MUI_LANGUAGE "Belarusian"
!insertmacro MUI_LANGUAGE "Icelandic"
!insertmacro MUI_LANGUAGE "Malay"
!insertmacro MUI_LANGUAGE "Bosnian"
!insertmacro MUI_LANGUAGE "Kurdish"
!insertmacro MUI_LANGUAGE "Irish"
!insertmacro MUI_LANGUAGE "Uzbek"
!insertmacro MUI_LANGUAGE "Galician"
!insertmacro MUI_LANGUAGE "Afrikaans"
;!insertmacro MUI_LANGUAGE "Catalan"

; Reserve files
!insertmacro MUI_RESERVEFILE_INSTALLOPTIONS

; MUI end ------

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "setup.exe"
InstallDir "$EXEDIR"

ShowInstDetails show

Function InstallDirectX
	InitPluginsDir
	File /oname=$PLUGINSDIR\dxwebsetup.exe "dxwebsetup.exe"
	Banner::show /NOUNLOAD "Installing lastest DirectX"
    ExecWait '"$PLUGINSDIR\dxwebsetup.exe"'
	Delete $PLUGINSDIR\dxwebsetup.exe
	Banner::destroy
FunctionEnd

Function InstallVisualStudioRuntime
	InitPluginsDir
	File /oname=$PLUGINSDIR\vcredist_x86.exe "vcredist_x86.exe"
	Banner::show /NOUNLOAD "Installing Visual Studio Runtime"
	ExecWait "vcredist_x86.exe /q /l log.txt"
	Delete $PLUGINSDIR\vcredist_x86.exe
	Banner::destroy
FunctionEnd

Section "Required Tools" SEC01
	Call InstallDirectX
	Call InstallVisualStudioRuntime
SectionEnd

Section -AdditionalIcons
	SetOutPath $EXEDIR
	CreateDirectory "$SMPROGRAMS\Rigs of Rods ${PRODUCT_VERSION}"
	CreateShortCut "$SMPROGRAMS\Rigs of Rods ${PRODUCT_VERSION}\Rigs of Rods.lnk" "$INSTDIR\RoR.exe"
	CreateShortCut "$SMPROGRAMS\Rigs of Rods ${PRODUCT_VERSION}\Configurator.lnk" "$INSTDIR\RoRConfig.exe"
	CreateShortCut "$SMPROGRAMS\Rigs of Rods ${PRODUCT_VERSION}\Key Sheet.lnk" "$INSTDIR\keysheet.pdf"
	CreateShortCut "$SMPROGRAMS\Rigs of Rods ${PRODUCT_VERSION}\Manual.lnk" "$INSTDIR\Things you can do in Rigs of Rods.pdf"
SectionEnd

Section -Post
	; add some registry info
	WriteRegStr HKCU "Software\RigsOfRods\${PRODUCT_VERSION}\" "directory" "$INSTDIR"
	WriteRegStr HKCU "Software\RigsOfRods\${PRODUCT_VERSION}\" "version" "${PRODUCT_VERSION}"

	; new protocols
	WriteRegStr HKCR "rorserver" "" "URL:Rigs of Rods Server"
	WriteRegStr HKCR "rorserver" "Url Protocol" ""
	WriteRegStr HKCR "rorserver\DefaultIcon" "" ""
	WriteRegStr HKCR "rorserver\shell\open\command" "" '"$INSTDIR\RoR.exe" -wd="$INSTDIR" -cmd="%1"'

	WriteRegStr HKCR "rorwsync" "" "URL:Rigs of Rods Wsync Service"
	WriteRegStr HKCR "rorwsync" "Url Protocol" ""
	WriteRegStr HKCR "rorwsync\DefaultIcon" "" ""
	WriteRegStr HKCR "rorwsync\shell\open\command" "" '"$INSTDIR\update.exe" -wd="$INSTDIR" -cmd="%1"'
	
	; new file types
	WriteRegStr HKCR ".sa" "" "Angelscript"
	WriteRegStr HKCR ".sa\shell\open\command" "" '"$INSTDIR\script_compiler.exe" "%1"'
	SetRebootFlag true
SectionEnd


