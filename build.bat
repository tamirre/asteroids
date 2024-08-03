@echo off

rem SET PATH=%PATH%;%VULKAN_SDK%/Include
rem echo %PATH%
SET includes=/Isrc /I%VULKAN_SDK%/Include 
SET links=/link /LIBPATH:%VULKAN_SDK%/Lib vulkan-1.lib User32.lib
SET defines=/D DEBUG=1 /Fo"..\\..\\build\\" /Fe:"..\\..\\bin\\game.exe" 
SET src=%~dp0\src\platform\win32_platform.cpp

SET out=%~dp0\build\
SET bin=%~dp0\bin\

IF NOT EXIST %out% mkdir %out%
IF NOT EXIST %bin% mkdir %bin%

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
cl /EHsc /Z7 %includes% %defines% %src% %links% 

