// See README-fuzzing.md for some build and run instructions.
//
// 2016-2019, Georg Sauthoff <mail@gms.tf>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <string>
#include <deque>
#include <exception>


#include <xfsx/xml2ber.hh>
#include <xfsx/tap.hh>
#include <xfsx/ber_writer_arguments.hh>
#include <xfsx/scratchpad.hh>

#include <test/test.hh>

using namespace std;

extern "C" int LLVMFuzzerTestOneInput(const xfsx::u8 *d, size_t n) {
    string tap_filename(test::path::in()
            + "/../../libgrammar/test/in/asn1/tap_3_12_strip.asn1");
    deque<string> asn_filenames = {tap_filename};
    xfsx::BER_Writer_Arguments args;
    xfsx::tap::apply_grammar(asn_filenames, args);

    using namespace xfsx;
    const char *e = reinterpret_cast<const char*>(d);
    auto inp = scratchpad::mk_simple_reader(e, e+n);
    auto v = scratchpad::mk_simple_writer<u8>();

    try {
        xfsx::xml::write_ber(inp, v, args);
    } catch (std::exception &e) {
        // as long it is erroring out in a controlled fashion it is fine
    }
    return 0;  // Non-zero return values are reserved for future use.
}

// int main() { return 0; }
