#!/usr/bin/bash

set -x
set -e
set -u

# clean the environment, we don't want that stuff like DBUS*, XDG_*, DESKTOP_SESSION etc. is leaked
[ -v HOME ] && exec -c "$0" "$@"

mkdir -p fake_home

export HOME=$PWD/fake_home

src_dir="$1"
build_dir="$2"

cd "$build_dir"

export TEST_IN_BASE="$src_dir"/test

r=0

# suppress 'leaks' from system libraries
export LSAN_OPTIONS=suppressions="$src_dir"/ci/leak.sup

./ut && true
: $((r+=$?))


exit $((r>127?127:r))
