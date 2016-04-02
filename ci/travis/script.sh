#!/bin/bash

set -eux

function run_osx()
{
  cd build
  ninja -j2 bed xfsx xfsx_static check
}

case $TRAVIS_OS_NAME in
  osx)
    run_osx
    ;;
  linux)
    ;;
esac
