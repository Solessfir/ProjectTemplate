@echo off
setlocal

set "PROJECT_ROOT=%~dp0"
set "PROJECT_REQUESTED_ACTION=%~1"

if defined PROJECT_REQUESTED_ACTION if /I not "%PROJECT_REQUESTED_ACTION%"=="vs2026" if /I not "%PROJECT_REQUESTED_ACTION%"=="vs2022" (
    echo Unsupported Premake action "%PROJECT_REQUESTED_ACTION%". Use vs2026 or vs2022. 1>&2
    set "PROJECT_EXIT_CODE=2"
    goto :error
)

set "PROJECT_VISUAL_STUDIO_ARGUMENTS="
if /I "%PROJECT_REQUESTED_ACTION%"=="vs2026" set "PROJECT_VISUAL_STUDIO_ARGUMENTS=-VisualStudioVersion 2026"
if /I "%PROJECT_REQUESTED_ACTION%"=="vs2022" set "PROJECT_VISUAL_STUDIO_ARGUMENTS=-VisualStudioVersion 2022"

for /f "usebackq delims=" %%A in (`powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%PROJECT_ROOT%Scripts\Setup.ps1" -PrintVisualStudioAction %PROJECT_VISUAL_STUDIO_ARGUMENTS% 2^>nul`) do set "PROJECT_ACTION=%%A"
if not defined PROJECT_ACTION (
    echo Could not find a supported Visual Studio toolchain. Run Setup.bat first. 1>&2
    set "PROJECT_EXIT_CODE=1"
    goto :error
)

for /f "usebackq delims=" %%P in (`powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%PROJECT_ROOT%Scripts\Setup.ps1" -PrintPremakePath 2^>nul`) do set "PROJECT_PREMAKE=%%P"
if not defined PROJECT_PREMAKE (
    echo Could not find project-local Premake. Run Setup.bat first. 1>&2
    set "PROJECT_EXIT_CODE=1"
    goto :error
)

"%PROJECT_PREMAKE%" --file="%PROJECT_ROOT%premake5.lua" %PROJECT_ACTION%
set "PROJECT_EXIT_CODE=%ERRORLEVEL%"
if not "%PROJECT_EXIT_CODE%"=="0" goto :error

if /I "%PROJECT_ACTION%"=="vs2026" set "PROJECT_SOLUTION=%PROJECT_ROOT%ProjectTemplate.slnx"
if /I "%PROJECT_ACTION%"=="vs2022" set "PROJECT_SOLUTION=%PROJECT_ROOT%ProjectTemplate.sln"

if not exist "%PROJECT_SOLUTION%" (
    echo Premake completed without generating the expected solution: "%PROJECT_SOLUTION%" 1>&2
    set "PROJECT_EXIT_CODE=1"
    goto :error
)

echo Generated solution: "%PROJECT_SOLUTION%"
exit /b 0

:error
if not defined CI if not defined PROJECTTEMPLATE_NO_PAUSE pause
exit /b %PROJECT_EXIT_CODE%
