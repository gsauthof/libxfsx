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

#include <xfsx/path.hh>
#include <xfsx/xfsx.hh>

#include <string>

using namespace std;

BOOST_AUTO_TEST_SUITE(xfsx_)

  BOOST_AUTO_TEST_SUITE(path_)


      using namespace xfsx;

  BOOST_AUTO_TEST_SUITE(plain)

      BOOST_AUTO_TEST_CASE(empty)
      {
        auto r = path::parse(string());
        BOOST_CHECK(r.first.empty());
        BOOST_CHECK_EQUAL(r.second, false);
      }

      BOOST_AUTO_TEST_CASE(numbers)
      {
        auto r = path::parse("/12/23/42");
        BOOST_REQUIRE(!r.first.empty());
        BOOST_CHECK_EQUAL(r.first[0], 12);
        BOOST_CHECK_EQUAL(r.first[1], 23);
        BOOST_CHECK_EQUAL(r.first[2], 42);
        BOOST_CHECK_EQUAL(r.second, false);
      }
      BOOST_AUTO_TEST_CASE(numbers_everywhere)
      {
        auto r = path::parse("12/23/42");
        BOOST_REQUIRE(!r.first.empty());
        BOOST_CHECK_EQUAL(r.first[0], 12);
        BOOST_CHECK_EQUAL(r.first[1], 23);
        BOOST_CHECK_EQUAL(r.first[2], 42);
        BOOST_CHECK_EQUAL(r.second, true);
      }
      BOOST_AUTO_TEST_CASE(numbers_gone_wild)
      {
        auto r = path::parse("12/*/42");
        BOOST_REQUIRE(!r.first.empty());
        BOOST_CHECK_EQUAL(r.first[0], 12);
        BOOST_CHECK_EQUAL(r.first[1], 0);
        BOOST_CHECK_EQUAL(r.first[2], 42);
        BOOST_CHECK_EQUAL(r.second, true);
      }
  BOOST_AUTO_TEST_SUITE_END() // plain 

  BOOST_AUTO_TEST_SUITE(named) 

      BOOST_AUTO_TEST_CASE(empty)
      {
        Name_Translator t;
        auto r = path::parse(string(), t);
        BOOST_CHECK(r.first.empty());
        BOOST_CHECK_EQUAL(r.second, false);
      }

      BOOST_AUTO_TEST_CASE(numbers)
      {
        std::unordered_map<std::string,
          std::tuple<bool, uint32_t, uint32_t> > m = {
            { "foo", make_tuple( true, 1u, 12u) },
            { "bar", make_tuple( true, 1u, 23u) },
            { "baz", make_tuple( true, 1u, 42u) }
          };
        Name_Translator t(std::move(m));
        auto r = path::parse("/foo/bar/baz", t);
        BOOST_REQUIRE(!r.first.empty());
        BOOST_CHECK_EQUAL(r.first[0], 12);
        BOOST_CHECK_EQUAL(r.first[1], 23);
        BOOST_CHECK_EQUAL(r.first[2], 42);
        BOOST_CHECK_EQUAL(r.second, false);
      }
      BOOST_AUTO_TEST_CASE(numbers_everywhere)
      {
        std::unordered_map<std::string,
          std::tuple<bool, uint32_t, uint32_t> > m = {
            { "foo", make_tuple( true, 1u, 12u) },
            { "bar", make_tuple( true, 1u, 23u) },
            { "baz", make_tuple( true, 1u, 42u) }
          };
        Name_Translator t(std::move(m));
        auto r = path::parse("foo/bar/baz", t);
        BOOST_REQUIRE(!r.first.empty());
        BOOST_CHECK_EQUAL(r.first[0], 12);
        BOOST_CHECK_EQUAL(r.first[1], 23);
        BOOST_CHECK_EQUAL(r.first[2], 42);
        BOOST_CHECK_EQUAL(r.second, true);
      }
      BOOST_AUTO_TEST_CASE(numbers_gone_wild)
      {
        std::unordered_map<std::string,
          std::tuple<bool, uint32_t, uint32_t> > m = {
            { "foo", make_tuple( true, 1u, 12u) },
            { "bar", make_tuple( true, 1u, 23u) },
            { "baz", make_tuple( true, 1u, 42u) }
          };
        Name_Translator t(std::move(m));
        auto r = path::parse("foo/*/baz", t);
        BOOST_REQUIRE(!r.first.empty());
        BOOST_CHECK_EQUAL(r.first[0], 12);
        BOOST_CHECK_EQUAL(r.first[1], 0);
        BOOST_CHECK_EQUAL(r.first[2], 42);
        BOOST_CHECK_EQUAL(r.second, true);
      }
      BOOST_AUTO_TEST_CASE(numbers_mixed)
      {
        std::unordered_map<std::string,
          std::tuple<bool, uint32_t, uint32_t> > m = {
            { "foo", make_tuple( true, 1u, 12u) },
            { "bar", make_tuple( true, 1u, 23u) },
            { "baz", make_tuple( true, 1u, 42u) }
          };
        Name_Translator t(std::move(m));
        auto r = path::parse("foo/*/42", t);
        BOOST_REQUIRE(!r.first.empty());
        BOOST_CHECK_EQUAL(r.first[0], 12);
        BOOST_CHECK_EQUAL(r.first[1], 0);
        BOOST_CHECK_EQUAL(r.first[2], 42);
        BOOST_CHECK_EQUAL(r.second, true);
      }

      BOOST_AUTO_TEST_CASE(invalid_path)
      {
        std::unordered_map<std::string,
          std::tuple<bool, uint32_t, uint32_t> > m = {
            { "foo", make_tuple( true, 1u, 12u) },
            { "bar", make_tuple( true, 1u, 23u) },
            { "baz", make_tuple( true, 1u, 42u) }
          };
        Name_Translator t(std::move(m));
        BOOST_CHECK_THROW(path::parse("foo/*/notfound", t), std::range_error);
      }

  BOOST_AUTO_TEST_SUITE_END() // named__

  BOOST_AUTO_TEST_SUITE_END() // path_

BOOST_AUTO_TEST_SUITE_END() // xfsx_
