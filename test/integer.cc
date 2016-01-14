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

#include <limits>

#include <xfsx/integer.hh>


using namespace std;

BOOST_AUTO_TEST_SUITE(xfsx_)

  BOOST_AUTO_TEST_SUITE(integer_)

    using namespace xfsx::integer;

    BOOST_AUTO_TEST_CASE(uinttoint)
    {
      int64_t i = -1;
      uint_to_int(i);
      BOOST_CHECK_EQUAL(int64_t(numeric_limits<uint32_t>::max()), i);
      i = 1;
      uint_to_int(i);
      BOOST_CHECK_EQUAL(int64_t(1), i);
      i = -2;
      uint_to_int(i);
      BOOST_CHECK_EQUAL(int64_t(4294967294), i);
    }

  BOOST_AUTO_TEST_SUITE_END() // integer_

BOOST_AUTO_TEST_SUITE_END() // xfsx_
