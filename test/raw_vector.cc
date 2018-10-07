#include <catch.hpp>

#include <string>

#include <xfsx/raw_vector.hh>

using namespace std;

TEST_CASE( "raw vector " "basic", "[raw][vector]" )
{
    const char s[] = "0123456789";
    xfsx::Raw_Vector<char> v(s, s+sizeof s - 1);
    CHECK(string(v.begin(), v.end()) == s);
    v.resize(4);
    CHECK(string(v.begin(), v.end()) == "0123");
    v.resize(10);
    CHECK(string(v.begin(), v.end()) == s);
}
