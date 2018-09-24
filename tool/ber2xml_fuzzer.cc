/*

## How to build clang/libFuzzer

cf. http://llvm.org/docs/LibFuzzer.html

## How to build the fuzzer

    CXXFLAGS='-fsanitize-coverage=edge -fsanitize=address -g' \
      LDFLAGS=-L$HOME/src/llvm/fuzzer/build \
      CC=$HOME/src/llvm/build/bin/clang \
      CXX=$HOME/src/llvm/build/bin/clang++  \
      cmake ../../libxfsx -DCMAKE_BUILD_TYPE=Release -G Ninja
    ninja-build ber2xml_fuzzer
    TEST_IN_BASE=../../libxfsx/test ./ber2xml_fuzzer ../corpus

Notes:

- also use other sanitizers e.g. undefined, memory
- copy some BER files into the corpus directory
  (e.g. the ones in the `test/in` directory)

   */

#include <string>
#include <deque>
#include <exception>


#include <xfsx/byte.hh>
#include <xfsx/xml_writer_arguments.hh>
#include <xfsx/ber2xml.hh>

#include <test/test.hh>

using namespace std;

extern "C" int LLVMFuzzerTestOneInput(const u8 *d, size_t n) {
  string tap_filename(test::path::in()
      + "/../../libgrammar/test/in/asn1/tap_3_12_strip.asn1");
  deque<string> asn_filenames = {tap_filename};
  xfsx::xml::Pretty_Writer_Arguments args(asn_filenames);
  xfsx::byte::writer::Memory w;
  try {
    xfsx::xml::pretty_write(d, d+n, w, args);
  } catch (std::exception &e) {
    // as long it is erroring out in a controlled fashion it is fine
  }
  return 0;  // Non-zero return values are reserved for future use.
}

// int main() { return 0; }
