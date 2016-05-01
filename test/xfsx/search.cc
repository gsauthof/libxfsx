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
#include <test/test.hh>

#include <vector>
#include <array>
#include <limits>

#include <boost/filesystem.hpp>

#include <xfsx/xfsx.hh>
#include <xfsx/search.hh>

#include <ixxx/util.hh>


using namespace std;

BOOST_AUTO_TEST_SUITE(xfsx_)

  BOOST_AUTO_TEST_SUITE(search)

    BOOST_AUTO_TEST_CASE(find_absolute_aci)
    {
      using namespace xfsx;
      boost::filesystem::path in(test::path::in());
      in /= "tap_3_12_valid.ber";
      ixxx::util::Mapped_File f(in.generic_string());
      vector<Tag_Int> tags = { 1, 15};
      auto r = xfsx::search(f.begin(), f.end(), tags, false);
      ssize_t off = r - f.begin();
      BOOST_CHECK_EQUAL(off, 740);
    }

    BOOST_AUTO_TEST_CASE(find_wildcard_aci)
    {
      using namespace xfsx;
      boost::filesystem::path in(test::path::in());
      in /= "tap_3_12_valid.ber";
      ixxx::util::Mapped_File f(in.generic_string());
      vector<Tag_Int> tags = { 0, 15};
      auto r = xfsx::search(f.begin(), f.end(), tags, false);
      ssize_t off = r - f.begin();
      BOOST_CHECK_EQUAL(off, 740);
    }

    BOOST_AUTO_TEST_CASE(find_relative_aci)
    {
      using namespace xfsx;
      boost::filesystem::path in(test::path::in());
      in /= "tap_3_12_valid.ber";
      ixxx::util::Mapped_File f(in.generic_string());
      vector<Tag_Int> tags = { 15};
      auto r = xfsx::search(f.begin(), f.end(), tags, true);
      ssize_t off = r - f.begin();
      BOOST_CHECK_EQUAL(off, 740);
    }

    BOOST_AUTO_TEST_CASE(find_relative_chargetype)
    {
      using namespace xfsx;
      boost::filesystem::path in(test::path::in());
      in /= "tap_3_12_valid.ber";
      ixxx::util::Mapped_File f(in.generic_string());
      // i.e. ChargeDetailList/ChargeDetail/Charge
      vector<Tag_Int> tags = { 64, 63, 62};
      auto r = xfsx::search(f.begin(), f.end(), tags, true);
      ssize_t off = r - f.begin();
      BOOST_CHECK_EQUAL(off, 378);
    }

    BOOST_AUTO_TEST_CASE(find_not)
    {
      using namespace xfsx;
      boost::filesystem::path in(test::path::in());
      in /= "tap_3_12_valid.ber";
      ixxx::util::Mapped_File f(in.generic_string());
      // i.e. ChargeDetailList/ChargeDetail/Charge
      vector<Tag_Int> tags = { 64, 63, 1};
      auto r = xfsx::search(f.begin(), f.end(), tags, true);
      BOOST_CHECK(r == f.end());
    }

    BOOST_AUTO_TEST_CASE(find_not_with_absolute)
    {
      using namespace xfsx;
      boost::filesystem::path in(test::path::in());
      in /= "tap_3_12_valid.ber";
      ixxx::util::Mapped_File f(in.generic_string());
      vector<Tag_Int> tags = { 15};
      auto r = xfsx::search(f.begin(), f.end(), tags, false);
      BOOST_CHECK(r == f.end());
    }

  BOOST_AUTO_TEST_SUITE_END() // search


BOOST_AUTO_TEST_SUITE_END() // xfsx_
