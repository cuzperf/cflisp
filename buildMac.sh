rm -rf build

git submodule update --init --recursive
cmake -B build -G "Xcode" -DCMAKE_BUILD_TYPE=Debug
cmake --build build
build/unit_test
cmake --build build
cp build/Debug/cflisp cflisp

open build/*.xcodeproj
