// Copyright 2015, Georg Sauthoff <mail@georg.so>

/* {{{ LGPLv3

    This file is part of libxfsx.

    libxfsx is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    libxfsx is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with libxfsx.  If not, see <http://www.gnu.org/licenses/>.

}}} */

#include "test.hh"
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include <xfsx/byte.hh>
#include <xfsx/scratchpad.hh>

#include <string>
#include <array>
#include <fcntl.h>

using namespace std;
namespace bf = boost::filesystem;

BOOST_AUTO_TEST_SUITE(xfsx_)

  BOOST_AUTO_TEST_SUITE(byte_)

    BOOST_AUTO_TEST_SUITE(writer_)

      using namespace xfsx::byte::writer;

      BOOST_AUTO_TEST_CASE(mem)
      {
          using namespace xfsx;
        scratchpad::Simple_Writer<char> x(unique_ptr<scratchpad::Writer<char>>(
                    new scratchpad::Scratchpad_Writer<char>()));
        byte::writer::Base w(x);
        const char inp[] = "Hello";
        x.write(inp, inp + sizeof(inp)-1);
        x.flush();
        const auto &pad = dynamic_cast<scratchpad::Scratchpad_Writer<char>*>(
                x.backend())->pad();
        BOOST_CHECK_EQUAL(string(pad.prelude(), pad.begin()), "Hello");
        w << " World";
        x.flush();
        BOOST_CHECK_EQUAL(string(pad.prelude(), pad.begin()), "Hello World");
        auto r = x.begin_write(3);
        const char inp2[] = " 23";
        copy(inp2, inp2 + sizeof(inp2) - 1, r);
        x.commit_write(3);
        x.flush();
        BOOST_CHECK_EQUAL(string(pad.prelude(), pad.begin()), "Hello World 23");
      }

      BOOST_AUTO_TEST_CASE(mem_small)
      {
          using namespace xfsx;
        scratchpad::Simple_Writer<char> x(unique_ptr<scratchpad::Writer<char>>(
                    new scratchpad::Scratchpad_Writer<char>()));
        auto be = dynamic_cast<scratchpad::Scratchpad_Writer<char>*>(
                x.backend());
        be->set_increment(5);
        const auto &pad = dynamic_cast<scratchpad::Scratchpad_Writer<char>*>(
                x.backend())->pad();
        byte::writer::Base w(x);
        const char inp[] = "Hello";
        x.write(inp, inp + sizeof(inp)-1);
        x.flush();
        BOOST_CHECK_EQUAL(string(pad.prelude(), pad.begin()), "Hello");
        w << "World";
        x.flush();
        BOOST_CHECK_EQUAL(string(pad.prelude(), pad.begin()), "HelloWorld");
        auto r = x.begin_write(3);
        const char inp2[] = " 23";
        copy(inp2, inp2 + sizeof(inp2) - 1, r);
        x.commit_write(3);
        x.flush();
        BOOST_CHECK_EQUAL(string(pad.prelude(), pad.begin()), "HelloWorld 23");
      }

      BOOST_AUTO_TEST_CASE(ints)
      {
          using namespace xfsx;
        scratchpad::Simple_Writer<char> x(unique_ptr<scratchpad::Writer<char>>(
                    new scratchpad::Scratchpad_Writer<char>()));
        const auto &pad = dynamic_cast<scratchpad::Scratchpad_Writer<char>*>(
                x.backend())->pad();
        byte::writer::Base w(x);
        w << "Hello " << (-23) << " World " << 23u << '\n';
        x.flush();
        BOOST_CHECK_EQUAL(string(pad.prelude(), pad.begin()), "Hello -23 World 23\n");
      }

      BOOST_AUTO_TEST_CASE(file)
      {
          using namespace xfsx;
        bf::path out_path(test::path::out());
        out_path /= "writer";
        bf::path out(out_path);
        out /= "test_01";
        BOOST_TEST_CHECKPOINT("Removing: " << out);
        bf::remove(out);
        BOOST_TEST_CHECKPOINT("Create directories: " << out);
        bf::create_directories(out.parent_path());

        {
          BOOST_TEST_CHECKPOINT("Open: " << out);
            scratchpad::Simple_Writer<char> x(unique_ptr<scratchpad::Writer<char>>(
                        new scratchpad::File_Writer<char>(out.generic_string())));
          BOOST_TEST_CHECKPOINT("Writing: " << out);
          byte::writer::Base w(x);
          const char inp[] = "Hello";
          x.write(inp, inp + sizeof(inp)-1);
          w << "Worl";
          auto r = x.begin_write(3);
          const char inp2[] = " 23";
          copy(inp2, inp2 + sizeof(inp2) - 1, r);
          x.commit_write(3);
          x.flush();
        }
        BOOST_TEST_CHECKPOINT("Comparing: " << out);
        {
          auto f = ixxx::util::mmap_file(out.generic_string());
          BOOST_CHECK_EQUAL(string(f.s_begin(), f.s_end()), "HelloWorl 23");
        }
      }

      BOOST_AUTO_TEST_CASE(literals)
      {
          using namespace xfsx;
        scratchpad::Simple_Writer<char> x(unique_ptr<scratchpad::Writer<char>>(
                    new scratchpad::Scratchpad_Writer<char>()));
        const auto &pad = dynamic_cast<scratchpad::Scratchpad_Writer<char>*>(
                x.backend())->pad();
        byte::writer::Base w(x);

        const char inp[] = "Hello";
        w << make_pair(inp, sizeof(inp)-1);
        x.flush();
        BOOST_CHECK_EQUAL(string(pad.prelude(), pad.begin()), "Hello");
        w << make_pair(" World", 6);
        x.flush();
        BOOST_CHECK_EQUAL(string(pad.prelude(), pad.begin()), "Hello World");
      }

      BOOST_AUTO_TEST_CASE(ranges)
      {
          using namespace xfsx;
        scratchpad::Simple_Writer<char> x(unique_ptr<scratchpad::Writer<char>>(
                    new scratchpad::Scratchpad_Writer<char>()));
        const auto &pad = dynamic_cast<scratchpad::Scratchpad_Writer<char>*>(
                x.backend())->pad();
        byte::writer::Base w(x);

        const char inp[] = "Hello";
        w << make_pair(inp, inp + sizeof(inp)-1);
        x.flush();
        BOOST_CHECK_EQUAL(string(pad.prelude(), pad.begin()), "Hello");
        const char inp2[] = " World";
        w << make_pair(inp2, inp2 + sizeof(inp2)-1);
        x.flush();
        BOOST_CHECK_EQUAL(string(pad.prelude(), pad.begin()), "Hello World");
      }

      BOOST_AUTO_TEST_CASE(fill)
      {
          using namespace xfsx;
        scratchpad::Simple_Writer<char> x(unique_ptr<scratchpad::Writer<char>>(
                    new scratchpad::Scratchpad_Writer<char>()));
        const auto &pad = dynamic_cast<scratchpad::Scratchpad_Writer<char>*>(
                x.backend())->pad();
        byte::writer::Base w(x);

        const char inp[] = "Hello";
        w << make_pair(inp, inp + sizeof(inp)-1);
        x.flush();
        BOOST_CHECK_EQUAL(string(pad.prelude(), pad.begin()), "Hello");
        Indent i(5);
        w << i;
        const char inp2[] = "World";
        w << make_pair(inp2, inp2 + sizeof(inp2)-1);
        x.flush();
        BOOST_CHECK_EQUAL(string(pad.prelude(), pad.begin()), "Hello     World");
      }

      BOOST_AUTO_TEST_CASE(newline)
      {
          using namespace xfsx;
        bf::path out_path(test::path::out());
        out_path /= "writer";
        bf::path out(out_path);
        out /= "newline";
        BOOST_TEST_CHECKPOINT("Removing: " << out);
        bf::remove(out);
            scratchpad::Simple_Writer<char> x(unique_ptr<scratchpad::Writer<char>>(
                        new scratchpad::File_Writer<char>(out.generic_string())));
          byte::writer::Base w(x);
        w << "Hello\n";
        w << "World\n";
        x.flush();
        BOOST_CHECK_EQUAL(boost::filesystem::file_size(out), 12);
      }

      BOOST_AUTO_TEST_CASE(directly_write_inc_multiples)
      {
          using namespace xfsx;
        bf::path out_path(test::path::out());
        out_path /= "writer";
        bf::path out(out_path);
        out /= "inc_mult";
        BOOST_TEST_CHECKPOINT("Removing: " << out);
        bf::remove(out);
        {
            scratchpad::Simple_Writer<char> x(unique_ptr<scratchpad::Writer<char>>(
                        new scratchpad::File_Writer<char>(out.generic_string())));
            auto be = dynamic_cast<scratchpad::File_Writer<char>*>(x.backend());
            be->sink().set_increment(3);
          byte::writer::Base w(x);
        const char inp1[] = "Hello World";
        const char inp2[] = "foobar23";
        x.write(inp1, inp1 + sizeof(inp1)-1);
        BOOST_CHECK_EQUAL(x.pos(), sizeof(inp1)-1);
        BOOST_CHECK_EQUAL(boost::filesystem::file_size(out), 9);

        x.write(inp2, inp2 + sizeof(inp2)-1);
        BOOST_CHECK_EQUAL(x.pos(), sizeof(inp1)-1+sizeof(inp2)-1);
        BOOST_CHECK_EQUAL(boost::filesystem::file_size(out), 18);
        x.flush();
        }
        BOOST_CHECK_EQUAL(boost::filesystem::file_size(out), 19);
        auto f = ixxx::util::mmap_file(out.generic_string());
        BOOST_CHECK_EQUAL(string(f.s_begin(), f.s_end()), "Hello Worldfoobar23");
      }

      BOOST_AUTO_TEST_CASE(write_without_flush)
      {
          using namespace xfsx;
        bf::path out_path(test::path::out());
        out_path /= "writer";
        bf::path out(out_path);
        out /= "woflush";
        BOOST_TEST_CHECKPOINT("Removing: " << out);
        bf::remove(out);
        {
            scratchpad::Simple_Writer<char> x(unique_ptr<scratchpad::Writer<char>>(
                        new scratchpad::File_Writer<char>(out.generic_string())));
            byte::writer::Base w(x);
            static const char foo[] = "Hello\n";
            w << foo;
            w.fill(4);
            BOOST_CHECK_EQUAL(boost::filesystem::file_size(out), 0);

            static const char bar[] = "World\n";
            w << bar;
            BOOST_CHECK_EQUAL(boost::filesystem::file_size(out), 0);
            // no x.flush()!
        }
        BOOST_CHECK_EQUAL(boost::filesystem::file_size(out), 16);
      }


    BOOST_AUTO_TEST_SUITE_END() // writer_

  BOOST_AUTO_TEST_SUITE_END() // byte_

BOOST_AUTO_TEST_SUITE_END() // xfsx_
