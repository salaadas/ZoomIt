#!/bin/sh

set -e

g++ -Wall -Wextra -ggdb -std=c++11 -I/usr/include/SDL2/ zoomit.cpp -o zoomit  -lX11 -lXrandr -lSDL2 -lGL
