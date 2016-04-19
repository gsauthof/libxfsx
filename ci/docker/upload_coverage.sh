#!/usr/bin/bash

set -eux

src_dir="$1"
build_dir="$2"

cd "$build_dir"

bash <(curl -s https://codecov.io/bash) || echo "Codecov.io reporting failed"
