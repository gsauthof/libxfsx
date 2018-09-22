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

#include "xml2ber.hh"

#include <boost/test/unit_test.hpp>
#include <boost/test/parameterized_test.hpp>
#include <boost/filesystem.hpp>

#include <ixxx/util.hh>

#include <array>
#include <algorithm>
#include <iostream>


#include <xfsx/ber2xml.hh>
#include <xfsx/xml2ber.hh>
#include <xfsx/xfsx.hh>
#include <xfsx/tap.hh>
#include <grammar/grammar.hh>

#include "test.hh"

using namespace std;

namespace bf = boost::filesystem;

static void basic_check_xmlber_back(const xfsx::BER_Writer_Arguments &args,
    const char *rel_filename_str)
{
  const char suffix[] = "xmlber_back";

  bf::path rel_filename(rel_filename_str);
  bf::path in(test::path::in());
  in /= rel_filename;

  bf::path out_xml(test::path::out());
  out_xml /= suffix;
  out_xml /= rel_filename;
  out_xml.replace_extension("xml");

  bf::path out_ber(test::path::out());
  out_ber /= suffix;
  out_ber /= rel_filename;
  out_ber.replace_extension("ber");

  bf::path out_xml2(test::path::out());
  out_xml2 /= suffix;
  out_xml2 /= rel_filename;
  out_xml2.replace_extension("");
  out_xml2 += "_final";
  out_xml2.replace_extension("xml");


  BOOST_TEST_MESSAGE("in: " << in);
  BOOST_TEST_MESSAGE("xml: " << out_xml);
  BOOST_TEST_MESSAGE("ber: " << out_ber);
  BOOST_TEST_MESSAGE("final: " << out_xml2);

  BOOST_TEST_CHECKPOINT("remove: " << out_xml);
  bf::remove(out_xml);
  BOOST_TEST_CHECKPOINT("remove: " << out_ber);
  bf::remove(out_ber);
  BOOST_TEST_CHECKPOINT("remove: " << out_xml2);
  bf::remove(out_xml2);
  BOOST_TEST_CHECKPOINT("create directory: " << out_xml.parent_path());
  bf::create_directories(out_xml.parent_path());


  BOOST_TEST_CHECKPOINT("write: " << out_xml);
  {
    auto a = ixxx::util::mmap_file(in.generic_string());
    xfsx::xml::write(a.begin(), a.end(), out_xml.generic_string());
  }
  BOOST_TEST_CHECKPOINT("write: " << out_ber);
  {
    auto f = ixxx::util::mmap_file(out_xml.generic_string());
    xfsx::xml::write_ber(f.s_begin(), f.s_end(), out_ber.generic_string(), args);
  }
  BOOST_TEST_CHECKPOINT("write: " << out_xml2);
  {
    auto in = ixxx::util::mmap_file(out_ber.generic_string());
    xfsx::xml::write(in.begin(), in.end(), out_xml2.generic_string());
  }

  BOOST_TEST_CHECKPOINT("comparing files: " << out_xml << " vs. " << out_xml2);
  BOOST_REQUIRE(bf::file_size(out_xml) && bf::file_size(out_xml2));
  {
    // mmap of zero-length file fails as specified by POSIX ...
    auto a = ixxx::util::mmap_file(out_xml.generic_string());
    auto b = ixxx::util::mmap_file(out_xml2.generic_string());
    bool are_equal = std::equal(a.begin(), a.end(), b.begin(), b.end());
    if (!are_equal) {
      cerr << "Files are not equal: " << out_xml << " vs. " << out_xml2 << "\n";
    }
    BOOST_CHECK(are_equal);
  }
}

static void check_xmlber_back(const char *rel_filename_str)
{
  xfsx::BER_Writer_Arguments args;
  basic_check_xmlber_back(args, rel_filename_str);
}

static void check_tap_xmlber_back(const char *rel_filename_str)
{
  xfsx::BER_Writer_Arguments args;

  deque<string> asn_filenames;
  asn_filenames.push_back(test::path::in()
      + "/../../libgrammar/test/in/asn1/tap_3_12_strip.asn1");
  BOOST_TEST_CHECKPOINT("Reading ASN.1 file: " << asn_filenames.front());
  grammar::Grammar g = xfsx::tap::read_asn_grammar(asn_filenames);
  args.translator = xfsx::Name_Translator(
      grammar::map_name_to_shape_klasse_tag(g));
  xfsx::tap::init_dereferencer(g, args.dereferencer);
  xfsx::tap::init_typifier(args.typifier);

  basic_check_xmlber_back(args, rel_filename_str);
}

boost::unit_test::test_suite *create_xml2ber_suite()
{
  const array<const char *, 15> filenames = {
      "asn1c/asn1c/tests/data-62/data-62-01.ber",
      "asn1c/asn1c/tests/data-62/data-62-10.ber",
      "asn1c/asn1c/tests/data-62/data-62-11.ber",
      "asn1c/asn1c/tests/data-62/data-62-14.ber",
      "asn1c/asn1c/tests/data-62/data-62-16.ber",
      "asn1c/asn1c/tests/data-62/data-62-20.ber",
      "asn1c/asn1c/tests/data-62/data-62-22.ber",
      "asn1c/asn1c/tests/data-62/data-62-25.ber",
      "asn1c/asn1c/tests/data-62/data-62-27.ber",
      "asn1c/asn1c/tests/data-62/data-62-32.ber",
      "asn1c/examples/sample.source.LDAP3/sample-LDAPMessage-1.ber",
      "asn1c/examples/sample.source.PKIX1/sample-Certificate-1.der",
      "asn1c/examples/sample.source.TAP3/sample-DataInterChange-1.ber",
      // bad, but still readable on a lexical level
      "asn1c/asn1c/tests/data-62/data-62-13-B.ber",
      "asn1c/asn1c/tests/data-62/data-62-15-B.ber"

  };
  const array<const char *, 2> tap_filenames = {
      "asn1c/examples/sample.source.TAP3/sample-DataInterChange-1.ber",
      "tap_3_12_valid.ber"
  };

  auto xfsx_ = BOOST_TEST_SUITE("xfsx_");
  auto xml2ber = BOOST_TEST_SUITE("xml2ber");
  auto back = BOOST_TEST_SUITE("back");
  auto tap_back = BOOST_TEST_SUITE("tap_back");

  back->add(BOOST_PARAM_TEST_CASE(&check_xmlber_back,
        filenames.begin(), filenames.end()));
  tap_back->add(BOOST_PARAM_TEST_CASE(&check_tap_xmlber_back,
        tap_filenames.begin(), tap_filenames.end()));

  xml2ber->add(back);
  xml2ber->add(tap_back);
  xfsx_->add(xml2ber);
  return xfsx_;
}

BOOST_AUTO_TEST_SUITE(xfsx_)

  BOOST_AUTO_TEST_SUITE(ber2xml)

    BOOST_AUTO_TEST_SUITE(basic)

      BOOST_AUTO_TEST_CASE(invalid_tag)
      {
        const char inp[] = R"(
<c tag='1' class='APPLICATION'>
    <c tag='4' class='APPLICATION'>
        <p tag='196' class='APPLICATION'>WERFD</p>
        <p tag='182' class='APPLICATION'>XLKJE</p>
        <p tag='109' class='APPLICATION'>31707</p>
        <c tag='108' class='APPLICATION'
            <p tag='16' class='APPLICATION'>20050405090547</p>
            <p tag='231' class='APPLICATION'>+0200</p>
        </c>
    </c>
</c>
          )";
        bf::path out_path(test::path::out());
        out_path /= "xml_ber";
        bf::path out(out_path);
        out /= "invalid.ber";
        BOOST_TEST_CHECKPOINT("Create directories: " << out);
        bf::create_directories(out.parent_path());
        BOOST_TEST_CHECKPOINT("Writing: " << out);
        BOOST_CHECK_THROW(
            xfsx::xml::write_ber(inp, inp+sizeof(inp)-1, out.generic_string()),
            std::runtime_error
            );
      }

      BOOST_AUTO_TEST_CASE(invalid_attr)
      {
        const char inp[] = R"(
<c tag='1' class='APPLICATION'>
    <c tag='4' class='APPLICATION'>
        <p tag='196' class='APPLICATION'>WERFD</p>
        <p tag='182' class='APPLICATION'>XLKJE</p>
        <p tag='109' class='APPLICATION'>31707</p>
        <c tag='108' class='APPLICATION>
            <p tag='16' class='APPLICATION'>20050405090547</p>
            <p tag='231' class='APPLICATION'>+0200</p>
        </c>
    </c>
</c>
          )";
        bf::path out_path(test::path::out());
        out_path /= "xml_ber";
        bf::path out(out_path);
        out /= "invalid.ber";
        BOOST_TEST_CHECKPOINT("Create directories: " << out);
        bf::create_directories(out.parent_path());
        BOOST_TEST_CHECKPOINT("Writing: " << out);
        BOOST_CHECK_THROW(
            xfsx::xml::write_ber(inp, inp+sizeof(inp)-1, out.generic_string()),
            std::runtime_error
            );
      }


    BOOST_AUTO_TEST_SUITE_END()

  BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

