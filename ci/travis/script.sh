#!/bin/bash

set -eux

function run_osx()
{
  cd build
  ninja -j2 bed xfsx xfsx_static check
}

function run_linux()
{
  docker exec cxx-devel \
    /srv/src/libxfsx/ci/docker/run.sh \
    /srv/src/libxfsx \
    /srv/build/libxfsx
}

run_$TRAVIS_OS_NAME
