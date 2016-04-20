#!/usr/bin/bash

set -eux

src_dir="$1"
build_dir="$2"

cd "$build_dir"

# workaround coverage.cmake being identified as coverage report
bash <(curl -s https://codecov.io/bash | sed '/-not -name .*coverage.txt/a -not -name '"'"'*.cmake'"'"' \\') \
  -R "$src_dir" -p "$build_dir" \
  || echo "Codecov.io reporting failed"
