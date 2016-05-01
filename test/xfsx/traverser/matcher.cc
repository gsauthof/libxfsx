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
#include <xfsx/traverser/matcher.hh>
#include <xfsx/byte.hh>

#include <ixxx/util.hh>

#include <boost/filesystem.hpp>

#include <functional>


using namespace std;

BOOST_AUTO_TEST_SUITE(xfsx_)

  BOOST_AUTO_TEST_SUITE(traverser_)

    BOOST_AUTO_TEST_SUITE(matcher)

      struct Collect {
        map<uint32_t, string> map_;
        uint32_t code_ {0};
        uint32_t type_ {0};

        uint32_t count_ {0};

        template <typename T, typename Proxy>
          xfsx::traverser::Hint operator()(xfsx::traverser::Hint r,
              xfsx::traverser::Matcher_Result res, const Proxy &p, const T &t)
          {
            ++count_;
            if (res == xfsx::traverser::Matcher_Result::APPLY) {
              switch (p.tag(t)) {
                case 184 /*RecEntityCode*/ :
                  code_ = p.uint32(t);
                  break;
                case 186 /*RecEntityType*/ :
                  type_ = p.uint32(t);
                  break;
                case 400 /*RecEntityId*/ :
                  {
                  string s;
                  p.string(t, s);
                  s += " (";
                  auto n = s.size();
                  auto m = xfsx::byte::writer::encoded_length(type_);
                  s.resize(n + m);
                  // data() non-const -> C++17
                  xfsx::byte::writer::encode(type_, &*s.begin() + n, m);
                  s += ')';
                  map_[code_] = std::move(s);
                  }
                  break;
                default:
                  break;
              }
              return xfsx::traverser::Hint::SKIP_CHILDREN;
            }
            return r;
          }
      };

      BOOST_AUTO_TEST_CASE(basic)
      {
        using namespace xfsx;
        using namespace xfsx::tap::traverser;

        const array<Tag_Int, 11> seen_tags_ref = {
          1, 4, 5, 6, 234, 233, 232, 231, 188, 3, 15
        };
        vector<Tag_Int> seen_tags;
        const array<Matcher_Result, 11> seen_results_ref = {
          Matcher_Result::NONE,
          Matcher_Result::NONE,
          Matcher_Result::NONE,
          Matcher_Result::NONE,
          Matcher_Result::NONE,
          Matcher_Result::INIT,
          Matcher_Result::APPLY,
          Matcher_Result::APPLY,
          Matcher_Result::FINALIZE,
          Matcher_Result::NONE,
          Matcher_Result::NONE
        };
        vector<Matcher_Result> seen_results;
        boost::filesystem::path in(test::path::in());
        in /= "tap_3_12_valid.ber";
        ixxx::util::Mapped_File m(in.generic_string());
        Vertical_TLC t;

        Vertical_TLC_Proxy p(m.begin(), m.end(), t);
        vector<Tag_Int> path = {
          1,   // TransferBatch
          6,   // NetworkInfo
          234, // UtcTimeOffsetInfoList
          233  // UtcTimeOffsetInfo
        };
        // 232 /* UtcTimeOffsetCode */


        Basic_Matcher<Vertical_TLC_Proxy, Vertical_TLC> f(path);

        while (!p.eot(t)) {
          auto r = f(p, t);
          seen_tags.push_back(p.tag(t));
          seen_results.push_back(f.result_);
          if (r == xfsx::traverser::Hint::SKIP_CHILDREN)
            p.skip_children(t);
          else
            p.advance(t);
        }
        BOOST_CHECK_EQUAL_COLLECTIONS(seen_tags.begin(), seen_tags.end(),
            seen_tags_ref.begin(), seen_tags_ref.end());
        BOOST_CHECK_EQUAL_COLLECTIONS(seen_results.begin(), seen_results.end(),
            seen_results_ref.begin(), seen_results_ref.end());
      }

      BOOST_AUTO_TEST_CASE(list)
      {
        using namespace xfsx;
        boost::filesystem::path in(test::path::in());
        in /= "tap_3_12_valid.ber";
        ixxx::util::Mapped_File m(in.generic_string());
        Vertical_TLC t;

        using namespace xfsx::tap::traverser;
        Vertical_TLC_Proxy p(m.begin(), m.end(), t);
        vector<Tag_Int> path = {
          1,   // TransferBatch
          6,   // NetworkInfo
          188, // RecEntityInfoList
          183// RecEntityInformation
        };
        // 184 /* RecEntityCode */

        Collect c;
        Basic_Matcher<Vertical_TLC_Proxy, Vertical_TLC> f(path);
        using namespace std::placeholders;
        f.callback_ = std::bind(&Collect::operator()<Vertical_TLC, Vertical_TLC_Proxy>, &c, _1, _2, _3, _4);
        Traverse st;
        st(p, t, f);

        BOOST_REQUIRE_EQUAL(c.map_.size(), 3);
        BOOST_CHECK_EQUAL(c.map_.at(1), "foo (1)");
        BOOST_CHECK_EQUAL(c.map_.at(22), "bar (1)");
        BOOST_CHECK_EQUAL(c.map_.at(142), "baz (4)");

        BOOST_CHECK_EQUAL(c.count_, 20);

      }
    BOOST_AUTO_TEST_SUITE_END() // matcher

    // converted to matcher api
    BOOST_AUTO_TEST_SUITE(old_path_finder)

      BOOST_AUTO_TEST_CASE(iterate_cdr)
      {
        // off:
        // 258
        // 383
        // 508
        // 655
        using namespace xfsx;
        using namespace xfsx::traverser;
        boost::filesystem::path in(test::path::in());
        in /= "tap_3_12_valid.ber";
        ixxx::util::Mapped_File f(in.generic_string());
        vector<Tag_Int> tags = { 1, 3, 0 };

        Vertical_TLC t;
        Vertical_TLC_Proxy p(f.begin(), f.end(), t);
        Basic_Matcher<Vertical_TLC_Proxy, Vertical_TLC> pf(tags, false);

        BOOST_CHECK(!p.eot(t));
        auto r = advance_first_match(p, t, pf);
        BOOST_CHECK(!p.eot(t));
        BOOST_CHECK_EQUAL(t.begin - f.begin(), 258);
        r = advance_next_match(r, p, t, pf);
        BOOST_CHECK(!p.eot(t));
        BOOST_CHECK_EQUAL(t.begin - f.begin(), 383);
        r = advance_next_match(r, p, t, pf);
        BOOST_CHECK(!p.eot(t));
        BOOST_CHECK_EQUAL(t.begin - f.begin(), 508);
        r = advance_next_match(r, p, t, pf);
        BOOST_CHECK(!p.eot(t));
        BOOST_CHECK_EQUAL(t.begin - f.begin(), 655);
        r = advance_next_match(r, p, t, pf);
        BOOST_CHECK(p.eot(t));
      }

      BOOST_AUTO_TEST_CASE(iterate_chargeable_subscriber)
      {
        // off:
        // 264
        // 389
        // 517
        // 657
        using namespace xfsx;
        using namespace xfsx::traverser;
        boost::filesystem::path in(test::path::in());
        in /= "tap_3_12_valid.ber";
        ixxx::util::Mapped_File f(in.generic_string());
        vector<Tag_Int> tags = { 427 };

        Vertical_TLC t;
        Vertical_TLC_Proxy p(f.begin(), f.end(), t);
        Basic_Matcher<Vertical_TLC_Proxy, Vertical_TLC> pf(tags, true);

        BOOST_CHECK(!p.eot(t));
        auto r = advance_first_match(p, t, pf);
        BOOST_CHECK(!p.eot(t));
        BOOST_CHECK_EQUAL(t.begin - f.begin(), 264);
        r = advance_next_match(r, p, t, pf);
        BOOST_CHECK(!p.eot(t));
        BOOST_CHECK_EQUAL(t.begin - f.begin(), 389);
        r = advance_next_match(r, p, t, pf);
        BOOST_CHECK(!p.eot(t));
        BOOST_CHECK_EQUAL(t.begin - f.begin(), 517);
        r = advance_next_match(r, p, t, pf);
        BOOST_CHECK(!p.eot(t));
        BOOST_CHECK_EQUAL(t.begin - f.begin(), 657);
        r = advance_next_match(r, p, t, pf);
        BOOST_CHECK(p.eot(t));
      }
      BOOST_AUTO_TEST_CASE(iterate_call_event_start_time_stamp)
      {
        using namespace xfsx;
        using namespace xfsx::traverser;
        boost::filesystem::path in(test::path::in());
        in /= "tap_3_12_valid.ber";
        ixxx::util::Mapped_File f(in.generic_string());
        vector<Tag_Int> tags = { 44, 16};

        Vertical_TLC t;
        Vertical_TLC_Proxy p(f.begin(), f.end(), t);
        Basic_Matcher<Vertical_TLC_Proxy, Vertical_TLC> pf(tags, true);

        BOOST_CHECK(!p.eot(t));
        auto r = advance_first_match(p, t, pf);
        BOOST_CHECK(!p.eot(t));
        BOOST_CHECK_EQUAL(t.begin - f.begin(), 287);
        r = advance_next_match(r, p, t, pf);
        BOOST_CHECK(!p.eot(t));
        BOOST_CHECK_EQUAL(t.begin - f.begin(), 412);
        r = advance_next_match(r, p, t, pf);
        BOOST_CHECK(!p.eot(t));
        BOOST_CHECK_EQUAL(t.begin - f.begin(), 562);
        r = advance_next_match(r, p, t, pf);
        BOOST_CHECK(p.eot(t));
      }

    BOOST_AUTO_TEST_SUITE_END() // old_path_finder

  BOOST_AUTO_TEST_SUITE_END() // traverser_

BOOST_AUTO_TEST_SUITE_END() // xfsx_
