#!/bin/bash

set -eux

function run_osx()
{
  cd build
  ninja -j2 bed xfsx xfsx_static ut
  #ninja -j2 ut

  set +e
  ./ut --catch_system_errors=no
  r=$?
  set -e

  # which lldb
  # -> /usr/bin/lldb
  # lldb --help
  # -> understands --one-line etc.
  # ls -l /usr/bin/lldb
  # -> not a symlink - perhaps a hardlink?

  # we can avoid this dance when lldb supports something like `quit rc`
  # and `$_exitcode`, e.g. `quit $_exitcode` as gdb understands it
  if [ "$r" -gt 127 ]; then
      # tested with xcode10 - cf. .travis.yml
      PATH=/usr/bin:$PATH lldb --one-line r --one-line bt --one-line quit \
          -- ./ut --catch_system_errors=no
      exit $r
  fi
}

function run_linux()
{
  docker exec cxx-devel \
    /srv/src/libxfsx/ci/docker/run.sh \
    /srv/src/libxfsx \
    /srv/build/libxfsx \
    "$build_tag"
}

run_$TRAVIS_OS_NAME
