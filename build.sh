#!/bin/bash

gcc asteroids.c -Wall -o asteroids -Ithird_party/include -lraylib -lm -ldl -lpthread -lGL
