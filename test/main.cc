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

////#define BOOST_TEST_DYN_LINK
////#define BOOST_TEST_MAIN
////#define BOOST_TEST_MODULE xfsx

////#include <boost/test/unit_test.hpp>

// custom initialization function instead

#include <boost/test/included/unit_test.hpp>

#include "ber2xml.hh"
#include "ber2ber.hh"
#include "xml2ber.hh"

using namespace std;

// /usr/include/boost/test/impl/unit_test_main.ipp
::boost::unit_test::test_suite* init_unit_test_suite( int argc, char* argv[] );

boost::unit_test::test_suite *init_unit_test_suite(int /*argc*/, char ** /*argv*/)
{
  boost::unit_test::framework::master_test_suite().add(create_ber2xml_suite());
  boost::unit_test::framework::master_test_suite().add(create_ber2ber_suite());
  boost::unit_test::framework::master_test_suite().add(create_xml2ber_suite());
  return 0;
}
