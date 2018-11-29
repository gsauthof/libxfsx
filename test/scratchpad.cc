#include <catch.hpp>

#include <xfsx/scratchpad.hh>

#include <ixxx/util.hh>

#include <boost/filesystem.hpp>
#include <sstream>

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

TEST_CASE( "scratchpad " "write through", "[scratchpad]" )
{
    string out_dir(test::path::out() + "/scratchpad");
    bf::create_directories(out_dir);
    auto filename = out_dir + "/write_through";
    bf::remove(filename);
    auto w = scratchpad::mk_simple_writer<char>(filename);
    auto &sink = dynamic_cast<scratchpad::File_Writer<char>*>(w.backend())->sink();
    sink.set_increment(4);
    w.write("Hello World");
    w.write("Foo");
    w.write("Bar23");
    CHECK(w.pos() == 19);
    w.flush();
    auto m = ixxx::util::mmap_file(filename);
    CHECK(string(m.s_begin(), m.s_end()) == "Hello WorldFooBar23");
}

TEST_CASE( "scratchpad " "memory write through", "[scratchpad]" )
{
    using namespace xfsx::scratchpad;
    Simple_Writer<char> w(unique_ptr<Writer<char>>(new Scratchpad_Writer<char>));
    ostringstream o;
    for (int i = 0; i < 16 * 1024; ++i)
        o << i << ' ' << (i*i) << ' ' << i+i << ' ';
    string s(o.str());
    w.write(s.data(), s.data()+s.size());
    CHECK(w.pos() == 329157);
    w.flush();
    auto &pad = dynamic_cast<Scratchpad_Writer<char>*>(w.backend())->pad();
    string t(pad.prelude(), pad.cbegin());
    CHECK(t == s);
}
