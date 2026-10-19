@echo off
chcp 65001 >nul
color 0A
title COGMIND // SUNBERRY ZH
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0资源\Install.ps1" -Mode Menu
set "ZH_RESULT=%errorlevel%"
pause
exit /b %ZH_RESULT%
