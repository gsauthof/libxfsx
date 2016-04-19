#!/usr/bin/bash

set -eux

src_dir="$1"
build_dir="$2"

cd "$build_dir"

bash <(curl -s https://codecov.io/bash) \
  -R "$src_dir" -p "$build_dir" \
  || echo "Codecov.io reporting failed"
