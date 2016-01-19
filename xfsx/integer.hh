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

  }

}


#endif
