/*

## How to build clang/libFuzzer

cf. http://llvm.org/docs/LibFuzzer.html

## How to build the fuzzer

    CXXFLAGS='-fsanitize-coverage=edge -fsanitize=address -g' \
      LDFLAGS=-L$HOME/src/llvm/fuzzer/build \
      CC=$HOME/src/llvm/build/bin/clang \
      CXX=$HOME/src/llvm/build/bin/clang++  \
      cmake ../../libxfsx -DCMAKE_BUILD_TYPE=Release -G Ninja
    ninja-build xml2ber_fuzzer
    TEST_IN_BASE=../../libxfsx/test ./xml2ber_fuzzer ../corpus

Notes:

- also use other sanitizers e.g. undefined, memory
- copy some BER files into the corpus directory
  (e.g. the ones in the `test/in` directory)

   */

#include <string>
#include <deque>
#include <exception>


#include <xfsx/xml2ber.hh>
#include <xfsx/tap.hh>
#include <xfsx/ber_writer_arguments.hh>

#include <test/test.hh>

using namespace std;

extern "C" int LLVMFuzzerTestOneInput(const u8 *d, size_t n) {
  string tap_filename(test::path::in()
      + "/../../libgrammar/test/in/asn1/tap_3_12_strip.asn1");
  deque<string> asn_filenames = {tap_filename};
  xfsx::BER_Writer_Arguments args;
  xfsx::tap::apply_grammar(asn_filenames, args);
  vector<u8> v;
  try {
    const char *e = reinterpret_cast<const char*>(d);
    xfsx::xml::write_ber(e, e+n, v, args);
  } catch (std::exception &e) {
    // as long it is erroring out in a controlled fashion it is fine
  }
  return 0;  // Non-zero return values are reserved for future use.
}

// int main() { return 0; }
