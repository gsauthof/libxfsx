#!/bin/bash

set -eux


function prepare_osx()
{
  # BOOST_ROOT doesn't have to be explicitly set on OS X since
  # Homebrew directly links it under /usr/local

  mkdir build
  cd build

  CXX=$MY_CXX CC=$MY_CC cmake -G Ninja -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE \
    -DCMAKE_PREFIX_PATH=/usr/local/lua/5.2.4_3 \
    ..
}

function prepare_linux()
{
  : ${docker_image_b:=$docker_image}

  if [ "$docker_image" != "$docker_image_b" ] ; then
    docker stop cxx-devel
    docker rm cxx-devel

    build="$HOME"/build/libxfsx
    src="$PWD"/..

    docker create --name cxx-devel  \
      -v "$src":/srv/src:ro,Z \
      -v "$build":/srv/build:Z \
      $docker_image_b

    docker start cxx-devel
  fi
}

prepare_"$TRAVIS_OS_NAME"
