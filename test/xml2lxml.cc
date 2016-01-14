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

#include <xfsx/xml2lxml.hh>

#include <string>

using namespace std;

BOOST_AUTO_TEST_SUITE(xfsx_)

  BOOST_AUTO_TEST_SUITE(xml_)
  
    BOOST_AUTO_TEST_SUITE(to_l2_)

      using namespace xfsx::xml::l2;

      BOOST_AUTO_TEST_CASE(basic)
      {
        const char inp[] = "<root><foo/><bar>\n"
          "    <x>Hello</x><y>World</y></bar>\n"
          "</root>\n";
        xxxml::doc::Ptr doc = generate_tree(inp, inp+sizeof(inp)-1);
        const xmlNode* root = xxxml::doc::get_root_element(doc);
        BOOST_REQUIRE(root);
        BOOST_CHECK_EQUAL(xxxml::name(root), "root");
        BOOST_CHECK_EQUAL(xxxml::child_element_count(root), 2u);
      }

      BOOST_AUTO_TEST_CASE(content)
      {
        const char inp[] = "<root><foo/><bar>\n"
          "    <x>Hello</x><y>World</y></bar>\n"
          "</root>\n";
        xxxml::doc::Ptr doc = generate_tree(inp, inp+sizeof(inp)-1);
        const xmlNode* root = xxxml::doc::get_root_element(doc);
        BOOST_REQUIRE(root);
        pair<xxxml::Char_Ptr, size_t> dump =
          xxxml::doc::dump_format_memory(doc);
        string s(reinterpret_cast<const char*>(dump.first.get()), dump.second);
        BOOST_CHECK_EQUAL(s, "<?xml version=\"1.0\"?>\n"
            "<root><foo/><bar><x>Hello</x><y>World</y></bar>\n"
            "</root>\n");
      }

      BOOST_AUTO_TEST_CASE(throw_closing)
      {
        const char inp[] = "<root><foo/></bar>\n"
          "    </x>Hello</x><y>World</y></bar>\n"
          "</root>\n";
        BOOST_CHECK_THROW(generate_tree(inp, inp+sizeof(inp)-1),
            std::runtime_error);
      }

      BOOST_AUTO_TEST_CASE(count)
      {
        const char inp[] = "<root><foo/><bar>"
          "<x>Hello</x><y>World</y></bar>"
          "</root>\n";
        xxxml::doc::Ptr doc = generate_tree(inp, inp+sizeof(inp)-1, 4u);
        const xmlNode* root = xxxml::doc::get_root_element(doc);
        BOOST_REQUIRE(root);
        pair<xxxml::Char_Ptr, size_t> dump =
          xxxml::doc::dump_format_memory(doc);
        string s(reinterpret_cast<const char*>(dump.first.get()), dump.second);
        BOOST_CHECK_EQUAL(s, "<?xml version=\"1.0\"?>\n"
            "<root>\n  <foo/>\n  <bar>\n    <x>Hello</x>\n  </bar>\n"
            "</root>\n");
      }

    BOOST_AUTO_TEST_SUITE_END()

  BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
