#!/bin/bash

set -eux

function prepare_osx()
{
  # BOOST_ROOT doesn't have to be explicitly set on OS X since
  # Homebrew directly links it under /usr/local

  mkdir build
  cd build

  CXX=$MY_CXX CC=$MY_CC cmake -G Ninja -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE ..
}

function prepare_linux()
{
}

case $TRAVIS_OS_NAME in
  osx)
    prepare_osx
    ;;
  linux)
    prepare_linux
    ;;
esac
