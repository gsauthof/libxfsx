#!/bin/bash

set -eux


function prepare_osx()
{
  # apparently there are sudo-wrappers for brew
  # thus, sudo can be omitted
  brew update
  brew tap homebrew/versions

  brew install ragel
  brew install ninja
  brew install libxml2

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


case $TRAVIS_OS_NAME in
  osx)
    prepare_osx
    ;;
  linux)
    ;;
esac
