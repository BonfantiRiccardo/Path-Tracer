@echo off
echo Setting up Visual Studio environment...
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
echo.
echo Compiling...
cl.exe /Zi /EHsc /nologo /std:c++17 /I"%~dp0include" /Fe:"%~dp0out\main.exe" "%~dp0src\main.cpp"
if %ERRORLEVEL% EQU 0 (
    echo Build successful!
) else (
    echo Build failed with error code %ERRORLEVEL%
)
