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

#include <xfsx/tap.hh>
#include <xfsx/xfsx.hh>

#include <string>

using namespace std;

BOOST_AUTO_TEST_SUITE(xfsx_)

  BOOST_AUTO_TEST_SUITE(tap_)

    BOOST_AUTO_TEST_SUITE(aci_)

      using namespace xfsx;
      using namespace xfsx::tap;

      BOOST_AUTO_TEST_CASE(defaults)
      {
        auto v = aci_path();
        BOOST_REQUIRE_EQUAL(v.size(), 2u);
        BOOST_CHECK_EQUAL(v[0], 1u);
        BOOST_CHECK_EQUAL(v[1], 15u);
      }

      BOOST_AUTO_TEST_CASE(none)
      {
        Tag_Translator t;
        auto v = aci_path(t);
        BOOST_CHECK(v.empty());
      }

      BOOST_AUTO_TEST_CASE(tap)
      {
        Tag_Translator t;
        std::unordered_map<uint32_t, std::string> m = {
          { 1 , "TransferBatch"     },
          { 15, "AuditControlInfo" }
        };
        t.push(Klasse::APPLICATION, std::move(m));
        auto v = aci_path(t);
        BOOST_REQUIRE_EQUAL(v.size(), 2u);
        BOOST_CHECK_EQUAL(v[0], 1u);
        BOOST_CHECK_EQUAL(v[1], 15u);
      }

      BOOST_AUTO_TEST_CASE(tap_not)
      {
        Tag_Translator t;
        std::unordered_map<uint32_t, std::string> m = {
          { 1 , "foo"     },
          { 15, "bar" }
        };
        t.push(Klasse::APPLICATION, std::move(m));
        auto v = aci_path(t);
        BOOST_CHECK(v.empty());
      }


      BOOST_AUTO_TEST_CASE(rap)
      {
        Tag_Translator t;
        std::unordered_map<uint32_t, std::string> m = {
          { 534, "returnBatch"     },
          { 541, "RapAuditControlInfo" }
        };
        t.push(Klasse::APPLICATION, std::move(m));
        auto v = aci_path(t);
        BOOST_REQUIRE_EQUAL(v.size(), 2u);
        BOOST_CHECK_EQUAL(v[0], 534u);
        BOOST_CHECK_EQUAL(v[1], 541u);
      }

    BOOST_AUTO_TEST_SUITE_END() // aci_

    BOOST_AUTO_TEST_SUITE(cdr)
      using namespace xfsx;
      using namespace xfsx::tap;

      BOOST_AUTO_TEST_CASE(defaults)
      {
        auto v = kth_cdr_path();
        BOOST_REQUIRE_EQUAL(v.size(), 3u);
        BOOST_CHECK_EQUAL(v[0], 1u);
        BOOST_CHECK_EQUAL(v[1], 3u);
        BOOST_CHECK_EQUAL(v[2], 0u);
      }

      BOOST_AUTO_TEST_CASE(empty)
      {
        Tag_Translator t;
        auto v = kth_cdr_path(t);
        BOOST_CHECK(v.empty());
      }

      BOOST_AUTO_TEST_CASE(empty_no_match)
      {
        Tag_Translator t;
        std::unordered_map<uint32_t, std::string> m = {
          { 1, "TansferBatch"     },
          { 3, "CallEventDetailList" }
        };
        t.push(Klasse::APPLICATION, std::move(m));
        auto v = kth_cdr_path(t);
        BOOST_CHECK(v.empty());
      }

      BOOST_AUTO_TEST_CASE(tap_cdr)
      {
        Tag_Translator t;
        std::unordered_map<uint32_t, std::string> m = {
          { 1, "TransferBatch"     },
          { 3, "CallEventDetailList" }
        };
        t.push(Klasse::APPLICATION, std::move(m));
        auto v = kth_cdr_path(t);
        BOOST_REQUIRE_EQUAL(v.size(), 3u);
        BOOST_CHECK_EQUAL(v[0], 1u);
        BOOST_CHECK_EQUAL(v[1], 3u);
        BOOST_CHECK_EQUAL(v[2], 0u);
      }

      BOOST_AUTO_TEST_CASE(rap_cdr)
      {
        Tag_Translator t;
        std::unordered_map<uint32_t, std::string> m = {
          { 534, "returnBatch"     },
          { 536, "returnDetailList" }
        };
        t.push(Klasse::APPLICATION, std::move(m));
        auto v = kth_cdr_path(t);
        BOOST_REQUIRE_EQUAL(v.size(), 3u);
        BOOST_CHECK_EQUAL(v[0], 534u);
        BOOST_CHECK_EQUAL(v[1], 536u);
        BOOST_CHECK_EQUAL(v[2], 0u);
      }

    BOOST_AUTO_TEST_SUITE_END() // cdr


  BOOST_AUTO_TEST_SUITE_END() // tap_

BOOST_AUTO_TEST_SUITE_END() // xfsx_
