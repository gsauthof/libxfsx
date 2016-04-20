#!/usr/bin/bash

set -eux

src_dir="$1"
build_dir="$2"

cd "$build_dir"

# workaround coverage.cmake being identified as coverage report
# workaround script only searching in the build directory for reports
bash <(curl -s https://codecov.io/bash | sed '/-not -name .*coverage.txt/a -not -name '"'"'*.cmake'"'"' \\' | sed 's@files=.*find.*git_root.*coverage.*$@files=$(find "$proj_root" -type f \\( -name '"'"'*coverage.*'"'"' \\@' ) \
  -R "$src_dir" -p "$build_dir" \
  || echo "Codecov.io reporting failed"
