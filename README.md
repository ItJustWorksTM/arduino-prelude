# `ardpp`
_SMCE's replacement for Arduino's ctags-based preprocessor_

## Usage

Invoking
`ardpp path/to/sketch.ino`
will output on stdout a prelude for the sketch.

_We are fully aware that this is not the behavior we want to replicate - this is just a temporary._  
_The proper behavior is to print the whole original source file, with the prelude inserted right before the first function{,&blank;template} definition_

## How to build

Requirements:
- LLVM >= 12
- C++17 compiler

```
cmake -S . -B build
cmake --build ./build
```
