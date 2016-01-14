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
#ifndef XFSX_BCD_IMPL_ENCODE_HH
#define XFSX_BCD_IMPL_ENCODE_HH

#include <stdint.h>
#include <limits>

#include <stdlib.h>
#include <boost/endian/conversion.hpp>

#include <xfsx/bcd/util.hh>

namespace xfsx {

  namespace bcd {

    namespace impl {

      namespace encode {

        template <char A = 'a'>
          struct Char_To_Half_SAR {
            uint8_t operator()(char c) const
            {
                uint8_t a = uint8_t(A);
                uint8_t lower_case_bit = 0b00'10'0000u;
                uint8_t b = uint8_t(c) | lower_case_bit;
                uint8_t x = b - uint8_t('0');

                uint8_t is_geq_10 = b & 0b01'00'0000u;
                uint8_t geq_10_mask =  is_geq_10 - (is_geq_10 >> 6);

                uint8_t geq_10_off = a - uint8_t('0') - 10u;

                uint8_t result = x - (geq_10_mask & geq_10_off);
                return result;
            }
          };

        template <typename O> struct Two_Char_Encode {
          O operator()(const char *begin, const char *end, O o) const
          {
            auto c = begin;
            auto e = end - (end - begin)%2;
            Char_To_Half_SAR<> f;
            for (; c < e; ++c) {
              *o = f(*c) << 4;
              ++c;
              *o |= f(*c);
              ++o;
            }
            if (e < end) {
              *o = f(*c) << 4;
              *o |= 0x0fu;
              ++o;
            }
            return o;
          }
        };


        namespace Scatter {

          struct Direct {};
          struct Memcpy {};

          template <typename T, typename Tag> struct Base {};

          // has alignment and aliasing issues
          template <typename T> struct Base<T, Direct> {
            T operator()(const uint8_t *i) const
            {
              return *reinterpret_cast<const T*>(i);
            }
          };

          template <typename T> struct Base<T, Memcpy> {
            T operator()(const uint8_t *i) const
            {
              T r;
              memcpy(&r, i, sizeof(T));
              return r;
            }
          };
        }

        namespace Convert {

          struct Bit_Parallel;

          template <typename T, typename Tag> struct Base {};

          // ASCII chars '0'..'9' and 'a'..'f' all have
          // the sixth bit set, while 'A'..'F' can be converted
          // to lower case via setting the sixth bit.
          // Thus, or'ing with (1<<6) converts all upper case input
          // to lower case.
          template <typename T>
            struct Base<T, Bit_Parallel> {
              T operator()(T bb) const
              {
                uint8_t a = uint8_t('a');
                uint8_t lower_case_bit = 0b00'10'0000;
                T b = bb | dist_8<T>(lower_case_bit);
                T x = b - dist_8<T>('0');

                T is_geq_10 = b & dist_8<T>(0b01'00'0000u);
                T geq_10_mask =  is_geq_10 - (is_geq_10 >> 6);

                T geq_10_off = dist_8<T>(a - uint8_t('0') - 10u);

                T result = x - (geq_10_mask & geq_10_off);
                return result;
              }
            };

        }

        namespace Gather {

          struct Shift{};
          struct Shift_Big{};

          template <typename T, typename O, typename Tag> struct Base {};

          template <typename T, typename O>
            struct Base<T, O, Shift> {
              O operator()(T bb, O o) const
              {
                static_assert(sizeof(T) > 1, "need at least 2 bytes");

                  T b = boost::endian::big_to_native(bb);

                  T x = b;
                  T y = b >> 4;
                  T z = x | y;

                  for (uint8_t i = sizeof(T)*8-16; i > 0; i-=16) {
                    *o++ = ( z >> i) & 0xffu;
                  }
                  *o++ = z  & 0xffu;

                  return o;
              }
            };

          template <typename T, typename O>
            struct Base<T, O, Shift_Big> {
              O operator()(T b, O o) const
              {
                static_assert(sizeof(T) > 1, "need at least 2 bytes");

                  T x = b;
                  T y = b >> 4;
                  T z = x | y;

                  for (uint8_t i = sizeof(T)*8-16; i > 0; i-=16) {
                    *o++ = ( z >> i) & 0xffu;
                  }
                  *o++ = z  & 0xffu;

                  return o;
              }
            };

        }


        template <typename O, typename T,
                 typename Scatter_Policy = Scatter::Direct,
                 typename Convert_Policy = Convert::Bit_Parallel,
                 typename Gather_Policy  = Gather::Shift,
                 typename S = Scatter::Base<T, Scatter_Policy>,
                 typename C = Convert::Base<T, Convert_Policy>,
                 typename G = Gather::Base<T, O, Gather_Policy>
                   > struct Basic_Encode {

          O operator()(const char *begin, const char *end, O o) const
          {
            for (const char *i = begin; i < end; i += sizeof(T)) {
              T a = S()(reinterpret_cast<const uint8_t*>(i));
              T b = C()(a);
              o   = G()(b, o);
            }
            return o;
          }
        };

        template <typename O, typename T = uint64_t,
                 typename Scatter_Policy = Scatter::Memcpy,
                 typename Convert_Policy = Convert::Bit_Parallel,
                 typename Gather_Policy  = Gather::Shift
                 > struct Encode {


          O operator()(const char *begin, const char *end, O o) const
          {
            O r = o;

            // No alignment done because an odd numbers of bytes
            // before the aligned start would introduce an
            // shift of 4 bits for all coming increments ...
            const char *aligned_begin = begin; //A()(begin);

            uint8_t increment = sizeof(T);
            const char *first_end = aligned_begin +
              (end - aligned_begin) / increment * increment;

            r = Basic_Encode<O, T,
              Scatter_Policy, Convert_Policy, Gather_Policy>()(
                  aligned_begin, first_end, r);

            const char *second_end = first_end + (end-first_end)/2u*2u;
            r = Basic_Encode<O, uint16_t, Scatter_Policy>()(first_end, second_end, r);
            // in case the number of chars was odd a filler byte
            // (i.e. 0xf) must be added
            if (second_end < end) {
              *r = (Convert::Base<uint8_t, Convert::Bit_Parallel>()(*second_end) << 4) | 0x0fu;
              ++r;
            }

            return r;
          }

        };

      } // encode
    } // impl
  } // bcd
} // xfsx

#endif
