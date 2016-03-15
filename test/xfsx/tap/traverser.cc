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

#include <xfsx/traverser/lxml.hh>
#include <xfsx/xml_writer_arguments.hh>
#include <xfsx/ber2lxml.hh>
#include <xxxml/util.hh>

#include <ixxx/util.hh>

#include <xfsx/integer.hh>
#include <xfsx/s_pair.hh>

#include <deque>
#include <tuple>

#include <boost/filesystem.hpp>

using namespace std;

    class Fake_Proxy {
      public:
        using Tags = std::deque<std::tuple<xfsx::Tag_Int, std::string, uint32_t> >;

        xfsx::Tag_Int tag(const Tags &tags) const { return get<0>(tags.front()); }
      uint32_t height(const Tags &tags) const { return get<2>(tags.front()); }
      void string(const Tags &tags, std::string &s) const
      {
        s = get<1>(tags.front());
      }
      uint64_t uint64(const Tags &tags) const
      {
        return xfsx::integer::range_to_uint64(xfsx::s_pair::mk_s_pair(
              get<1>(tags.front()).c_str()));
      }
      uint32_t uint32(const Tags &tags) const
      {
        return xfsx::integer::range_to_uint32(xfsx::s_pair::mk_s_pair(
              get<1>(tags.front()).c_str()));
      }

      void advance(Tags &tags)
      {
        tags.pop_front();
      }

      void skip_children(Tags &t)
      {
        advance(t);
      }

      bool eot(const Tags &tags) const
      {
        return tags.empty();;
      }
    };

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

          BOOST_CHECK_EQUAL(f().first, "20140301140342");
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

          BOOST_CHECK_EQUAL(f().first, "20140301140342");
          BOOST_CHECK_EQUAL(f().second, "+0200");
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

          BOOST_CHECK_EQUAL(f().first, "20140302151200");
          BOOST_CHECK_EQUAL(f().second, "-0500");
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
          BOOST_CHECK_EQUAL(f3().first, "20140301140342");
          BOOST_CHECK_EQUAL(f3().second, "+0200");
          BOOST_CHECK_EQUAL(f4().first, "20140302150800");
          BOOST_CHECK_EQUAL(f4().second, "+0200");
        }

      BOOST_AUTO_TEST_SUITE_END() // basic

      BOOST_AUTO_TEST_SUITE(lxml)

        BOOST_AUTO_TEST_CASE(apply)
        {
          using namespace xfsx;
          boost::filesystem::path in(test::path::in());
          in /= "tap_3_12_valid.ber";
          ixxx::util::Mapped_File m(in.generic_string());
          deque<string> asn_filenames = { test::path::in()
            + "/../../libgrammar/test/in/asn1/tap_3_12_strip.asn1"  };
          xfsx::xml::Pretty_Writer_Arguments pargs(asn_filenames);
          auto doc = xfsx::xml::l2::generate_tree(m.begin(), m.end(), pargs);
          xxxml::util::DF_Traverser t(doc);

          using namespace xfsx::tap::traverser;
          LXML_Proxy p(pargs.name_translator);
          Audit_Control_Info aci;
          aci(p, t);

          BOOST_CHECK_EQUAL(aci.count(), 4);
          BOOST_CHECK_EQUAL(aci.sum(), 71200);
          BOOST_CHECK_EQUAL(aci.first_timestamp().first, "20140301140342");
          BOOST_CHECK_EQUAL(aci.first_timestamp().second, "+0200");
          BOOST_CHECK_EQUAL(aci.last_timestamp().first, "20140302150800");
          BOOST_CHECK_EQUAL(aci.last_timestamp().second, "+0200");
        }

      BOOST_AUTO_TEST_SUITE_END() // lxml


      BOOST_AUTO_TEST_SUITE(fake)

        BOOST_AUTO_TEST_CASE(min_timestamp)
        {
          using namespace xfsx::tap::traverser;
          using namespace grammar::tap;
          Fake_Proxy p;
          Fake_Proxy::Tags t = {
            make_tuple(Tag::TRANSFER_BATCH, string(), 0 ),
            make_tuple(Tag::NETWORK_INFO, string(), 1 ),
            make_tuple(Tag::UTC_TIME_OFFSET_CODE, string("1"), 3 ),
            make_tuple(Tag::UTC_TIME_OFFSET, string("-1000"), 3 ),
            make_tuple(Tag::UTC_TIME_OFFSET_CODE, string("2"), 3 ),
            make_tuple(Tag::UTC_TIME_OFFSET, string("-0400"), 3 ),
            make_tuple(Tag::CALL_EVENT_DETAIL_LIST, string(), 1 ),
            make_tuple(Tag::CALL_EVENT_START_TIME_STAMP, string(), 3 ),
            make_tuple(Tag::LOCAL_TIME_STAMP, string("20160310212323"), 4 ),
            make_tuple(Tag::UTC_TIME_OFFSET_CODE, string("2"), 4 ),
            make_tuple(23, string(), 2),
            make_tuple(Tag::CALL_EVENT_START_TIME_STAMP, string(), 3 ),
            make_tuple(Tag::LOCAL_TIME_STAMP, string("20160310202323"), 4 ),
            make_tuple(Tag::UTC_TIME_OFFSET_CODE, string("1"), 4 ),
            make_tuple(Tag::AUDIT_CONTROL_INFO, string(), 1 )
          };
          Timestamp<Less_Tag> f;
          Simple_Traverse st;
          st(p, t, f);

          BOOST_CHECK_EQUAL(f().first, "20160310212323");
          BOOST_CHECK_EQUAL(f().second, "-0400");
        }

        BOOST_AUTO_TEST_CASE(min_vs_charge_detail)
        {
          using namespace xfsx::tap::traverser;
          using namespace grammar::tap;
          Fake_Proxy p;
          Fake_Proxy::Tags t = {
            make_tuple(Tag::TRANSFER_BATCH, string(), 0 ),
            make_tuple(Tag::NETWORK_INFO, string(), 1 ),
            make_tuple(Tag::UTC_TIME_OFFSET_CODE, string("1"), 3 ),
            make_tuple(Tag::UTC_TIME_OFFSET, string("+0000"), 3 ),
            make_tuple(Tag::CALL_EVENT_DETAIL_LIST, string(), 1 ),
            make_tuple(Tag::CALL_EVENT_START_TIME_STAMP, string(), 3 ),
            make_tuple(Tag::LOCAL_TIME_STAMP, string("20160310212323"), 4 ),
            make_tuple(Tag::UTC_TIME_OFFSET_CODE, string("1"), 4 ),
            make_tuple(23, string(), 2),
            make_tuple(Tag::CHARGE_DETAIL_TIME_STAMP, string(), 3 ),
            make_tuple(Tag::LOCAL_TIME_STAMP, string("20160310202323"), 4 ),
            make_tuple(Tag::UTC_TIME_OFFSET_CODE, string("1"), 4 ),
            make_tuple(Tag::AUDIT_CONTROL_INFO, string(), 1 )
          };
          Timestamp<Less_Tag> f;
          Simple_Traverse st;
          st(p, t, f);

          BOOST_CHECK_EQUAL(f().first, "20160310212323");
          BOOST_CHECK_EQUAL(f().second, "+0000");
        }

        BOOST_AUTO_TEST_CASE(min_service_center_usage)
        {
          using namespace xfsx::tap::traverser;
          using namespace grammar::tap;
          Fake_Proxy p;
          Fake_Proxy::Tags t = {
            make_tuple(Tag::TRANSFER_BATCH, string(), 0 ),
            make_tuple(Tag::NETWORK_INFO, string(), 1 ),
            make_tuple(Tag::UTC_TIME_OFFSET_CODE, string("1"), 3 ),
            make_tuple(Tag::UTC_TIME_OFFSET, string("+0000"), 3 ),
            make_tuple(Tag::CALL_EVENT_DETAIL_LIST, string(), 1 ),
            make_tuple(Tag::DEPOSIT_TIME_STAMP, string(), 3 ),
            make_tuple(Tag::LOCAL_TIME_STAMP, string("20160310202323"), 4 ),
            make_tuple(Tag::UTC_TIME_OFFSET_CODE, string("1"), 4 ),
            make_tuple(23, string(), 2),
            make_tuple(Tag::COMPLETION_TIME_STAMP, string(), 3 ),
            make_tuple(Tag::LOCAL_TIME_STAMP, string("20160310212323"), 4 ),
            make_tuple(Tag::UTC_TIME_OFFSET_CODE, string("1"), 4 ),
            make_tuple(Tag::CHARGING_POINT, string("C"), 3 ),
            make_tuple(Tag::AUDIT_CONTROL_INFO, string(), 1 )
          };
          Timestamp<Less_Tag> f;
          Simple_Traverse st;
          st(p, t, f);

          BOOST_CHECK_EQUAL(f().first, "20160310212323");
          BOOST_CHECK_EQUAL(f().second, "+0000");
        }

      BOOST_AUTO_TEST_SUITE_END() // fake

    BOOST_AUTO_TEST_SUITE_END() // traverser_

  BOOST_AUTO_TEST_SUITE_END() // tap_

BOOST_AUTO_TEST_SUITE_END() // xfsx_
