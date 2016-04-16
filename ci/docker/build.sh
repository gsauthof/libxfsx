#!/usr/bin/bash

set -eux

: ${CMAKE_BUILD_TYPE:=Sanitize}
# -j1 : be nice about CI environments with low memory ...
: ${jobs:=3}
: ${targets:=bed xfsx xfsx_static ut}


src_dir="$1"
build_dir="$2"
tag=linux
if [ $# -gt 2 ] ; then
  tag="$3"
fi


function build_linux()
{
  cmake -G Ninja -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" "$src_dir"
  ninja-build -j"$jobs" $targets
}

function build_mingw64()
{
  # mingw does not support this build type
  if [ "$CMAKE_BUILD_TYPE" = Sanitize ]; then
    CMAKE_BUILD_TYPE=Debug
  fi
  mingw64-cmake -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" "$src_dir"
  mingw64-make -j"$jobs" $targets
}

mkdir -p "$build_dir"
cd "$build_dir"

build_"$tag"