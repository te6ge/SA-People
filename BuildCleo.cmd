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
cl /nologo /std:c++17 /utf-8 /EHsc /MT /W4 /WX /O2 /LD /DSA_PEOPLE_CLEO source\Plugin.cpp /Febuild\SA-People.cleo /Fobuild\CleoPlugin.obj /link user32.lib gdi32.lib gdiplus.lib strmiids.lib ole32.lib windowscodecs.lib d3d9.lib /INCREMENTAL:NO /MACHINE:X86
if not "%errorlevel%"=="0" exit /b 15
cl /nologo /std:c++17 /utf-8 /EHsc /MT /W4 /WX /O2 /LD source\CleoTestHost.cpp /Febuild\CleoTestHost.dll /Fobuild\CleoTestHost.obj /link /DEF:source\CleoTestHost.def /MACHINE:X86
if not "%errorlevel%"=="0" exit /b 16
cl /nologo /std:c++17 /utf-8 /EHsc /MT /W4 /WX /O2 source\CleoBridgeTests.cpp /Febuild\CleoBridgeTests.exe /Fobuild\CleoBridgeTests.obj /link /MACHINE:X86
if not "%errorlevel%"=="0" exit /b 17
build\CleoBridgeTests.exe
if not "%errorlevel%"=="0" exit /b 18
cl /nologo /std:c++17 /utf-8 /EHsc /MT /W4 /WX /O2 /LD source\GInputTestHost.cpp /Febuild\GInputTestHost.dll /Fobuild\GInputTestHost.obj /link /MACHINE:X86
if not "%errorlevel%"=="0" exit /b 19
cl /nologo /std:c++17 /utf-8 /EHsc /MT /W4 /WX /O2 source\GInputApiTests.cpp /Febuild\GInputApiTests.exe /Fobuild\GInputApiTests.obj /link /MACHINE:X86
if not "%errorlevel%"=="0" exit /b 20
build\GInputApiTests.exe
if not "%errorlevel%"=="0" exit /b 21
echo CLEO plugin and callback tests passed.
