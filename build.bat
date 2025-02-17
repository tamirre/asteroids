@echo off

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" amd64

set OUT_DIR=%~dp0\build
set BIN_DIR=%~dp0\bin
set OUT_EXE=asteroids
rem 
set INCLUDES=/I%~dp0\third_party\include 
set LIBS=/LIBPATH:%~dp0\third_party\lib raylib.lib winmm.lib kernel32.lib gdi32.lib user32.lib shell32.lib
set SOURCES=%~dp0\asteroids.c 
IF NOT EXIST %OUT_DIR% mkdir %OUT_DIR%
IF NOT EXIST %BIN_DIR% mkdir %BIN_DIR%
rem /O2 /Ob3 /G 
cl /nologo /Zi /MD /EHsc %INCLUDES% %SOURCES% /Fe%BIN_DIR%/%OUT_EXE%.exe /Fo%OUT_DIR%/ /link %LIBS%
