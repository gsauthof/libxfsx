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

#include <xfsx/xfsx.hh>
#include <xfsx/tap/traverser.hh>
#include <xfsx/traverser/tlc.hh>

#include <ixxx/util.hh>

#include <boost/filesystem.hpp>

BOOST_AUTO_TEST_SUITE(xfsx_)

  BOOST_AUTO_TEST_SUITE(tap_)

    BOOST_AUTO_TEST_SUITE(traverser_)

      BOOST_AUTO_TEST_SUITE(basic_)

        BOOST_AUTO_TEST_CASE(basic_count)
        {
          using namespace xfsx;
          boost::filesystem::path in(test::path::in());
          in /= "tap_3_12_valid.ber";
          ixxx::util::Mapped_File m(in.generic_string());
          Vertical_TLC t;

          using namespace xfsx::tap::traverser;
          Vertical_TLC_Proxy p(m.begin(), m.end(), t);
          CDR_Count f;
          Simple_Traverse st;
          st(p, t, f);

          BOOST_CHECK_EQUAL(f(), 4);
        }

        BOOST_AUTO_TEST_CASE(count)
        {
          using namespace xfsx;
          boost::filesystem::path in(test::path::in());
          in /= "tap_3_12_valid.ber";
          ixxx::util::Mapped_File m(in.generic_string());
          Vertical_TLC t;

          using namespace xfsx::tap::traverser;
          Vertical_TLC_Proxy p(m.begin(), m.end(), t);
          CDR_Count f;
          Traverse st;
          st(p, t, f);

          BOOST_CHECK_EQUAL(f(), 4);
        }

        BOOST_AUTO_TEST_CASE(basic_sum)
        {
          using namespace xfsx;
          boost::filesystem::path in(test::path::in());
          in /= "tap_3_12_valid.ber";
          ixxx::util::Mapped_File m(in.generic_string());
          Vertical_TLC t;

          using namespace xfsx::tap::traverser;
          Vertical_TLC_Proxy p(m.begin(), m.end(), t);
          Charge_Sum f;
          Simple_Traverse st;
          st(p, t, f);

          BOOST_CHECK_EQUAL(f(), 71200);
        }

        BOOST_AUTO_TEST_CASE(basic_timestamp)
        {
          using namespace xfsx;
          boost::filesystem::path in(test::path::in());
          in /= "tap_3_12_valid.ber";
          ixxx::util::Mapped_File m(in.generic_string());
          Vertical_TLC t;

          using namespace xfsx::tap::traverser;
          Vertical_TLC_Proxy p(m.begin(), m.end(), t);
          Timestamp<Less_Tag> f;
          Simple_Traverse st;
          st(p, t, f);

          BOOST_CHECK_EQUAL(f().first, "20140302150800");
          BOOST_CHECK_EQUAL(f().second, "+0200");
        }

        BOOST_AUTO_TEST_CASE(min_multi_off)
        {
          using namespace xfsx;
          boost::filesystem::path in(test::path::in());
          in /= "tap_3_12_timestamps.ber";
          ixxx::util::Mapped_File m(in.generic_string());
          Vertical_TLC t;

          using namespace xfsx::tap::traverser;
          Vertical_TLC_Proxy p(m.begin(), m.end(), t);
          Timestamp<Less_Tag> f;
          Simple_Traverse st;
          st(p, t, f);

          BOOST_CHECK_EQUAL(f().first, "20140302151159");
          BOOST_CHECK_EQUAL(f().second, "-0500");
        }

        BOOST_AUTO_TEST_CASE(max_multi_off)
        {
          using namespace xfsx;
          boost::filesystem::path in(test::path::in());
          in /= "tap_3_12_timestamps.ber";
          ixxx::util::Mapped_File m(in.generic_string());
          Vertical_TLC t;

          using namespace xfsx::tap::traverser;
          Vertical_TLC_Proxy p(m.begin(), m.end(), t);
          Timestamp<Greater_Tag> f;
          Simple_Traverse st;
          st(p, t, f);

          BOOST_CHECK_EQUAL(f().first, "20140302151159");
          BOOST_CHECK_EQUAL(f().second, "+0200");
        }

        BOOST_AUTO_TEST_CASE(multi_apply)
        {
          using namespace xfsx;
          boost::filesystem::path in(test::path::in());
          in /= "tap_3_12_valid.ber";
          ixxx::util::Mapped_File m(in.generic_string());
          Vertical_TLC t;

          using namespace xfsx::tap::traverser;
          Vertical_TLC_Proxy p(m.begin(), m.end(), t);
          CDR_Count f1;
          Charge_Sum f2;
          Timestamp<Less_Tag> f3;
          Timestamp<Greater_Tag> f4;
          Traverse st;
          st(p, t, f1, f2, f3, f4);

          BOOST_CHECK_EQUAL(f1(), 4);
          BOOST_CHECK_EQUAL(f2(), 71200);
          BOOST_CHECK_EQUAL(f3().first, "20140302150800");
          BOOST_CHECK_EQUAL(f3().second, "+0200");
          BOOST_CHECK_EQUAL(f4().first, "20140302150800");
          BOOST_CHECK_EQUAL(f4().second, "+0200");
        }

      BOOST_AUTO_TEST_SUITE_END() // basic

    BOOST_AUTO_TEST_SUITE_END() // traverser_

  BOOST_AUTO_TEST_SUITE_END() // tap_

BOOST_AUTO_TEST_SUITE_END() // xfsx_
