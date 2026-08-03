@echo off
REM project: SeamlyLayout
REM author: slspencer, copyright 2026
REM MIT License: https://opensource.org/licenses/MIT

powershell -ExecutionPolicy Bypass -File "%~dp0run_app.ps1"
if errorlevel 1 pause
