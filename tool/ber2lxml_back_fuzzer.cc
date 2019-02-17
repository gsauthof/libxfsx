// See README-fuzzing.md for some build and run instructions.
//
// 2016-2019, Georg Sauthoff <mail@gms.tf>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <string>
#include <deque>
#include <vector>
#include <algorithm>
#include <exception>

#include <xfsx/scratchpad.hh>
#include <xfsx/ber2xml.hh>
#include <xfsx/xml_writer_arguments.hh>
#include <xfsx/ber2lxml.hh>
#include <xfsx/lxml2ber.hh>
#include <xfsx/tap.hh>
#include <xfsx/ber_writer_arguments.hh>
#include <xxxml/xxxml.hh>

#include <test/test.hh>

using namespace std;
using namespace xfsx;

static void compare_bers(
    const pair<const u8*, const u8*> &a,
    const pair<const u8*, const u8*> &b,
    const xfsx::xml::Pretty_Writer_Arguments &args)
{
  auto x = scratchpad::mk_simple_writer<char>();
  auto y = scratchpad::mk_simple_writer<char>();
  xfsx::xml::pretty_write(a.first, a.second, x, args);
  xfsx::xml::pretty_write(b.first, b.second, y, args);
  if (x.pos() != y.pos())
    throw length_error("Input XML length " + to_string(x.pos())
        + " != output XML length " + to_string(y.pos()));
  const auto &v = dynamic_cast<scratchpad::Scratchpad_Writer<char>*>(x.backend())->pad();
  const auto &w = dynamic_cast<scratchpad::Scratchpad_Writer<char>*>(y.backend())->pad();
  if (!equal(v.prelude(), v.begin(), w.prelude(), w.begin()))
    throw domain_error("Resulting XML differs from input XML");
}

extern "C" int LLVMFuzzerTestOneInput(const u8 *d, size_t n) {
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
  auto x = scratchpad::mk_simple_writer<u8>();
  try {
    xxxml::doc::Ptr doc = xfsx::xml::l2::generate_tree(d, d+n, args);
    xfsx::xml::l2::write_ber(doc, x, wargs);
  } catch (std::exception &e) {
    // as long it is erroring out in a controlled fashion it is fine
    return 0;
  }
  const auto &v = dynamic_cast<scratchpad::Scratchpad_Writer<u8>*>(x.backend())->pad();
  compare_bers(make_pair(d, d+n), make_pair(v.prelude(), v.begin()), args);
  return 0;  // Non-zero return values are reserved for future use.
}

// int main() { return 0; }
