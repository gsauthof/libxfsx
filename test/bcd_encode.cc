struct Hex {
    unsigned char x;
    Hex(unsigned char x) : x(x) {}
    bool operator==(const Hex &o) const { return x == o.x; }
};
#include <ostream>
#include <ios>
static std::ostream &operator<<(std::ostream &o, const Hex &h)
{
    std::ios_base::fmtflags f(o.flags());
    o << "0x" << std::hex << unsigned(h.x);
    o.flags(f);
    return o;
}
#include <catch.hpp>

#include <xfsx/bcd.hh>
#include <xfsx/bcd/encode.hh>
#include <xfsx/octet.hh>

using namespace std;
using u8 = xfsx::u8;


struct Fn {
    const char *name;
    u8 *(*fn)(const char *begin, const char *end, u8 *o);
};
static const Fn fns[] = {
    { "default", [](const char *begin, const char *end, u8 *o) {
        return xfsx::bcd::encode(begin, end, o);
    } },
    { "bytewise", [](const char *begin, const char *end, u8 *o) {
        using namespace xfsx::bcd::impl::encode;
        encode_bytewise(begin, end, o);
        return o + ((end-begin)+1)/2;
    } },
    { "bytewise branch", [](const char *begin, const char *end, u8 *o) {
        using namespace xfsx::bcd::impl::encode;
        encode_bytewise<Convert::BRANCH>(begin, end, o);
        return o + ((end-begin)+1)/2;
    } },
    { "bytewise subtract", [](const char *begin, const char *end, u8 *o) {
        using namespace xfsx::bcd::impl::encode;
        encode_bytewise<Convert::ARITH>(begin, end, o);
        return o + ((end-begin)+1)/2;
    } },
    { "lookup", [](const char *begin, const char *end, u8 *o) {
        using namespace xfsx::bcd::impl::encode;
        encode_lookup(begin, end, o);
        return o + ((end-begin)+1)/2;
    } },
    { "swar", [](const char *begin, const char *end, u8 *o) {
        using namespace xfsx::bcd::impl::encode;
        encode_swar<Convert::DIRECT, Gather::LOOP>(begin, end, o);
        return o + ((end-begin)+1)/2;
    } }
#ifdef __BMI2__
    ,{ "swar PEXT", [](const char *begin, const char *end, u8 *o) {
        using namespace xfsx::bcd::impl::encode;
        encode_swar<Convert::DIRECT, Gather::PEXT>(begin, end, o);
        return o + ((end-begin)+1)/2;
    } }
#endif
#ifdef __SSSE3__
    ,{ "SIMD SSSE3", [](const char *begin, const char *end, u8 *o) {
        using namespace xfsx::bcd::impl::encode;
        encode_ssse3<Convert::DIRECT, Gather::LOOP>(begin, end, o);
        return o + ((end-begin)+1)/2;
    } }
    ,{ "SIMD SSSE3 shift shuffle convert", [](const char *begin, const char *end, u8 *o) {
        using namespace xfsx::bcd::impl::encode;
        encode_ssse3<Convert::SHIFT_SHUFFLE, Gather::LOOP>(begin, end, o);
        return o + ((end-begin)+1)/2;
    } }
    ,{ "SIMD SSSE3 saturate convert", [](const char *begin, const char *end, u8 *o) {
        using namespace xfsx::bcd::impl::encode;
        encode_ssse3<Convert::SATURATE_SHUFFLE, Gather::LOOP>(begin, end, o);
        return o + ((end-begin)+1)/2;
    } }
    ,{ "SIMD SSSE3 shift/and gather", [](const char *begin, const char *end, u8 *o) {
        using namespace xfsx::bcd::impl::encode;
        encode_ssse3<Convert::DIRECT, Gather::SHIFT_AND>(begin, end, o);
        return o + ((end-begin)+1)/2;
    } }
#ifdef __BMI2__
    ,{ "SIMD SSSE3 PEXT", [](const char *begin, const char *end, u8 *o) {
        using namespace xfsx::bcd::impl::encode;
        encode_ssse3<Convert::DIRECT, Gather::PEXT>(begin, end, o);
        return o + ((end-begin)+1)/2;
    } }
#endif
#endif // __SSSE3__
};
#define FOR_EACH_FN_BEGIN \
    for (auto &fn : fns) { \
        auto encode = fn.fn; \
        SECTION(fn.name) {
#define FOR_EACH_FN_END } }

TEST_CASE( "bcd encode " "empty", "[bcd][encode]" )
{
    FOR_EACH_FN_BEGIN

    array<char, 1> i;
    array<u8, 1> a = { 0x23 };
    encode(i.begin(), i.begin(), a.begin());
    CHECK(Hex(a[0]) == Hex(0x23u));

    FOR_EACH_FN_END
}

TEST_CASE( "bcd encode " "one", "[bcd][encode]" )
{
    FOR_EACH_FN_BEGIN

    const char i[] = "c";
    array<u8, 1> a;
    const char *begin = i;
    const char *end = begin + sizeof(i) - 1;
    encode(begin, end, a.begin());
    CHECK(Hex(a[0]) == Hex(0xcfu));

    FOR_EACH_FN_END
}

TEST_CASE( "bcd encode " "more", "[bcd][encode]" )
{
    FOR_EACH_FN_BEGIN

    const char i[] = "deadcafe";
    array<u8, 4> a;
    const char *begin = i;
    const char *end = begin + sizeof(i) - 1;
    encode(begin, end, a.begin());
    CHECK(Hex(a[0]) == Hex(0xdeu));
    CHECK(Hex(a[1]) == Hex(0xadu));
    CHECK(Hex(a[2]) == Hex(0xcau));
    CHECK(Hex(a[3]) == Hex(0xfeu));

    FOR_EACH_FN_END
}

TEST_CASE( "bcd encode " "filler", "[bcd][encode]" )
{
    FOR_EACH_FN_BEGIN

  const char i[] = "deadcafe1";
  array<u8, 5> a;
  const char *begin = i;
  const char *end = begin + sizeof(i) - 1;
  encode(begin, end, a.begin());
  CHECK(Hex(a[0]) == Hex(0xdeu));
  CHECK(Hex(a[1]) == Hex(0xadu));
  CHECK(Hex(a[2]) == Hex(0xcau));
  CHECK(Hex(a[3]) == Hex(0xfeu));
  CHECK(Hex(a[4]) == Hex(0x1fu));

    FOR_EACH_FN_END
}

TEST_CASE( "bcd encode " "filler_longer", "[bcd][encode]" )
{
    FOR_EACH_FN_BEGIN

  const char i[] = "133713371337133";
  vector<u8> a(8);
  const char *begin = i;
  const char *end = begin + sizeof(i) - 1;
  encode(begin, end, a.data());
  CHECK(Hex(a[7]) == Hex(0x3fu));

    FOR_EACH_FN_END
}

TEST_CASE( "bcd encode " "upper_case", "[bcd][encode]" )
{
    FOR_EACH_FN_BEGIN

  const char i[] = "DeAdcAfEdeADcaFEDEadCAfe";
  array<u8, 12> a;
  const char *begin = i;
  const char *end = begin + sizeof(i) - 1;
  encode(begin, end, a.begin());
  CHECK(Hex(a[0]) == Hex(0xdeu));
  CHECK(Hex(a[1]) == Hex(0xadu));
  CHECK(Hex(a[2]) == Hex(0xcau));
  CHECK(Hex(a[3]) == Hex(0xfeu));

  CHECK(Hex(a[4]) == Hex(0xdeu));
  CHECK(Hex(a[5]) == Hex(0xadu));
  CHECK(Hex(a[6]) == Hex(0xcau));
  CHECK(Hex(a[7]) == Hex(0xfeu));

  CHECK(Hex(a[8]) == Hex(0xdeu));
  CHECK(Hex(a[9]) == Hex(0xadu));
  CHECK(Hex(a[10]) == Hex(0xcau));
  CHECK(Hex(a[11]) == Hex(0xfeu));

    FOR_EACH_FN_END
}

