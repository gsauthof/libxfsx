One way to build libxfsx for Windows is to use the [Mingw64][mingw64]
compiler distribution. It includes the GCC, it is open source and it
provides lightweight wrappers for some POSIX/UNIX-like functions. Although
it is named Mingw64, it also supports the creation of 32 Bit binaries.

In contrast to Cygwin, it is more lightweight - i.e. it doesn't emulate
API's that are very foreign to windows and thus costly to implement (e.g.
fork(), link() etc.). Also, one doesn't have to link against a GPL'ed
runtime library.

Mingw64 is even available as a cross-compiler under Linux. Several
distributions package it. For example, the Fedora support is very good,
several bread and butter packages like Boost, libxml2, Qt etc. are all also
provided as Mingw64 and Mingw64-32 versions from the main package
repositories.

2016, Georg Sauthoff <mail@georg.so>

## Build Example

For example on Fedora, to cross-compile it:

    mkdir build-win
    cd build-win
    mingw64-cmake .. -DCMAKE_BUILD_TYPE=Release
    mingw64-make ut bed

A first test can be done under [Wine][wine]:

    wine64 ut

In case the mingw libraries can't be found:

    vim ~/.wine/system.reg
    # -> add ;Z:\\usr\\x86_64-w64-mingw32\\sys-root\\mingw\\bin"
    # to the PATH defintion

A fresh local wine configuration directory can be configured like this
(e.g. useful for testing a 32 Bit version in parallel):

    WINEPREFIX=~/.wine32 wine32 ut

The dependencies of the cross-compiled binary can be displayed via
[peldd][peldd]. For example, to deploy the binary with the needed dlls:

    peldd ut.exe -a | xargs cp -t /mnt/win/builds/

The resulting directory could contain following files:

    iconv.dll
    libboost_filesystem-mt.dll
    libboost_regex-mt.dll
    libboost_system-mt.dll
    libgcc_s_seh-1.dll
    libstdc++-6.dll
    libwinpthread-1.dll
    libxml2-2.dll
    zlib1.dll
    ut.exe


[mingw64]: http://mingw-w64.org/doku.php
[wine]: https://www.winehq.org/
[peldd]: https://github.com/gsauthof/pe-util
[mingw64]: http://mingw-w64.org/doku.php
