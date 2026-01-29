#!/bin/bash

PLATFORM=""
while getopts ":p:" opt; do
    case "$opt" in
        p) PLATFORM="$OPTARG" ;;
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

export GAME_NAME=asteroids
if [ "$PLATFORM" == "web" ]; then
# WEB PLATFORM:
export RAYLIB_PATH=~/raylib/src/
export EMSDK_QUIET=1
mkdir -p web
source ~/emsdk/emsdk_env.sh 
emcc -o web/$GAME_NAME.html asteroids.c \
	-Wall -std=c99 -D_DEFAULT_SOURCE \
	-Wno-missing-braces -Wunused-result -Os \
	-I. -I $RAYLIB_PATH \
	-I $RAYLIB_PATH/external \
	-L. -L $RAYLIB_PATH \
	-s USE_GLFW=3 -s ASYNCIFY -s TOTAL_MEMORY=67108864 -s FORCE_FILESYSTEM=1 \
	--shell-file ~/raylib/src/shell.html $RAYLIB_PATH/web/libraylib.web.a \
	-DPLATFORM_WEB -s 'EXPORTED_FUNCTIONS=["_free","_malloc","_main"]' -s EXPORTED_RUNTIME_METHODS=ccall \
	-s MIN_WEBGL_VERSION=2 \
	-s MAX_WEBGL_VERSION=2 \
	-s FULL_ES3=1 \
	--preload-file assets \
	--preload-file audio \
	--preload-file fonts \
	--preload-file shaders 
else 
	# gcc -fsanitize=address -g $GAME_NAME.c -Wall -o asteroids -Ithird_party/include -lraylib -lm -ldl -lpthread -lGL
	gcc asteroids.c -Wall -o $GAME_NAME -Ithird_party/include -lraylib -lm -ldl -lpthread -lGL
fi
