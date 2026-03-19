#!/bin/bash
SRC_DIR=~/asteroids/src/shaderTest
RAYLIB_PATH=~/raylib/src
RAYLIB_WIN_PATH=~/asteroids/src/third_party/lib
OUT_DIR=~/asteroids/src/shaderTest
INCLUDE_PATH=$SRC_DIR/../third_party/include
LIB_PATH=$SRC_DIR/../third_party/lib

x86_64-w64-mingw32-gcc \
	$SRC_DIR/main.c \
	-o $OUT_DIR/game.exe \
	-I$INCLUDE_PATH \
	-L$LIB_PATH \
	-lraylib \
	-lopengl32 \
	-lgdi32 \
	-lwinmm
