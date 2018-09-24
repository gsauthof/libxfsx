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

#include "ber2xml.hh"

#include <boost/test/unit_test.hpp>
#include <boost/test/parameterized_test.hpp>
#include <boost/filesystem.hpp>

#include <ixxx/util.hh>

#include <grammar/grammar.hh>
#include <grammar/asn1/mini_parser.hh>
#include <grammar/asn1/grammar.hh>

#include <array>
#include <algorithm>
#include <iostream>
#include <functional>

#include <xfsx/ber2xml.hh>
#include <xfsx/octet.hh>

#include "test.hh"

using namespace std;
using u8 = xfsx::u8;

namespace bf = boost::filesystem;

static void compare(
    const char *suffix_str,
    std::function<void(
      const u8 *begin, const u8 *end, const char *filename)>
      write_fn,
    const char *rel_filename_str
    )
{
  bf::path rel_filename(rel_filename_str);
  bf::path rel_filename_parent(rel_filename.parent_path());
  bf::path filename(rel_filename.filename());
  bf::path out_filename(filename.stem());
  out_filename += ".xml";

  bf::path suffix(suffix_str);
  bf::path in_path(test::path::in());
  in_path /= rel_filename_parent;
  bf::path in(in_path);
  in /= filename;
  bf::path ref_path(test::path::ref());
  ref_path /= suffix;
  ref_path /= rel_filename_parent;
  bf::path ref(ref_path);
  ref /= out_filename;
  bf::path out_path(test::path::out());
  out_path /= suffix;
  out_path /= rel_filename_parent;
  bf::path out(out_path);
  out /= out_filename;

  BOOST_TEST_MESSAGE("ref path: " << ref_path << ", in path: " << in_path
      <<  ", out path: " << out_path);
  BOOST_TEST_MESSAGE("ref: " << ref << ", in: " << in << ", out: " << out);

  BOOST_TEST_CHECKPOINT("remove all");
  bf::remove(out);
  BOOST_TEST_CHECKPOINT("create directories");
  bf::create_directories(out_path);

  {
    BOOST_TEST_CHECKPOINT("map file");
    auto f = ixxx::util::mmap_file(in.generic_string());
    BOOST_TEST_CHECKPOINT("write file" << out.generic_string());
    write_fn(f.begin(), f.end(), out.generic_string().c_str());
  }
  {
    BOOST_TEST_CHECKPOINT("opening ref file: "
        << ref << " (out: " << out << ")");
    auto r = ixxx::util::mmap_file(ref.generic_string());
    BOOST_TEST_CHECKPOINT("opening written file");
    auto o = ixxx::util::mmap_file(out.generic_string());
    bool are_equal = std::equal(r.begin(), r.end(), o.begin(), o.end());
    if (!are_equal) {
      cerr << "Files are not equal: " << ref << " vs. " << out << '\n';
    }
    BOOST_CHECK(are_equal);
  }


}

static void compare_tl_unber(const char *rel_filename_str)
{
  compare("unber_tl",
      static_cast<void(*)(const u8 *, const u8 *, const char*)>(
        xfsx::xml::write_unber_tl
        ),
      rel_filename_str);
}

static void compare_indent_tl_unber(const char *rel_filename_str)
{
  compare("indent_unber_tl",
      static_cast<void(*)(const u8 *, const u8 *, const char*)>(
        xfsx::xml::write_indent_unber_tl
        ),
      rel_filename_str);
}

static void write_xml(const u8 *begin, const u8 *end,
    const char* filename)
{
  xfsx::xml::write(begin, end, filename);
}

static void compare_xml(const char *rel_filename_str)
{
  compare("ber_xml", write_xml, rel_filename_str);
}

static void write_pretty_xml(const u8 *begin, const u8 *end,
    const char* filename)
{
  string tap_filename(test::path::in()
      + "/../../libgrammar/test/in/asn1/tap_3_12_strip.asn1");
  auto f = ixxx::util::mmap_file(tap_filename);
  grammar::asn1::mini::Parser parser;
  parser.set_filename(tap_filename);
  parser.read(f.s_begin(), f.s_end());
  grammar::Grammar g = parser.grammar();


  grammar::asn1::add_terminals(g);
  g.init_links();
  grammar::erase_unreachable_symbols(g);
  grammar::derive_tags(g);

  xfsx::xml::Pretty_Writer_Arguments args;
  args.translator.push(xfsx::Klasse::APPLICATION,
      grammar::tag_translations(g, 1));
  args.dereferencer.push(xfsx::Klasse::APPLICATION,
      grammar::tag_closure(g, "BCDString", 1),
      xfsx::Klasse::UNIVERSAL, 128u
      );
  args.dereferencer.push(xfsx::Klasse::APPLICATION,
      grammar::tag_closure(g, "OCTET STRING", 1),
      xfsx::Klasse::UNIVERSAL, xfsx::universal::OCTET_STRING);
  args.dereferencer.push(xfsx::Klasse::APPLICATION,
      grammar::tag_closure(g, "INTEGER", 1),
      xfsx::Klasse::UNIVERSAL, xfsx::universal::INTEGER);

  args.typifier.push(xfsx::Klasse::UNIVERSAL, 128u, xfsx::Type::BCD);
  args.typifier.push(xfsx::Klasse::UNIVERSAL, xfsx::universal::OCTET_STRING,
      xfsx::Type::OCTET_STRING);
  args.typifier.push(xfsx::Klasse::UNIVERSAL, xfsx::universal::INTEGER,
      xfsx::Type::INT_64);

  xfsx::xml::pretty_write(begin, end, filename, args);
}

static void write_pretty_xml_args(const u8 *begin, const u8 *end,
    const char* filename)
{
  string tap_filename(test::path::in()
      + "/../../libgrammar/test/in/asn1/tap_3_12_strip.asn1");
  auto f = ixxx::util::mmap_file(tap_filename);
  grammar::asn1::mini::Parser parser;
  parser.set_filename(tap_filename);
  parser.read(f.s_begin(), f.s_end());
  grammar::Grammar g = parser.grammar();


  grammar::asn1::add_terminals(g);
  g.init_links();
  grammar::erase_unreachable_symbols(g);
  grammar::derive_tags(g);

  xfsx::xml::Pretty_Writer_Arguments args;
  args.indent_size = 2;
  args.hex_dump = true;
  args.dump_tl = true;
  args.dump_t = true;
  args.dump_length = true;
  args.dump_offset = true;

  args.translator.push(xfsx::Klasse::APPLICATION,
      grammar::tag_translations(g, 1));
  args.dereferencer.push(xfsx::Klasse::APPLICATION,
      grammar::tag_closure(g, "BCDString", 1),
      xfsx::Klasse::UNIVERSAL, 128u
      );
  args.dereferencer.push(xfsx::Klasse::APPLICATION,
      grammar::tag_closure(g, "OCTET STRING", 1),
      xfsx::Klasse::UNIVERSAL, xfsx::universal::OCTET_STRING);
  args.dereferencer.push(xfsx::Klasse::APPLICATION,
      grammar::tag_closure(g, "INTEGER", 1),
      xfsx::Klasse::UNIVERSAL, xfsx::universal::INTEGER);

  args.typifier.push(xfsx::Klasse::UNIVERSAL, 128u, xfsx::Type::BCD);
  args.typifier.push(xfsx::Klasse::UNIVERSAL, xfsx::universal::OCTET_STRING,
      xfsx::Type::OCTET_STRING);
  args.typifier.push(xfsx::Klasse::UNIVERSAL, xfsx::universal::INTEGER,
      xfsx::Type::INT_64);

  xfsx::xml::pretty_write(begin, end, filename, args);
}

static void compare_pretty_xml(const char *rel_filename_str)
{
  compare("ber_pretty_xml", write_pretty_xml, rel_filename_str);
}

static void compare_pretty_xml_args(const char *rel_filename_str)
{
  compare("ber_pretty_xml_args", write_pretty_xml_args, rel_filename_str);
}

boost::unit_test::test_suite *create_ber2xml_suite()
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
  const array<const char *, 6> tap_filenames = {
      "asn1c/examples/sample.source.TAP3/sample-DataInterChange-1.ber",
      "tap_3_12_valid_add_osi.ber",
      "tap_3_12_valid_att.ber",
      "tap_3_12_valid.ber",
      "tap_3_12_valid_removed.ber",
      "tap_3_12_valid_replaced.ber"
  };

  //auto xfsx_ = BOOST_TEST_SUITE("xfsx_");
  auto ber2xml = BOOST_TEST_SUITE("xfsx_ber2xml");
  auto tl_unber = BOOST_TEST_SUITE("tl_unber");
  auto indent_tl_unber = BOOST_TEST_SUITE("indent_tl_unber");
  auto xml = BOOST_TEST_SUITE("xml");
  auto pretty_xml = BOOST_TEST_SUITE("pretty_xml");
  auto pretty_xml_args = BOOST_TEST_SUITE("pretty_xml_args");

  //ber2xml->add( BOOST_TEST_CASE( &test_case1 ) );

  tl_unber->add(BOOST_PARAM_TEST_CASE(compare_tl_unber,
        filenames.begin(), filenames.end()));
  indent_tl_unber->add(BOOST_PARAM_TEST_CASE(compare_indent_tl_unber,
        filenames.begin(), filenames.end()));
  xml->add(BOOST_PARAM_TEST_CASE(compare_xml,
        filenames.begin(), filenames.end()));
  pretty_xml->add(BOOST_PARAM_TEST_CASE(compare_pretty_xml,
        tap_filenames.begin(), tap_filenames.end()));
  pretty_xml_args->add(BOOST_PARAM_TEST_CASE(compare_pretty_xml_args,
        tap_filenames.begin(), tap_filenames.end()));
  ber2xml->add(tl_unber);
  ber2xml->add(indent_tl_unber);
  ber2xml->add(xml);
  ber2xml->add(pretty_xml);
  ber2xml->add(pretty_xml_args);
  //xfsx_->add(ber2xml);
  return ber2xml;
}

BOOST_AUTO_TEST_SUITE(xfsx_)

  BOOST_AUTO_TEST_SUITE(ber2xml)

      BOOST_AUTO_TEST_CASE(truncate)
      {
        bf::path out_path(test::path::out());
        out_path /= "ber_xml";
        bf::path out(out_path);
        out /= "truncated.xml";
        BOOST_TEST_CHECKPOINT("Removing: " << out);
        bf::remove(out);
        BOOST_TEST_CHECKPOINT("Create directories: " << out);
        bf::create_directories(out.parent_path());

        BOOST_TEST_CHECKPOINT("Writing Files: " << test::path::in());
        {
          auto f = ixxx::util::mmap_file(test::path::in()
           + "/asn1c/examples/sample.source.PKIX1/sample-Certificate-1.der");
          xfsx::xml::write(f.begin(), f.end(), out.generic_string());
        }
        size_t n = bf::file_size(out);
        {
          auto f = ixxx::util::mmap_file(test::path::in()
           + "/asn1c/examples/sample.source.TAP3/sample-DataInterChange-1.ber");
          xfsx::xml::write(f.begin(), f.end(), out.generic_string());
        }
        size_t m = bf::file_size(out);
        BOOST_CHECK(m < n);
      }

      BOOST_AUTO_TEST_CASE(content_overflow)
      {
        using namespace xfsx;
        Unit u;
        u.init_tag(23);
        u.init_length(16258469694287840439lu);
        array<u8, 16> a;
        auto r = u.write(a.begin(), a.end());
        BOOST_CHECK_EQUAL(r-a.begin(), 10);

        xfsx::byte::writer::Memory w;
        BOOST_CHECK_THROW(xfsx::xml::write(a.begin(), a.end(), w),
            std::range_error);
      }

  BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

