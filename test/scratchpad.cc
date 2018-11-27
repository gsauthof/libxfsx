#include <catch.hpp>

#include <xfsx/scratchpad.hh>


#include <boost/filesystem.hpp>

#include "test.hh"

using namespace std;
using namespace xfsx;
namespace bf = boost::filesystem;


// XXX move some scratchpad centric test cases from test/byte.cc here

TEST_CASE( "scratchpad " "double flush", "[scratchpad]" )
{
    string out_dir(test::path::out() + "/scratchpad");
    bf::create_directories(out_dir);
    auto filename = out_dir + "/double_flush";
    bf::remove(filename);
    auto w = scratchpad::mk_simple_writer<char>(filename);
    w.write("Hello World");
    w.flush();
    w.write("Foo bar 23");
    w.flush();
    CHECK(w.pos() == 21);
}
