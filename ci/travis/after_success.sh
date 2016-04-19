#!/bin/bash

set -eux


if [[ ! "$CMAKE_BUILD_TYPE" =~ "Coverage" ]]; then
  exit 0
fi


function run_osx()
{
  cd build
  bash <(curl -s https://codecov.io/bash) || echo "Codecov.io reporting failed"
}

function run_linux()
{
  :
  # run in docker ...
  #bash <(curl -s https://codecov.io/bash) -p
}

run_$TRAVIS_OS_NAME