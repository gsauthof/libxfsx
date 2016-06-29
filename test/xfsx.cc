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

#include <vector>
#include <array>
#include <iostream>
#include <limits>

#include <boost/filesystem.hpp>

#include <xfsx/xfsx.hh>

#include <ixxx/util.hh>

#include "test.hh"

using namespace std;

BOOST_AUTO_TEST_SUITE(xfsx_)

  BOOST_AUTO_TEST_SUITE(unit)

   BOOST_AUTO_TEST_SUITE(read)
    // {{{

    BOOST_AUTO_TEST_CASE(klasses)
    {
      using namespace xfsx;
      const char inp1[] = "UNIVERSAL";
      const char inp2[] = "APPLICATION";
      BOOST_CHECK_EQUAL(str_to_klasse(make_pair(inp1, inp1+sizeof(inp1)-1)),
          Klasse::UNIVERSAL);
      BOOST_CHECK_EQUAL(str_to_klasse(make_pair(inp2, inp2+sizeof(inp2)-1)),
          Klasse::APPLICATION);
      BOOST_CHECK_THROW(str_to_klasse(make_pair(inp1, inp1+3)),
          std::range_error);
    }


    BOOST_AUTO_TEST_CASE(primitive)
    {
      using namespace xfsx;
      Unit u;
      const array<uint8_t, 4> a = {
        0b01'0'00101u,
        0b0'000'0010u,
        0u,
        0u
      };
      const uint8_t *r = u.read(a.begin(), a.end());
      BOOST_CHECK_EQUAL(u.shape, Shape::PRIMITIVE);
      BOOST_CHECK(r == a.end());
    }

    BOOST_AUTO_TEST_CASE(constructed)
    {
      using namespace xfsx;
      Unit u;
      const array<uint8_t, 4> a = {
        0b01'1'00101u,
        0b0'000'0010u,
        0u,
        0u
      };
      const uint8_t *r = u.read(a.begin(), a.end());
      BOOST_CHECK_EQUAL(u.shape, Shape::CONSTRUCTED);
      BOOST_CHECK(r == a.end()-2);
    }

    BOOST_AUTO_TEST_CASE(too_short)
    {
      using namespace xfsx;
      Unit u;
      const array<uint8_t, 3> a = {
        0b01'0'00101u,
        0b0'000'0010u,
        0u,
      };
      BOOST_CHECK_THROW(u.read(a.begin(), a.end()), std::range_error);
      BOOST_CHECK_EQUAL(u.tl_size, 2);
      BOOST_CHECK_EQUAL(u.length, 2);
    }

    BOOST_AUTO_TEST_CASE(klassen)
    {
      using namespace xfsx;
      const array<Klasse, 4> ks = { Klasse::UNIVERSAL,
        Klasse::APPLICATION,
        Klasse::CONTEXT_SPECIFIC,
        Klasse::PRIVATE };
      for (auto k : ks) {
        Unit u;
        array<uint8_t, 4> a = {
          0b01'0'00101u,
          0b0'000'0010u,
          0u,
          0u
        };
        a[0] &= 0b00'11'1111u;
        a[0] |= static_cast<uint8_t>(k);
        const uint8_t *r = u.read(a.begin(), a.end());
        BOOST_CHECK_EQUAL(u.klasse, k);
        BOOST_CHECK(r == a.end());
      }
    }

    BOOST_AUTO_TEST_CASE(short_tags)
    {
      using namespace xfsx;
      struct Input { size_t tag; bool is_long_tag; uint8_t t_size; };
      //const array<Input, 4> ts = {
      const Input ts[4] = {
        { 0, false, 1 },
        { 1, false, 1 },
        { 29, false, 1 },
        { 30, false, 1 }
      };
      for (auto t : ts) {
        Unit u;
        const array<uint8_t, 4> a = {
          0b01'0'00101u,
          0b0'000'0010u,
          0u,
          0u
        };
        vector<uint8_t> v;
        v.push_back(a[0]);
        v[0] &= 0b11'1'00000;
        uint8_t b = t.tag;
        v[0] |= b;
        v.insert(v.end(), a.begin()+1, a.end());
        const uint8_t *r = u.read(v.data(), v.data() + v.size());
        BOOST_CHECK_EQUAL(u.tag, t.tag);
        BOOST_CHECK(r == v.data() + v.size());
        BOOST_CHECK_EQUAL(u.t_size, t.t_size);
        BOOST_CHECK_EQUAL(u.tl_size, t.t_size+1);
        BOOST_CHECK_EQUAL(u.is_long_tag, false);
      }
    }

    BOOST_AUTO_TEST_CASE(long_tags)
    {
      using namespace xfsx;
      struct Input { size_t tag; bool is_long_tag; uint8_t t_size; };
      //const array<Input, 4> ts = {
      const Input ts[12] = {
        { 31, true, 2 },
        { 32, true, 2 },
        { 126, true, 2 },
        { 127, true, 2 },
        { 128, true, 3 },
        { 130, true, 3 },
        { 1024, true, 3 },
        { 128*128-1, true, 3 },
        { 128*128, true, 4 },
        { 128*128+1, true, 4 },
        { numeric_limits<uint32_t>::max()-1, true, 6 },
        { numeric_limits<uint32_t>::max(), true, 6 }
      };
      for (auto t : ts) {
        Unit u;
        const array<uint8_t, 4> a = {
          0b01'0'11111u,
          0b0'000'0010u,
          0u,
          0u
        };
        vector<uint8_t> v;
        v.push_back(a[0]);
        size_t x = t.tag;
        unsigned i = sizeof(size_t)*8/7;
        for (; i>0; --i) {
          size_t y = x >> (i*7);
          if (y)
            break;
        }
        for (; i>0; --i) {
          size_t y = x >> (i*7);
          y &= 0b0'111'1111u;
          uint8_t b = y;
          b |= 0b1'000'0000u;
          v.push_back(b);
        }
        size_t y = x;
        y &= 0b0'111'1111u;
        uint8_t b = y;
        v.push_back(b);
        v.insert(v.end(), a.begin()+1, a.end());
        const uint8_t *r = u.read(v.data(), v.data() + v.size());
        BOOST_CHECK_EQUAL(u.tag, t.tag);
        BOOST_CHECK(r == v.data() + v.size());
        BOOST_CHECK_EQUAL(u.t_size, t.t_size);
        BOOST_CHECK_EQUAL(u.tl_size, t.t_size+1);
        BOOST_CHECK_EQUAL(u.is_long_tag, true);
      }
    }

    BOOST_AUTO_TEST_CASE(short_lengths)
    {
      using namespace xfsx;
      const array<uint8_t, 8> ls = {
        0, 1, 2, 3, 23, 125, 126, 127
      };
      for (auto l : ls) {
        Unit u;
        array<uint8_t, 256> a = {
          0b01'0'00101u,
          0b0'000'0010u,
          0u,
          0u
        };
        a[1] = l;
        const uint8_t *r = u.read(a.begin(), a.end());
        BOOST_CHECK_EQUAL(u.length, l);
        BOOST_CHECK(r == a.begin() + 2 + l);
        BOOST_CHECK_EQUAL(u.is_long_definite, false);
      }
    }

    BOOST_AUTO_TEST_CASE(long_lengths)
    {
      using namespace xfsx;
      vector<size_t> ls = {
        128, 129, 130,
        size_t(numeric_limits<uint16_t>::max() ) - size_t(1),
        numeric_limits<uint16_t>::max(),
        size_t(numeric_limits<uint16_t>::max() ) + size_t(1),
        size_t(numeric_limits<uint32_t>::max() ) - size_t(1),
        numeric_limits<uint32_t>::max(),
        size_t(numeric_limits<uint32_t>::max() ) + size_t(1),
      };
      if (sizeof(size_t) < 8) {
        ls.resize(6);
      }
      const array<uint8_t, 9> tls = {
        3, 3, 3,
        4,
        4,
        5,
        6,
        6,
        7
      };
      auto tl = tls.begin();
      for (auto l : ls) {
        Unit u;
        uint8_t fst = 0b01'1'00101u;
        vector<uint8_t> v;
        v.push_back(fst);
        v.push_back(0);
        unsigned i = sizeof(size_t);
        for (; i>0; --i) {
          size_t x = l >> ((i-1)*8);
          if (x)
            break;
        }
        for (; i>0; --i) {
          size_t x = l >> ((i-1)*8);
          x &= 0xffu;
          uint8_t b = x;
          v.push_back(b);
          ++v[1];
        }
        v[1] |= 0b1'000'0000;
        const uint8_t *r = u.read(v.data(),
            (uint8_t*)(numeric_limits<ssize_t>::max()));
        BOOST_CHECK_EQUAL(u.length, l);
        BOOST_CHECK_EQUAL(u.t_size, 1);
        BOOST_CHECK_EQUAL(u.tl_size, *tl);
        BOOST_CHECK(r == v.data() + *tl);
        BOOST_CHECK_EQUAL(u.is_long_definite, true);
        ++tl;
      }
    }

    BOOST_AUTO_TEST_CASE(long_lengths_non_minimal)
    {
      using namespace xfsx;
      vector<size_t> ls = {
        128, 129, 130,
        size_t(numeric_limits<uint16_t>::max() ) - size_t(1),
        numeric_limits<uint16_t>::max(),
        size_t(numeric_limits<uint16_t>::max() ) + size_t(1),
        size_t(numeric_limits<uint32_t>::max() ) - size_t(1),
        numeric_limits<uint32_t>::max(),
        size_t(numeric_limits<uint32_t>::max() ) + size_t(1),
      };
      if (sizeof(size_t) < 8) {
        ls.resize(6);
      }
      for (auto l : ls) {
        Unit u;
        uint8_t fst = 0b01'1'00101u;
        vector<uint8_t> v;
        v.push_back(fst);
        v.push_back(0);
        unsigned i = sizeof(size_t);
        for (; i>0; --i) {
          size_t x = l >> ((i-1)*8);
          x &= 0xffu;
          uint8_t b = x;
          v.push_back(b);
          ++v[1];
        }
        v[1] |= 0b1'000'0000;
        const uint8_t *r = u.read(v.data(),
            (uint8_t*)(numeric_limits<ssize_t>::max()));
        BOOST_CHECK_EQUAL(u.length, l);
        BOOST_CHECK_EQUAL(u.t_size, 1);
        BOOST_CHECK_EQUAL(u.tl_size, 2+sizeof(size_t));
        BOOST_CHECK(r == v.data() + 2+sizeof(size_t));
        BOOST_CHECK_EQUAL(u.is_long_definite, true);
      }
    }

    BOOST_AUTO_TEST_CASE(tag_overflow)
    {
      using namespace xfsx;
      Unit u;
      const array<uint8_t, 16> a = {
        0b01'0'11111u,
        0b11'01'0101u,
        0b11'01'0101u,
        0b11'01'0101u,
        0b11'01'0101u,
        0b11'01'0101u,
        0b11'01'0101u,
        0b11'01'0101u,
        0b11'01'0101u,
        0b11'01'0101u,
        0b11'01'0101u,
        0b11'01'0101u,
        0b01'01'0101u,
        0b0'000'0010u,
        0u,
        0u
      };
      BOOST_CHECK_THROW(u.read(a.begin(), a.end()), std::overflow_error);
      /*
      const uint8_t *r = u.read(a.begin(), a.end());
      BOOST_CHECK_EQUAL(u.t_size, 13);
      BOOST_CHECK_EQUAL(u.tl_size, 14);
      BOOST_CHECK(r == a.end());
      */
    }

    BOOST_AUTO_TEST_CASE(tag_overflow_max_u32_plus_1)
    {
      using namespace xfsx;
      Unit u;
      const array<uint8_t, 9> a = {
        0b01'0'11111u, // first byte
        0b10'01'0000u, // long tag, max_uint32 + 1
        0b10'00'0000u,
        0b10'00'0000u,
        0b10'00'0000u,
        0b00'00'0000u,
        0b0'000'0010u, // length
        0u,
        0u
      };
      BOOST_CHECK_THROW(u.read(a.begin(), a.end()), std::overflow_error);
    }

    BOOST_AUTO_TEST_CASE(truncated_tag)
    {
      using namespace xfsx;
      Unit u;
      const array<uint8_t, 2> a = { 0x7fu, 0x85u };
      BOOST_CHECK_THROW(u.read(a.begin(), a.end()), std::overflow_error);
    }

    BOOST_AUTO_TEST_CASE(indefinite_tl_size)
    {
      using namespace xfsx;
      Unit u;
      const array<uint8_t, 2> a = {
        0b01'1'00001u, 0b1'000'0000u
      };
      const uint8_t *r = u.read(a.begin(), a.end());
      BOOST_CHECK_EQUAL(u.t_size, 1);
      BOOST_CHECK_EQUAL(u.tl_size, 2);
      BOOST_CHECK(r == a.end());
    }

    BOOST_AUTO_TEST_CASE(eoc)
    {
      using namespace xfsx;
      Unit u;
      const array<uint8_t, 8> a = { 0u };
      const uint8_t *r = u.read(a.begin(), a.end());
      BOOST_CHECK_EQUAL(u.t_size, 1);
      BOOST_CHECK_EQUAL(u.tl_size, 2);
      BOOST_CHECK(r == a.begin()+2);
      BOOST_CHECK_EQUAL(u.is_eoc(), true);
    }

    BOOST_AUTO_TEST_CASE(not_eoc)
    {
      using namespace xfsx;
      Unit u;
      BOOST_CHECK_EQUAL(u.is_eoc(), false);
      const array<uint8_t, 8> a = { 0u };
      const uint8_t *r = u.read(a.begin(), a.end());
      BOOST_CHECK(r == a.begin()+2);
      u.t_size = 0;
      u.tl_size = 0;
      BOOST_CHECK_EQUAL(u.is_eoc(), false);
    }

    // }}}
   BOOST_AUTO_TEST_SUITE_END()

   BOOST_AUTO_TEST_SUITE(write)
    // {{{
     
    BOOST_AUTO_TEST_CASE(basic)
    {
      using namespace xfsx;
      Unit u;
      u.klasse           = Klasse::APPLICATION;
      u.shape            = Shape::PRIMITIVE;
      u.is_long_tag      = false;
      u.is_indefinite    = false;
      u.is_long_definite = false;
      u.t_size           = 1;
      u.tl_size          = 2;
      u.tag              = 23;
      u.length           = 13;
      array<uint8_t, 16> a;
      auto r = u.write(a.begin(), a.end());
      BOOST_CHECK_EQUAL(a[0], 0b01'0'10111u);
      BOOST_CHECK_EQUAL(a[1], 0b0'000'1101u);
      BOOST_CHECK(r == a.begin() + 2);
    }

    BOOST_AUTO_TEST_CASE(indefinite)
    {
      using namespace xfsx;
      Unit u;
      u.klasse           = Klasse::APPLICATION;
      u.shape            = Shape::PRIMITIVE;
      u.is_long_tag      = false;
      u.is_indefinite    = true;
      u.is_long_definite = false;
      u.t_size           = 1;
      u.tl_size          = 2;
      u.tag              = 23;
      u.length           = 0;
      array<uint8_t, 16> a;
      auto r = u.write(a.begin(), a.end());
      BOOST_CHECK_EQUAL(a[0], 0b01'0'10111u);
      BOOST_CHECK_EQUAL(a[1], 0b1'000'0000u);
      BOOST_CHECK(r == a.begin() + 2);
    }

    BOOST_AUTO_TEST_CASE(non_minimal_tag)
    {
      using namespace xfsx;
      Unit u;
      u.klasse           = Klasse::APPLICATION;
      u.shape            = Shape::PRIMITIVE;
      u.is_long_tag      = true;
      u.is_indefinite    = false;
      u.is_long_definite = false;
      u.t_size           = 3;
      u.tl_size          = 4;
      u.tag              = 23;
      u.length           = 13;
      array<uint8_t, 16> a;
      auto r = u.write(a.begin(), a.end());
      BOOST_CHECK_EQUAL(a[0], 0b01'0'11111u);
      BOOST_CHECK_EQUAL(a[1], 0b10'0'00000u);
      BOOST_CHECK_EQUAL(a[2], 0b00'0'10111u);
      BOOST_CHECK_EQUAL(a[3], 0b0'000'1101u);
      BOOST_CHECK(r == a.begin() + 4);
    }

    BOOST_AUTO_TEST_CASE(non_minimal_length)
    {
      using namespace xfsx;
      Unit u;
      u.klasse           = Klasse::APPLICATION;
      u.shape            = Shape::PRIMITIVE;
      u.is_long_tag      = true;
      u.is_indefinite    = false;
      u.is_long_definite = true;
      u.t_size           = 3;
      u.tl_size          = 6;
      u.tag              = 23;
      u.length           = 13;
      array<uint8_t, 16> a;
      auto r = u.write(a.begin(), a.end());
      BOOST_CHECK_EQUAL(a[0], 0b01'0'11111u);
      BOOST_CHECK_EQUAL(a[1], 0b10'0'00000u);
      BOOST_CHECK_EQUAL(a[2], 0b00'0'10111u);
      BOOST_CHECK_EQUAL(a[3], 0b10'0'00010u);
      BOOST_CHECK_EQUAL(a[4], 0b00'0'00000u);
      BOOST_CHECK_EQUAL(a[5], 0b0'000'1101u);
      BOOST_CHECK(r == a.begin() + 6);
    }

    BOOST_AUTO_TEST_CASE(throw_too_small)
    {
      using namespace xfsx;
      Unit u;
      u.klasse           = Klasse::APPLICATION;
      u.shape            = Shape::PRIMITIVE;
      u.is_long_tag      = false;
      u.is_indefinite    = false;
      u.is_long_definite = false;
      u.t_size           = 1;
      u.tl_size          = 2;
      u.tag              = 23;
      u.length           = 13;
      array<uint8_t, 16> a;
      BOOST_CHECK_THROW(u.write(a.begin(), a.begin()+1), std::overflow_error);
    }

    BOOST_AUTO_TEST_CASE(eoc)
    {
      using namespace xfsx;
      Unit u{Unit::EOC()};
      array<uint8_t, 8> a { 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u };
      uint8_t *r = u.write(a.begin(), a.end());
      BOOST_CHECK(r == a.begin() + 2);
      BOOST_CHECK(a[0] == 0u);
      BOOST_CHECK(a[1] == 0u);
    }

    BOOST_AUTO_TEST_CASE(tl_minimal)
    {
      using namespace xfsx;
      // <P O="24" T="[APPLICATION 109]" TL="3" V="5">31707</P>
      Unit u(109);
      u.init_length(5);
      BOOST_CHECK_EQUAL(u.t_size, 2u);
      BOOST_CHECK_EQUAL(u.tl_size, 3u);
    }

    // }}}
   BOOST_AUTO_TEST_SUITE_END() // write

   BOOST_AUTO_TEST_SUITE(init)

    BOOST_AUTO_TEST_SUITE(constructed)
     // {{{
     BOOST_AUTO_TEST_CASE(indefinite)
     {
       using namespace xfsx;
       Unit u;
       BOOST_CHECK_EQUAL(u.klasse, Klasse::UNIVERSAL);
       u.init_constructed_from(30u);
       BOOST_CHECK_EQUAL(u.klasse, Klasse::UNIVERSAL);
       BOOST_CHECK_EQUAL(u.shape, Shape::CONSTRUCTED);
       BOOST_CHECK_EQUAL(u.is_long_tag, false);
       BOOST_CHECK_EQUAL(u.is_indefinite, true);
       BOOST_CHECK_EQUAL(u.is_long_definite, false);
       BOOST_CHECK_EQUAL(u.t_size, 1u);
       BOOST_CHECK_EQUAL(u.tl_size, 2u);
       BOOST_CHECK_EQUAL(u.tag, 30u);
       BOOST_CHECK_EQUAL(u.length, 0u);
     }

     BOOST_AUTO_TEST_CASE(long_indefinite)
     {
       using namespace xfsx;
       Unit u;
       u.klasse = Klasse::APPLICATION;
       u.init_constructed_from(31u);
       BOOST_CHECK_EQUAL(u.klasse, Klasse::APPLICATION);
       BOOST_CHECK_EQUAL(u.shape, Shape::CONSTRUCTED);
       BOOST_CHECK_EQUAL(u.is_long_tag, true);
       BOOST_CHECK_EQUAL(u.is_indefinite, true);
       BOOST_CHECK_EQUAL(u.is_long_definite, false);
       BOOST_CHECK_EQUAL(u.t_size, 2u);
       BOOST_CHECK_EQUAL(u.tl_size, 3u);
       BOOST_CHECK_EQUAL(u.tag, 31u);
       BOOST_CHECK_EQUAL(u.length, 0u);
     }

     BOOST_AUTO_TEST_CASE(definite)
     {
       using namespace xfsx;
       Unit u;
       u.klasse = Klasse::APPLICATION;
       u.init_constructed_from(23u, 127u);
       BOOST_CHECK_EQUAL(u.klasse, Klasse::APPLICATION);
       BOOST_CHECK_EQUAL(u.shape, Shape::CONSTRUCTED);
       BOOST_CHECK_EQUAL(u.is_long_tag, false);
       BOOST_CHECK_EQUAL(u.is_indefinite, false);
       BOOST_CHECK_EQUAL(u.is_long_definite, false);
       BOOST_CHECK_EQUAL(u.t_size, 1u);
       BOOST_CHECK_EQUAL(u.tl_size, 2u);
       BOOST_CHECK_EQUAL(u.tag, 23u);
       BOOST_CHECK_EQUAL(u.length, 127u);
     }

     BOOST_AUTO_TEST_CASE(definite_long)
     {
       using namespace xfsx;
       Unit u;
       u.klasse = Klasse::APPLICATION;
       u.init_constructed_from(23u, numeric_limits<uint16_t>::max());
       BOOST_CHECK_EQUAL(u.klasse, Klasse::APPLICATION);
       BOOST_CHECK_EQUAL(u.shape, Shape::CONSTRUCTED);
       BOOST_CHECK_EQUAL(u.is_long_tag, false);
       BOOST_CHECK_EQUAL(u.is_indefinite, false);
       BOOST_CHECK_EQUAL(u.is_long_definite, true);
       BOOST_CHECK_EQUAL(u.t_size, 1u);
       BOOST_CHECK_EQUAL(u.tl_size, 4u);
       BOOST_CHECK_EQUAL(u.tag, 23u);
       BOOST_CHECK_EQUAL(u.length, numeric_limits<uint16_t>::max());
     }

     BOOST_AUTO_TEST_CASE(definite_long_long)
     {
       using namespace xfsx;
       Unit u;
       u.klasse = Klasse::APPLICATION;
       u.init_constructed_from(255u, numeric_limits<uint16_t>::max());
       BOOST_CHECK_EQUAL(u.klasse, Klasse::APPLICATION);
       BOOST_CHECK_EQUAL(u.shape, Shape::CONSTRUCTED);
       BOOST_CHECK_EQUAL(u.is_long_tag, true);
       BOOST_CHECK_EQUAL(u.is_indefinite, false);
       BOOST_CHECK_EQUAL(u.is_long_definite, true);
       BOOST_CHECK_EQUAL(u.t_size, 3u);
       BOOST_CHECK_EQUAL(u.tl_size, 6u);
       BOOST_CHECK_EQUAL(u.tag, 255u);
       BOOST_CHECK_EQUAL(u.length, numeric_limits<uint16_t>::max());
     }

     // }}}
    BOOST_AUTO_TEST_SUITE_END()

    BOOST_AUTO_TEST_SUITE(primitive)
      // {{{

      BOOST_AUTO_TEST_CASE(basic_int32)
      {
        using namespace xfsx;
        Unit u;
        BOOST_CHECK_EQUAL(u.klasse, Klasse::UNIVERSAL);
        u.klasse = Klasse::APPLICATION;
        int32_t x = 2;
        u.init_from(30, x);
        BOOST_CHECK_EQUAL(u.klasse, Klasse::APPLICATION);
        BOOST_CHECK_EQUAL(u.shape, Shape::PRIMITIVE);
        BOOST_CHECK_EQUAL(u.is_long_tag, false);
        BOOST_CHECK_EQUAL(u.is_indefinite, false);
        BOOST_CHECK_EQUAL(u.is_long_definite, false);
        BOOST_CHECK_EQUAL(u.t_size, 1u);
        BOOST_CHECK_EQUAL(u.tl_size, 2u);
        BOOST_CHECK_EQUAL(u.tag, 30u);
        BOOST_CHECK_EQUAL(u.length, 1u);
      }

      BOOST_AUTO_TEST_CASE(basic_int32_two)
      {
        using namespace xfsx;
        Unit u;
        int32_t x = 256;
        u.init_from(30, x);
        BOOST_CHECK_EQUAL(u.klasse, Klasse::UNIVERSAL);
        BOOST_CHECK_EQUAL(u.shape, Shape::PRIMITIVE);
        BOOST_CHECK_EQUAL(u.is_long_tag, false);
        BOOST_CHECK_EQUAL(u.is_indefinite, false);
        BOOST_CHECK_EQUAL(u.is_long_definite, false);
        BOOST_CHECK_EQUAL(u.t_size, 1u);
        BOOST_CHECK_EQUAL(u.tl_size, 2u);
        BOOST_CHECK_EQUAL(u.tag, 30u);
        BOOST_CHECK_EQUAL(u.length, 2u);
      }

      // }}}
    BOOST_AUTO_TEST_SUITE_END()

   BOOST_AUTO_TEST_SUITE_END()

  BOOST_AUTO_TEST_SUITE_END()

  BOOST_AUTO_TEST_SUITE(unit_content)

   BOOST_AUTO_TEST_SUITE(ints)
    // {{{
    BOOST_AUTO_TEST_CASE(int_basic)
    {
      using namespace xfsx;
      TLC u;
      const array<uint8_t, 3> a = {
        0b01'0'00001u, 0b0'000'0001u,
        23u
      };
      const uint8_t *r = u.read(a.begin(), a.end());
      (void)r;
      int64_t x = 0;
      u.copy_content(x);
      BOOST_CHECK_EQUAL(x, 23);
    }

    // an extra function is available that can be invoked
    // for checking minimal encoding
    BOOST_AUTO_TEST_CASE(allow_non_minimal)
    {
      using namespace xfsx;
      TLC u;
      const array<uint8_t, 6> a = {
        0b01'0'00001u, 0b0'000'0100u,
        0, 0, 0, 23u
      };
      const uint8_t *r = u.read(a.begin(), a.end());
      (void)r;
      int64_t x = 0;
      u.copy_content(x);
      BOOST_CHECK_EQUAL(x, 23);
    }

    BOOST_AUTO_TEST_CASE(throw_int_to_big)
    {
      using namespace xfsx;
      TLC u;
      const array<uint8_t, 11> a = {
        0b01'0'00001u, 0b0'000'1001u,
        0, 0, 0, 0, 0, 0, 0, 0, 23u
      };
      const uint8_t *r = u.read(a.begin(), a.end());
      (void)r;
      int64_t x = 0;
      BOOST_CHECK_THROW(u.copy_content(x), std::overflow_error);
    }

    BOOST_AUTO_TEST_CASE(throw_constructed)
    {
      using namespace xfsx;
      TLC u;
      const array<uint8_t, 11> a = {
        0b01'1'00001u, 0b0'000'1001u,
        0, 0, 0, 0, 0, 0, 0, 0, 23u
      };
      const uint8_t *r = u.read(a.begin(), a.end());
      (void)r;
      int64_t x = 0;
      BOOST_CHECK_THROW(u.copy_content(x), std::range_error);
    }

    BOOST_AUTO_TEST_CASE(int_uinit_dest)
    {
      using namespace xfsx;
      TLC u;
      const array<uint8_t, 3> a = {
        0b01'0'00001u, 0b0'000'0001u,
        23u
      };
      const uint8_t *r = u.read(a.begin(), a.end());
      (void)r;
      int64_t x = -1;
      u.copy_content(x);
      BOOST_CHECK_EQUAL(x, 23);
    }

    BOOST_AUTO_TEST_CASE(int_neg)
    {
      using namespace xfsx;
      TLC u;
      const array<uint8_t, 3> a = {
        0b01'0'00001u, 0b0'000'0001u,
        static_cast<uint8_t>(-23)
      };
      const uint8_t *r = u.read(a.begin(), a.end());
      (void)r;
      int64_t x = 0;
      u.copy_content(x);
      BOOST_CHECK_EQUAL(x, -23);
    }

    BOOST_AUTO_TEST_CASE(ints)
    {
      using namespace xfsx;
      const array<int64_t, 14> inps = {
        0, 1, 2, 3,
        numeric_limits<int32_t>::max()-1,
        numeric_limits<int32_t>::max(),
        int64_t(numeric_limits<int32_t>::max())+1,
        numeric_limits<int32_t>::min()+1,
        numeric_limits<int32_t>::min(),
        int64_t(numeric_limits<int32_t>::min())-1,
        numeric_limits<int64_t>::min()+1,
        numeric_limits<int64_t>::min(),
        numeric_limits<int64_t>::max()-1,
        numeric_limits<int64_t>::max()
      };
      for (auto inp : inps) {
        TLC u;
        vector<uint8_t> v;
        v.push_back(0b01'0'00001u);
        uint8_t i = sizeof(int64_t);
        if (inp<0)
          for (; i>0; --i) {
            uint8_t b = uint64_t(0xffu) & uint64_t(inp>>(8*(i-1)));
            if (b != 0xffu)
              break;
          }
        else
          for (; i>0; --i) {
            uint8_t b = uint64_t(0xffu) & uint64_t(inp>>(8*(i-1)));
            if (b)
              break;
          }
        i = std::min(size_t(i+1), sizeof(int64_t));
        v.push_back(i);
        for (; i>0; --i) {
          uint8_t b =  uint64_t(0xffu) & uint64_t(inp>>(8*(i-1)));
          v.push_back(b);
        }
        const uint8_t *r = u.read(v.data(), v.data() + v.size());
        (void)r;
        int64_t x = 0;
        BOOST_TEST_CHECKPOINT("Integer value: " << inp);
        u.copy_content(x);
        BOOST_CHECK_EQUAL(x, inp);
      }
    }

    BOOST_AUTO_TEST_CASE(uints)
    {
      using namespace xfsx;
      const array<uint64_t, 9> inps = {
        0, 1, 2, 3,
        numeric_limits<uint32_t>::max()-1,
        numeric_limits<uint32_t>::max(),
        uint64_t(numeric_limits<uint32_t>::max())+1,
        numeric_limits<uint64_t>::max()-1,
        numeric_limits<uint64_t>::max()
      };
      for (auto inp : inps) {
        TLC u;
        vector<uint8_t> v;
        v.push_back(0b01'0'00001u);
        uint8_t i = sizeof(uint64_t);
        for (; i>0; --i) {
          uint8_t b = uint64_t(0xffu) & uint64_t(inp>>(8*(i-1)));
          if (b)
            break;
        }
        i = std::min(size_t(i+1), sizeof(uint64_t));
        v.push_back(i);
        for (; i>0; --i) {
          uint8_t b =  uint64_t(0xffu) & uint64_t(inp>>(8*(i-1)));
          v.push_back(b);
        }
        const uint8_t *r = u.read(v.data(), v.data() + v.size());
        (void)r;
        uint64_t x = 0;
        BOOST_TEST_CHECKPOINT("Integer value: " << inp);
        u.copy_content(x);
        BOOST_CHECK_EQUAL(x, inp);
      }
    }

    BOOST_AUTO_TEST_CASE(ints32)
    {
      using namespace xfsx;
      const array<int32_t, 8> inps = {
        0, 1, 2, 3,
        numeric_limits<int32_t>::max()-1,
        numeric_limits<int32_t>::max(),
        numeric_limits<int32_t>::min()+1,
        numeric_limits<int32_t>::min()
      };
      for (auto inp : inps) {
        TLC u;
        vector<uint8_t> v;
        v.push_back(0b01'0'00001u);
        uint8_t i = sizeof(int32_t);
        if (inp<0)
          for (; i>0; --i) {
            uint8_t b = uint32_t(0xffu) & uint32_t(inp>>(8*(i-1)));
            if (b != 0xffu)
              break;
          }
        else
          for (; i>0; --i) {
            uint8_t b = uint32_t(0xffu) & uint32_t(inp>>(8*(i-1)));
            if (b)
              break;
          }
        i = std::min(size_t(i+1), sizeof(int32_t));
        v.push_back(i);
        for (; i>0; --i) {
          uint8_t b =  uint32_t(0xffu) & uint32_t(inp>>(8*(i-1)));
          v.push_back(b);
        }
        const uint8_t *r = u.read(v.data(), v.data() + v.size());
        (void)r;
        int32_t x = 0;
        BOOST_TEST_CHECKPOINT("Integer value: " << inp);
        u.copy_content(x);
        BOOST_CHECK_EQUAL(x, inp);
      }
    }

    BOOST_AUTO_TEST_CASE(ints16)
    {
      using namespace xfsx;
      const array<int16_t, 8> inps = {
        0, 1, 2, 3,
        numeric_limits<int16_t>::max()-1,
        numeric_limits<int16_t>::max(),
        numeric_limits<int16_t>::min()+1,
        numeric_limits<int16_t>::min(),
      };
      for (auto inp : inps) {
        TLC u;
        vector<uint8_t> v;
        v.push_back(0b01'0'00001u);
        uint8_t i = sizeof(int16_t);
        if (inp<0)
          for (; i>0; --i) {
            uint8_t b = uint16_t(0xffu) & uint16_t(inp>>(8*(i-1)));
            if (b != 0xffu)
              break;
          }
        else
          for (; i>0; --i) {
            uint8_t b = uint16_t(0xffu) & uint16_t(inp>>(8*(i-1)));
            if (b)
              break;
          }
        i = std::min(size_t(i+1), sizeof(int16_t));
        v.push_back(i);
        for (; i>0; --i) {
          uint8_t b =  uint16_t(0xffu) & uint16_t(inp>>(8*(i-1)));
          v.push_back(b);
        }
        const uint8_t *r = u.read(v.data(), v.data() + v.size());
        (void)r;
        int16_t x = 0;
        BOOST_TEST_CHECKPOINT("Integer value: " << inp);
        u.copy_content(x);
        BOOST_CHECK_EQUAL(x, inp);
      }
    }

    BOOST_AUTO_TEST_CASE(small_int)
    {
      using namespace xfsx;
      TLC u;
      const array<uint8_t, 3> a = {
        0b01'0'00001u, 0b0'000'0001u,
        static_cast<uint8_t>(-23)
      };
      const uint8_t *r = u.read(a.begin(), a.end());
      (void)r;
      int8_t x = 0;
      u.copy_content(x);
      BOOST_CHECK_EQUAL(x, -23);
    }
    // }}}
   BOOST_AUTO_TEST_SUITE_END()


   BOOST_AUTO_TEST_SUITE(other)
    // {{{
    BOOST_AUTO_TEST_CASE(bool_true)
    {
      using namespace xfsx;
      const array<uint8_t, 5> inps = { 1u, 2u, 255u, 23u, 128u };
      for (auto inp : inps) {
        TLC u;
        array<uint8_t, 3> a = {
          0b01'0'00001u, 0b0'000'0001u
        };
        a[2] = inp;
        const uint8_t *r = u.read(a.begin(), a.end());
        (void)r;
        bool x = false;
        u.copy_content(x);
        BOOST_CHECK_EQUAL(x, true);
        x = true;
        u.copy_content(x);
        BOOST_CHECK_EQUAL(x, true);
      }
    }

    BOOST_AUTO_TEST_CASE(bool_false)
    {
      using namespace xfsx;
      TLC u;
      const array<uint8_t, 3> a = {
        0b01'0'00001u, 0b0'000'0001u,
        0u
      };
      const uint8_t *r = u.read(a.begin(), a.end());
      (void)r;
      bool x = false;
      u.copy_content(x);
      BOOST_CHECK_EQUAL(x, false);
      x = true;
      u.copy_content(x);
      BOOST_CHECK_EQUAL(x, false);
    }

    BOOST_AUTO_TEST_CASE(unsigned_range)
    {
      using namespace xfsx;
      TLC u;
      const array<uint8_t, 7> a = {
        0b01'0'00001u, 0b0'000'0101u,
        uint8_t('h'), uint8_t('e'), uint8_t('l'), uint8_t('l'), uint8_t('o')
      };
      const uint8_t *r = u.read(a.begin(), a.end());
      (void)r;
      std::pair<const uint8_t *, const uint8_t *> x;
      u.copy_content(x);
      BOOST_CHECK(x.first  == a.begin()+2);
      BOOST_CHECK(x.second == a.end());
    }

    BOOST_AUTO_TEST_CASE(signed_range)
    {
      using namespace xfsx;
      TLC u;
      const array<uint8_t, 7> a = {
        0b01'0'00001u, 0b0'000'0101u,
        uint8_t('h'), uint8_t('e'), uint8_t('l'), uint8_t('l'), uint8_t('o')
      };
      const uint8_t *r = u.read(a.begin(), a.end());
      (void)r;
      std::pair<const char *, const char *> x;
      u.copy_content(x);
      string s(x.first, x.second);
      BOOST_CHECK_EQUAL(s, "hello");
    }

    // }}}
   BOOST_AUTO_TEST_SUITE_END()

  BOOST_AUTO_TEST_SUITE_END()

  BOOST_AUTO_TEST_SUITE(decode_)

    BOOST_AUTO_TEST_SUITE(bcd)
    // {{{

      BOOST_AUTO_TEST_CASE(empty)
      {
        using namespace xfsx;
        const uint8_t a[1] = {0};
        BCD_String b;
        decode(a, 0, b);
        BOOST_CHECK_EQUAL(b.empty(), true);
      }

      BOOST_AUTO_TEST_CASE(basic)
      {
        using namespace xfsx;
        array<uint8_t, 4> a = {
          0xDEu, 0xADu, 0xCAu, 0x0Eu
        };
        BCD_String b;
        decode(a.begin(), a.size(), b);
        const string &s = b;
        BOOST_CHECK_EQUAL(s, "deadca0e");
      }

      BOOST_AUTO_TEST_CASE(filler_in_the_middle)
      {
        using namespace xfsx;
        array<uint8_t, 4> a = {
          0xDEu, 0xADu, 0xCAu, 0xFEu
        };
        BCD_String b;
        decode(a.begin(), a.size(), b);
        const string &s = b;
        // no special handling (e.g. throw for fillers in the middle)
        BOOST_CHECK_EQUAL(s, "deadcafe");
      }

      BOOST_AUTO_TEST_CASE(filler_at_the_end)
      {
        using namespace xfsx;
        array<uint8_t, 4> a = {
          0xDEu, 0xADu, 0xCAu, 0xEFu
        };
        BCD_String b;
        decode(a.begin(), a.size(), b);
        const string &s = b;
        BOOST_CHECK_EQUAL(s, "deadcae");
      }

      BOOST_AUTO_TEST_CASE(all)
      {
        using namespace xfsx;
        array<uint8_t, 8> a = {
          0x12u, 0x34u, 0x56u, 0x78u, 0x90u, 0xABu, 0xCDu, 0xEFu
        };
        BCD_String b;
        decode(a.begin(), a.size(), b);
        const string &s = b;
        BOOST_CHECK_EQUAL(s, "1234567890abcde");
      }

      BOOST_AUTO_TEST_CASE(uneven)
      {
        using namespace xfsx;
        array<uint8_t, 9> a = {
          0x12u, 0x34u, 0x56u, 0x78u, 0x90u, 0xABu, 0xCDu, 0xEFu, 0xCAu
        };
        BCD_String b;
        decode(a.begin(), a.size(), b);
        const string &s = b;
        BOOST_CHECK_EQUAL(s, "1234567890abcdefca");
      }
 
    // }}}
    BOOST_AUTO_TEST_SUITE_END()

    BOOST_AUTO_TEST_SUITE(hex)
    // {{{

      BOOST_AUTO_TEST_CASE(longer)
      {
        using namespace xfsx;
        const char inp[] = "&#xde;&#xad;\xca\xfe world\\xca\\xfe";
        const uint8_t *begin = reinterpret_cast<const uint8_t*>(inp);
        const uint8_t *end = begin + sizeof(inp) - 1;
        Hex_String b;
        decode(begin, end-begin, b);
        const string &s = b;
        BOOST_CHECK_EQUAL(s, "&#xde;&#xad;\\xca\\xfe world\\x5cxca\\x5cxfe");
      }

      BOOST_AUTO_TEST_CASE(longer_xml)
      {
        using namespace xfsx;
        const char inp[] = "&#xde;&#xad;\xca\xfe world\\xca\\xfe";
        const uint8_t *begin = reinterpret_cast<const uint8_t*>(inp);
        const uint8_t *end = begin + sizeof(inp) - 1;
        Hex_XML_String b;
        decode(begin, end-begin, b);
        const string &s = b;
        BOOST_CHECK_EQUAL(s, "&#x26;#xde;&#x26;#xad;&#xca;&#xfe; world\\xca\\xfe");
      }

    // }}}
    BOOST_AUTO_TEST_SUITE_END()

  BOOST_AUTO_TEST_SUITE_END()

  BOOST_AUTO_TEST_SUITE(encode_)

   BOOST_AUTO_TEST_SUITE(ints)
    // {{{
    BOOST_AUTO_TEST_CASE(basic)
    {
      using namespace xfsx;
      int64_t x = 23;
      array<uint8_t, 8> a;
      uint8_t *r = encode(x, a.begin(), minimally_encoded_length(x));
      BOOST_REQUIRE(r == a.begin() + 1);
      int64_t y = 0;
      decode(a.begin(), 1, y);
      BOOST_CHECK_EQUAL(y, 23);
    }

    BOOST_AUTO_TEST_CASE(non_minimal)
    {
      using namespace xfsx;
      int64_t x = 23;
      array<uint8_t, 8> a;
      uint8_t *r = encode(x, a.begin(), 4);
      BOOST_REQUIRE(r == a.begin() + 4);
      int64_t y = 0;
      decode(a.begin(), 4, y);
      BOOST_CHECK_EQUAL(y, 23);
    }

    BOOST_AUTO_TEST_CASE(throw_too_big)
    {
      using namespace xfsx;
      int64_t x = 23;
      array<uint8_t, 8> a;
      BOOST_CHECK_THROW(encode(x, a.begin(), 9), std::overflow_error);
    }

    BOOST_AUTO_TEST_CASE(ints)
    {
      using namespace xfsx;
      const array<int64_t, 14> inps = {
        0, 1, 2, 3,
        numeric_limits<int32_t>::max()-1,
        numeric_limits<int32_t>::max(),
        int64_t(numeric_limits<int32_t>::max())+1,
        numeric_limits<int32_t>::min()+1,
        numeric_limits<int32_t>::min(),
        int64_t(numeric_limits<int32_t>::min())-1,
        numeric_limits<int64_t>::min()+1,
        numeric_limits<int64_t>::min(),
        numeric_limits<int64_t>::max()-1,
        numeric_limits<int64_t>::max()
      };
      for (auto inp : inps) {
        uint8_t l = minimally_encoded_length(inp);
        array<uint8_t, 8> a;
        uint8_t *r = encode(inp, a.begin(), l);
        BOOST_REQUIRE(r == a.begin() + l);
        int64_t y = 0;
        decode(a.begin(), l, y);
        BOOST_CHECK_EQUAL(y, inp);
      }
    }

    BOOST_AUTO_TEST_CASE(uints)
    {
      using namespace xfsx;
      const array<uint64_t, 9> inps = {
        0, 1, 2, 3,
        numeric_limits<uint32_t>::max()-1,
        numeric_limits<uint32_t>::max(),
        uint64_t(numeric_limits<uint32_t>::max())+1,
        numeric_limits<uint64_t>::max()-1,
        numeric_limits<uint64_t>::max()
      };
      for (auto inp : inps) {
        uint8_t l = minimally_encoded_length(inp);
        array<uint8_t, 8> a;
        uint8_t *r = encode(inp, a.begin(), l);
        BOOST_REQUIRE(r == a.begin() + l);
        uint64_t y = 0;
        decode(a.begin(), l, y);
        BOOST_CHECK_EQUAL(y, inp);
      }
    }

    BOOST_AUTO_TEST_CASE(ints32)
    {
      using namespace xfsx;
      const array<int32_t, 8> inps = {
        0, 1, 2, 3,
        numeric_limits<int32_t>::max()-1,
        numeric_limits<int32_t>::max(),
        numeric_limits<int32_t>::min()+1,
        numeric_limits<int32_t>::min()
      };
      for (auto inp : inps) {
        uint8_t l = minimally_encoded_length(inp);
        array<uint8_t, 8> a;
        uint8_t *r = encode(inp, a.begin(), l);
        BOOST_REQUIRE(r == a.begin() + l);
        int32_t y = 0;
        decode(a.begin(), l, y);
        BOOST_CHECK_EQUAL(y, inp);
      }
    }

    BOOST_AUTO_TEST_CASE(ints16)
    {
      using namespace xfsx;
      const array<int16_t, 11> inps = {
        0, 1, 2, 3,
        -1, -2, -3,
        numeric_limits<int16_t>::max()-1,
        numeric_limits<int16_t>::max(),
        numeric_limits<int16_t>::min()+1,
        numeric_limits<int16_t>::min(),
      };
      for (auto inp : inps) {
        uint8_t l = minimally_encoded_length(inp);
        array<uint8_t, 8> a;
        uint8_t *r = encode(inp, a.begin(), l);
        BOOST_REQUIRE(r == a.begin() + l);
        int16_t y = 0;
        decode(a.begin(), l, y);
        BOOST_CHECK_EQUAL(y, inp);
      }
    }

    BOOST_AUTO_TEST_CASE(ints8)
    {
      using namespace xfsx;
      const array<int8_t, 11> inps = {
        0, 1, 2, 3,
        -1, -2, -3,
        numeric_limits<int8_t>::max()-1,
        numeric_limits<int8_t>::max(),
        numeric_limits<int8_t>::min()+1,
        numeric_limits<int8_t>::min(),
      };
      for (auto inp : inps) {
        uint8_t l = minimally_encoded_length(inp);
        array<uint8_t, 8> a;
        uint8_t *r = encode(inp, a.begin(), l);
        BOOST_REQUIRE(r == a.begin() + l);
        int8_t y = 0;
        decode(a.begin(), l, y);
        BOOST_CHECK_EQUAL(y, inp);
      }
    }
    // }}}
   BOOST_AUTO_TEST_SUITE_END()

   BOOST_AUTO_TEST_SUITE(other)
    // {{{
    BOOST_AUTO_TEST_CASE(boolean)
    {
      using namespace xfsx;
      array<uint8_t, 8> a;
      bool x = true;
      uint8_t *r = encode(x, a.begin(), 1);
      BOOST_REQUIRE(r == a.begin() + 1);
      bool y = false;
      decode(a.begin(), 1, y);
      BOOST_CHECK_EQUAL(y, true);
    }

    BOOST_AUTO_TEST_CASE(boolean_false)
    {
      using namespace xfsx;
      array<uint8_t, 8> a;
      bool x = false;
      uint8_t *r = encode(x, a.begin(), 1);
      BOOST_REQUIRE(r == a.begin() + 1);
      bool y = true;
      decode(a.begin(), 1, y);
      BOOST_CHECK_EQUAL(y, false);
    }

    BOOST_AUTO_TEST_CASE(boolean_throw)
    {
      using namespace xfsx;
      array<uint8_t, 8> a;
      bool x = false;
      BOOST_CHECK_THROW(encode(x, a.begin(), 0), std::overflow_error);
    }

    BOOST_AUTO_TEST_CASE(boolean_throw_too_large)
    {
      using namespace xfsx;
      array<uint8_t, 8> a;
      bool x = false;
      BOOST_CHECK_THROW(encode(x, a.begin(), 2), std::overflow_error);
    }

    BOOST_AUTO_TEST_CASE(uchar_pair)
    {
      using namespace xfsx;
      array<uint8_t, 8> a;
      const char s[] = "hello";
      pair<const uint8_t*, const uint8_t*> x(
          reinterpret_cast<const uint8_t*>(s),
          reinterpret_cast<const uint8_t*>(s+sizeof(s)-1));
      uint8_t *r = encode(x, a.begin(), 5);
      BOOST_CHECK(r == a.begin() + 5);
      pair<const uint8_t*, const uint8_t*> y;
      decode(a.begin(), 5, y);
      string t(reinterpret_cast<const char*>(y.first),
          reinterpret_cast<const char*>(y.second));
      BOOST_CHECK_EQUAL(t, "hello");
    }

    BOOST_AUTO_TEST_CASE(char_pair)
    {
      using namespace xfsx;
      array<uint8_t, 8> a;
      const char s[] = "hello";
      pair<const char*, const char*> x(s, s+sizeof(s)-1);
      uint8_t *r = encode(x, a.begin(), 5);
      BOOST_CHECK(r == a.begin() + 5);
      pair<const char*, const char*> y;
      decode(a.begin(), 5, y);
      string t(y.first, y.second);
      BOOST_CHECK_EQUAL(t, "hello");
    }
    BOOST_AUTO_TEST_CASE(pair_throw)
    {
      using namespace xfsx;
      array<uint8_t, 8> a;
      const char s[] = "hello";
      pair<const char*, const char*> x(s, s+sizeof(s)-1);
      BOOST_CHECK_THROW(encode(x, a.begin(), 4), std::overflow_error);
    }
    BOOST_AUTO_TEST_CASE(pair_throw_too_large)
    {
      using namespace xfsx;
      array<uint8_t, 8> a;
      const char s[] = "hello";
      pair<const char*, const char*> x(s, s+sizeof(s)-1);
      BOOST_CHECK_THROW(encode(x, a.begin(), 6), std::overflow_error);
    }

    BOOST_AUTO_TEST_CASE(str)
    {
      using namespace xfsx;
      array<uint8_t, 8> a;
      string x("hello");
      uint8_t *r = encode(x, a.begin(), 5);
      BOOST_CHECK(r == a.begin() + 5);
      pair<const char*, const char*> y;
      decode(a.begin(), 5, y);
      string t(y.first, y.second);
      BOOST_CHECK_EQUAL(t, "hello");
    }

    BOOST_AUTO_TEST_CASE(str_throw)
    {
      using namespace xfsx;
      array<uint8_t, 8> a;
      string x("hello");
      BOOST_CHECK_THROW(encode(x, a.begin(), 4), std::overflow_error);
      BOOST_CHECK_THROW(encode(x, a.begin(), 6), std::overflow_error);
    }

    BOOST_AUTO_TEST_CASE(bcd_simple)
    {
      using namespace xfsx;
      BCD_String x("deadcafe");
      array<uint8_t, 8> a;
      uint8_t *r = encode(x, a.begin(), 4);
      BOOST_REQUIRE(r == a.begin() + 4);
      BOOST_CHECK_EQUAL(a[0], 0xdeu);
      BOOST_CHECK_EQUAL(a[1], 0xadu);
      BOOST_CHECK_EQUAL(a[2], 0xcau);
      BOOST_CHECK_EQUAL(a[3], 0xfeu);
    }

    BOOST_AUTO_TEST_CASE(bcd_filler)
    {
      using namespace xfsx;
      BCD_String x("deadcafe2");
      array<uint8_t, 8> a;
      uint8_t *r = encode(x, a.begin(), 5);
      BOOST_REQUIRE(r == a.begin() + 5);
      BOOST_CHECK_EQUAL(a[0], 0xdeu);
      BOOST_CHECK_EQUAL(a[1], 0xadu);
      BOOST_CHECK_EQUAL(a[2], 0xcau);
      BOOST_CHECK_EQUAL(a[3], 0xfeu);
      BOOST_CHECK_EQUAL(a[4], 0x2fu);
    }

    BOOST_AUTO_TEST_CASE(bcd_throw)
    {
      using namespace xfsx;
      BCD_String x("deadcafe2");
      array<uint8_t, 8> a;
      BOOST_CHECK_THROW(encode(x, a.begin(), 4), std::overflow_error);
    }

    BOOST_AUTO_TEST_CASE(hex)
    {
      using namespace xfsx;
      Hex_String x("\\xde\\xad&#xca;&#xfe;23");
      array<uint8_t, 32> a;
      uint8_t *r = encode(x, a.begin(), 16);
      BOOST_REQUIRE(r == a.begin() + 16);
      BOOST_CHECK_EQUAL(a[0], 0xdeu);
      BOOST_CHECK_EQUAL(a[1], 0xadu);
      BOOST_CHECK_EQUAL(
          string((const char*)(a.begin() + 2), (const char*)(a.begin() + 16)),
            "&#xca;&#xfe;23");
    }

    BOOST_AUTO_TEST_CASE(hex_xml)
    {
      using namespace xfsx;
      Hex_XML_String x("\\xde\\xad&#xca;&#xfe;23");
      array<uint8_t, 32> a;
      uint8_t *r = encode(x, a.begin(), 12);
      BOOST_REQUIRE(r == a.begin() + 12);
      BOOST_CHECK_EQUAL(a[8], 0xcau);
      BOOST_CHECK_EQUAL(a[9], 0xfeu);
      BOOST_CHECK_EQUAL(
          string((const char*)(a.begin()), (const char*)(a.begin() + 8)),
            "\\xde\\xad");
      BOOST_CHECK_EQUAL(
          string((const char*)(a.begin()+10), (const char*)(a.begin() + 12)),
            "23");
    }

    BOOST_AUTO_TEST_CASE(hex_throw)
    {
      using namespace xfsx;
      Hex_String x("\\xde\\xad&#xca;&#xfe;23");
      array<uint8_t, 32> a;
      BOOST_CHECK_THROW(encode(x, a.begin(), 15), std::overflow_error);
      BOOST_CHECK_THROW(encode(x, a.begin(), 17), std::overflow_error);
    }

    BOOST_AUTO_TEST_CASE(xml_content)
    {
      using namespace xfsx;
      const char inp[] = "fo&#x0d;o";
      XML_Content c(make_pair(inp, inp+sizeof(inp)-1));
      array<uint8_t, 32> a;
      auto r = encode(c, a.begin(), 4);
      BOOST_CHECK(r == a.begin() + 4);
      BOOST_CHECK(a[0] == 'f');
      BOOST_CHECK(a[1] == 'o');
      BOOST_CHECK(a[2] == '\x0d');
      BOOST_CHECK(a[3] == 'o');
      BOOST_CHECK_THROW(encode(c, a.begin(), 3), std::overflow_error);
      BOOST_CHECK_THROW(encode(c, a.begin(), 5), std::overflow_error);
    }
    BOOST_AUTO_TEST_CASE(xml_content_zero)
    {
      using namespace xfsx;
      XML_Content c;
      array<uint8_t, 32> a;
      auto r = encode(c, a.begin(), minimally_encoded_length(c));
      BOOST_CHECK(r == a.begin());
    }

    // }}}
   BOOST_AUTO_TEST_SUITE_END() // other

  BOOST_AUTO_TEST_SUITE_END()

  BOOST_AUTO_TEST_SUITE(encoded_length)
    // {{{
    BOOST_AUTO_TEST_CASE(basic)
    {
      using namespace xfsx;
      BOOST_CHECK_EQUAL(minimally_encoded_length(int8_t(1))     , 1u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(uint8_t(1))    , 1u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(uint16_t(0))   , 1u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(uint16_t(1))   , 1u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(uint16_t(255)) , 1u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(int16_t(-1))   , 1u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(int16_t(127))  , 1u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(int16_t(-128)) , 1u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(uint32_t(0))   , 1u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(uint32_t(1))   , 1u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(uint32_t(255)) , 1u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(int32_t(-1))   , 1u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(int32_t(127))  , 1u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(int32_t(-128)) , 1u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(uint64_t(0))   , 1u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(uint64_t(1))   , 1u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(uint64_t(255)) , 1u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(int64_t(-1))   , 1u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(int64_t(127))  , 1u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(int64_t(-128)) , 1u);
    }

    BOOST_AUTO_TEST_CASE(two)
    {
      using namespace xfsx;
      BOOST_CHECK_EQUAL(minimally_encoded_length(uint16_t(256)), 2u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(
            uint16_t(numeric_limits<uint16_t>::max())), 2u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(int16_t(-129)), 2u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(int16_t(
              numeric_limits<int16_t>::min())), 2u);

      BOOST_CHECK_EQUAL(minimally_encoded_length(uint32_t(256)), 2u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(uint32_t(
              numeric_limits<uint16_t>::max())), 2u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(int32_t(-129)), 2u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(int32_t(
              numeric_limits<int16_t>::min())), 2u);

      BOOST_CHECK_EQUAL(minimally_encoded_length(uint64_t(256)), 2u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(uint64_t(
              numeric_limits<uint16_t>::max())), 2u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(int64_t(-129)), 2u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(int64_t(
              numeric_limits<int16_t>::min())), 2u);
    }

    BOOST_AUTO_TEST_CASE(larger)
    {
      using namespace xfsx;
      BOOST_CHECK_EQUAL(minimally_encoded_length(
            numeric_limits<uint32_t>::max()), 4u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(uint32_t(
              numeric_limits<uint32_t>::max() - 1)), 4u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(uint32_t(
              numeric_limits<uint32_t>::max() - 2)), 4u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(
            
            numeric_limits<int32_t>::max()), 4u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(int32_t(
            numeric_limits<int32_t>::max() - 1)), 4u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(int32_t(
            numeric_limits<int32_t>::max() - 2)), 4u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(
            numeric_limits<int32_t>::min()), 4u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(int32_t(
            numeric_limits<int32_t>::min() + 1)), 4u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(int32_t(
            numeric_limits<int32_t>::min() + 2)), 4u);


      BOOST_CHECK_EQUAL(minimally_encoded_length(
            numeric_limits<uint64_t>::max()), 8u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(uint64_t(
            numeric_limits<uint64_t>::max()-1)), 8u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(uint64_t(
            numeric_limits<uint64_t>::max()-2)), 8u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(
            numeric_limits<int64_t>::max()), 8u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(int64_t(
            numeric_limits<int64_t>::max() - 1)), 8u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(int64_t(
            numeric_limits<int64_t>::max() - 2)), 8u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(
            numeric_limits<int64_t>::min()), 8u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(int64_t(
            numeric_limits<int64_t>::min() + 1)), 8u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(int64_t(
            numeric_limits<int64_t>::min() + 2)), 8u);
    }

    BOOST_AUTO_TEST_CASE(corner_16)
    {
      using namespace xfsx;
      BOOST_CHECK_EQUAL(minimally_encoded_length(int16_t(
            numeric_limits<int16_t>::max()-1)), 2u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(
            numeric_limits<int16_t>::max()), 2u);
    }

    BOOST_AUTO_TEST_CASE(corner_clz_clrsb)
    {
      using namespace xfsx;
      BOOST_CHECK_EQUAL(minimally_encoded_length(uint64_t(int64_t(-1))>>32), 4);
      BOOST_CHECK_EQUAL(minimally_encoded_length(uint64_t(int64_t(-1))>>48), 2);
      BOOST_CHECK_EQUAL(minimally_encoded_length(uint32_t(int32_t(-1))>>16), 2);
      BOOST_CHECK_EQUAL(minimally_encoded_length(uint32_t(int32_t(-1))>>24), 1);

      BOOST_CHECK_EQUAL(minimally_encoded_length(int64_t(uint64_t(int64_t(-1))>>32)), 5);
      BOOST_CHECK_EQUAL(minimally_encoded_length(int64_t(uint64_t(int64_t(-1))>>48)), 3);
      BOOST_CHECK_EQUAL(minimally_encoded_length(int32_t(uint32_t(int32_t(-1))>>16)), 3);
      BOOST_CHECK_EQUAL(minimally_encoded_length(int32_t(uint32_t(int32_t(-1))>>24)), 2);
    }

    BOOST_AUTO_TEST_CASE(str)
    {
      using namespace xfsx;
      BOOST_CHECK_EQUAL(minimally_encoded_length(string()), 0u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(string("x")), 1u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(string("Hello World")), 11u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(string(256, ' ')), 256u);
    }

    BOOST_AUTO_TEST_CASE(upair)
    {
      using namespace xfsx;
      const char s[] = "Hello World";
      pair<const uint8_t *, const uint8_t *> x(
          reinterpret_cast<const uint8_t*>(s),
          reinterpret_cast<const uint8_t*>(s+sizeof(s)-1));
      BOOST_CHECK_EQUAL(minimally_encoded_length(x), 11u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(make_pair(
              reinterpret_cast<const uint8_t*>(s),
              reinterpret_cast<const uint8_t*>(s))), 0u);
    }

    BOOST_AUTO_TEST_CASE(spair)
    {
      using namespace xfsx;
      const char s[] = "Hello World";
      pair<const char *, const char *> x(s, s+sizeof(s)-1);
      BOOST_CHECK_EQUAL(minimally_encoded_length(x), 11u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(make_pair(s, s)), 0u);
    }

    BOOST_AUTO_TEST_CASE(bcd_string)
    {
      using namespace xfsx;
      BCD_String s("01fa");
      BOOST_CHECK_EQUAL(minimally_encoded_length(s), 2u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(BCD_String("01234")), 3u);
    }

    BOOST_AUTO_TEST_CASE(hex_string)
    {
      using namespace xfsx;
      BOOST_CHECK_EQUAL(minimally_encoded_length(
            Hex_String("&#x20; \\xfaworld")), 13u);
      BOOST_CHECK_EQUAL(minimally_encoded_length(
            Hex_XML_String("&#x20; \\xfaworld")), 11u);
    }

    BOOST_AUTO_TEST_CASE(xml_content)
    {
      using namespace xfsx;
      const char inp[] = "fo&#x0d;o";
      XML_Content c(make_pair(inp, inp + sizeof(inp) -1));
      BOOST_CHECK_EQUAL(minimally_encoded_length(c), 4u);
    }

    // }}}
  BOOST_AUTO_TEST_SUITE_END() // encoded length

  BOOST_AUTO_TEST_SUITE(tlv)
    // {{{


    struct Visitor {
      typedef void result_type;
      int32_t x {0};
      string s;
      pair<const char*, const char*> p;

      void operator()(int32_t v)
      {
        x = v;
      }
      void operator()(const string &v)
      {
        s = v;
      }
      void operator()(const pair<const char*, const char *> &v)
      {
        p = v;
      }
      template <typename T> void operator()(const T &t) { }

    };

    BOOST_AUTO_TEST_CASE(basic)
    {
      using namespace xfsx;
      int32_t x = 23;
      TLV tlv(30, x);
      tlv.klasse = Klasse::APPLICATION;
      BOOST_CHECK_EQUAL(tlv.klasse, Klasse::APPLICATION);
      BOOST_CHECK_EQUAL(tlv.shape, Shape::PRIMITIVE);
      BOOST_CHECK_EQUAL(tlv.is_long_tag, false);
      BOOST_CHECK_EQUAL(tlv.is_indefinite, false);
      BOOST_CHECK_EQUAL(tlv.is_long_definite, false);
      BOOST_CHECK_EQUAL(tlv.t_size, 1u);
      BOOST_CHECK_EQUAL(tlv.tl_size, 2u);
      BOOST_CHECK_EQUAL(tlv.tag, 30u);
      BOOST_CHECK_EQUAL(tlv.length, 1u);
      Visitor v;
      tlv.value().accept(v);
      BOOST_CHECK_EQUAL(v.x, 23);
    }

    BOOST_AUTO_TEST_CASE(str_move)
    {
      using namespace xfsx;
      TLV tlv(31, string("Hello World"));
      BOOST_CHECK_EQUAL(tlv.klasse, Klasse::UNIVERSAL);
      BOOST_CHECK_EQUAL(tlv.shape, Shape::PRIMITIVE);
      BOOST_CHECK_EQUAL(tlv.is_long_tag, true);
      BOOST_CHECK_EQUAL(tlv.is_indefinite, false);
      BOOST_CHECK_EQUAL(tlv.is_long_definite, false);
      BOOST_CHECK_EQUAL(tlv.t_size, 2u);
      BOOST_CHECK_EQUAL(tlv.tl_size, 3u);
      BOOST_CHECK_EQUAL(tlv.tag, 31u);
      BOOST_CHECK_EQUAL(tlv.length, 11u);
      Visitor v;
      tlv.value().accept(v);
      BOOST_CHECK_EQUAL(v.s, "Hello World");
    }

    BOOST_AUTO_TEST_CASE(pair_assign_move)
    {
      using namespace xfsx;
      TLV tlv(31);
      const char s[] = "Hello World";
      pair<const char *, const char*> x(s, s+sizeof(s)-1);
      tlv = std::move(x);
      BOOST_CHECK_EQUAL(tlv.klasse, Klasse::UNIVERSAL);
      BOOST_CHECK_EQUAL(tlv.shape, Shape::PRIMITIVE);
      BOOST_CHECK_EQUAL(tlv.is_long_tag, true);
      BOOST_CHECK_EQUAL(tlv.is_indefinite, false);
      BOOST_CHECK_EQUAL(tlv.is_long_definite, false);
      BOOST_CHECK_EQUAL(tlv.t_size, 2u);
      BOOST_CHECK_EQUAL(tlv.tl_size, 3u);
      BOOST_CHECK_EQUAL(tlv.tag, 31u);
      BOOST_CHECK_EQUAL(tlv.length, 11u);
      Visitor v;
      tlv.value().accept(v);
      BOOST_CHECK_EQUAL(string(v.p.first, v.p.second), "Hello World");
    }

    BOOST_AUTO_TEST_CASE(int_assign)
    {
      using namespace xfsx;
      int32_t x = 23;
      TLV tlv(30);
      tlv = x;
      BOOST_CHECK_EQUAL(tlv.klasse, Klasse::UNIVERSAL);
      BOOST_CHECK_EQUAL(tlv.shape, Shape::PRIMITIVE);
      BOOST_CHECK_EQUAL(tlv.is_long_tag, false);
      BOOST_CHECK_EQUAL(tlv.is_indefinite, false);
      BOOST_CHECK_EQUAL(tlv.is_long_definite, false);
      BOOST_CHECK_EQUAL(tlv.t_size, 1u);
      BOOST_CHECK_EQUAL(tlv.tl_size, 2u);
      BOOST_CHECK_EQUAL(tlv.tag, 30u);
      BOOST_CHECK_EQUAL(tlv.length, 1u);
      Visitor v;
      tlv.value().accept(v);
      BOOST_CHECK_EQUAL(v.x, 23);
    }

    BOOST_AUTO_TEST_CASE(simple_write)
    {
      using namespace xfsx;
      array<uint8_t, 30> a;
      Unit u(42);
      uint8_t *p = u.write(a.begin(), a.end());
      TLV x(13, int32_t(6));
      x.klasse = Klasse::APPLICATION;
      p = x.write(p, a.end());
      TLV y(14, string("Hello World"));
      p = y.write(p, a.end());
      Unit eoc{Unit::EOC()};
      p = eoc.write(p, a.end());
      BOOST_CHECK_EQUAL(p - a.begin(), 21);

      {
        TLC tlc;
        Unit &u = tlc;
        const uint8_t *p = u.read(a.begin(), a.end());
        BOOST_CHECK_EQUAL(u.tag, 42u);
        BOOST_CHECK_EQUAL(u.klasse, Klasse::UNIVERSAL);
        BOOST_CHECK_EQUAL(u.shape, Shape::CONSTRUCTED);
        BOOST_CHECK_EQUAL(u.is_indefinite, true);
        p = tlc.read(p, a.end());
        BOOST_CHECK_EQUAL(u.tag, 13u);
        BOOST_CHECK_EQUAL(u.klasse, Klasse::APPLICATION);
        BOOST_CHECK_EQUAL(u.shape, Shape::PRIMITIVE);
        int32_t x = tlc.lexical_cast<int32_t>();
        BOOST_CHECK_EQUAL(x, 6);
        p = tlc.read(p, a.end());
        BOOST_CHECK_EQUAL(u.tag, 14u);
        pair<const char*, const char*> y;
        tlc.copy_content(y);
        BOOST_CHECK_EQUAL(string(y.first, y.second), "Hello World");
        p = u.read(p, a.end());
        BOOST_CHECK_EQUAL(u.is_eoc(), true);
      }
    }

    BOOST_AUTO_TEST_CASE(constructed_write)
    {
      using namespace xfsx;
      array<uint8_t, 32> a;
      TLV x(13);
      x.shape = Shape::CONSTRUCTED;
      x.klasse = Klasse::APPLICATION;
      x.init_indefinite();
      auto o = x.write(a.begin(), a.end());
      TLV eoc{Unit::EOC()};
      o = eoc.write(o, a.end());
      {
        TLC tlc;
        const uint8_t *p = tlc.read(a.begin(), a.end());
        BOOST_CHECK_EQUAL(tlc.tag, 13u);
        BOOST_CHECK_EQUAL(tlc.shape, Shape::CONSTRUCTED);
        BOOST_CHECK_EQUAL(tlc.klasse, Klasse::APPLICATION);
        BOOST_CHECK(tlc.is_indefinite);
        p = tlc.read(p, a.end());
        BOOST_CHECK_EQUAL(tlc.tag, 0u);
        BOOST_CHECK_EQUAL(tlc.shape, Shape::PRIMITIVE);
        BOOST_CHECK_EQUAL(tlc.klasse, Klasse::UNIVERSAL);
        BOOST_CHECK(!tlc.is_indefinite);
        BOOST_CHECK(tlc.is_eoc());
      }
    }

    BOOST_AUTO_TEST_CASE(xml_content)
    {
      using namespace xfsx;
      array<uint8_t, 32> a;
      TLV x(13, XML_Content());
      x.klasse = Klasse::APPLICATION;
      auto o = x.write(a.begin(), a.end());
      (void)o;
      {
        TLC tlc;
        const uint8_t *p = tlc.read(a.begin(), a.end());
        (void)p;
        BOOST_CHECK_EQUAL(tlc.tag, 13u);
        BOOST_CHECK_EQUAL(tlc.shape, Shape::PRIMITIVE);
        BOOST_CHECK_EQUAL(tlc.klasse, Klasse::APPLICATION);
        BOOST_CHECK_EQUAL(tlc.length, 0u);
        BOOST_CHECK(!tlc.is_indefinite);
      }
    }

    BOOST_AUTO_TEST_CASE(end_of_c)
    {
      using namespace xfsx;
      {
      TLV x{Unit::EOC()};
      BOOST_CHECK_EQUAL(x.t_size, 1u);
      BOOST_CHECK_EQUAL(x.tl_size, 2u);
      BOOST_CHECK_EQUAL(x.klasse, Klasse::UNIVERSAL);
      BOOST_CHECK_EQUAL(x.shape, Shape::PRIMITIVE);

      TLV y(std::move(x));
      BOOST_CHECK_EQUAL(y.t_size, 1u);
      BOOST_CHECK_EQUAL(y.tl_size, 2u);
      BOOST_CHECK_EQUAL(y.klasse, Klasse::UNIVERSAL);
      BOOST_CHECK_EQUAL(y.shape, Shape::PRIMITIVE);
      }
      /*
      {
      TLC x{EOC};
      BOOST_CHECK_EQUAL(x.t_size, 1u);
      BOOST_CHECK_EQUAL(x.tl_size, 2u);
      }
      */
      {
      Unit x{Unit::EOC()};
      BOOST_CHECK_EQUAL(x.t_size, 1u);
      BOOST_CHECK_EQUAL(x.tl_size, 2u);
      BOOST_CHECK_EQUAL(x.klasse, Klasse::UNIVERSAL);
      BOOST_CHECK_EQUAL(x.shape, Shape::PRIMITIVE);
      }
    }

    // }}}
  BOOST_AUTO_TEST_SUITE_END() // tlv


  BOOST_AUTO_TEST_SUITE(vertical_tlc)
  // {{{


    BOOST_AUTO_TEST_CASE(throw_unmatched_eoc)
    {
      using namespace xfsx;
      array<uint8_t, 8> a { 0u, 0u };
      Vertical_TLC t;
      BOOST_CHECK_THROW(t.read(a.begin(), a.end()), std::range_error);
    }
    BOOST_AUTO_TEST_CASE(throw_unmatched_eoc2)
    {
      using namespace xfsx;
      array<uint8_t, 8> a { 0u, 0u };
      Unit c(23);
      auto r = c.write(a.begin(), a.end());
      Unit eoc{Unit::EOC()};
      r = eoc.write(r, a.end());
      r = eoc.write(r, a.end());
      Vertical_TLC t;
      auto p = t.read(a.begin(), a.end());
      p = t.read(p, a.end());
      BOOST_CHECK_THROW(t.read(p, a.end()), std::range_error);
    }
    BOOST_AUTO_TEST_CASE(throw_cut_tag)
    {
      using namespace xfsx;
      Unit c(23);
      c.init_length(8);
      array<uint8_t, 32> a;
      auto r = c.write(a.begin(), a.end());
      TLV v(30, string("Hello World"));
      r = v.write(r, a.end());

      Vertical_TLC t;
      auto p = t.read(a.begin(), a.end());
      BOOST_CHECK_THROW(t.read(p, a.end()), std::overflow_error);
    }

    BOOST_AUTO_TEST_CASE(zero_length_constructed_and_eoc_height)
    {
      using namespace xfsx;
      Unit c(23);
      array<uint8_t, 32> a;
      auto r = c.write(a.begin(), a.end());
      Unit x(11);
      x.init_length(0);
      r = x.write(r, a.end());
      r = x.write(r, a.end());
      Unit eoc{Unit::EOC{}};
      r = eoc.write(r, a.end());

      Vertical_TLC t;
      auto p = t.read(a.begin(), a.end());
      BOOST_CHECK_EQUAL(t.height, 0u);
      p = t.read(p, a.end());
      BOOST_CHECK_EQUAL(t.height, 1u);
      p = t.read(p, a.end());
      BOOST_CHECK_EQUAL(t.height, 1u);
      p = t.read(p, a.end());
      BOOST_CHECK_EQUAL(t.height, 0u);
      BOOST_CHECK_EQUAL(t.is_eoc(), true);
    }

    BOOST_AUTO_TEST_CASE(some_heights)
    {
      using namespace xfsx;
      array<uint8_t, 128> a;
      Unit c1(23);
      auto r = c1.write(a.begin(), a.end());
      TLV v1(3, string("Hello"));
      r = v1.write(r, a.end());
      TLV v2(4, int32_t(23));
      r = v2.write(r, a.end());
      Unit c2(24);
      r = c2.write(r, a.end());
      TLV v3(5, int64_t(4242));
      r = v3.write(r, a.end());
      Unit eoc{Unit::EOC{}};
      r = eoc.write(r, a.end());
      r = eoc.write(r, a.end());

      Vertical_TLC t;
      auto p = t.read(a.begin(), a.end());
      BOOST_CHECK_EQUAL(t.height, 0u);
      p = t.read(p, a.end());
      BOOST_CHECK_EQUAL(t.height, 1u);
      p = t.read(p, a.end());
      BOOST_CHECK_EQUAL(t.height, 1u);
      p = t.read(p, a.end());
      BOOST_CHECK_EQUAL(t.height, 1u);
      p = t.read(p, a.end());
      BOOST_CHECK_EQUAL(t.height, 2u);
      p = t.read(p, a.end());
      BOOST_CHECK_EQUAL(t.height, 1u);
      p = t.read(p, a.end());
      BOOST_CHECK_EQUAL(t.height, 0u);
    }

    BOOST_AUTO_TEST_CASE(skip_over_root)
    {
      using namespace xfsx;
      boost::filesystem::path in(test::path::in());
      in /= "tap_3_12_valid.ber";
      ixxx::util::Mapped_File f(in.generic_string());
      Vertical_TLC t;
      auto r = t.read(f.begin(), f.end());
      r = t.skip(r, f.end());
      BOOST_CHECK(r == f.end());
    }

    BOOST_AUTO_TEST_CASE(skip_over_cdrs)
    {
      using namespace xfsx;
      boost::filesystem::path in(test::path::in());
      in /= "tap_3_12_valid.ber";
      ixxx::util::Mapped_File f(in.generic_string());
      Vertical_TLC t;
      auto r = t.read(f.begin() + 254, f.end());
      r = t.skip(r, f.end());
      BOOST_CHECK(r == f.begin() + 740); // ACI
    }

    BOOST_AUTO_TEST_CASE(skip_over_indefinite_cdrs)
    {
      using namespace xfsx;
      boost::filesystem::path in(test::path::in());
      in /= "tap_3_12_valid_some_cdr_indefinite.ber";
      ixxx::util::Mapped_File f(in.generic_string());
      Vertical_TLC t;
      // CallEventDetailList and some CDRs inside of indefinite
      auto r = t.read(f.begin() + 254, f.end());
      r = t.skip_children(r, f.end());
      BOOST_CHECK(r == f.begin() + 743); // ACI
    }



  // }}}
  BOOST_AUTO_TEST_SUITE_END() // vertical_tlc

  BOOST_AUTO_TEST_SUITE(reader)
    // {{{
    BOOST_AUTO_TEST_CASE(skip_eoc)
    {
      using namespace xfsx;
      array<uint8_t, 128> a;
      Unit c1(23);
      auto p = c1.write(a.begin(), a.end());
      TLV v1(3, string("Hello"));
      p = v1.write(p, a.end());
      TLV v2(4, int32_t(23));
      p = v2.write(p, a.end());
      Unit c2(24);
      p = c2.write(p, a.end());
      TLV v3(5, int64_t(4242));
      p = v3.write(p, a.end());
      Unit eoc{Unit::EOC{}};
      p = eoc.write(p, a.end());
      p = eoc.write(p, a.end());

      Skip_EOC_Reader r(a.begin(), p);
      auto i = r.begin();
      BOOST_REQUIRE(i != r.end());

      BOOST_CHECK_EQUAL((*i).height, 0u);
      ++i;
      BOOST_REQUIRE(i != r.end());
      BOOST_CHECK_EQUAL((*i).height, 1u);
      ++i;
      BOOST_REQUIRE(i != r.end());
      BOOST_CHECK_EQUAL((*i).height, 1u);
      ++i;
      BOOST_REQUIRE(i != r.end());
      BOOST_CHECK_EQUAL((*i).height, 1u);
      ++i;
      BOOST_REQUIRE(i != r.end());
      BOOST_CHECK_EQUAL((*i).height, 2u);
      ++i;
      BOOST_REQUIRE(i == r.end());
    }
    // }}}
  BOOST_AUTO_TEST_SUITE_END() //reader

  BOOST_AUTO_TEST_SUITE(tags)

    BOOST_AUTO_TEST_CASE(default_translate)
    {
      using namespace xfsx;
      Tag_Translator t;
      BOOST_CHECK_THROW(t.translate(Klasse::APPLICATION, 23u),
          std::range_error);
    }
  
    BOOST_AUTO_TEST_CASE(default_dereference)
    {
      using namespace xfsx;
      Tag_Dereferencer d;
      auto p = d.dereference(Klasse::APPLICATION, 23u);
      BOOST_CHECK_EQUAL(p.first, Klasse::APPLICATION);
      BOOST_CHECK_EQUAL(p.second, 23u);
    }

    BOOST_AUTO_TEST_CASE(default_typify)
    {
      using namespace xfsx;
      Tag_Typifier t;
      auto type = t.typify(Klasse::APPLICATION, 23u);
      BOOST_CHECK(type == Type::OCTET_STRING);
    }

  BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
