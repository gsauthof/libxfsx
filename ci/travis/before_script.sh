#!/bin/bash

set -eux

: ${docker_image_b:=$docker_image}

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
  if [ "$docker_image" != "$docker_image_b" ] ; then
    docker stop cxx-devel
    docker rm cxx-devel

    docker create --name cxx-devel  \
      -v "$src":/srv/src:ro,Z \
      -v "$build":/srv/build:Z \
      $docker_image_b

    docker start cxx-devel
  fi
}

case $TRAVIS_OS_NAME in
  osx)
    prepare_osx
    ;;
  linux)
    prepare_linux
    ;;
esac
