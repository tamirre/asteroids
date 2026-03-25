@echo off
setlocal enabledelayedexpansion

REM ----------------------------------------
REM LOAD MSVC ENVIRONMENT
REM ----------------------------------------
if not defined VCINSTALLDIR (
    echo Setting up MSVC environment...
    if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
        call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    ) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" (
        call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
    ) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
        call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    ) else (
        echo ERROR: Could not find vcvars64.bat
        exit /b 1
    )
)

REM ----------------------------------------
REM CONFIGURATION
REM ----------------------------------------
set GAME_NAME=asteroids
set SRC_DIR=%~dp0..
set BIN_DIR=%~dp0..\..\bin

if not exist %BIN_DIR% mkdir %BIN_DIR%

set LIB_DIR=%SRC_DIR%\third_party\lib
set INCLUDE_FLAGS=-I%SRC_DIR%\third_party\include

REM ----------------------------------------
REM COMPILER / LINKER
REM ----------------------------------------
set CC=clang
set LINKER=lld-link

REM ----------------------------------------
REM DEFINES & FLAGS
REM ----------------------------------------
set DEFINES=-DPLATFORM_WINDOWS=1 -D_CRT_SECURE_NO_WARNINGS

set DEBUG=1

if "%DEBUG%"=="1" (
    set CFLAGS=-gcodeview -O0 %DEFINES% %INCLUDE_FLAGS%
) else (
    set CFLAGS=-O2 %DEFINES% %INCLUDE_FLAGS%
)

REM Full system libs for Raylib/GLFW
set SYS_LIBS=kernel32.lib user32.lib gdi32.lib winmm.lib opengl32.lib shell32.lib ole32.lib uuid.lib

REM ----------------------------------------
REM CLEAN
REM ----------------------------------------
echo Cleaning...
del /q game.obj host.obj 2>nul
del /q %SRC_DIR%\game_*.dll 2>nul
del /q %SRC_DIR%\game.pdb 2>nul
del /q %BIN_DIR%\*.pdb 2>nul

REM ----------------------------------------
REM COMPILE SOURCES
REM ----------------------------------------
echo Compiling game.c...
%CC% -c %SRC_DIR%\game.c -o game.obj %CFLAGS%
if errorlevel 1 exit /b 1

echo Compiling host.c...
%CC% -c %SRC_DIR%\host.c -o host.obj %CFLAGS%
if errorlevel 1 exit /b 1

REM ----------------------------------------
REM LINK GAME DLL (stable name for hot reload)
REM ----------------------------------------
echo Linking game.dll...
%LINKER% game.obj ^
    /DLL ^
    /OUT:%SRC_DIR%\game.dll ^
    /DEBUG ^
    /PDB:%SRC_DIR%\game.pdb ^
    /LIBPATH:%LIB_DIR% ^
    raylib.lib %SYS_LIBS%

if errorlevel 1 exit /b 1

REM ----------------------------------------
REM LINK HOST EXE
REM ----------------------------------------
echo Linking host executable...
set OUT_EXE=%BIN_DIR%\%GAME_NAME%.exe

%LINKER% host.obj ^
    /OUT:%OUT_EXE% ^
    /DEBUG ^
    /PDB:%BIN_DIR%\%GAME_NAME%.pdb ^
    /SUBSYSTEM:CONSOLE ^
    /LIBPATH:%LIB_DIR% ^
    raylib.lib %SYS_LIBS%

if errorlevel 1 exit /b 1

REM ----------------------------------------
REM DONE
REM ----------------------------------------
echo.
echo ==============================
echo Build complete!
echo EXE: %OUT_EXE%
echo PDB: %BIN_DIR%\%GAME_NAME%.pdb
echo DLL: %SRC_DIR%\game.dll
echo PDB: %SRC_DIR%\game.pdb
echo ==============================
endlocal
