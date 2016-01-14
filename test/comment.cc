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

#include <boost/test/unit_test.hpp>

#include <xfsx/comment.hh>

#include <string>

using namespace std;

BOOST_AUTO_TEST_SUITE(xfsx_)

  BOOST_AUTO_TEST_SUITE(comment_)

    using namespace xfsx::comment;


    BOOST_AUTO_TEST_CASE(none)
    {
      const char inp[] = "Hello World";
      XML_Splicer r(inp, inp+sizeof(inp)-1);
      auto i = r.begin();
      BOOST_REQUIRE(i != r.end());
      auto p = *i;
      BOOST_CHECK_EQUAL(string(p.first, p.second), inp);
      ++i;
      BOOST_REQUIRE(i == r.end());
    }

    BOOST_AUTO_TEST_CASE(middle)
    {
      const char inp[] = "Hello <!-- ignore -->World";
      XML_Splicer r(inp, inp+sizeof(inp)-1);
      auto i = r.begin();
      BOOST_REQUIRE(i != r.end());
      auto p = *i;
      BOOST_CHECK_EQUAL(string(p.first, p.second), "Hello ");
      ++i;
      BOOST_REQUIRE(i != r.end());
      p = *i;
      BOOST_CHECK_EQUAL(string(p.first, p.second), "World");
      ++i;
      BOOST_REQUIRE(i == r.end());
    }

    BOOST_AUTO_TEST_CASE(open)
    {
      const char inp[] = "Hello <!-- ignore World";
      XML_Splicer r(inp, inp+sizeof(inp)-1);
      auto i = r.begin();
      BOOST_REQUIRE(i != r.end());
      auto p = *i;
      BOOST_CHECK_EQUAL(string(p.first, p.second), "Hello ");
      ++i;
      BOOST_REQUIRE(i == r.end());
    }

    BOOST_AUTO_TEST_CASE(stray)
    {
      const char inp[] = "Hello --> ig<!--nore--> World";
      XML_Splicer r(inp, inp+sizeof(inp)-1);
      auto i = r.begin();
      BOOST_REQUIRE(i != r.end());
      auto p = *i;
      BOOST_CHECK_EQUAL(string(p.first, p.second), "Hello --> ig");
      ++i;
      BOOST_REQUIRE(i != r.end());
      p = *i;
      BOOST_CHECK_EQUAL(string(p.first, p.second), " World");
      ++i;
      BOOST_REQUIRE(i == r.end());
    }

    BOOST_AUTO_TEST_CASE(complete)
    {
      const char inp[] = "<!-- ignore -->";
      XML_Splicer r(inp, inp+sizeof(inp)-1);
      auto i = r.begin();
      BOOST_REQUIRE(i != r.end());
      auto p = *i;
      BOOST_CHECK_EQUAL(string(p.first, p.second), "");
      ++i;
      BOOST_REQUIRE(i == r.end());
    }

    BOOST_AUTO_TEST_CASE(more)
    {
      const char inp[] = "<!-- ignore -->Hello<!----><!-- --> <!-- -->foo bar<!-- -->";
      XML_Splicer r(inp, inp+sizeof(inp)-1);
      auto i = r.begin();
      BOOST_REQUIRE(i != r.end());
      auto p = *i;
      BOOST_CHECK_EQUAL(string(p.first, p.second), "");
      ++i;
      BOOST_REQUIRE(i != r.end());
      p = *i;
      BOOST_CHECK_EQUAL(string(p.first, p.second), "Hello");
      ++i;
      BOOST_REQUIRE(i != r.end());
      p = *i;
      BOOST_CHECK_EQUAL(string(p.first, p.second), "");
      ++i;
      BOOST_REQUIRE(i != r.end());
      p = *i;
      BOOST_CHECK_EQUAL(string(p.first, p.second), " ");
      ++i;
      BOOST_REQUIRE(i != r.end());
      p = *i;
      BOOST_CHECK_EQUAL(string(p.first, p.second), "foo bar");
      ++i;
      BOOST_REQUIRE(i == r.end());
    }

    BOOST_AUTO_TEST_CASE(dash)
    {
      const char inp[] = "<!-- ignore -->";
      Dash_Splicer r(inp, inp+sizeof(inp)-1);
      auto i = r.begin();
      BOOST_REQUIRE(i != r.end());
      auto p = *i;
      BOOST_CHECK_EQUAL(string(p.first, p.second), "<!");
      ++i;
      BOOST_REQUIRE(i != r.end());
      p = *i;
      BOOST_CHECK_EQUAL(string(p.first, p.second), ">");
      ++i;
      BOOST_REQUIRE(i == r.end());
    }



  BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
