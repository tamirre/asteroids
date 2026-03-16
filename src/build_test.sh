gcc -shared -fPIC game.c -o game.so \
-Ithird_party/include \
-lraylib -lm -ldl -lpthread -lGL

gcc host.c -o asteroids \
-Ithird_party/include \
-lraylib -lm -ldl -lpthread -lGL \
-rdynamic
