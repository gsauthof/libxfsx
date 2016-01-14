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

#include <xfsx/xml.hh>
#include <xfsx/s_pair.hh>

#include <string>

using namespace std;

BOOST_AUTO_TEST_SUITE(xfsx_)

  BOOST_AUTO_TEST_SUITE(xml_)

    using namespace xfsx;
    using namespace xfsx::xml;


    BOOST_AUTO_TEST_CASE(none)
    {
      const char inp[] = "Hello World";
      Element_Finder f(inp, inp+sizeof(inp)-1);
      auto i = f.begin();
      BOOST_REQUIRE(i == f.end());
    }

    BOOST_AUTO_TEST_CASE(one)
    {
      const char inp[] = "Hello <foo>World";
      Element_Finder f(inp, inp+sizeof(inp)-1);
      auto i = f.begin();
      BOOST_REQUIRE(i != f.end());
      auto &p = *i;
      BOOST_CHECK_EQUAL(string(p.first, p.second), "foo");
      ++i;
      BOOST_REQUIRE(i == f.end());
    }

    BOOST_AUTO_TEST_CASE(more)
    {
      const char inp[] = "<foo>Hello </foo><baz/>World<bar></bar>";
      Element_Finder f(inp, inp+sizeof(inp)-1);
      auto i = f.begin();
      BOOST_REQUIRE(i != f.end());
      auto &p = *i;
      BOOST_CHECK_EQUAL(string(p.first, p.second), "foo");
      ++i;
      BOOST_REQUIRE(i != f.end());
      BOOST_CHECK_EQUAL(string((*i).first, (*i).second), "/foo");
      ++i;
      BOOST_REQUIRE(i != f.end());
      BOOST_CHECK_EQUAL(string((*i).first, (*i).second), "baz/");
      ++i;
      BOOST_REQUIRE(i != f.end());
      BOOST_CHECK_EQUAL(string((*i).first, (*i).second), "bar");
      ++i;
      BOOST_REQUIRE(i != f.end());
      BOOST_CHECK_EQUAL(string((*i).first, (*i).second), "/bar");
      ++i;
      BOOST_REQUIRE(i == f.end());
    }

    BOOST_AUTO_TEST_CASE(content)
    {
      const char inp[] = "<records><record><a>hello</a><b>world</b></record></records>";
      Element_Finder f(inp, inp+sizeof(inp)-1);
      auto i = f.begin();
      BOOST_REQUIRE(i != f.end());
      BOOST_CHECK_EQUAL(s_pair::mk_string(element_name(*i)), "records");
      BOOST_CHECK_EQUAL(is_start_end_tag(*i), false);
      ++i;
      BOOST_REQUIRE(i != f.end());
      BOOST_CHECK_EQUAL(s_pair::mk_string(element_name(*i)), "record");
      BOOST_CHECK_EQUAL(is_start_end_tag(*i), false);
      ++i;
      BOOST_REQUIRE(i != f.end());
      BOOST_CHECK_EQUAL(s_pair::mk_string(element_name(*i)), "a");
      BOOST_CHECK_EQUAL(is_start_end_tag(*i), false);
      auto p = *i;
      ++i;
      BOOST_REQUIRE(i != f.end());
      BOOST_CHECK_EQUAL(s_pair::mk_string(element_name(*i)), "a");
      BOOST_CHECK_EQUAL(is_end_tag(*i), true);
      auto q = *i;
      BOOST_CHECK_EQUAL(string(p.second+1, q.first-1), "hello");
    }

    BOOST_AUTO_TEST_CASE(traverser)
    {
      const char inp[] = "<records><record><a>hello</a><b>world</b></record></records>";
      Element_Traverser t(inp, inp+sizeof(inp)-1);
      BOOST_REQUIRE(t.has_more());
      BOOST_CHECK_EQUAL(s_pair::mk_string(element_name(*t)), "records");
      BOOST_CHECK_EQUAL(is_start_end_tag(*t), false);
      ++t;
      BOOST_REQUIRE(t.has_more());
      BOOST_CHECK_EQUAL(s_pair::mk_string(element_name(*t)), "record");
      BOOST_CHECK_EQUAL(is_start_end_tag(*t), false);
      ++t;
      BOOST_REQUIRE(t.has_more());
      BOOST_CHECK_EQUAL(s_pair::mk_string(element_name(*t)), "a");
      BOOST_CHECK_EQUAL(is_start_end_tag(*t), false);
      auto p = *t;
      ++t;
      BOOST_REQUIRE(t.has_more());
      BOOST_CHECK_EQUAL(s_pair::mk_string(element_name(*t)), "a");
      BOOST_CHECK_EQUAL(is_end_tag(*t), true);
      auto q = *t;
      BOOST_CHECK_EQUAL(string(p.second+1, q.first-1), "hello");
      ++t;
      BOOST_REQUIRE(t.has_more());
      ++t;
      BOOST_REQUIRE(t.has_more());
      ++t;
      BOOST_REQUIRE(t.has_more());
      ++t;
      BOOST_REQUIRE(t.has_more());
      ++t;
      BOOST_REQUIRE(!t.has_more());
    }
    BOOST_AUTO_TEST_CASE(traverser_with_comments)
    {
      const char inp[] = "<!-- splice this --><records><record> <!-- and that --> <a>hello</a><b>world</b></record></records><!-- end-->";
      Element_Traverser t(inp, inp+sizeof(inp)-1);
      BOOST_REQUIRE(t.has_more());
      BOOST_CHECK_EQUAL(s_pair::mk_string(element_name(*t)), "records");
      BOOST_CHECK_EQUAL(is_start_end_tag(*t), false);
      ++t;
      BOOST_REQUIRE(t.has_more());
      BOOST_CHECK_EQUAL(s_pair::mk_string(element_name(*t)), "record");
      BOOST_CHECK_EQUAL(is_start_end_tag(*t), false);
      ++t;
      BOOST_REQUIRE(t.has_more());
      BOOST_CHECK_EQUAL(s_pair::mk_string(element_name(*t)), "a");
      BOOST_CHECK_EQUAL(is_start_end_tag(*t), false);
      auto p = *t;
      ++t;
      BOOST_REQUIRE(t.has_more());
      BOOST_CHECK_EQUAL(s_pair::mk_string(element_name(*t)), "a");
      BOOST_CHECK_EQUAL(is_end_tag(*t), true);
      auto q = *t;
      BOOST_CHECK_EQUAL(string(p.second+1, q.first-1), "hello");
      ++t;
      BOOST_REQUIRE(t.has_more());
      BOOST_CHECK_EQUAL(s_pair::mk_string(element_name(*t)), "b");
      ++t;
      BOOST_REQUIRE(t.has_more());
      BOOST_CHECK_EQUAL(s_pair::mk_string(element_name(*t)), "b");
      ++t;
      BOOST_REQUIRE(t.has_more());
      BOOST_CHECK_EQUAL(s_pair::mk_string(element_name(*t)), "record");
      ++t;
      BOOST_REQUIRE(t.has_more());
      BOOST_CHECK_EQUAL(s_pair::mk_string(element_name(*t)), "records");
      ++t;
      BOOST_REQUIRE(!t.has_more());
    }

    BOOST_AUTO_TEST_CASE(equal_content)
    {
      const char inp[] = "<abc>hello</abc>";
      Element_Traverser t(inp, inp+sizeof(inp)-1);
      BOOST_REQUIRE(t.has_more());
      BOOST_CHECK_EQUAL(s_pair::equal(*t, "abc", 3), true);
      BOOST_CHECK_EQUAL(is_start_end_tag(*t), false);
      auto p = *t;
      ++t;
      BOOST_REQUIRE(t.has_more());
      BOOST_CHECK_EQUAL(xfsx::s_pair::equal(element_name(*t), "abc", 3), true);
      BOOST_CHECK_EQUAL(is_end_tag(*t), true);
      BOOST_CHECK_EQUAL(
          xfsx::s_pair::equal(xfsx::xml::content(p, *t), "hello", 5), true);
    }

    BOOST_AUTO_TEST_CASE(is_end)
    {
      const char inp[] = "/foo";
      BOOST_CHECK_EQUAL(is_end_tag(make_pair(inp, inp+sizeof(inp)-1)), true);
      BOOST_CHECK_EQUAL(is_end_tag(make_pair(inp+1, inp+sizeof(inp)-1)), false);
      BOOST_CHECK_EQUAL(is_end_tag(make_pair(inp, inp)), false);
    }

    BOOST_AUTO_TEST_CASE(is_start_end)
    {
      const char inp[] = "/foo/";
      BOOST_CHECK_EQUAL(
          is_start_end_tag(make_pair(inp, inp+sizeof(inp)-1)), true);
      BOOST_CHECK_EQUAL(
          is_start_end_tag(make_pair(inp+1, inp+sizeof(inp)-1)), true);
      BOOST_CHECK_EQUAL(
          is_start_end_tag(make_pair(inp+1, inp+sizeof(inp)-2)), false);
      BOOST_CHECK_EQUAL(is_start_end_tag(make_pair(inp, inp)), false);
      BOOST_CHECK_EQUAL(
          is_start_end_tag(make_pair(inp+sizeof(inp)-2, inp+sizeof(inp)-2)),
          false);
    }

    BOOST_AUTO_TEST_CASE(names)
    {
      const char inp[] = "foo x='12'";
      auto p = element_name(make_pair(inp, inp+3));
      BOOST_CHECK_EQUAL(string(p.first, p.second), "foo");
      p = element_name(make_pair(inp, inp+4));
      BOOST_CHECK_EQUAL(string(p.first, p.second), "foo");
      p = element_name(make_pair(inp, inp+sizeof(inp)-1));
      BOOST_CHECK_EQUAL(string(p.first, p.second), "foo");
      BOOST_CHECK_THROW(element_name(make_pair(inp, inp)), std::runtime_error);
      BOOST_CHECK_THROW(
          element_name(make_pair(inp+3, inp+4)), std::runtime_error);
    }

    BOOST_AUTO_TEST_CASE(att_name)
    {
      const char inp[] = "foo xyz='12'  ";
      auto p = attribute_name(inp+3, inp+sizeof(inp)-1);
      BOOST_CHECK_EQUAL(string(p.first, p.second), "xyz");
    }
    BOOST_AUTO_TEST_CASE(att_none)
    {
      const char inp[] = "foo    ";
      auto p = attribute_name(inp+3, inp+sizeof(inp)-1);
      BOOST_CHECK(p.first == p.second);
      BOOST_CHECK(p.second == inp+sizeof(inp)-1);
    }

    BOOST_AUTO_TEST_CASE(att_space)
    {
      const char inp[] = "foo xyz = '12'  ";
      auto p = attribute_name(inp+3, inp+sizeof(inp)-1);
      BOOST_CHECK_EQUAL(string(p.first, p.second), "xyz");
      p = attribute_value(p.second+1, inp+sizeof(inp)-1);
      BOOST_CHECK_EQUAL(string(p.first, p.second), "12");
      BOOST_CHECK_THROW(attribute_value(inp+7, inp+13), std::runtime_error);
      BOOST_CHECK_THROW(
          attribute_value(inp+9, inp+sizeof(inp)-1), std::runtime_error);
    }
    BOOST_AUTO_TEST_CASE(att_quote_mismatch)
    {
      const char inp[] = "foo xyz = '12\"  ";
      auto p = attribute_name(inp+3, inp+sizeof(inp)-1);
      BOOST_CHECK_EQUAL(string(p.first, p.second), "xyz");
      BOOST_CHECK_THROW(attribute_value(p.second+1, inp+sizeof(inp)-1),
          std::runtime_error);
    }

    BOOST_AUTO_TEST_CASE(att_traverser_none)
    {
      const char inp[] = "name";
      auto p = make_pair(inp, inp+sizeof(inp)-1);
      Attribute_Traverser t(p, p);
      BOOST_CHECK(t.has_more() == false);
    }

    BOOST_AUTO_TEST_CASE(att_traverser)
    {
      const char inp[] = "name att1='23' att2= '42' ";
      auto p = make_pair(inp, inp+sizeof(inp)-1);
      Attribute_Traverser t(p, make_pair(inp, inp+4));
      BOOST_REQUIRE(t.has_more() == true);
      BOOST_CHECK_EQUAL(s_pair::mk_string(t.name()), "att1");
      BOOST_CHECK_EQUAL(s_pair::mk_string(t.value()), "23");
      ++t;
      BOOST_REQUIRE(t.has_more() == true);
      BOOST_CHECK_EQUAL(s_pair::mk_string(t.name()), "att2");
      BOOST_CHECK_EQUAL(s_pair::mk_string(t.value()), "42");
      ++t;
      BOOST_REQUIRE(t.has_more() == false);
    }

    BOOST_AUTO_TEST_CASE(start_end_att)
    {
      const char inp[] = "c tag='153' class='APPLICATION'/";
      auto p = make_pair(inp, inp+sizeof(inp)-1);
      Attribute_Traverser t(p, make_pair(inp, inp+1));
      BOOST_REQUIRE(t.has_more() == true);
      ++t;
      BOOST_REQUIRE(t.has_more() == true);
      ++t;
      BOOST_REQUIRE(t.has_more() == false);
    }



  BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
