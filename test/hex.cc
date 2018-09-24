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

#include <boost/mpl/list.hpp>

#include <vector>
#include <array>
#include <iostream>
#include <limits>
#include <string>

#include <xfsx/hex_impl.hh>
#include <xfsx/octet.hh>

using namespace std;

using u8 = xfsx::u8;

BOOST_AUTO_TEST_SUITE(xfsx_)

  BOOST_AUTO_TEST_SUITE(hex_)

    BOOST_AUTO_TEST_SUITE(impl_)

      using namespace xfsx::hex;
      using namespace xfsx::hex::impl;

      BOOST_AUTO_TEST_SUITE(decode_)

        BOOST_AUTO_TEST_SUITE(sub)

          BOOST_AUTO_TEST_CASE(escape_c_style)
          {
            u8 b = u8('A');
            array<char, 16> a;
            auto r = Escape<Style::C>()(b, a.begin());
            BOOST_REQUIRE(r == a.begin() + 4);
            BOOST_CHECK_EQUAL(string(a.begin(), a.begin() + 4), "\\x41");
          }

          BOOST_AUTO_TEST_CASE(escape_xml_style)
          {
            u8 b = u8('A');
            array<char, 16> a;
            auto r = Escape<Style::XML>()(b, a.begin());
            BOOST_REQUIRE(r == a.begin() + 6);
            BOOST_CHECK_EQUAL(string(a.begin(), a.begin() + 6), "&#x41;");
          }
          
        BOOST_AUTO_TEST_SUITE_END()

        BOOST_AUTO_TEST_SUITE(main)

          BOOST_AUTO_TEST_CASE(empty)
          {
            const char inp[] = "";
            const u8 *begin = reinterpret_cast<const u8*>(inp);
            const u8 *end = begin + sizeof(inp)-1;
            array<char, 16> a;
            auto r = impl::decode<Style::C>(begin, end, a.begin());
            BOOST_CHECK(r == a.begin());
          }
          BOOST_AUTO_TEST_CASE(one)
          {
            const char inp[] = "a";
            const u8 *begin = reinterpret_cast<const u8*>(inp);
            const u8 *end = begin + sizeof(inp)-1;
            array<char, 16> a;
            auto r = impl::decode<Style::C>(begin, end, a.begin());
            BOOST_REQUIRE(r == a.begin()+1);
            BOOST_CHECK_EQUAL(string(a.begin(), a.begin()+1), "a");
          }
          BOOST_AUTO_TEST_CASE(one_special)
          {
            const char inp[] = "\\";
            const u8 *begin = reinterpret_cast<const u8*>(inp);
            const u8 *end = begin + sizeof(inp)-1;
            array<char, 16> a;
            auto r = impl::decode<Style::C>(begin, end, a.begin());
            BOOST_REQUIRE(r == a.begin()+4);
            BOOST_CHECK_EQUAL(string(a.begin(), a.begin()+4), "\\x5c");
          }
          BOOST_AUTO_TEST_CASE(large)
          {
            const char inp[] = "He\\llo &Wor\x01ld - foo \x03\x04\x05""bar!";
            const u8 *begin = reinterpret_cast<const u8*>(inp);
            const u8 *end = begin + sizeof(inp)-1;
            array<char, 64> a;
            auto r = impl::decode<Style::C>(begin, end, a.begin());
            const char ref[] = "He\\x5cllo &Wor\\x01ld - foo \\x03\\x04\\x05bar!";
            BOOST_REQUIRE_EQUAL(size_t(r - a.begin()), strlen(ref));
            BOOST_CHECK_EQUAL(string(a.begin(), a.begin() + strlen(ref)), ref);
          }
          BOOST_AUTO_TEST_CASE(large_xml)
          {
            const char inp[] = "He\\llo &Wor\x01ld - foo \x03\x04\x05" "bar!";
            const u8 *begin = reinterpret_cast<const u8*>(inp);
            const u8 *end = begin + sizeof(inp)-1;
            array<char, 64> a;
            auto r = impl::decode<Style::XML>(begin, end, a.begin());
            const char ref[] = "He\\llo &#x26;Wor&#x01;ld - foo &#x03;&#x04;&#x05;bar!";
            BOOST_REQUIRE_EQUAL(size_t(r - a.begin()), strlen(ref));
            BOOST_CHECK_EQUAL(string(a.begin(), a.begin() + strlen(ref)), ref);
          }
          BOOST_AUTO_TEST_CASE(large_gt_127)
          {
            const char inp[] = "foo\xca\xfe\xff";
            const u8 *begin = reinterpret_cast<const u8*>(inp);
            const u8 *end = begin + sizeof(inp)-1;
            array<char, 64> a;
            auto r = impl::decode<Style::C>(begin, end, a.begin());
            const char ref[] = "foo\\xca\\xfe\\xff";
            BOOST_REQUIRE_EQUAL(size_t(r - a.begin()), strlen(ref));
            BOOST_CHECK_EQUAL(string(a.begin(), a.begin() + strlen(ref)), ref);
          }

          BOOST_AUTO_TEST_CASE(xml_gt_lt)
          {
            const char inp[] = "<foo>Hello</foo><bar>World</bar>";
            const u8 *begin = reinterpret_cast<const u8*>(inp);
            const u8 *end = begin + sizeof(inp)-1;
            array<char, 128> a;
            auto r = impl::decode<Style::XML>(begin, end, a.begin());
            const char ref[] = "&#x3c;foo&#x3e;Hello&#x3c;/foo&#x3e;&#x3c;bar&#x3e;World&#x3c;/bar&#x3e;";
            BOOST_REQUIRE_EQUAL(size_t(r - a.begin()), sizeof(ref)-1);
            BOOST_CHECK_EQUAL(string(a.begin(), a.begin() + sizeof(ref)-1), ref);
          }

        BOOST_AUTO_TEST_SUITE_END()

      BOOST_AUTO_TEST_SUITE_END()

      BOOST_AUTO_TEST_SUITE(encode_)

        using namespace xfsx::hex::impl;

        BOOST_AUTO_TEST_SUITE(sub)

          BOOST_AUTO_TEST_SUITE(next_quoted)

            BOOST_AUTO_TEST_CASE(none)
            {
              const char inp[] = "hello world";
              const char *begin = inp;
              const char *end = inp + sizeof(inp)-1;
              auto r = Next_Quoted::Base<Style::XML>()(begin, end);
              BOOST_REQUIRE(r == end);
            }

            BOOST_AUTO_TEST_CASE(one)
            {
              const char inp[] = "hello&#x20;world";
              const char *begin = inp;
              const char *end = inp + sizeof(inp)-1;
              auto r = Next_Quoted::Base<Style::XML>()(begin, end);
              BOOST_REQUIRE(r == begin+5);
            }

          BOOST_AUTO_TEST_SUITE_END()

        BOOST_AUTO_TEST_SUITE_END()

        BOOST_AUTO_TEST_SUITE(main)

          BOOST_AUTO_TEST_CASE(empty)
          {
            const char inp[] = "";
            const char *begin = inp;
            const char *end = inp + sizeof(inp)-1;
            array<u8, 16> a;
            auto r = impl::encode<Style::XML>(begin, end, a.begin());
            BOOST_REQUIRE(r == a.begin());
          }

          BOOST_AUTO_TEST_CASE(none)
          {
            const char inp[] = "Hello World";
            const char *begin = inp;
            const char *end = inp + sizeof(inp)-1;
            array<u8, 16> a;
            auto r = impl::encode<Style::XML>(begin, end, a.begin());
            BOOST_REQUIRE_EQUAL(r - a.begin(), end-begin);
            const char *b = reinterpret_cast<const char*>(a.begin());
            const char *e = b + (end-begin);
            string s(b, e);
            BOOST_CHECK_EQUAL(inp, s);
          }

          BOOST_AUTO_TEST_CASE(one)
          {
            const char inp[] = "Hello&#x20;World";
            const char *begin = inp;
            const char *end = inp + sizeof(inp)-1;
            array<u8, 16> a;
            auto r = impl::encode<Style::XML>(begin, end, a.begin());
            BOOST_REQUIRE_EQUAL(r - a.begin(), 11);
            const char *b = reinterpret_cast<const char*>(a.begin());
            const char *e = b + 11;
            string s(b, e);
            const char ref[] = "Hello World";
            BOOST_CHECK_EQUAL(s, ref);
          }

          BOOST_AUTO_TEST_CASE(one_c)
          {
            const char inp[] = "Hello\\x20World";
            const char *begin = inp;
            const char *end = inp + sizeof(inp)-1;
            array<u8, 16> a;
            auto r = impl::encode<Style::C>(begin, end, a.begin());
            BOOST_REQUIRE_EQUAL(r - a.begin(), 11);
            const char *b = reinterpret_cast<const char*>(a.begin());
            const char *e = b + 11;
            string s(b, e);
            const char ref[] = "Hello World";
            BOOST_CHECK_EQUAL(s, ref);
          }

          BOOST_AUTO_TEST_CASE(end)
          {
            const char inp[] = "Hello World&#x";
            const char *begin = inp;
            const char *end = inp + sizeof(inp)-1;
            array<u8, 16> a;
            auto r = impl::encode<Style::XML>(begin, end, a.begin());
            BOOST_REQUIRE_EQUAL(r - a.begin(), 14);
            const char *b = reinterpret_cast<const char*>(a.begin());
            const char *e = b + 14;
            string s(b, e);
            const char ref[] = "Hello World&#x";
            BOOST_CHECK_EQUAL(s, ref);
          }

          BOOST_AUTO_TEST_CASE(end_c)
          {
            const char inp[] = "Hello World\\x";
            const char *begin = inp;
            const char *end = inp + sizeof(inp)-1;
            array<u8, 16> a;
            auto r = impl::encode<Style::C>(begin, end, a.begin());
            BOOST_REQUIRE_EQUAL(r - a.begin(), 13);
            const char *b = reinterpret_cast<const char*>(a.begin());
            const char *e = b + 13;
            string s(b, e);
            const char ref[] = "Hello World\\x";
            BOOST_CHECK_EQUAL(s, ref);
          }

          BOOST_AUTO_TEST_CASE(larger)
          {
            const char inp[] = "&#xde;Hel&#xad;&#xca;&#xfe;&#x01;&#x02;lo Wor&#x23;ld";
            const char *begin = inp;
            const char *end = inp + sizeof(inp)-1;
            array<u8, 64> a;
            auto r = impl::encode<Style::XML>(begin, end, a.begin());
            const char ref[] = "\xdeHel\xad\xca\xfe\x01\x02lo Wor\x23ld";
            size_t m = sizeof(ref)-1;
            BOOST_REQUIRE_EQUAL(r - a.begin(), ssize_t(m));
            const char *b = reinterpret_cast<const char*>(a.begin());
            const char *e = b + m;
            string s(b, e);
            BOOST_CHECK_EQUAL(s, ref);
          }

          BOOST_AUTO_TEST_CASE(larger_id_c)
          {
            const char inp[] = "\xdeHel\xad\xca\xfe\x01\x02lo Wor\x23ld";
            const char *begin = inp;
            const char *end = inp + sizeof(inp)-1;
            array<u8, 64> a;
            auto r = impl::encode<Style::C>(begin, end, a.begin());
            const char ref[] = "\xdeHel\xad\xca\xfe\x01\x02lo Wor\x23ld";
            size_t m = sizeof(ref)-1;
            BOOST_REQUIRE_EQUAL(r - a.begin(), ssize_t(m));
            const char *b = reinterpret_cast<const char*>(a.begin());
            const char *e = b + m;
            string s(b, e);
            BOOST_CHECK_EQUAL(s, ref);
          }

          BOOST_AUTO_TEST_CASE(larger_c)
          {
            const char inp[] = "\\xdeHel\\xad\\xca\\xfe\\x01\\x02lo Wor\\x23ld";
            const char *begin = inp;
            const char *end = inp + sizeof(inp)-1;
            array<u8, 64> a;
            auto r = impl::encode<Style::C>(begin, end, a.begin());
            const char ref[] = "\xdeHel\xad\xca\xfe\x01\x02lo Wor\x23ld";
            size_t m = sizeof(ref)-1;
            BOOST_REQUIRE_EQUAL(r - a.begin(), ssize_t(m));
            const char *b = reinterpret_cast<const char*>(a.begin());
            const char *e = b + m;
            string s(b, e);
            BOOST_CHECK_EQUAL(s, ref);
          }

          BOOST_AUTO_TEST_CASE(upper_c)
          {
            const char inp[] = "\\xDEHel\\xAd\\xcA\\xfe\\x01\\x02lo Wor\\x23ld";
            const char *begin = inp;
            const char *end = inp + sizeof(inp)-1;
            array<u8, 64> a;
            auto r = impl::encode<Style::C>(begin, end, a.begin());
            const char ref[] = "\xdeHel\xad\xca\xfe\x01\x02lo Wor\x23ld";
            size_t m = sizeof(ref)-1;
            BOOST_REQUIRE_EQUAL(r - a.begin(), ssize_t(m));
            const char *b = reinterpret_cast<const char*>(a.begin());
            const char *e = b + m;
            string s(b, e);
            BOOST_CHECK_EQUAL(s, ref);
          }

        BOOST_AUTO_TEST_SUITE_END()

      BOOST_AUTO_TEST_SUITE_END()


    BOOST_AUTO_TEST_SUITE_END() // impl_

    BOOST_AUTO_TEST_SUITE(main)

      using namespace xfsx::hex;

      BOOST_AUTO_TEST_SUITE(decode_)

        BOOST_AUTO_TEST_CASE(basic)
        {
          const char inp[] = "\x10""foo bar32\x00";
          const u8 *begin = reinterpret_cast<const u8*>(inp);
          const u8 *end = begin + sizeof(inp)-1;
          size_t n = decoded_size<Style::XML>(begin, end);
          vector<char> a(n);
          auto r = decode<Style::XML>(begin, end, a.data());
          BOOST_REQUIRE_EQUAL(size_t(r-a.data()), n);
          string s(a.begin(), a.end());
          const char ref[] = "&#x10;foo bar32&#x00;";
          BOOST_CHECK_EQUAL(s, ref);
        }

        BOOST_AUTO_TEST_CASE(raw)
        {
          const char inp[] = "Hello";
          const u8 *begin = reinterpret_cast<const u8*>(inp);
          const u8 *end = begin + sizeof(inp)-1;
          size_t n = decoded_size<Style::Raw>(begin, end);
          BOOST_REQUIRE_EQUAL(n, 10u);
          vector<char> a(n);
          auto r = decode<Style::Raw>(begin, end, a.data());
          BOOST_REQUIRE_EQUAL(size_t(r-a.data()), n);
          string s(a.begin(), a.end());
          const char ref[] = "48656c6c6f";
          BOOST_CHECK_EQUAL(s, ref);
        }

      BOOST_AUTO_TEST_SUITE_END()

      BOOST_AUTO_TEST_SUITE(encode_)

        BOOST_AUTO_TEST_CASE(basic)
        {
          const char inp[] = "&#xca;Divide&#xfe; and Conquer";
          const char *begin = inp;
          const char *end = inp + sizeof(inp)-1;
          array<u8, 64> a;
          auto r = encode<Style::XML>(begin, end, a.begin());
          BOOST_REQUIRE_EQUAL(r - a.begin(),
              ssize_t(encoded_size<Style::XML>(begin, end)));
          const char ref[] = "\xca""Divide\xfe and Conquer";
          size_t m = sizeof(ref)-1;
          const char *b = reinterpret_cast<const char*>(a.begin());
          const char *e = b + m;
          string s(b, e);
          BOOST_CHECK_EQUAL(s, ref);
        }

        BOOST_AUTO_TEST_CASE(raw)
        {
          const char inp[] = "48656c6c6f";
          const char *begin = inp;
          const char *end = inp + sizeof(inp)-1;
          array<u8, 64> a;
          auto r = encode<Style::Raw>(begin, end, a.begin());
          BOOST_REQUIRE_EQUAL(r - a.begin(),
              ssize_t(encoded_size<Style::Raw>(begin, end)));
          const char ref[] = "Hello";
          size_t m = sizeof(ref)-1;
          const char *b = reinterpret_cast<const char*>(a.begin());
          const char *e = b + m;
          string s(b, e);
          BOOST_CHECK_EQUAL(s, ref);
        }

      BOOST_AUTO_TEST_SUITE_END()

    BOOST_AUTO_TEST_SUITE_END()

  BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
