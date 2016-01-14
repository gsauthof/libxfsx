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


#include <xfsx/character.hh>


using namespace std;

BOOST_AUTO_TEST_SUITE(xfsx_)

  BOOST_AUTO_TEST_SUITE(character_)

    using namespace xfsx::character;

    BOOST_AUTO_TEST_CASE(filename_part)
    {
      BOOST_CHECK_NO_THROW(verify_filename_part("123"));
      BOOST_CHECK_NO_THROW(verify_filename_part(" ..   \\ abc"));
      BOOST_CHECK_THROW(verify_filename_part("/123"), std::range_error);
      const char null_inp[] = { '\0', '1', '2', '3', '\0' };
      BOOST_CHECK_THROW(
          verify_filename_part(null_inp, null_inp + sizeof(null_inp)-1),
          std::range_error);
      BOOST_CHECK_THROW(verify_filename_part("\n123"), std::range_error);
      BOOST_CHECK_THROW(verify_filename_part("\r123"), std::range_error);
    }

  BOOST_AUTO_TEST_SUITE_END() // character_

BOOST_AUTO_TEST_SUITE_END() // xfsx_
