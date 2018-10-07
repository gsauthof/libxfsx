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
#ifndef XFSX_BCD_IMPL_HH
#define XFSX_BCD_IMPL_HH

#include <stdint.h>
#include <limits>

#include <stdlib.h>
#include <boost/endian/conversion.hpp>

#include <xfsx/octet.hh>

namespace xfsx { namespace bcd { namespace impl {


    template <typename T> T movbe(const u8 *i)
    {
        // GCC <= 8.2 on x86-64 unrolls this loop but doesn't optimize
        // it into a movbe - in contrast to when one manually
        // unrolls the loop ...
        // T r = i[0];
        // for (uint8_t k = 1; k < sizeof(T); ++k) {
        //     r <<= 8;
        //     r |= i[k];
        // }
        // In contrast to that, GCC >= 7.1 optimizes this into a movbe:
        T r;
        memcpy(&r, i, sizeof(T));
        boost::endian::big_to_native_inplace(r);
        return r;
    }

    // broadcast a byte to all byte positions
    // related trick: T(-1)/0x11 creates pattern 0x0f0f..0f
    // or just use bcast<T>(0xf) as it's constexpr
    template<typename T> constexpr T bcast(uint8_t b)
    {
        static_assert(!std::numeric_limits<T>::is_signed, "must be unsigned");
        // std::numeric_limits<T>::max() == T(-1) if T is unsigned
        // std::numeric_limits<T>::max()/T(255)
        // creates patern: 0x...0101
        return T(b) * (std::numeric_limits<T>::max() / T(255));
    }



      // distribute byte into all bytes of the destination
      //
      // i.e. the byte is 'broadcasted'
      //
      // non-constexpr version
      template<typename T> inline T f_dist_8(uint8_t b)
      {
        static_assert(std::numeric_limits<T>::is_signed == false,
            "should be unsigned");
        // std::numeric_limits<T>::max()/T(255u)
        // creates the patern 0x...0101
        T r = T(b) * (std::numeric_limits<T>::max()/T(255u));
        return r;
      }
      // distribute byte into all bytes of the destination
      //
      // i.e. the byte is 'broadcasted'
      //
      template<typename T> constexpr T dist_8(uint8_t b)
      {
        static_assert(std::numeric_limits<T>::is_signed == false,
            "should be unsigned");
        // std::numeric_limits<T>::max()/T(255u)
        // creates the patern 0x...0101
        return T(b) * (std::numeric_limits<T>::max()/T(255u));
      }

      template<typename U, typename T> inline T *next_aligned_address(T *addr)
      {
        size_t x = reinterpret_cast<size_t>(addr);
        // -sizeof(U) creates pattern:
        //
        //   | sizeof(U) * 8 bits
        //   +-----------+
        // 0b...111100...0
        //          +----+
        //          | log2(sizeof(U)) bits
        //
        // Address is aligned when log2(sizeof(U)) least significant
        // bits are zero.
        //          
        size_t y = (x + sizeof(U) - 1u) & size_t((-ssize_t(sizeof(U))));
        T *r = reinterpret_cast<T*>(y);
        return r;
      }

        namespace Align {
          struct Yes {};
          struct No {};

          template <typename T, typename Tag> struct Base {};

          template <typename T> struct Base<T, Yes> {
            enum { alignment = sizeof(T) };
            const u8 *operator()(const u8 *begin)
            {
              const u8 *r = next_aligned_address<T>(begin);
              return r;
            }
          };
          template <typename T> struct Base<T, No> {
            enum { alignment = 1};
            const u8 *operator()(const u8 *begin)
            {
              return begin;
            }
          };
        }



    } // impl
  } // bcd
} // xfsx

#endif
