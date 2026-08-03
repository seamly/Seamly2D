@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
    echo Failed to initialize VS 2025 x64 environment
    exit /b 1
)
echo VS 2025 x64 environment initialized

cd /d c:\Users\susan\Projects\Seamly2D-private\src\app\seamlylayout\qt_frontend

echo.
echo === Configuring CMake (debug) ===
cmake --preset debug -DCMAKE_PREFIX_PATH="C:/Qt/6.11.1/msvc2022_64"
if errorlevel 1 (
    echo CMake configure failed
    exit /b 1
)

echo.
echo === Building ===
cmake --build --preset debug
if errorlevel 1 (
    echo Build failed
    exit /b 1
)

echo.
echo === Build successful ===
