#!/bin/sh

COMPILER="gcc"
COMPILER_FLAGS="-Wall -Wextra -Wpedantic"
LINKER_FLAGS="-lGL -lX11 -lXinerama"
SOURCES="./code/knave_linux.c ./code/knave_opengl.c"
TARGET="knave"

$COMPILER $COMPILER_FLAGS -o $TARGET $SOURCES $LINKER_FLAGS
