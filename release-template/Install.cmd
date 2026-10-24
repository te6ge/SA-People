@echo off
setlocal EnableExtensions DisableDelayedExpansion
cd /d "%~dp0" || exit /b 3
set "GAME=%~1"
if "%GAME%"=="" set /p "GAME=GTA San Andreas folder (the one with gta_sa.exe): "
set "GAME=%GAME:"=%"
if not exist "%GAME%\gta_sa.exe" (
  echo gta_sa.exe was not found in "%GAME%".
  exit /b 1
)
for %%F in ("%GAME%\gta_sa.exe") do set "SIZE=%%~zF"
if not "%SIZE%"=="14383616" (
  echo WARNING: gta_sa.exe is %SIZE% bytes. Only 1.0 US ^(14383616 bytes^) is supported.
  choice /m "Install anyway"
  if errorlevel 2 exit /b 2
)
rem Validate the payload and folder conflicts before replacing any game file.
for %%F in ("SA-People.asi" "SA-People\languages.tsv" "SA-People\settings.ini" "loader\dinput8.dll" "loader\Ultimate-ASI-Loader-LICENSE.txt") do (
  if not exist "%%~F" (
    echo Missing package file: "%%~F". Extract the complete archive first.
    exit /b 3
  )
)
for %%D in (assets audio backgrounds) do if not exist "SA-People\%%D\" (
  echo Missing package folder: "SA-People\%%D".
  exit /b 3
)
for %%D in ("%GAME%\SA-People" "%GAME%\SA-People\assets" "%GAME%\SA-People\audio" "%GAME%\SA-People\backgrounds" "%GAME%\SA-People\licenses") do (
  if exist "%%~D" if not exist "%%~D\" (
    echo A file is blocking the required folder: "%%~D".
    exit /b 3
  )
)
set "NEEDLOADER=1"
set "VSIZE=0"
if exist "%GAME%\vorbisFile.dll" for %%F in ("%GAME%\vorbisFile.dll") do set "VSIZE=%%~zF"
if %VSIZE% GTR 100000 set "NEEDLOADER="
if exist "%GAME%\dinput8.dll" set "NEEDLOADER="
rem Refuse read-only bundled targets before any game file is replaced.
powershell -NoProfile -Command "$ErrorActionPreference='Stop'; $base=$PWD.Path; $files=@(Get-Item -LiteralPath 'SA-People.asi')+@(Get-ChildItem -LiteralPath 'SA-People' -Recurse -File | Where-Object {$_.Name -ne 'settings.ini'}); foreach($f in $files){$relative=$f.FullName.Substring($base.Length+1); $target=Join-Path $env:GAME $relative; if((Test-Path -LiteralPath $target -PathType Leaf) -and ((Get-Item -LiteralPath $target).Attributes -band [IO.FileAttributes]::ReadOnly)){throw ('Read-only target: '+$target)}}" >nul
if errorlevel 1 goto failed
set "STAMP="
for /f %%i in ('powershell -NoProfile -Command "Get-Date -Format yyyyMMdd-HHmmss-fff"') do set "STAMP=%%i"
if not defined STAMP (
  echo Could not create a backup timestamp. Installation stopped.
  exit /b 3
)
set "BACKUP=%GAME%\SA-People\backup-%STAMP%"
if exist "%GAME%\SA-People.asi" (
  mkdir "%BACKUP%" || goto failed
  copy /y "%GAME%\SA-People.asi" "%BACKUP%\" >nul || goto failed
  for %%F in (people.dat people.dat.bak languages.tsv settings.ini) do if exist "%GAME%\SA-People\%%F" (
    copy /y "%GAME%\SA-People\%%F" "%BACKUP%\" >nul || goto failed
  )
  echo Previous version backed up to "%BACKUP%".
)
if not exist "%GAME%\SA-People\" mkdir "%GAME%\SA-People" || goto failed
copy /y "SA-People.asi" "%GAME%\" >nul || goto failed
copy /y "SA-People\languages.tsv" "%GAME%\SA-People\" >nul || goto failed
if not exist "%GAME%\SA-People\settings.ini" (
  copy "SA-People\settings.ini" "%GAME%\SA-People\" >nul || goto failed
)
rem robocopy keeps non-ASCII file names (xcopy stops at them); exit codes 8+ are failures.
for %%D in (assets audio backgrounds) do (
  robocopy "SA-People\%%D" "%GAME%\SA-People\%%D" /e /r:1 /w:1 /njh /njs /nfl /ndl /np >nul
  if errorlevel 8 goto failed
)
if not exist "%GAME%\SA-People\licenses\" mkdir "%GAME%\SA-People\licenses" || goto failed
copy /y "loader\Ultimate-ASI-Loader-LICENSE.txt" "%GAME%\SA-People\licenses\" >nul || goto failed
if defined NEEDLOADER (
  copy /y "loader\dinput8.dll" "%GAME%\" >nul || goto failed
  echo Ultimate ASI Loader installed as dinput8.dll.
)
echo.
echo SA People installed. Existing settings.ini and people.dat were kept.
echo Bundled assets were updated; unrelated extra sounds and pictures were kept.
echo Start the game; check SA-People\SA-People.log if something does not work.
exit /b 0

:failed
echo Installation failed. Not all files were copied. Check write permissions, read-only files and whether the game is running.
exit /b 3
