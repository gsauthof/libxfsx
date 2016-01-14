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
#ifndef XFSX_BCD_IMPL_DECODE_HH
#define XFSX_BCD_IMPL_DECODE_HH

#include <stdint.h>
#include <limits>

#include <stdlib.h>
#include <boost/endian/conversion.hpp>

#include <xfsx/bcd/util.hh>

namespace xfsx {

  namespace bcd {

    namespace impl {

      namespace decode {

        template <char A = 'a'>
          struct Half_To_Char_Branch {
            char operator()(uint8_t b) const
            {
              if (b < 10u) {
                uint8_t ascii_off = uint8_t('0');
                return char(ascii_off + b);
              } else {
                uint8_t ascii_off = uint8_t(A);
                return char(ascii_off + (b-10u));
              }
            }
          };

        template <char A = 'a'>
          struct Half_To_Char_Cmp {
            char operator()(uint8_t b) const
            {
              uint8_t m = b > 9;
              uint8_t geq_10_off = uint8_t(A) - uint8_t('0') - 10u;
              uint8_t r = uint8_t('0') + b + m * geq_10_off;
              return char(r);
            }
          };

        // should be slower because of the division
        template <char A = 'a'>
          struct Half_To_Char_Div {
            char operator()(uint8_t b) const
            {
              uint8_t m = b / 10u;
              uint8_t geq_10_off = uint8_t(A) - uint8_t('0') - 10u;
              uint8_t r = uint8_t('0') + b + m * geq_10_off;
              return char(r);
            }
          };

        // cf.
        // https://en.wikipedia.org/wiki/SWAR
        // http://wm.ite.pl/articles/convert-to-hex.html
        template <char A = 'a'>
          struct Half_To_Char_SAR {
            char operator()(uint8_t b) const
            {
              uint8_t discriminator = 128u - 10u + b;
              uint8_t is_geq_10 = 0b1'000'0000u & discriminator;
              uint8_t geq_10_mask = is_geq_10 - (is_geq_10 >> 7);
              uint8_t geq_10_off = uint8_t(A) - uint8_t('0') - 10u;

              uint8_t result = uint8_t('0') + b + (geq_10_mask & geq_10_off);
              return char(result);
            }
          };

        template <typename O, typename F = Half_To_Char_SAR<> >
          struct Two_Half_Decode {
            void operator()(const uint8_t *begin, const uint8_t *end,
                O &o)
            {
              F f;
              for (const uint8_t *i = begin; i < end; ++i) {
                uint8_t b = *i;
                uint8_t l = b >> 4;
                uint8_t r = b & 0b00'00'1111u;
                *o++ = f(l);
                *o++ = f(r);
              }
            }
          };



        namespace Convert {


          struct Bit_Parallel;

          template <typename T, typename Tag, char A = 'a'>
            struct Base {
            };

          // more on the general idea of bitwise parallel algorithms:
          //
          // https://en.wikipedia.org/wiki/Bitap_algorithm
          // https://en.wikipedia.org/wiki/SWAR
          //
          // the discriminator trick comes from:
          //
          // http://wm.ite.pl/articles/convert-to-hex.html
          template <typename T, char A>
            struct Base<T, Bit_Parallel, A> {
              T operator()(T b) const
              {
                T discriminator = dist_8<T>(128u - 10u) + b;
                T is_geq_10 = dist_8<T>(0b1'000'0000u) & discriminator;
                T geq_10_mask = is_geq_10 - (is_geq_10 >> 7);
                T geq_10_off = dist_8<T>(uint8_t(A) - uint8_t('0') - 10u);

                T result = dist_8<T>(uint8_t('0')) + b + (geq_10_mask & geq_10_off);
                return result;
              }
            };


        }

        namespace Gather {

          struct Slow;
          struct Memcpy;
          struct Direct;
          struct Reverse;

          template <typename O, typename T, typename Tag>
            struct Base {
            };

          template <typename O, typename T>
            struct Base<O, T, Slow> {
              O operator()(T r, O o) const
              {
                for (uint8_t k = sizeof(T)*8-8; k > 0; k -= 8)
                  *o++ = (r >> k) & 0xffu;
                *o++ = r        & 0xffu;
                return o;
              }
            };

          template <typename O, typename T>
            struct Base<O, T, Memcpy> {
              O operator()(T r, O o) const
              {
                boost::endian::big_to_native_inplace(r);
                memcpy(o, reinterpret_cast<const char*>(&r), sizeof(T));
                o += sizeof(T);
                return o;
              }
            };

          template <typename O, typename T>
            struct Base<O, T, Direct> {

              O operator()(T r, O o) const
              {
                *reinterpret_cast<T*>(o) = boost::endian::big_to_native(r);
                o += sizeof(T);
                return o;
              }
            };

          template <typename O, typename T>
            struct Base<O, T, Reverse> {

              O operator()(T r, O o) const
              {
                *reinterpret_cast<T*>(o) = (r);
                o += sizeof(T);
                return o;
              }
            };


        }

        namespace Scatter {

          struct Shift;
          struct Reverse;

          template <typename T, typename Tag> struct Base {
          };

          template <typename T> struct Base<T, Shift> {
            // read the characters as big endian
            T operator()(const uint8_t *i) const
            {
              uint8_t increment = sizeof(T)/2u;
              T b = *i;
              for (uint8_t k = 1; k < increment; ++k) {
                b <<= 16;
                b |= *(i+k);
              }
              // std::numeric_limits<T>::max()/0x11u
              // creates pattern: 0x..0f0f0f
              T x = (b << 4) & std::numeric_limits<T>::max()/0x11u;
              T y =  b       & std::numeric_limits<T>::max()/0x11u;
              x |= y;
              return x;
            }
          };

          template <typename T> struct Base<T, Reverse> {
            // read the characters as little endian
            // such that endian conversion at the end is not necessary
            T operator()(const uint8_t *i) const
            {
              uint8_t increment = sizeof(T)/2u;
              T b = 0;
              for (uint8_t k = increment - 1; k > 0; --k) {
                b |= *(i+k);
                b <<= 16;
              }
              b |= *i;

              // std::numeric_limits<T>::max()/0x11u
              // creates pattern: 0x..0f0f0f
              T x = (b >> 4) & std::numeric_limits<T>::max()/0x11u;
              T y = (b << 8) & std::numeric_limits<T>::max()/0x11u;
              x |= y;
              return x;
            }
          };


        }

        template <typename O,
                 typename T,
                 typename Scatter_Policy = Scatter::Shift,
                 typename Convert_Policy = Convert::Bit_Parallel,
                 typename Gather_Policy = Gather::Memcpy,
                 typename S = Scatter::Base<T, Scatter_Policy>,
                 typename C = Convert::Base<T, Convert_Policy>,
                 typename G = Gather::Base<O, T, Gather_Policy>
                   >
                   struct Basic_Decode {
                     O operator()(const uint8_t *begin, const uint8_t *end,
                         O o) const
                     {
                       O r = o;
                       uint8_t increment = sizeof(T)/2u;
                       for (const uint8_t *i = begin; i < end; i += increment) {
                         T a = S()(i);
                         // XXX use references on C and G, i.e. for a and b?
                         T b = C()(a);
                         r   = G()(b, r);
                       }
                       return r;
                     }
                   };


        template <typename O,
                 typename T = uint64_t,
                 typename Scatter_Policy = Scatter::Shift,
                 typename Convert_Policy = Convert::Bit_Parallel,
                 typename Gather_Policy = Gather::Memcpy,
                 typename Align_Policy = Align::No,
                 typename A = Align::Base<T, Align_Policy>
                   >
           struct Decode {

             O operator()(const uint8_t *begin, const uint8_t *end,
                 O o) const
             {
               O r = o;
               const uint8_t *aligned_begin = A()(begin);
               if (A::alignment > 1) {
                 r = Basic_Decode<O, uint16_t>()(begin, aligned_begin, r);
               }

               uint8_t increment = sizeof(T)/2u;
               const uint8_t *first_end = begin + (end-begin) / increment * increment;
               r = Basic_Decode<O, T, Scatter_Policy, Convert_Policy,
                 Gather_Policy>()(aligned_begin, first_end, r);

               if (increment > 1) {
                 r = Basic_Decode<O, uint16_t>()(first_end, end, r);
               }
               return r;
             }
           };


      } // decode
    } // impl
  } // bcd
} // xfsx

#endif
