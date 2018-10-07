#include <xfsx/bcd.hh>
#include <xfsx/bcd/decode.hh>
#include <xfsx/octet.hh>

#include <catch.hpp>

#include <array>
#include <string>

using namespace std;
using u8 = xfsx::u8;

struct Fn {
    const char *name;
    char *(*fn)(const u8 *begin, const u8 *end, char *o);
};
static const Fn fns[] = {
    { "default", [](const u8 *begin, const u8 *end, char *o) {
        return xfsx::bcd::decode(begin, end, o);
    } },
    { "bytewise", [](const u8 *begin, const u8 *end, char *o) {
      using namespace xfsx::bcd::impl::decode;
      decode_bytewise<Convert::DIRECT>(begin, end, o);
      return o + (end-begin)*2;
    } },
    { "bytewise_branch", [](const u8 *begin, const u8 *end, char *o) {
      using namespace xfsx::bcd::impl::decode;
      decode_bytewise<Convert::BRANCH>(begin, end, o);
      return o + (end-begin)*2;
    } },
    { "lookup", [](const u8 *begin, const u8 *end, char *o) {
      using namespace xfsx::bcd::impl::decode;
      decode_lookup(begin, end, o);
      return o + (end-begin)*2;
    } },
    { "lookup small table", [](const u8 *begin, const u8 *end, char *o) {
      using namespace xfsx::bcd::impl::decode;
      decode_lookup<Convert::SMALL>(begin, end, o);
      return o + (end-begin)*2;
    } },
    { "swar", [](const u8 *begin, const u8 *end, char *o) {
      using namespace xfsx::bcd::impl::decode;
      decode_swar<Scatter::LOOP>(begin, end, o);
      return o + (end-begin)*2;
    } },
#if defined(__BMI2__)
    { "swar pdep", [](const u8 *begin, const u8 *end, char *o) {
      using namespace xfsx::bcd::impl::decode;
      decode_swar<Scatter::PDEP>(begin, end, o);
      return o + (end-begin)*2;
    } },
#endif
#ifdef __SSE3__
    { "simd", [](const u8 *begin, const u8 *end, char *o) {
      using namespace xfsx::bcd::impl::decode;
      decode_ssse3<Scatter::LOOP>(begin, end, o);
      return o + (end-begin)*2;
    } },
    { "simd arith", [](const u8 *begin, const u8 *end, char *o) {
      using namespace xfsx::bcd::impl::decode;
      decode_ssse3<Scatter::LOOP, Convert::ARITH>(begin, end, o);
      return o + (end-begin)*2;
    } },
    #if defined(__BMI2__)
    { "simd pdep", [](const u8 *begin, const u8 *end, char *o) {
      using namespace xfsx::bcd::impl::decode;
      decode_ssse3<Scatter::PDEP>(begin, end, o);
      return o + (end-begin)*2;
    } },
    #endif
#endif
};
#define FOR_EACH_FN_BEGIN \
    for (auto &fn : fns) { \
        auto decode = fn.fn; \
        SECTION(fn.name) {
#define FOR_EACH_FN_END } }

TEST_CASE( "bcd decode " "empty", "[bcd][decode]" )
{
    FOR_EACH_FN_BEGIN

    array<u8, 0> in;
    array<char, 16> out = { 'x' }; 
    decode(in.begin(), in.end(), out.data());
    REQUIRE(out[0] == 'x');

    FOR_EACH_FN_END
}

TEST_CASE( "bcd decode " "single", "[bcd][decode]" )
{
    FOR_EACH_FN_BEGIN

    array<u8, 1> in = { 0xCAu };
    array<char, 3> out = { 'x', 'x', 0 }; 
    decode(in.begin(), in.end(), out.data());
    REQUIRE(out.data() == string("ca"));

    FOR_EACH_FN_END
}
TEST_CASE( "bcd decode " "basic", "[bcd][decode]" )
{
    FOR_EACH_FN_BEGIN

    array<u8, 4> a = {
        0xDEu, 0xADu, 0xCAu, 0x0Eu
    };
    string s(a.size()*2, ' ');
    char *p = &s[0];
    decode(a.begin(), a.end(), p);
    REQUIRE(s == "deadca0e");

    FOR_EACH_FN_END
}
TEST_CASE( "bcd decode " "middle filler", "[bcd][decode]" )
{
    FOR_EACH_FN_BEGIN

    array<u8, 4> a = {
        0xDEu, 0xADu, 0xCAu, 0xFEu
    };
    string s(a.size()*2, ' ');
    char *p = &s[0];
    decode(a.begin(), a.end(), p);
    // no special handling (e.g. throw for fillers in the middle)
    REQUIRE(s == "deadcafe");

    FOR_EACH_FN_END
}

TEST_CASE( "bcd decode " "end filler", "[bcd][decode]" )
{
    FOR_EACH_FN_BEGIN

    array<u8, 4> a = {
        0xDEu, 0xADu, 0xCAu, 0xEFu
    };
    string s(a.size()*2, ' ');
    char *p = &s[0];
    decode(a.begin(), a.end(), p);
    // no special handling, must be handled in the caller
    REQUIRE(s == "deadcaef");

    FOR_EACH_FN_END
}
TEST_CASE( "bcd decode " "all", "[bcd][decode]" )
{
    FOR_EACH_FN_BEGIN

    array<u8, 8> a = {
        0x12u, 0x34u, 0x56u, 0x78u, 0x90u, 0xABu, 0xCDu, 0xEFu
    };
    string s(a.size()*2, ' ');
    char *p = &s[0];
    decode(a.begin(), a.end(), p);
    REQUIRE(s == "1234567890abcdef");

    FOR_EACH_FN_END
}
TEST_CASE( "bcd decode " "odd", "[bcd][decode]" )
{
    FOR_EACH_FN_BEGIN

    array<u8, 7> a = {
        0x12u, 0x34u, 0x56u, 0x78u, 0x90u, 0xABu, 0xCDu
    };
    string s(a.size()*2, ' ');
    char *p = &s[0];
    decode(a.begin(), a.end(), p);
    REQUIRE(s == "1234567890abcd");

    FOR_EACH_FN_END
}
TEST_CASE( "bcd decode " "odd more", "[bcd][decode]" )
{
    FOR_EACH_FN_BEGIN

    array<u8, 9> a = {
        0x12u, 0x34u, 0x56u, 0x78u, 0x90u, 0xABu, 0xCDu, 0xEFu, 0x23u
    };
    string s(a.size()*2, ' ');
    char *p = &s[0];
    decode(a.begin(), a.end(), p);
    REQUIRE(s == "1234567890abcdef23");

    FOR_EACH_FN_END
}
TEST_CASE( "bcd decode " "all long", "[bcd][decode]" )
{
    FOR_EACH_FN_BEGIN

        array<u8, 64> a = {
          0x12u, 0x34u, 0x56u, 0x78u, 0x90u, 0xABu, 0xCDu, 0xEFu,
          0x12u, 0x34u, 0x56u, 0x78u, 0x90u, 0xABu, 0xCDu, 0xEFu,
          0x12u, 0x34u, 0x56u, 0x78u, 0x90u, 0xABu, 0xCDu, 0xEFu,
          0x12u, 0x34u, 0x56u, 0x78u, 0x90u, 0xABu, 0xCDu, 0xEFu,
          0x12u, 0x34u, 0x56u, 0x78u, 0x90u, 0xABu, 0xCDu, 0xEFu,
          0x12u, 0x34u, 0x56u, 0x78u, 0x90u, 0xABu, 0xCDu, 0xEFu,
          0x12u, 0x34u, 0x56u, 0x78u, 0x90u, 0xABu, 0xCDu, 0xEFu,
          0x12u, 0x34u, 0x56u, 0x78u, 0x90u, 0xABu, 0xCDu, 0xEFu
        };
        string s(a.size()*2, ' ');
        char *p = &s[0];
        decode(a.begin(), a.end(), p);
        REQUIRE(s ==
            "1234567890abcdef"
            "1234567890abcdef"
            "1234567890abcdef"
            "1234567890abcdef"
            "1234567890abcdef"
            "1234567890abcdef"
            "1234567890abcdef"
            "1234567890abcdef"
            );

    FOR_EACH_FN_END
}
