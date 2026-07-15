rm -rf build

cmake -B build -DCOVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 4
ulimit -s 16384
build/unit_test
cmake --build build --target coverage
# cp build/cflisp cflisp
