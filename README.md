# Jerryscript-optimizer

## Requirements

  * `clang-format` >= `10.0.0`
  * `cmake` >= `3.5.0`
  * `gcc`
  * `g++`

**Install dependencies**:

```sh
apt-get install -y \
  clang-format-10 \
  cmake \
  gcc \
  g++
```

## How to build

**Command**:

```sh
cmake -Bbuild -H. -DCMAKE_BUILD_TYPE="Debug"
make -Cbuild -j
```
