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
#include <boost/filesystem.hpp>
#include <test/test.hh>

#include <bed/command.hh>
#include <bed/command/ber_commands.hh>
#include <bed/arguments.hh>

#include <xfsx/detector.hh>

#include <deque>
#include <string>


using namespace std;
namespace bf = boost::filesystem;

BOOST_AUTO_TEST_SUITE(xfsx_)

  BOOST_AUTO_TEST_SUITE(detector_)

    using namespace xfsx::detector;

    BOOST_AUTO_TEST_CASE(basic)
    {
      BOOST_TEST_CHECKPOINT("detector basic");
      bf::path input(test::path::in());
      input /= "asn1c/examples/sample.source.TAP3/sample-DataInterChange-1.ber";
      bf::path config(test::path::in() + "/../../config");
      config /= "detector.json";

      bf::path odir(test::path::out());
      odir /= "detector";
      bf::create_directories(odir);
      bf::path asn_link(odir);
      asn_link /= "tap_3_10.asn1";
      bf::remove(asn_link);
      bf::path zsv_link(odir);
      zsv_link /= "tap_3_10_constraints.zsv";
      bf::remove(zsv_link);
      bf::path pp_link(odir);
      pp_link /= "pretty_tap.lua";
      bf::remove(pp_link);
#if (defined(__MINGW32__) || defined(__MINGW64__))
      bf::copy_file(
          bf::absolute(test::path::in()
            + "/../../libgrammar/test/in/asn1").generic_string()
          + "/tap_3_12.asn1",
          asn_link);
#else
      bf::create_symlink(
          bf::absolute(test::path::in()
            + "/../../libgrammar/test/in/asn1").generic_string()
          + "/tap_3_12.asn1",
          asn_link);
#endif
      bf::copy_file(
          bf::absolute(test::path::in()
            + "/../../libgrammar/grammar/xml").generic_string()
          + "/tap_3_12_constraints.zsv",
          zsv_link);
      bf::copy_file(
          bf::absolute(test::path::in()
            + "/../../config").generic_string()
          + "/pretty_tap.lua",
          pp_link);

      deque<string> asn_search_path = {
        test::path::in() + "/../../libgrammar/test/in/asn1",
        test::path::in() + "/../../libgrammar/grammar/xml",
        odir.generic_string(),
        "." };

      Result r(detect(input.generic_string(),
            config.generic_string(),
            read_ber_header,
            asn_search_path));
      BOOST_CHECK_EQUAL(r.major, 3u);
      BOOST_CHECK_EQUAL(r.minor, 10u);
      BOOST_CHECK_EQUAL(r.name, "TAP");
      BOOST_CHECK_EQUAL(r.long_name, "Transferred Account Procedure");
      BOOST_CHECK_EQUAL(r.asn_filenames.size(), 1u);
      BOOST_CHECK(
          r.asn_filenames.front().find("tap_3_10.asn1") != string::npos);
    }

    BOOST_AUTO_TEST_CASE(basic_3_12)
    {
      BOOST_TEST_CHECKPOINT("detector basic 3_12");
      bf::path input(test::path::in());
      input /= "tap_3_12_valid.ber";
      bf::path config(test::path::in() + "/../../config");
      config /= "detector.json";

      bf::path odir(test::path::out());
      odir /= "detector";
      bf::create_directories(odir);

      deque<string> asn_search_path = {
        test::path::in() + "/../../libgrammar/test/in/asn1",
        test::path::in() + "/../../libgrammar/grammar/xml",
        odir.generic_string(),
        "." };

      Result r(detect_ber(input.generic_string(),
            config.generic_string(),
            asn_search_path));
      BOOST_CHECK_EQUAL(r.major, 3u);
      BOOST_CHECK_EQUAL(r.minor, 12u);
      BOOST_CHECK_EQUAL(r.name, "TAP");
      BOOST_CHECK_EQUAL(r.long_name, "Transferred Account Procedure");
      BOOST_CHECK_EQUAL(r.asn_filenames.size(), 1u);
      BOOST_CHECK(
          r.asn_filenames.front().find("tap_3_12.asn1") != string::npos);
      BOOST_REQUIRE_EQUAL(r.constraint_filenames.size(), 2u);
      BOOST_CHECK(
          r.constraint_filenames.front().find("tap_3_12_constraints.zsv") != string::npos);
      BOOST_CHECK(
          r.constraint_filenames.back().find("tadig_codes.zsv") != string::npos);
    }

    BOOST_AUTO_TEST_CASE(not_detected)
    {
      BOOST_TEST_CHECKPOINT("detector not detected");
      bf::path input(test::path::in());
      input /= "tap_3_12_valid.ber";
      bf::path config(test::path::in() + "/../../config");
      config /= "detector_tap_only.json";

      bf::path odir(test::path::out());
      odir /= "detector";
      bf::create_directories(odir);

      deque<string> asn_search_path = {
        test::path::in() + "/../../libgrammar/test/in/asn1",
        test::path::in() + "/../../libgrammar/grammar/xml",
        odir.generic_string(),
        "." };

      bf::path asn(test::path::in());
      asn /= "../../libgrammar/test/in/asn1/tap_3_12_strip.asn1";

      bf::path out(odir);
      out /= "tap_3_12_no_release.ber";
      bf::remove(out);

      vector<string> argvv = { "./bed", "edit", "-c", "remove",
        "//ReleaseVersionNumber",
        "--asn",
        asn.generic_string(), input.generic_string(), out.generic_string()
        };
      vector<char *> argv;
      for (auto &s : argvv)
        argv.push_back(&*s.begin());
      argv.push_back(nullptr);
      bed::Arguments args(argvv.size(), argv.data());
      bed::command::execute(args);

      BOOST_CHECK_THROW(detect(out.generic_string(),
            config.generic_string(),
            read_ber_header,
            asn_search_path), std::range_error);
    }

    BOOST_AUTO_TEST_CASE(basic_xml)
    {
      bf::path input(test::path::ref());
      input /= "ber_pretty_xml/tap_3_12_valid.xml";
      bf::path config(test::path::in() + "/../../config");
      config /= "detector.json";

      bf::path odir(test::path::out());
      odir /= "detector";
      bf::create_directories(odir);
      bf::path asn_link(odir);
      asn_link /= "tap_3_10.asn1";
      bf::remove(asn_link);
#if (defined(__MINGW32__) || defined(__MINGW64__))
      bf::copy_file(
          bf::absolute(test::path::in()
            + "/../../libgrammar/test/in/asn1").generic_string()
          + "/tap_3_12.asn1",
          asn_link);
#else
      bf::create_symlink(
          bf::absolute(test::path::in()
            + "/../../libgrammar/test/in/asn1").generic_string()
          + "/tap_3_12.asn1",
          asn_link);
#endif

      deque<string> asn_search_path = {
        test::path::in() + "/../../libgrammar/test/in/asn1",
        test::path::in() + "/../../libgrammar/grammar/xml",
        odir.generic_string(),
        "." };

      {
        Result r(detect(input.generic_string(),
              config.generic_string(),
              read_xml_header,
              asn_search_path));
        BOOST_CHECK_EQUAL(r.major, 3u);
        BOOST_CHECK_EQUAL(r.minor, 12u);
        BOOST_CHECK_EQUAL(r.name, "TAP");
        BOOST_CHECK_EQUAL(r.long_name, "Transferred Account Procedure");
        BOOST_CHECK_EQUAL(r.asn_filenames.size(), 1u);
        BOOST_CHECK(
            r.asn_filenames.front().find("tap_3_12.asn1") != string::npos);
      }
      {
        Result r(detect_xml(input.generic_string(),
              config.generic_string(),
              asn_search_path));
        BOOST_CHECK_EQUAL(r.major, 3u);
        BOOST_CHECK_EQUAL(r.minor, 12u);
        BOOST_CHECK_EQUAL(r.name, "TAP");
        BOOST_CHECK_EQUAL(r.long_name, "Transferred Account Procedure");
        BOOST_CHECK_EQUAL(r.asn_filenames.size(), 1u);
        BOOST_CHECK(
            r.asn_filenames.front().find("tap_3_12.asn1") != string::npos);
      }
    }


  BOOST_AUTO_TEST_SUITE_END() // detector_

BOOST_AUTO_TEST_SUITE_END() // xfsx_
