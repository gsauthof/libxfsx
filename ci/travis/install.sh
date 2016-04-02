#!/bin/bash

set -eux

# not really install, but obtain some infos from the environment
function install_osx()
{
  sw_vers
  uname -a

  # find /usr/local/ '(' -type f -or -type l ')'  '(' -name 'g++*' -or -name 'clang++*' ')'
  # find /usr/local/ -name 'libboost*' -ls
  # brew --prefix boost
  # brew cat boost
  # ls -l /usr/local/opt/boost/lib/libboost_regex-mt.dylib
  # nm -gU /usr/local/opt/boost/lib/libboost_regex-mt.dylib
}

case $TRAVIS_OS_NAME in
  osx)
    install_osx
    ;;
  linux)
    ;;
esac
