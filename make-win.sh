#!/usr/bin/sh
cmake -B build/ -GNinja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON $@
cp -r csv/ build/
cp -r resources/ build/
cp raylib-5.5_win64_mingw20241203/lib/raylib* build/

