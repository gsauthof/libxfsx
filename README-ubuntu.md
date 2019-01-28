Compiling libxfsx under Ubuntu isn't really that special. The
most challenging part is perhaps to install the right Lua
development package. This is because Ubuntu usually provides
multiple versions of Lua and thus works with path suffixes. The
CMakeLists.txt file contains some path suffixes for Lua, but
can't possibly contain all past and future suffixes.

2019, Georg Sauthoff

## Ubuntu 16.04 LTS Build Environment

For example, with Ubuntu 16, one need to install the following
packages as build dependencies:

    apt-get install cmake g++ git libboost-dev libboost-filesystem-dev \
        libboost-regex-dev libboost-test-dev liblua5.3-dev libxml2-dev \
        ninja-build ragel

The ninja-build package is optional, but ninja is just faster
than make.
