@echo off
setlocal

set "PROJECT_ROOT=%~dp0"
set "PROJECT_REQUESTED_ACTION=%~1"

if defined PROJECT_REQUESTED_ACTION if /I not "%PROJECT_REQUESTED_ACTION%"=="vs2026" if /I not "%PROJECT_REQUESTED_ACTION%"=="vs2022" (
    echo Unsupported Premake action "%PROJECT_REQUESTED_ACTION%". Use vs2026 or vs2022. 1>&2
    exit /b 2
)

set "PROJECT_VISUAL_STUDIO_ARGUMENTS="
if /I "%PROJECT_REQUESTED_ACTION%"=="vs2026" set "PROJECT_VISUAL_STUDIO_ARGUMENTS=-VisualStudioVersion 2026"
if /I "%PROJECT_REQUESTED_ACTION%"=="vs2022" set "PROJECT_VISUAL_STUDIO_ARGUMENTS=-VisualStudioVersion 2022"

for /f "usebackq delims=" %%A in (`powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%PROJECT_ROOT%Scripts\Setup.ps1" -PrintVisualStudioAction %PROJECT_VISUAL_STUDIO_ARGUMENTS%`) do set "PROJECT_ACTION=%%A"
if not defined PROJECT_ACTION (
    echo Failed to resolve a supported Visual Studio action. Run Setup.bat first. 1>&2
    exit /b 1
)

for /f "usebackq delims=" %%P in (`powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%PROJECT_ROOT%Scripts\Setup.ps1" -PrintPremakePath`) do set "PROJECT_PREMAKE=%%P"
if not defined PROJECT_PREMAKE (
    echo Failed to resolve the project-local Premake executable. Run Setup.bat first. 1>&2
    exit /b 1
)

"%PROJECT_PREMAKE%" --file="%PROJECT_ROOT%premake5.lua" %PROJECT_ACTION%
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

if /I "%PROJECT_ACTION%"=="vs2026" set "PROJECT_SOLUTION=%PROJECT_ROOT%ProjectTemplate.slnx"
if /I "%PROJECT_ACTION%"=="vs2022" set "PROJECT_SOLUTION=%PROJECT_ROOT%ProjectTemplate.sln"

if not exist "%PROJECT_SOLUTION%" (
    echo Premake completed without generating the expected solution: "%PROJECT_SOLUTION%" 1>&2
    exit /b 1
)

echo Generated solution: "%PROJECT_SOLUTION%"
exit /b 0
