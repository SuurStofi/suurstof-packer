@echo off
echo ====================================
echo  Building Stub Executable
echo ====================================

set MINGW_PATH=C:\Qt\Tools\mingw1310_64
set PATH=%MINGW_PATH%\bin;%PATH%

echo.
echo Compiling stub.cpp...
g++ -o stub.exe stub.cpp ^
    -static ^
    -std=c++17 ^
    -O2 ^
    -s ^
    -lshlwapi ^
    -lshell32 ^
    -mwindows ^
    -municode

if errorlevel 1 (
    echo.
    echo ====================================
    echo  BUILD FAILED!
    echo ====================================
    pause
    exit /b 1
)

echo.
echo ====================================
echo  BUILD SUCCESS!
echo ====================================
echo Stub executable: stub.exe
dir stub.exe
echo.

REM Copy to parent directory
copy stub.exe ..\stub.exe

echo Copied to main directory
pause
