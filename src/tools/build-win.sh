#!/bin/bash
SRC_DIR=~/asteroids/src/shaderTest
RAYLIB_PATH=~/raylib/src
RAYLIB_WIN_PATH=~/raylib/src/win
OUT_DIR=~/asteroids/src/shaderTest

x86_64-w64-mingw32-gcc \
	$SRC_DIR/main.c \
	-o $OUT_DIR/game.exe \
	-I$RAYLIB_PATH \
	-L$RAYLIB_WIN_PATH\
	-lraylib \
	-lopengl32 \
	-lgdi32 \
	-lwinmm
