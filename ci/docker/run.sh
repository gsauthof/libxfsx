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
tag=linux
if [ $# -gt 2 ] ; then
  tag="$3"
fi


function run_linux()
{
  # suppress 'leaks' from system libraries
  export LSAN_OPTIONS=suppressions="$src_dir"/ci/leak.sup

  ./ut && true
  : $((r+=$?))
}

function run_mingw64()
{
  rm -rf ~/.wine
  # just let it fail once such that ~/.wine is populated ...
  winepath
  # TERM=vt100 wineconsole --backend=curses ./ut && true
  pwd
  # otherwise $HOME/.wine/system.reg might not be there yet ...
  sync
  sleep 3
  sed 's/^\("PATH".*\)"$/\1;Z:\\\\usr\\\\x86_64-w64-mingw32\\\\sys-root\\\\mingw\\\\bin"/' -i $HOME/.wine/system.reg

  TERM=vt100 wineconsole --backend=curses ./ut && true
  : $((r+=$?))
}


cd "$build_dir"

export TEST_IN_BASE="$src_dir"/test

r=0

run_"$tag"


exit $((r>127?127:r))
