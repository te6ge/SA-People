@echo off
setlocal
cd /d "%~dp0"
call Build.cmd
if not "%errorlevel%"=="0" exit /b %errorlevel%
call BuildCleo.cmd
if not "%errorlevel%"=="0" exit /b %errorlevel%
echo Normal ASI and CLEO built and tested from the same source.
