#!/usr/bin/sh
cmake -G"Visual Studio 17 2022" -B build-msvc/ -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cp -r csv/ build-msvc/
cp -r resources/ build-msvc/
cp raylib-5.5_win64_msvc16/lib/raylib* build-msvc/
