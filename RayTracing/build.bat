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

call "%VCVARS_BAT%" >nul 2>&1
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
        echo Usage: build.bat [Debug^|Release] [source_file]
        popd >nul
        exit /b 1
    )
)

set "SOURCE_FILE=%ROOT%src\main.cpp"
if not "%~2"=="" set "SOURCE_FILE=%~2"

if not exist "%SOURCE_FILE%" if exist "%ROOT%%SOURCE_FILE%" set "SOURCE_FILE=%ROOT%%SOURCE_FILE%"

if not exist "%SOURCE_FILE%" (
    echo Build failed: source file not found "%SOURCE_FILE%"
    popd >nul
    exit /b 1
)

for %%I in ("%SOURCE_FILE%") do set "SOURCE_EXT=%%~xI"
if /I not "%SOURCE_EXT%"==".c" if /I not "%SOURCE_EXT%"==".cc" if /I not "%SOURCE_EXT%"==".cpp" if /I not "%SOURCE_EXT%"==".cxx" (
    echo Build failed: unsupported source extension "%SOURCE_EXT%". Use .c, .cc, .cpp, or .cxx.
    popd >nul
    exit /b 1
)

for %%I in ("%SOURCE_FILE%") do set "TARGET_NAME=%%~nI"
set "OUTPUT_EXE=%ROOT%out\%TARGET_NAME%.exe"
set "BUILD_LOG=%TEMP%\rtw_build_%TARGET_NAME%_%RANDOM%.log"

rem main also needs the translation units that compile the header-only libraries.
set "EXTRA_SOURCES="
if /I "%TARGET_NAME%"=="main" set EXTRA_SOURCES="%ROOT%src\cgltf_impl.cpp" "%ROOT%src\tinyobj_impl.cpp" "%ROOT%src\stb_image_impl.cpp"

echo Compiling %TARGET_NAME%%SOURCE_EXT%...
if not exist "%ROOT%out" mkdir "%ROOT%out"

if /I "%BUILD_MODE%"=="Release" (
    cl.exe /O2 /Ot /GL /Gy /DNDEBUG /EHsc /nologo /std:c++17 /I"%ROOT%include" /Fo"%ROOT%out\\" /Fe:"%OUTPUT_EXE%" "%SOURCE_FILE%" %EXTRA_SOURCES% /link /LTCG >"%BUILD_LOG%" 2>&1
) else (
    cl.exe /Zi /EHsc /nologo /std:c++17 /I"%ROOT%include" /Fo"%ROOT%out\\" /Fd"%ROOT%out\\" /Fe:"%OUTPUT_EXE%" "%SOURCE_FILE%" %EXTRA_SOURCES% >"%BUILD_LOG%" 2>&1
)
set "CL_EXIT=%ERRORLEVEL%"

rem main loads models/ and images/ relative to its own folder, so mirror them next to it.
if "%CL_EXIT%"=="0" if /I "%TARGET_NAME%"=="main" (
    for %%D in (models images) do (
        if exist "%ROOT%%%D" (
            robocopy "%ROOT%%%D" "%ROOT%out\%%D" /E /NJH /NJS /NFL /NDL /NP >nul
            if errorlevel 8 echo Warning: could not copy %%D next to the executable.
        )
    )
)

if "%CL_EXIT%"=="0" (
    echo Build successful! %BUILD_MODE%
) else (
    if exist "%BUILD_LOG%" type "%BUILD_LOG%"
    echo Build failed %BUILD_MODE% with error code %CL_EXIT%
)

if exist "%BUILD_LOG%" del /q "%BUILD_LOG%" >nul 2>&1

popd >nul
exit /b %CL_EXIT%
