rm -rf build

cmake -B build -G "Xcode" -DCMAKE_BUILD_TYPE=Debug
cmake --build build  --parallel 4
build/unit_test
cmake --build build
cp build/Debug/cflisp cflisp

open build/*.xcodeproj
