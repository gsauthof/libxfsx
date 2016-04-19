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
  docker exec cxx-devel \
    /srv/src/libxfsx/ci/docker/upload_coverage.sh \
    /srv/src/libxfsx \
    /srv/build/libxfsx \
    "$build_tag"
}

run_$TRAVIS_OS_NAME
