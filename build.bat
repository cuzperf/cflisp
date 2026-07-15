@echo off

rmdir /S /Q build > NUL 2>&1

set "FILE=examples/test.lsp"
if not exist "%FILE%" (
  > "%FILE%" echo ; ingored by git, edit for debug
)

cmake -B build -G "Visual Studio 17 2022" -DExampleID=test.lsp
cmake --build build --config Debug --parallel 4
build\Debug\unit_test
@REM cp build\Debug\cflisp.exe cflisp.exe

start build\cflisp.sln
