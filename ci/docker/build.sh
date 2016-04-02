#!/usr/bin/bash

set -x
set -e
set -u

: ${CMAKE_BUILD_TYPE:=Sanitize}
# -j1 : be nice about CI environments with low memory ...
: ${jobs:=3}
: ${targets:=bed xfsx xfsx_static ut}


src_dir="$1"
build_dir="$2"

mkdir -p "$build_dir"
cd "$build_dir"

cmake -G Ninja -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" "$src_dir"
ninja-build -j"$jobs" $targets
