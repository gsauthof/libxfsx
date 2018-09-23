#!/bin/bash

set -eux


function prepare_osx()
{
  # BOOST_ROOT doesn't have to be explicitly set on OS X since
  # Homebrew directly links it under /usr/local

  mkdir build
  cd build

  # Not necessary:
  #
  # -DCMAKE_PREFIX_PATH=
  #
  # system path is:
  #
  # CMAKE_SYSTEM_PREFIX_PATH: /usr/local;/usr;/;/usr/local/Cellar/cmake/3.11.4;/usr/local;/usr/X11R6;/usr/pkg;/opt;/sw;/opt/local
  #
  # Thus, cmake should find /usr/local/lib/liblua.dylib
  # and /usr/local/include/lua just with an extra suffix hint

  cxxflagsw="-O1 -Wall -Wextra -Wno-missing-field-initializers \
    -Wno-parentheses -Wno-missing-braces \
    -Wno-unused-local-typedefs \
    -Wfloat-equal \
    -Wpointer-arith -Wcast-align \
    -Wnull-dereference \
    -Wnon-virtual-dtor -Wmissing-declarations \
    -Werror=multichar -Werror=sizeof-pointer-memaccess -Werror=return-type \
    -Werror=delete-non-virtual-dtor \
    -fstrict-aliasing"

  CXXFLAGS="$MY_CXXFLAGS $cxxflagsw" CXX=$MY_CXX CC=$MY_CC cmake -G Ninja \
      -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE ..
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
