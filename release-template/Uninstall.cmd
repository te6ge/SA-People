@echo off
setlocal EnableExtensions
set "GAME=%~1"
if "%GAME%"=="" set /p "GAME=GTA San Andreas folder (the one with gta_sa.exe): "
set "GAME=%GAME:"=%"
if not exist "%GAME%\gta_sa.exe" (
  echo gta_sa.exe was not found in "%GAME%".
  exit /b 1
)
del /q "%GAME%\SA-People.asi" 2>nul
if exist "%GAME%\SA-People.asi" (echo Could not remove SA-People.asi. Close the game first. & exit /b 3)
echo SA-People.asi removed. The folder "%GAME%\SA-People" ^(relationships, settings, your sounds^) was kept.
echo Delete that folder by hand if you want to remove everything.
endlocal
