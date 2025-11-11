@echo off
setlocal enabledelayedexpansion

echo ================================================
echo Building SuurStof-Packer.exe
echo ================================================

set PATH=C:\Qt\Tools\mingw1310_64\bin;C:\Qt\6.10.0\mingw_64\bin;%PATH%

cd /d "%~dp0"

echo.
echo [Step 1/3] Generating MOC...
C:\Qt\6.10.0\mingw_64\bin\moc.exe src\gui\MainWindow.h -o build\moc\moc_MainWindow.cpp
if errorlevel 1 goto error

set INCLUDES=-I. -Isrc -Isrc/gui -Isrc/core -Isrc/utils -IC:/Qt/6.10.0/mingw_64/include -IC:/Qt/6.10.0/mingw_64/include/QtWidgets -IC:/Qt/6.10.0/mingw_64/include/QtGui -IC:/Qt/6.10.0/mingw_64/include/QtCore -Ibuild/moc -IC:/Qt/6.10.0/mingw_64/mkspecs/win32-g++
set FLAGS=-c -O2 -std=c++17 -Wall -fexceptions -mthreads -DUNICODE -D_UNICODE -DWIN32 -DQT_NO_DEBUG -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB -DQT_NEEDS_QMAIN

echo.
echo [Step 2/3] Compiling...
echo   [1/7] main.cpp
g++ %FLAGS% %INCLUDES% -o build\obj\main.o src\main.cpp
if errorlevel 1 goto error

echo   [2/7] MainWindow.cpp
g++ %FLAGS% %INCLUDES% -o build\obj\MainWindow.o src\gui\MainWindow.cpp
if errorlevel 1 goto error

echo   [3/7] PEParser.cpp
g++ %FLAGS% %INCLUDES% -o build\obj\PEParser.o src\core\PEParser.cpp
if errorlevel 1 goto error

echo   [4/7] ResourceEmbedder.cpp
g++ %FLAGS% %INCLUDES% -o build\obj\ResourceEmbedder.o src\core\ResourceEmbedder.cpp
if errorlevel 1 goto error

echo   [5/7] Obfuscator.cpp
g++ %FLAGS% %INCLUDES% -o build\obj\Obfuscator.o src\core\Obfuscator.cpp
if errorlevel 1 goto error

echo   [6/7] StubGenerator.cpp
g++ %FLAGS% %INCLUDES% -o build\obj\StubGenerator.o src\core\StubGenerator.cpp
if errorlevel 1 goto error

echo   [7/7] moc_MainWindow.cpp
g++ %FLAGS% %INCLUDES% -o build\obj\moc_MainWindow.o build\moc\moc_MainWindow.cpp
if errorlevel 1 goto error

echo.
echo [Step 3/3] Linking...
g++ -Wl,-subsystem,windows -mthreads -o build\SuurStof-Packer.exe build\obj\main.o build\obj\MainWindow.o build\obj\PEParser.o build\obj\ResourceEmbedder.o build\obj\Obfuscator.o build\obj\StubGenerator.o build\obj\moc_MainWindow.o -LC:/Qt/6.10.0/mingw_64/lib -lQt6Widgets -lQt6Gui -lQt6Core -lmingw32 C:/Qt/6.10.0/mingw_64/lib/libQt6EntryPoint.a
if errorlevel 1 goto error

echo.
echo ================================================
echo BUILD SUCCESS!
echo ================================================
dir build\SuurStof-Packer.exe
echo.
echo IMPORTANT: Copy stub.exe to build folder:
echo   copy stub-project\stub_new.exe build\stub.exe
pause
exit /b 0

:error
echo.
echo ================================================
echo BUILD FAILED!
echo ================================================
pause
exit /b 1
