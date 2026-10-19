@echo off
chcp 65001 >nul
color 0A
title COGMIND // SUNBERRY ZH v0.1.0
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Install.ps1" -Mode Install
set "ZH_RESULT=%errorlevel%"
pause
exit /b %ZH_RESULT%
