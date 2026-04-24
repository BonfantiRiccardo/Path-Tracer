@echo off
setlocal EnableExtensions

set "ROOT=%~dp0"
pushd "%ROOT%" >nul

echo Setting up Visual Studio environment...

set "VCVARS_BAT="
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if exist "%VSWHERE%" (
    for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS_INSTALL=%%i"
    if defined VS_INSTALL if exist "%VS_INSTALL%\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS_BAT=%VS_INSTALL%\VC\Auxiliary\Build\vcvars64.bat"
)

if not defined VCVARS_BAT if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS_BAT=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

if not defined VCVARS_BAT (
    echo Build failed: could not locate vcvars64.bat. Install Visual Studio C++ tools.
    popd >nul
    exit /b 1
)

call "%VCVARS_BAT%"
if %ERRORLEVEL% NEQ 0 (
    echo Build failed while initializing Visual Studio environment.
    popd >nul
    exit /b 1
)

set "BUILD_MODE=Debug"
if not "%~1"=="" (
    if /I "%~1"=="Debug" (
        set "BUILD_MODE=Debug"
    ) else if /I "%~1"=="Release" (
        set "BUILD_MODE=Release"
    ) else (
        echo Invalid build mode: "%~1"
        echo Usage: build.bat [Debug^|Release]
        popd >nul
        exit /b 1
    )
)

echo.
echo Compiling (%BUILD_MODE%)...
if not exist "%ROOT%out" mkdir "%ROOT%out"

if /I "%BUILD_MODE%"=="Release" (
    cl.exe /O2 /Ot /GL /Gy /DNDEBUG /EHsc /nologo /std:c++17 /I"%ROOT%include" /Fe:"%ROOT%out\main.exe" "%ROOT%src\main.cpp" /link /LTCG
) else (
    cl.exe /Zi /EHsc /nologo /std:c++17 /I"%ROOT%include" /Fe:"%ROOT%out\main.exe" "%ROOT%src\main.cpp"
)
set "CL_EXIT=%ERRORLEVEL%"
if "%CL_EXIT%"=="0" (
    echo Build successful! %BUILD_MODE%
) else (
    echo Build failed %BUILD_MODE% with error code %CL_EXIT%
)

popd >nul
exit /b %CL_EXIT%
