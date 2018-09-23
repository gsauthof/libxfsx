#!/bin/bash

set -eux

: ${build_tag:=none}

function prepare_osx()
{
  # apparently there are sudo-wrappers for brew
  # thus, sudo can be omitted
  brew update

  # as of 2018-09, cmake recent enough: 3.11.4
  # brew upgrade cmake
  brew info cmake

  brew install ragel
  brew install ninja
  brew install libxml2
  brew install lua
  # find /usr/local -name '*lua*'
  # i.e. is linked via /usr/local/{lib,include}
  # ls -ld /usr/local/include/lua
  # -> ../Cellar/lua/5.3.5_1/include/lua
  # ls -ld /usr/local/include/lua5.3
  # -> ../Cellar/lua/5.3.5_1/include/lua5.3
  # ls -ld /usr/local/Cellar/lua/5.3.5_1/include/lua
  # -> no symlink, perhaps hardlink?
  # ls -ld /usr/local/Cellar/lua/5.3.5_1/include/lua5.3
  # -> no symlink

  # as of 2018-09, default boost is at 1.67
  # note that brew upgrade exits with exit status != 0
  # if the latest version is already installed ...
  # (whereas brew install just issues a warning and does nothing
  # if a package is already installed ...)
  # brew upgrade boost
  brew info boost

  # this would give us 5.3 (or higher)
  # but this gcc then didn't link correctly with boost in ~ 2016
  # brew unlink gcc
  # brew install gcc

  # cf. http://apple.stackexchange.com/questions/227026/how-to-install-recent-clang-with-homebrew
  # as of 2018-09, homebrew/versions isn't a thing, anymore
  # and the default xcode clang travis includes is recent enough
  # brew install llvm
  # brew info llvm
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
