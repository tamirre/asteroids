@echo off
subst w: d:\programming
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
set path=w:\programming\;%path%
CD /D w:
