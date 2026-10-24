@echo off
setlocal
cd /d "%~dp0"
if not exist build mkdir build
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VSROOT="
if not exist "%VSWHERE%" (
  echo Visual Studio Installer not found.
  exit /b 2
)
"%VSWHERE%" -all -latest -products * -property installationPath > build\vs-installation.txt
if not "%errorlevel%"=="0" exit /b 2
for /f "usebackq delims=" %%i in ("build\vs-installation.txt") do set "VSROOT=%%i"
if not defined VSROOT (
  echo Visual Studio installation not found.
  exit /b 2
)
call "%VSROOT%\VC\Auxiliary\Build\vcvars32.bat"
if not "%errorlevel%"=="0" exit /b 3
cl /nologo /std:c++17 /utf-8 /EHsc /MT /W4 /WX /Od source\Tests.cpp /Febuild\PeopleTests.exe /Fobuild\Tests.obj
if not "%errorlevel%"=="0" exit /b 4
build\PeopleTests.exe
if not "%errorlevel%"=="0" exit /b 5
cl /nologo /std:c++17 /utf-8 /EHsc /MT /W4 /WX /O2 source\ArchiveCheck.cpp /Febuild\ArchiveCheck.exe /Fobuild\ArchiveCheck.obj
if not "%errorlevel%"=="0" exit /b 11
cl /nologo /std:c++17 /utf-8 /EHsc /MT /W4 /WX /O2 source\AudioCheck.cpp /Febuild\AudioCheck.exe /Fobuild\AudioCheck.obj /link strmiids.lib ole32.lib
if not "%errorlevel%"=="0" exit /b 9
cl /nologo /std:c++17 /utf-8 /EHsc /MT /W4 /WX /O2 source\GlyphCheck.cpp /Febuild\GlyphCheck.exe /Fobuild\GlyphCheck.obj /link user32.lib gdi32.lib gdiplus.lib d3d9.lib
if not "%errorlevel%"=="0" exit /b 10
cl /nologo /std:c++17 /utf-8 /EHsc /MT /W4 /WX /O2 /LD source\Plugin.cpp /Febuild\SA-People.asi /Fobuild\Plugin.obj /link user32.lib gdi32.lib gdiplus.lib strmiids.lib ole32.lib windowscodecs.lib d3d9.lib /INCREMENTAL:NO /MACHINE:X86
if not "%errorlevel%"=="0" exit /b 6
cl /nologo /std:c++17 /utf-8 /EHsc /MT /W4 /WX /O2 source\Preview.cpp /Febuild\PanelPreview.exe /Fobuild\Preview.obj /link gdiplus.lib user32.lib gdi32.lib
if not "%errorlevel%"=="0" exit /b 7
build\PanelPreview.exe
if not "%errorlevel%"=="0" exit /b 8
cl /nologo /std:c++17 /utf-8 /EHsc /MT /W4 /WX /O2 source\Installer.cpp /Febuild\Installer.exe /Fobuild\Installer.obj /link /SUBSYSTEM:WINDOWS /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" user32.lib ole32.lib shell32.lib
if not "%errorlevel%"=="0" exit /b 12
echo Build and simulation tests passed.
