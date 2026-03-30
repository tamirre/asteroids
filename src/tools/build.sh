#!/bin/bash
SECONDS=0
PLATFORM=""
REGENERATE_ATLAS=0
REGENERATE_LOCALIZATION=0
REGENERATE_AUDIO=0
while getopts ":p:a:l:s:d" opt; do
    case "$opt" in
        p) PLATFORM="$OPTARG" ;;
        a) REGENERATE_ATLAS=1 ;;
        l) REGENERATE_LOCALIZATION=1 ;;
        s) REGENERATE_AUDIO=1 ;;
		d) DEBUG=1 ;;
        :)
            echo "Option -$OPTARG requires a value"
            exit 1
            ;;
        \?)
            echo "Invalid option: -$OPTARG"
            exit 1
            ;;
    esac
done

if grep -qi microsoft /proc/version 2>/dev/null && [ "$PLATFORM" == "" ]; then
	PLATFORM="windows"
fi
export GAME_NAME=asteroids
export SRC_DIR=$(dirname $0)/../
export BIN_DIR=$(dirname $0)/../../bin
mkdir -p $BIN_DIR
if [ "$REGENERATE_LOCALIZATION" == "1" ]; then
	python $SRC_DIR/tools/genLoc.py
fi
if [ "$REGENERATE_AUDIO" == "1" ]; then
	python $SRC_DIR/tools/genAudio.py
fi
if [ "$REGENERATE_ATLAS" == "1" ]; then
	aseprite -b -script $SRC_DIR/tools/exportAtlas.lua	
	echo "Regenerated atlas"
fi
if [ "$PLATFORM" == "web" ]; then
	# WEB PLATFORM GUIDE:
	# # clone emsdk into raylib/emsdk
	# git clone https://github.com/emscripten-core/emsdk.git
	# # Enter that directory
	# cd emsdk
	# # Download and install the latest SDK tools.
	# ./emsdk install latest
	# # Make the "latest" SDK "active" for the current user. (writes .emscripten file)
	# ./emsdk activate latest --permanent
	# # Activate PATH and other environment variables in the current terminal
	# source ./emsdk_env.sh
	# # Update make file of raylib with emsdk paths and PLATFORM=PLATFORM_WEB
	# # Then in raylib/src/ run:
	# make -e PLATFORM=PLATFORM_WEB -B
	# # Python web server:
	# python -m http.server 

	export RAYLIB_PATH=~/raylib/src/
	export EMSDK_QUIET=1
	source ~/raylib/emsdk/emsdk_env.sh 

	WEB_DIR=$SRC_DIR/../web
	mkdir -p $WEB_DIR

	if [ "$DEBUG" == "1" ]; then
		DEBUG_FLAGS="-g -s ASSERTIONS=2 -s STACK_OVERFLOW_CHECK=2 -s SAFE_HEAP=1 -gsource-map"
	else
		DEBUG_FLAGS=""
	fi
	INCLUDE_FLAGS="-I. -I$RAYLIB_PATH -I$RAYLIB_PATH/external -I$SRC_DIR/third_party/include"
	LINK_FLAGS="-L. -L$RAYLIB_PATH -L$RAYLIB_PATH/web"
	DEFINES="-DPLATFORM_WEB"

	CC=emcc
	# ---------------------------
	# build SIDE MODULE (game)
	# ---------------------------
	$CC $SRC_DIR/game.c \
		-o $WEB_DIR/game.wasm \
		-s SIDE_MODULE=1 \
		$INCLUDE_FLAGS \
		$LINK_FLAGS \
		$DEBUG_FLAGS \
		$DEFINES \
		-s EXPORT_ALL=1 

	echo "Built game.wasm"

	# ---------------------------
	# build MAIN MODULE (main)
	# ---------------------------
	$CC $SRC_DIR/main.c \
		-o $WEB_DIR/index.html \
		-Wall -std=c99 -D_DEFAULT_SOURCE \
		-Wno-missing-braces \
		-s MAIN_MODULE=1 \
		$INCLUDE_FLAGS \
		$LINK_FLAGS \
		$DEBUG_FLAGS \
		$DEFINES \
		$RAYLIB_PATH/web/libraylib.web.a \
		-s USE_GLFW=3 \
		-s ASYNCIFY \
		-s STACK_SIZE=256MB \
		-s INITIAL_MEMORY=256MB \
		-s TOTAL_MEMORY=512MB \
		-s FORCE_FILESYSTEM=1 \
		--shell-file $RAYLIB_PATH/minshell.html \
		--preload-file assets/textures/atlas \
		--preload-file assets/audio \
		--preload-file assets/fonts \
		--preload-file src/shaders \
		-s EXPORT_ALL=1 

	echo "Built index.html"

	# ZIP FOR ITCH.IO
	zip -r ${GAME_NAME}_web.zip web/
elif [ "$PLATFORM" == "windows" ]; then
	CC=x86_64-w64-mingw32-gcc

	if [ "$DEBUG" == "1" ]; then
		DEBUG_FLAGS="-g -g3"
	# else 
		# DEBUG_FLAGS="-O2"
	fi


	DEFINES="-DPLATFORM_WINDOWS=1"
	INCLUDE_FLAGS="-I$SRC_DIR/third_party/include"
	LIB_DIR="$SRC_DIR/third_party/lib"

	# Windows raylib + system libs
	# LINK_FLAGS="-L$LIB_DIR -lraylib -lopengl32 -lgdi32 -lwinmm"
	LINK_FLAGS="-L$LIB_DIR -lraylib -lopengl32 -lgdi32 -lwinmm"

	mkdir -p $BIN_DIR

	rm $SRC_DIR/game_*.dll 2> /dev/null

	# ---------------------------
	# build DLL (game)
	# ---------------------------
	$CC $SRC_DIR/game.c \
		-shared -o $SRC_DIR/game_tmp.dll \
		$DEFINES \
		$DEBUG_FLAGS \
		$INCLUDE_FLAGS \
		$LINK_FLAGS 

	# ---------------------------
	# build EXE (main)
	# ---------------------------
	OUT_EXE="$BIN_DIR/${GAME_NAME}.exe"

	$CC $SRC_DIR/main.c \
		-o $OUT_EXE \
		$DEFINES \
		$DEBUG_FLAGS \
		$INCLUDE_FLAGS \
		$LINK_FLAGS 

	# ensure replace
	mv $SRC_DIR/game_tmp.dll $SRC_DIR/game.dll
	rm ./*.obj 2> /dev/null
	echo "Built game.dll"
	echo "Built Windows executable: $OUT_EXE"
else
	if [ "$DEBUG" == "1" ]; then
		DEBUG_FLAGS="-g -g3"
	else 
		DEBUG_FLAGS=""
	fi
	INCLUDE_FLAGS="-I$SRC_DIR/third_party/include"
	LINK_FLAGS="-lraylib -lm -ldl -lpthread -lGL"
	CC=gcc

	rm $SRC_DIR/game_*.so 2> /dev/null
	# Write to game_tmp.so instead of game.so. 
	# We need to do this because otherwise main.c will
	# try to load the .so before it is fully written, since we only
	# check the timestamp
	$CC $DEBUG_FLAGS -shared -fPIC $SRC_DIR/game.c -o $SRC_DIR/game_tmp.so \
		$INCLUDE_FLAGS \
		$LINK_FLAGS

	# This is needed so the game.so is only finished when gcc is done
	# game.so is then copied by main.c to load into the game
	mv $SRC_DIR/game_tmp.so $SRC_DIR/game.so

	$CC $DEBUG_FLAGS $SRC_DIR/main.c -o $BIN_DIR/$GAME_NAME \
		$INCLUDE_FLAGS \
		$LINK_FLAGS \
		-rdynamic

fi

seconds=$SECONDS
ELAPSED="Total time: $(($seconds / 3600))hrs $((($seconds / 60) % 60))min $(($seconds % 60))sec"
echo $ELAPSED
