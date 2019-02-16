
#include <catch.hpp>

#include <xfsx/scratchpad.hh>
#include <xfsx/tlc_reader.hh>
#include <xfsx/xfsx.hh>

#include "test.hh"

using namespace xfsx;

TEST_CASE( "tlc reader " "length overflow", "[tlcreader]" )
{
    auto in = test::path::in();
    auto r = scratchpad::mk_simple_reader<u8>(in + "/length_overflow.ber");
    TLC u;
    CHECK_THROWS_AS(read_next(r, u), std::range_error );
}

TEST_CASE( "tlc reader " "length overflow unit", "[tlcreader]" )
{
    auto in = test::path::in();
    auto r = scratchpad::mk_simple_reader<u8>(in + "/length_overflow.ber");
    Unit u;
    CHECK_THROWS_AS(read_next(r, u), std::range_error );
}
