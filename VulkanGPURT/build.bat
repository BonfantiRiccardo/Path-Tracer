@echo off
setlocal EnableExtensions

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"
pushd "%ROOT%" >nul

where cmake >nul 2>nul
if errorlevel 1 (
    echo Build failed: cmake not found in PATH.
    popd >nul
    exit /b 1
)

echo Configuring project...
cmake -S "%ROOT%" -B "%ROOT%\build"
if errorlevel 1 (
    echo Configure failed.
    popd >nul
    exit /b 1
)

echo Building project...
cmake --build "%ROOT%\build" --config Release
if errorlevel 1 (
    echo Build failed.
    popd >nul
    exit /b 1
)

set "BIN_MULTI=%ROOT%\build\Release\vulkan_gpu_rt.exe"
set "BIN_SINGLE=%ROOT%\build\vulkan_gpu_rt.exe"
set "SHADER_BUILD=%ROOT%\build\shaders\path_tracer.comp.spv"
set "OUT_DIR=%ROOT%\out"
set "OUT_EXE=%OUT_DIR%\vulkan_gpu_rt.exe"
set "OUT_SHADER_DIR=%OUT_DIR%\shaders"
set "OUT_SHADER=%OUT_SHADER_DIR%\path_tracer.comp.spv"

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"
if not exist "%OUT_SHADER_DIR%" mkdir "%OUT_SHADER_DIR%"

if exist "%BIN_MULTI%" (
    copy /Y "%BIN_MULTI%" "%OUT_EXE%" >nul
) else if exist "%BIN_SINGLE%" (
    copy /Y "%BIN_SINGLE%" "%OUT_EXE%" >nul
) else (
    echo Build completed but executable was not found.
    popd >nul
    exit /b 1
)

if exist "%SHADER_BUILD%" (
    copy /Y "%SHADER_BUILD%" "%OUT_SHADER%" >nul
)

echo Build successful: %OUT_EXE%

echo.
echo Run with:
echo   "%OUT_EXE%" --output image.ppm

popd >nul
