@echo off
setlocal

powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0Scripts\Cleanup.ps1" %*
set "PROJECTTEMPLATE_EXIT_CODE=%ERRORLEVEL%"
if not "%PROJECTTEMPLATE_EXIT_CODE%"=="0" if not defined CI if not defined PROJECTTEMPLATE_NO_PAUSE pause
exit /b %PROJECTTEMPLATE_EXIT_CODE%
