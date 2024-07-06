@echo off

IF NOT EXIST w:\build mkdir w:\build
pushd w:\build
cl -D INTERNAL=1 -D DEBUG=1 -FC -Zi w:\src\win32_test.cpp user32.lib Gdi32.lib
popd
