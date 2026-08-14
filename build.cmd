@echo off
setlocal enabledelayedexpansion

:: Find VS Installation Path
set "VS_PATH="
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath`) do (
    set "VS_PATH=%%i"
)

if not defined VS_PATH (
    echo Visual Studio installation not found!
    exit /b 1
)

:: Locate MSBuild
set "MSBUILD_PATH=!VS_PATH!\MSBuild\Current\Bin\MSBuild.exe"
if not exist "!MSBUILD_PATH!" (
    echo MSBuild not found at !MSBUILD_PATH!
    exit /b 1
)

:: Run MSBuild
echo Building OpenKey...
"!MSBUILD_PATH!" "Sources\OpenKey\win32\OpenKey\OpenKey.sln" /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=v145 /t:Restore,Build

if %ERRORLEVEL% neq 0 (
    echo Build failed!
    exit /b %ERRORLEVEL%
)

echo Build succeeded!
:: Automatically open the output folder
start "" "Sources\OpenKey\win32\OpenKey\x64\Release"
:: Automatically run the compiled exe
start "" "Sources\OpenKey\win32\OpenKey\x64\Release\OpenKey64.exe"

exit /b 0
