/*

## How to build clang/libFuzzer

cf. http://llvm.org/docs/LibFuzzer.html

## How to build the fuzzer

    CXXFLAGS='-fsanitize-coverage=edge -fsanitize=address -g' \
      LDFLAGS=-L$HOME/src/llvm/fuzzer/build \
      CC=$HOME/src/llvm/build/bin/clang \
      CXX=$HOME/src/llvm/build/bin/clang++  \
      cmake ../../libxfsx -DCMAKE_BUILD_TYPE=Release -G Ninja
    ninja-build ber2lxml_back_fuzzer
    TEST_IN_BASE=../../libxfsx/test ./ber2lxml_back_fuzzer ../corpus

or even

    TEST_IN_BASE=../../libxfsx/test ./ber2lxml_back_fuzzer -detect_leaks=0 ../corpus

Notes:

- also use other sanitizers e.g. undefined, memory
- copy some BER files into the corpus directory
  (e.g. the ones in the `test/in` directory)

   */

#include <string>
#include <deque>
#include <vector>
#include <algorithm>
#include <exception>


#include <xfsx/byte.hh>
#include <xfsx/ber2xml.hh>
#include <xfsx/xml_writer_arguments.hh>
#include <xfsx/ber2lxml.hh>
#include <xfsx/lxml2ber.hh>
#include <xfsx/tap.hh>
#include <xfsx/ber_writer_arguments.hh>
#include <xxxml/xxxml.hh>

#include <test/test.hh>

using namespace std;

static void compare_bers(
    const pair<const uint8_t*, const uint8_t*> &a,
    const pair<const uint8_t*, const uint8_t*> &b,
    const xfsx::xml::Pretty_Writer_Arguments &args)
{
  xfsx::byte::writer::Memory x;
  xfsx::byte::writer::Memory y;
  xfsx::xml::pretty_write(a.first, a.second, x, args);
  xfsx::xml::pretty_write(b.first, b.second, y, args);
  if (x.written() != y.written())
    throw length_error("Input XML length " + to_string(x.written()) 
        + " != output XML length " + to_string(y.written()));
  if (!equal(x.begin(), x.end(), y.begin(), y.end()))
    throw domain_error("Resulting XML differs from input XML");
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *d, size_t n) {
  // Disabling the ASN1 usage to avoid false-positives in the
  // XML comparison - e.g. when (by chance) a primitive tag
  // is generated with a constructed tag number
  // string tap_filename(test::path::in()
  //     + "/../../libgrammar/test/in/asn1/tap_3_12_strip.asn1");
  // deque<string> asn_filenames = {tap_filename};
  // xfsx::xml::Pretty_Writer_Arguments args(asn_filenames);
  xfsx::xml::Pretty_Writer_Arguments args;
  xfsx::BER_Writer_Arguments wargs;
  // xfsx::tap::apply_grammar(asn_filenames, wargs);
  vector<uint8_t> v;
  try {
    xxxml::doc::Ptr doc = xfsx::xml::l2::generate_tree(d, d+n, args);
    xfsx::xml::l2::write_ber(doc, v, wargs);
  } catch (std::exception &e) {
    // as long it is erroring out in a controlled fashion it is fine
    return 0;
  }
  compare_bers(make_pair(d, d+n), make_pair(v.data(), v.data()+v.size()), args);
  return 0;  // Non-zero return values are reserved for future use.
}

// int main() { return 0; }
