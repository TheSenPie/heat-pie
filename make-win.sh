#!/usr/bin/sh
cmake -B build/ -GNinja
cp -r csv/ build/
cp raylib-5.5_win64_msvc16/lib/raylib* build/
