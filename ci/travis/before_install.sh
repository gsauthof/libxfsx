#!/bin/bash

set -eux

: ${build_tag:=none}

function prepare_osx()
{
  # apparently there are sudo-wrappers for brew
  # thus, sudo can be omitted
  brew update
  brew tap homebrew/versions

  brew unlink cmake
  brew install cmake
  brew info cmake

  brew install ragel
  brew install ninja
  brew install libxml2
  brew install lua
  #find /usr/local -name '*lua*'
  # i.e. is linked via /usr/local/{lib,include}

  # brew complains:
  # Error: boost-1.55.0_2 already installed
  # To install this version, first `brew unlink boost`
  # ( -> we need at least 1.58)
  brew unlink boost
  brew install boost

  # this would give us 5.3 (or higher)
  # but this gcc then doesn't link correctly with boost ...
  # brew unlink gcc
  # brew install gcc

  # cf. http://apple.stackexchange.com/questions/227026/how-to-install-recent-clang-with-homebrew
  if [ "$MY_CXX" = clang++-3.7 ]; then
    brew install llvm37
  fi
}

function prepare_linux()
{
  # as of 2016-06, there is no fedora mingw lua package, yet
  if [ "$build_tag" = mingw64 ] ; then
    git clone --branch lua-5.3 https://github.com/LuaDist/lua.git
  fi

  build="$HOME"/build/libxfsx
  src="$PWD"/..

  mkdir -p $build
  chmod 770 "$HOME"/build
  # The compile-user inside the docker image has uid 1000, where ubuntu,
  # by default uses 1001 - thus, we have to make it world-writable
  # Because of that it is put into a 770 parent
  chmod 777 "$build"

  # Travis' Ubuntu probably doesn't come with SELinux enabled, but
  # shouldn't hurt
  docker create --name cxx-devel \
    -v "$src":/srv/src:ro,Z \
    -v "$build":/srv/build:Z \
    $docker_image

  docker start cxx-devel

  sleep 4

  docker ps
}

prepare_$TRAVIS_OS_NAME
