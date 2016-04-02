#!/bin/bash

set -eux

function run_osx()
{
}

function run_linux()
{
  # for travis, it is not really necessary to do some cleanup,
  # because travis wipes the environment anyway, after each job

  docker stop cxx-devel

  docker rm cxx-devel
}

run_$TRAVIS_OS_NAME
