Building the project under some UNIX (TM) shouldn't be that
different than building it under Linux - assuming availability of
a C++11/C++14 compiler and reasonable POSIX conformance.

Challenges may arise due to lack of a good package manager for
installing dependencies and a uncommon environment.

2016, Georg Sauthoff <mail@georg.so>

## Solaris 10 Example

In general, install as much dependencies via [OpenCSW][csw], as
possible - e.g. GCC 4.9. The Solaris Studio 12.3/12.4 compiler
doesn't support enough C++11/14 to compile this project and Boost.

Use a clean environment:

    unset LD_LIBRARY_PATH LD_LIBRARY_PATH_32 LD_LIBRARY_PATH_64

Use as much standard conforming and/or modern commands as
possible:

    export PATH=/opt/csw/gnu:/opt/csw/bin:\
        /usr/xpg6/bin:/usr/xpg4/bin:/usr/ccs/bin:/usr/bin

The CMake call:

    BOOST_ROOT=/path/to/boost CFLAGS="-m64 -D_XOPEN_SOURCE=600" \
      CXXFLAGS="-m64 -D_XOPEN_SOURCE=600" CC=gcc CXX=g++ \
      cmake /path/to/project/src  -DCMAKE_BUILD_TYPE=Release

In detail:

- `-m64` - compilers like GCC generate 32 Bit code, by default, on
  Solaris
- `-D_XOPEN_SOURCE=600` - otherwise, newer POSIX API isn't
  available. This is a C pre-processor flag, but
  CMake doesn't pick up CPPFLAGS, yet
- `CC=gcc`, `CXX=g++` - otherwise, a default compiler linked
  via `/usr/bin/{cc,CC}` has higher priority


The actual compile:

    make

or:

    gmake

to make sure that GNU make is used

[csw]: https://en.wikipedia.org/wiki/OpenCSW
