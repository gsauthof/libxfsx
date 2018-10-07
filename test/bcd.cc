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
#include <xfsx/raw_vector.hh>

using u8 = xfsx::u8;

using namespace std;

BOOST_AUTO_TEST_SUITE(xfsx_)

  BOOST_AUTO_TEST_SUITE(bcd_)

    BOOST_AUTO_TEST_SUITE(impl_)


     BOOST_AUTO_TEST_SUITE(encoders)

       using namespace xfsx::bcd::impl;
       using namespace xfsx::bcd::impl::encode;

      typedef boost::mpl::list<
            Encode<u8*>
          , Encode<u8*, uint32_t>
          , Encode<u8*, uint64_t, Scatter::Direct>
          , Two_Char_Encode<u8*>
        > encoder_types;

       BOOST_AUTO_TEST_CASE_TEMPLATE(empty, T, encoder_types)
       {
         array<char, 1> i;
         array<u8, 1> a;
         auto r = T()(i.begin(), i.begin(), a.begin());
         BOOST_CHECK(r == a.begin());
       }

       BOOST_AUTO_TEST_CASE_TEMPLATE(one, T, encoder_types)
       {
         const char i[] = "c";
         array<u8, 1> a;
         const char *begin = i;
         const char *end = begin + sizeof(i) - 1;
         auto r = T()(begin, end, a.begin());
         BOOST_CHECK(r == a.end());
         BOOST_CHECK_EQUAL(a[0], 0xcfu);
       }

       BOOST_AUTO_TEST_CASE_TEMPLATE(more, T, encoder_types)
       {
         const char i[] = "deadcafe";
         array<u8, 4> a;
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
         array<u8, 5> a;
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
         vector<u8> a(8);
         const char *begin = i;
         const char *end = begin + sizeof(i) - 1;
         auto r = T()(begin, end, a.data());
         BOOST_CHECK(r == a.data()+a.size());
         BOOST_CHECK_EQUAL(a[7], 0x3fu);
       }

       BOOST_AUTO_TEST_CASE_TEMPLATE(upper_case, T, encoder_types)
       {
         const char i[] = "DeAdcAfEdeADcaFEDEadCAfe";
         array<u8, 12> a;
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
          vector<u8> a(8);
          auto r = xfsx::bcd::encode(i, i + sizeof(i) - 1, a.data());
          BOOST_CHECK(r == a.data() + a.size());
        }

      BOOST_AUTO_TEST_SUITE_END() // encode_


    BOOST_AUTO_TEST_SUITE_END() // front

    BOOST_AUTO_TEST_SUITE(raw_vector)
        BOOST_AUTO_TEST_CASE(raw_vector_resize)
        {
            const char s[] = "0123456789";
            xfsx::Raw_Vector<char> v(s, s+sizeof s - 1);
            BOOST_CHECK_EQUAL(string(v.begin(), v.end()), s);
            v.resize(4);
            BOOST_CHECK_EQUAL(string(v.begin(), v.end()), "0123");
            v.resize(10);
            BOOST_CHECK_EQUAL(string(v.begin(), v.end()), s);
        }
    BOOST_AUTO_TEST_SUITE_END() // raw_vector

  BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
