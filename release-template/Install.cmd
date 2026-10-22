@echo off
setlocal EnableExtensions
cd /d "%~dp0"
set "GAME=%~1"
if "%GAME%"=="" set /p "GAME=GTA San Andreas folder (the one with gta_sa.exe): "
set "GAME=%GAME:"=%"
if not exist "%GAME%\gta_sa.exe" (
  echo gta_sa.exe was not found in "%GAME%".
  exit /b 1
)
for %%F in ("%GAME%\gta_sa.exe") do set "SIZE=%%~zF"
if not "%SIZE%"=="14383616" (
  echo WARNING: gta_sa.exe is %SIZE% bytes. This mod supports only the 1.0 US executable ^(14383616 bytes^).
  choice /m "Install anyway"
  if errorlevel 2 exit /b 2
)
if not exist "%GAME%\vorbisFile.dll" if not exist "%GAME%\dinput8.dll" (
  echo WARNING: no ASI loader found ^(vorbisFile.dll or dinput8.dll^). The mod will not start without one.
)
for /f %%i in ('powershell -NoProfile -Command "Get-Date -Format yyyyMMdd-HHmmss"') do set "STAMP=%%i"
set "BACKUP=%GAME%\SA-People\backup-%STAMP%"
if exist "%GAME%\SA-People.asi" (
  mkdir "%BACKUP%" >nul 2>&1
  copy /y "%GAME%\SA-People.asi" "%BACKUP%\" >nul
  if exist "%GAME%\SA-People\people.dat" copy /y "%GAME%\SA-People\people.dat" "%BACKUP%\" >nul
  if exist "%GAME%\SA-People\languages.tsv" copy /y "%GAME%\SA-People\languages.tsv" "%BACKUP%\" >nul
  echo Previous version backed up to "%BACKUP%".
)
if not exist "%GAME%\SA-People" mkdir "%GAME%\SA-People"
copy /y "SA-People.asi" "%GAME%\" >nul || (echo Could not copy SA-People.asi. Is the game running? & exit /b 3)
copy /y "SA-People\languages.tsv" "%GAME%\SA-People\" >nul
if not exist "%GAME%\SA-People\settings.ini" copy "SA-People\settings.ini" "%GAME%\SA-People\" >nul
xcopy /e /i /y "SA-People\assets" "%GAME%\SA-People\assets" >nul
xcopy /e /i /y "SA-People\audio" "%GAME%\SA-People\audio" >nul
if not exist "%GAME%\SA-People\audio\README.txt" copy "SA-People\audio\README.txt" "%GAME%\SA-People\audio\" >nul
xcopy /e /i /y "SA-People\backgrounds" "%GAME%\SA-People\backgrounds" >nul
if not exist "%GAME%\SA-People\backgrounds\README.txt" copy "SA-People\backgrounds\README.txt" "%GAME%\SA-People\backgrounds\" >nul
echo.
echo SA People installed. Your settings.ini and people.dat were kept; your own extra sounds and pictures stay.
echo Start the game; check SA-People\SA-People.log if something does not work.
endlocal
