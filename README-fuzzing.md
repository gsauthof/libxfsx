The directory `tool/` contains some fuzzers for the BER and XML
readers that use the fine [libFuzzer][1] from the LLVM/Clang
project.

[1]: http://llvm.org/docs/LibFuzzer.html

## How to build a Fuzzer

Since Clang 6, the libFuzzer is integrated with clang. Thus, it's
basically just a matter of adding `-fsanitize=fuzzer,address` and
compiling with Clang.

With cmake, one can't just add -fsanitize=fuzzer to the CXXFLAGS
because this makes all the cmake feature checks fail. Thus, our
`CMakeLists.txt` sets this switch for the fuzzer targets, only.
Actually, it sets `fsanitize=fuzzer,address,undefined`.

The actual build command is thus something like this:

    cd build-fuzz
    CXX=clang++ CXXFLAGS='-g -O1' cmake .. -DCMAKE_BUILD_TYPE=None -G Ninja
    ninja ber2xml_fuzzer xml2ber_fuzzer ...

Compiling with optimizations is important to speed up the fuzzing
and expose undefined behaviour. The libFuzzer manual uses `-O1`
in its examples, but `-O3` should also be fine.

See also the [libFuzzer manual][1].

## How to run a Fuzzer

Copy some files from test/in into a corpus directory, e.g.:

    cd build-fuzz
    mkdir corpus
    find ../test/in -type f | xargs cp -i -t corpus
    rm corpus/*xml corpus/README

Run a fuzzer:

    TEST_IN_BASE=../../libxfsx/test ./ber2xml_fuzzer corpus

See also the [libFuzzer manual][1] for more details, e.g. for
running a fuzzer in parallel (e.g. add `-jobs=3 -workers=3` to
the command line).

