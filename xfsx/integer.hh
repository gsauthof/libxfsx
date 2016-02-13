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
#ifndef XFSX_INTEGER_HH
#define XFSX_INTEGER_HH

#include <stdint.h>
#include <utility>

namespace xfsx {

  namespace integer {

    uint32_t range_to_uint32(const std::pair<const char*, const char*> &p);
    uint64_t range_to_uint64(const std::pair<const char*, const char*> &p);
    int64_t range_to_int64(const std::pair<const char*, const char*> &p);

    // Interpret integer as 32 bit unsigned and convert it to
    // 64 bit signed.
    void uint_to_int(int64_t &i);


    // assertions for below clz_*, clsb_* macros
    inline void assert_counting()
    {
      static_assert(sizeof(unsigned int)==4, "couldn't find 32 bit clz variant");
      static_assert(sizeof(unsigned long) == 8 || sizeof(unsigned long long) == 8, "couldn't find 64 bit clz variant");
      static_assert(sizeof(int) == 4, "couldn't find 32 bit clrsb variant");
      static_assert(sizeof(long) == 8 || sizeof(long long) == 8, "couldn't find 64 bit clrsb variant");
    }

  }

}


// count leading signed bits
#define clz_uint32(A) __builtin_clz(A)
#define clz_uint64(A) (sizeof(unsigned long) == 8 ? __builtin_clzl(A) : __builtin_clzll(A))

// count leading redundant signed bits
// clang (<= 3.8) doesn't provide clrsb builtins
#if defined(__clang__)

  #define clrsb_int32(A) ( (!int32_t(A)||int32_t(A)==-1) ? 31 : ((A)>0 ? clz_uint32(A)-1 : clz_uint32(~(A))-1  ) )
  #define clrsb_int64(A) ( (!int64_t(A)||int64_t(A)==-1) ? 63 : ((A)>0 ? clz_uint64(A)-1 : clz_uint64(~(A))-1  ) )

#else

  #define clrsb_int32(A) __builtin_clrsb(A)
  #define clrsb_int64(A) (sizeof(long) == 8 ? __builtin_clrsbl(A) : __builtin_clrsbll(A))

#endif



#endif
