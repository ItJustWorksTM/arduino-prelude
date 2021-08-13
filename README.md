# `arduino-prelude`
_SMCE's replacement for Arduino's ctags-based preprocessor_

## Usage

Invoking
`arduino-prelude path/to/sketch.ino`
will output on stdout a prelude for the sketch.

This prelude can then be force-included (GNU's `-include`, MSVC's `/FI`) on your sketch compilation command.

## How to build

Requirements:
- LLVM >= 12
- C++20 compiler

```
cmake -S . -B build
cmake --build ./build
```
