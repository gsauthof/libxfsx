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

    // C either u8, unsigned char or char ...
    template <typename T, typename C> T movbe(const C *i)
    {
        static_assert(sizeof(C) == 1,
                "makes only sense with char/unsigned char");
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

    // broadcast (distribute) a byte to all byte positions
    // related tricks:
    //   - T(-1)/0x11 creates pattern 0x0f0f..0f
    //     or just use bcast<T>(0xf) as it's constexpr
    //   - T(b) * (T(-1)/T(65535) broadcast to each even byte
    template<typename T> constexpr T bcast(uint8_t b)
    {
        static_assert(!std::numeric_limits<T>::is_signed, "must be unsigned");
        // std::numeric_limits<T>::max() == T(-1) if T is unsigned
        // std::numeric_limits<T>::max()/T(255)
        // creates patern: 0x...0101
        return T(b) * (std::numeric_limits<T>::max() / T(255));
    }


    } // impl
  } // bcd
} // xfsx

#endif
