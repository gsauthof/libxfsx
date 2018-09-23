#include <boost/test/unit_test.hpp>

#include <boost/mpl/list.hpp>
#include <boost/predef.h>

#include <vector>
#include <array>
#include <iostream>
#include <limits>
#include <string>

#include <xfsx/bcd_impl.hh>
#include <xfsx/bcd.hh>


using namespace std;

BOOST_AUTO_TEST_SUITE(xfsx_)

  BOOST_AUTO_TEST_SUITE(bcd_)

    BOOST_AUTO_TEST_SUITE(impl_)


     BOOST_AUTO_TEST_SUITE(basic)

      using namespace xfsx::bcd::impl;
      using namespace xfsx::bcd::impl::decode;

      BOOST_AUTO_TEST_CASE(basic_manual)
      {
        Two_Half_Decode<char*> d;
        array<uint8_t, 2> a = { 0xDEu, 0xADu };
        string s(4, ' ');
        char *o = &s[0];
        d(a.begin(), a.end(), o);
        BOOST_CHECK_EQUAL(s, "dead");
      }

      BOOST_AUTO_TEST_CASE(basic_non_default)
      {
        Two_Half_Decode<char*, Half_To_Char_Branch<> > d;
        array<uint8_t, 2> a = { 0xDEu, 0xADu };
        string s(4, ' ');
        char *o = &s[0];
        d(a.begin(), a.end(), o);
        BOOST_CHECK_EQUAL(s, "dead");
      }

      BOOST_AUTO_TEST_CASE(already_aligned)
      {
        array<char, 128> a;
        size_t x = (reinterpret_cast<size_t>(a.data()) + 16u) & (~0x111u);
        char *p = reinterpret_cast<char *>(x);
        char *r = next_aligned_address<uint64_t>(p);
        BOOST_CHECK(r == p);
      }

      BOOST_AUTO_TEST_CASE(align_up)
      {
        array<char, 128> a;
        size_t x = (reinterpret_cast<size_t>(a.data()) + 16u) & (~0x111u);
        char *p = reinterpret_cast<char *>(x);
        char *r = next_aligned_address<uint64_t>(p-1);
        BOOST_CHECK(r == p);
      }

      BOOST_AUTO_TEST_CASE(align_up_more)
      {
        array<char, 128> a;
        size_t x = (reinterpret_cast<size_t>(a.data()) + 16u) & (~0x111u);
        char *p = reinterpret_cast<char *>(x);
        char *r = next_aligned_address<uint64_t>(p+1);
        BOOST_CHECK(r == p+8);
      }

      BOOST_AUTO_TEST_CASE(char_to_half_single)
      {
        using namespace xfsx::bcd::impl;
        char inp[] = "0123456789abcdef";
        for (uint8_t i = 0; i<16; ++i) {
          // extra parentheses protect against
          // macro substitution gone wrong
          BOOST_CHECK_EQUAL((encode::Convert::Base<uint8_t,
                encode::Convert::Bit_Parallel>()(inp[i])), i);
        }
      }

      BOOST_AUTO_TEST_CASE(char_to_half_ui64)
      {
        using namespace xfsx::bcd::impl;
        char inp_s[] = "0123456789abcdef";
        uint64_t inp;
        memcpy(&inp, inp_s, sizeof(inp));
        inp = boost::endian::little_to_native(inp);
        uint64_t r = encode::Convert::Base<uint64_t,
                 encode::Convert::Bit_Parallel>()(inp);
        for (uint8_t i = 0; i < sizeof(uint64_t); ++i) {
          BOOST_CHECK_EQUAL( (r >> (i*8)) & 0xff, i);
        }
      }

      BOOST_AUTO_TEST_CASE(encode_gather)
      {
        using namespace xfsx::bcd::impl;
        uint64_t i = 0x0e0f0a0c0d0a0e0dlu;
        i = boost::endian::native_to_little(i);
        uint64_t o = 0x00000000deadcafelu;
        (void)o;
        array<uint8_t, 4> a;
        auto r = encode::Gather::Base<uint64_t, uint8_t*,
             encode::Gather::Shift>()(i, a.begin());
        BOOST_REQUIRE(r == a.end());

        BOOST_CHECK_EQUAL(a[0], 0xdeu);
        BOOST_CHECK_EQUAL(a[1], 0xadu);
        BOOST_CHECK_EQUAL(a[2], 0xcau);
        BOOST_CHECK_EQUAL(a[3], 0xfeu);
      }

      BOOST_AUTO_TEST_CASE(encode_gather_big)
      {
        using namespace xfsx::bcd::impl;
        uint64_t i = 0x0d0e0a0d0c0a0f0elu;
        uint64_t o = 0x00000000deadcafelu;
        (void)o;
        array<uint8_t, 4> a;
        auto r = encode::Gather::Base<uint64_t, uint8_t*,
             encode::Gather::Shift_Big>()(i, a.begin());
        BOOST_REQUIRE(r == a.end());

        BOOST_CHECK_EQUAL(a[0], 0xdeu);
        BOOST_CHECK_EQUAL(a[1], 0xadu);
        BOOST_CHECK_EQUAL(a[2], 0xcau);
        BOOST_CHECK_EQUAL(a[3], 0xfeu);
      }


     BOOST_AUTO_TEST_SUITE_END() // basic
      
     BOOST_AUTO_TEST_SUITE(decoders)

      using namespace xfsx::bcd::impl;
      using namespace xfsx::bcd::impl::decode;

      typedef boost::mpl::list<
            Two_Half_Decode<char *>
          , Two_Half_Decode<char *, Half_To_Char_Branch<> >
          , Two_Half_Decode<char *, Half_To_Char_Cmp<> >
          , Two_Half_Decode<char *, Half_To_Char_Div<> >
          , Decode<char *, uint64_t>
          , Decode<char *, uint32_t>
          , Decode<char *, uint16_t>
#if BOOST_ENDIAN_LITTLE_BYTE
          , Decode<char *, uint64_t, Scatter::Reverse, Convert::Bit_Parallel,
                   Gather::Reverse >
#endif
          // already tested above due to template defaults
          //, Decode<char *, uint64_t, Scatter::Shift, Convert::Bit_Parallel,
          //         Gather::Memcpy >
          , Decode<char *, uint64_t, Scatter::Shift, Convert::Bit_Parallel,
                   Gather::Direct >
          , Decode<char *, uint64_t, Scatter::Shift, Convert::Bit_Parallel,
                   Gather::Slow >
          , Decode<char *, uint64_t, Scatter::Shift, Convert::Bit_Parallel,
                   Gather::Memcpy, Align::Yes >
        > decoder_types;

      BOOST_AUTO_TEST_CASE_TEMPLATE(empty, T, decoder_types)
      {
        T d;
        const uint8_t a[1] = {0};
        string s;
        char *p = &s[0];
        d(a, a, p);
        BOOST_CHECK_EQUAL(s.empty(), true);
      }

      BOOST_AUTO_TEST_CASE_TEMPLATE(single, T, decoder_types)
      {
        T d;
        array<uint8_t, 1> a = { 0xCAu };
        string s(2, ' ');;
        char *p = &s[0];
        d(a.begin(), a.end(), p);
        BOOST_CHECK_EQUAL(s, "ca");
      }

      BOOST_AUTO_TEST_CASE_TEMPLATE(basic, T, decoder_types)
      {
        T d;
        array<uint8_t, 4> a = {
          0xDEu, 0xADu, 0xCAu, 0x0Eu
        };
        string s(a.size()*2, ' ');
        char *p = &s[0];
        d(a.begin(), a.end(), p);
        BOOST_CHECK_EQUAL(s, "deadca0e");
      }

      BOOST_AUTO_TEST_CASE_TEMPLATE(filler_in_the_middle, T, decoder_types)
      {
        T d;
        using namespace xfsx;
        array<uint8_t, 4> a = {
          0xDEu, 0xADu, 0xCAu, 0xFEu
        };
        string s(a.size()*2, ' ');
        char *p = &s[0];
        d(a.begin(), a.end(), p);
        // no special handling (e.g. throw for fillers in the middle)
        BOOST_CHECK_EQUAL(s, "deadcafe");
      }

      BOOST_AUTO_TEST_CASE_TEMPLATE(filler_at_the_end, T, decoder_types)
      {
        T d;
        using namespace xfsx;
        array<uint8_t, 4> a = {
          0xDEu, 0xADu, 0xCAu, 0xEFu
        };
        string s(a.size()*2, ' ');
        char *p = &s[0];
        d(a.begin(), a.end(), p);
        // no special handling, must be handle in the caller
        BOOST_CHECK_EQUAL(s, "deadcaef");
      }

      BOOST_AUTO_TEST_CASE_TEMPLATE(all, T, decoder_types)
      {
        T d;
        using namespace xfsx;
        array<uint8_t, 8> a = {
          0x12u, 0x34u, 0x56u, 0x78u, 0x90u, 0xABu, 0xCDu, 0xEFu
        };
        string s(a.size()*2, ' ');
        char *p = &s[0];
        d(a.begin(), a.end(), p);
        BOOST_CHECK_EQUAL(s, "1234567890abcdef");
      }

      BOOST_AUTO_TEST_CASE_TEMPLATE(odd, T, decoder_types)
      {
        T d;
        using namespace xfsx;
        array<uint8_t, 7> a = {
          0x12u, 0x34u, 0x56u, 0x78u, 0x90u, 0xABu, 0xCDu
        };
        string s(a.size()*2, ' ');
        char *p = &s[0];
        d(a.begin(), a.end(), p);
        BOOST_CHECK_EQUAL(s, "1234567890abcd");
      }

      BOOST_AUTO_TEST_CASE_TEMPLATE(odd_plus_one, T, decoder_types)
      {
        T d;
        using namespace xfsx;
        array<uint8_t, 9> a = {
          0x12u, 0x34u, 0x56u, 0x78u, 0x90u, 0xABu, 0xCDu, 0xEFu, 0x23u
        };
        string s(a.size()*2, ' ');
        char *p = &s[0];
        d(a.begin(), a.end(), p);
        BOOST_CHECK_EQUAL(s, "1234567890abcdef23");
      }

      BOOST_AUTO_TEST_CASE_TEMPLATE(all_long, T, decoder_types)
      {
        T d;
        using namespace xfsx;
        array<uint8_t, 64> a = {
          0x12u, 0x34u, 0x56u, 0x78u, 0x90u, 0xABu, 0xCDu, 0xEFu,
          0x12u, 0x34u, 0x56u, 0x78u, 0x90u, 0xABu, 0xCDu, 0xEFu,
          0x12u, 0x34u, 0x56u, 0x78u, 0x90u, 0xABu, 0xCDu, 0xEFu,
          0x12u, 0x34u, 0x56u, 0x78u, 0x90u, 0xABu, 0xCDu, 0xEFu,
          0x12u, 0x34u, 0x56u, 0x78u, 0x90u, 0xABu, 0xCDu, 0xEFu,
          0x12u, 0x34u, 0x56u, 0x78u, 0x90u, 0xABu, 0xCDu, 0xEFu,
          0x12u, 0x34u, 0x56u, 0x78u, 0x90u, 0xABu, 0xCDu, 0xEFu,
          0x12u, 0x34u, 0x56u, 0x78u, 0x90u, 0xABu, 0xCDu, 0xEFu
        };
        string s(a.size()*2, ' ');
        char *p = &s[0];
        d(a.begin(), a.end(), p);
        BOOST_CHECK_EQUAL(s,
            "1234567890abcdef"
            "1234567890abcdef"
            "1234567890abcdef"
            "1234567890abcdef"
            "1234567890abcdef"
            "1234567890abcdef"
            "1234567890abcdef"
            "1234567890abcdef"
            );
      }

     BOOST_AUTO_TEST_SUITE_END() // decoders


     BOOST_AUTO_TEST_SUITE(encoders)

       using namespace xfsx::bcd::impl;
       using namespace xfsx::bcd::impl::encode;

      typedef boost::mpl::list<
            Encode<uint8_t*>
          , Encode<uint8_t*, uint32_t>
          , Encode<uint8_t*, uint64_t, Scatter::Direct>
          , Two_Char_Encode<uint8_t*>
        > encoder_types;

       BOOST_AUTO_TEST_CASE_TEMPLATE(empty, T, encoder_types)
       {
         array<char, 1> i;
         array<uint8_t, 1> a;
         auto r = T()(i.begin(), i.begin(), a.begin());
         BOOST_CHECK(r == a.begin());
       }

       BOOST_AUTO_TEST_CASE_TEMPLATE(one, T, encoder_types)
       {
         const char i[] = "c";
         array<uint8_t, 1> a;
         const char *begin = i;
         const char *end = begin + sizeof(i) - 1;
         auto r = T()(begin, end, a.begin());
         BOOST_CHECK(r == a.end());
         BOOST_CHECK_EQUAL(a[0], 0xcfu);
       }

       BOOST_AUTO_TEST_CASE_TEMPLATE(more, T, encoder_types)
       {
         const char i[] = "deadcafe";
         array<uint8_t, 4> a;
         const char *begin = i;
         const char *end = begin + sizeof(i) - 1;
         auto r = T()(begin, end, a.begin());
         BOOST_CHECK(r == a.end());
         BOOST_CHECK_EQUAL(a[0], 0xdeu);
         BOOST_CHECK_EQUAL(a[1], 0xadu);
         BOOST_CHECK_EQUAL(a[2], 0xcau);
         BOOST_CHECK_EQUAL(a[3], 0xfeu);
       }

       BOOST_AUTO_TEST_CASE_TEMPLATE(filler, T, encoder_types)
       {
         const char i[] = "deadcafe1";
         array<uint8_t, 5> a;
         const char *begin = i;
         const char *end = begin + sizeof(i) - 1;
         auto r = T()(begin, end, a.begin());
         BOOST_CHECK(r == a.end());
         BOOST_CHECK_EQUAL(a[0], 0xdeu);
         BOOST_CHECK_EQUAL(a[1], 0xadu);
         BOOST_CHECK_EQUAL(a[2], 0xcau);
         BOOST_CHECK_EQUAL(a[3], 0xfeu);
         BOOST_CHECK_EQUAL(a[4], 0x1fu);
       }

       BOOST_AUTO_TEST_CASE_TEMPLATE(filler_longer, T, encoder_types)
       {
         const char i[] = "133713371337133";
         vector<uint8_t> a(8);
         const char *begin = i;
         const char *end = begin + sizeof(i) - 1;
         auto r = T()(begin, end, a.data());
         BOOST_CHECK(r == a.data()+a.size());
         BOOST_CHECK_EQUAL(a[7], 0x3fu);
       }

       BOOST_AUTO_TEST_CASE_TEMPLATE(upper_case, T, encoder_types)
       {
         const char i[] = "DeAdcAfEdeADcaFEDEadCAfe";
         array<uint8_t, 12> a;
         const char *begin = i;
         const char *end = begin + sizeof(i) - 1;
         auto r = T()(begin, end, a.begin());
         BOOST_CHECK(r == a.end());
         BOOST_CHECK_EQUAL(a[0], 0xdeu);
         BOOST_CHECK_EQUAL(a[1], 0xadu);
         BOOST_CHECK_EQUAL(a[2], 0xcau);
         BOOST_CHECK_EQUAL(a[3], 0xfeu);

         BOOST_CHECK_EQUAL(a[4], 0xdeu);
         BOOST_CHECK_EQUAL(a[5], 0xadu);
         BOOST_CHECK_EQUAL(a[6], 0xcau);
         BOOST_CHECK_EQUAL(a[7], 0xfeu);

         BOOST_CHECK_EQUAL(a[8], 0xdeu);
         BOOST_CHECK_EQUAL(a[9], 0xadu);
         BOOST_CHECK_EQUAL(a[10], 0xcau);
         BOOST_CHECK_EQUAL(a[11], 0xfeu);
       }


     BOOST_AUTO_TEST_SUITE_END() // encoders

    BOOST_AUTO_TEST_SUITE_END() // impl_


    BOOST_AUTO_TEST_SUITE(front)

      BOOST_AUTO_TEST_SUITE(encode_)

        BOOST_AUTO_TEST_CASE(filler)
        {
          const char i[] = "133713371337133";
          vector<uint8_t> a(8);
          auto r = xfsx::bcd::encode(i, i + sizeof(i) - 1, a.data());
          BOOST_CHECK(r == a.data() + a.size());
        }

      BOOST_AUTO_TEST_SUITE_END() // encode_


    BOOST_AUTO_TEST_SUITE_END() // front

  BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
