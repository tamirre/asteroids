#!/bin/bash
SECONDS=0
PLATFORM=""
REGENERATE_ATLAS=0
while getopts ":p:a" opt; do
    case "$opt" in
        p) PLATFORM="$OPTARG" ;;
        a) REGENERATE_ATLAS=1 ;;
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
python genLoc.py

if [ "$REGENERATE_ATLAS" == "1" ]; then
	aseprite -b -script ./assets/tools/exportAtlas.lua	
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
mkdir -p web
source ~/raylib/emsdk/emsdk_env.sh 
emcc -o web/index.html asteroids.c \
	-Wall -std=c99 -D_DEFAULT_SOURCE \
	-Wno-missing-braces -Wunused-result -Os \
	-I. -I $RAYLIB_PATH \
	-I $RAYLIB_PATH/external \
	-L. -L $RAYLIB_PATH \
	-s STACK_SIZE=256MB \
	-s INITIAL_MEMORY=256MB \
	-s USE_GLFW=3 -s ASYNCIFY -s TOTAL_MEMORY=512MB -s FORCE_FILESYSTEM=1 \
	--shell-file ~/raylib/src/shell.html $RAYLIB_PATH/web/libraylib.web.a \
	-DPLATFORM_WEB -s 'EXPORTED_FUNCTIONS=["_free","_malloc","_main"]' -s EXPORTED_RUNTIME_METHODS=ccall \
	--preload-file assets/atlas \
	--preload-file audio \
	--preload-file fonts \
	--preload-file shaders 
	# -s ASSERTIONS=2 -g
zip -r ${GAME_NAME}_web.zip web/
else 
	# gcc asteroids.c -Wall -o $GAME_NAME -Ithird_party/include -lraylib -lm -ldl -lpthread -lGL -fsanitize=address -g 
	gcc asteroids.c -Wall -o $GAME_NAME -Ithird_party/include -lraylib -lm -ldl -lpthread -lGL
fi

seconds=$SECONDS
ELAPSED="Total time: $(($seconds / 3600))hrs $((($seconds / 60) % 60))min $(($seconds % 60))sec"
echo $ELAPSED
