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
        const char inp[] = "Hello";
        Memory w(4096, 1024);
        w.write(inp, inp + sizeof(inp)-1);
        BOOST_CHECK_EQUAL(string(w.begin(), w.end()), "Hello");
        w << " World";
        BOOST_CHECK_EQUAL(string(w.begin(), w.end()), "Hello World");
        auto r = w.obtain_chunk(3);
        const char inp2[] = " 23";
        copy(inp2, inp2 + sizeof(inp2) - 1, r);
        BOOST_CHECK_EQUAL(string(w.begin(), w.end()), "Hello World 23");
      }

      BOOST_AUTO_TEST_CASE(mem_small)
      {
        Memory w(5, 5);
        const char inp[] = "Hello";
        w.write(inp, inp + sizeof(inp)-1);
        BOOST_CHECK_EQUAL(string(w.begin(), w.end()), "Hello");
        w << "World";
        // not significant
        w.flush();
        BOOST_CHECK_EQUAL(string(w.begin(), w.end()), "HelloWorld");
        auto r = w.obtain_chunk(3);
        const char inp2[] = " 23";
        copy(inp2, inp2 + sizeof(inp2) - 1, r);
        BOOST_CHECK_EQUAL(string(w.begin(), w.end()), "HelloWorld 23");
      }

      BOOST_AUTO_TEST_CASE(ints)
      {
        Memory w(4096, 1024);
        w << "Hello " << (-23) << " World " << 23u << '\n';
        BOOST_CHECK_EQUAL(string(w.begin(), w.end()), "Hello -23 World 23\n");
      }

      BOOST_AUTO_TEST_CASE(file)
      {
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
          ixxx::util::FD fd(out.generic_string(), O_CREAT | O_WRONLY, 0666);
          BOOST_TEST_CHECKPOINT("Writing: " << out);
          File w(fd, 5);
          const char inp[] = "Hello";
          w.write(inp, inp + sizeof(inp)-1);
          BOOST_CHECK_EQUAL(string(w.begin(), w.end()), "Hello");
          w << "World";
          BOOST_CHECK_EQUAL(string(w.begin(), w.end()), "World");
          auto r = w.obtain_chunk(3);
          const char inp2[] = " 23";
          copy(inp2, inp2 + sizeof(inp2) - 1, r);
          BOOST_CHECK_EQUAL(string(w.begin(), w.end()), " 23");
          // also called at scope exit, but here it is allowed to throw
          w.flush();
        }
        BOOST_TEST_CHECKPOINT("Comparing: " << out);
        {
          ixxx::util::Mapped_File f(out.generic_string());
          BOOST_CHECK_EQUAL(string(f.s_begin(), f.s_end()), "HelloWorld 23");
        }
      }

      BOOST_AUTO_TEST_CASE(literals)
      {
        const char inp[] = "Hello";
        Memory w(4096, 1024);
        w << make_pair(inp, sizeof(inp)-1);
        BOOST_CHECK_EQUAL(string(w.begin(), w.end()), "Hello");
        w << make_pair(" World", 6);
        BOOST_CHECK_EQUAL(string(w.begin(), w.end()), "Hello World");
      }

      BOOST_AUTO_TEST_CASE(ranges)
      {
        const char inp[] = "Hello";
        Memory w(4096, 1024);
        w << make_pair(inp, inp + sizeof(inp)-1);
        BOOST_CHECK_EQUAL(string(w.begin(), w.end()), "Hello");
        const char inp2[] = " World";
        w << make_pair(inp2, inp2 + sizeof(inp2)-1);
        BOOST_CHECK_EQUAL(string(w.begin(), w.end()), "Hello World");
      }

      BOOST_AUTO_TEST_CASE(fill)
      {
        const char inp[] = "Hello";
        Memory w(4096, 1024);
        w << make_pair(inp, inp + sizeof(inp)-1);
        BOOST_CHECK_EQUAL(string(w.begin(), w.end()), "Hello");
        w.fill(5);
        const char inp2[] = "World";
        w << make_pair(inp2, inp2 + sizeof(inp2)-1);
        BOOST_CHECK_EQUAL(string(w.begin(), w.end()), "Hello     World");
      }


    BOOST_AUTO_TEST_SUITE_END()

  BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
