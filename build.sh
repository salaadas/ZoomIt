#!/bin/sh

set -e

g++ -Wall -Wextra -ggdb -std=c++0x -I/usr/include/SDL2/ zoomit.cpp -o zoomit -lX11 -lSDL2 -lGL -lGLEW -lGLU
