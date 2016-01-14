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
#ifndef XFSX_BER2BER_HH
#define XFSX_BER2BER_HH

#include <stdint.h>
#include <string>

namespace xfsx {

  namespace ber {

    void write_identity(const uint8_t *ibegin, const uint8_t *iend,
        uint8_t *begin, uint8_t *end);


    uint8_t *write_indefinite(const uint8_t *ibegin, const uint8_t *iend,
        uint8_t *begin, uint8_t *end);
    void write_indefinite(const uint8_t *ibegin, const uint8_t *iend,
        const std::string &filename);


    uint8_t *write_definite(const uint8_t *ibegin, const uint8_t *iend,
        uint8_t *begin, uint8_t *end);
    void write_definite(const uint8_t *ibegin, const uint8_t *iend,
        const std::string &filename);

  }

}

#endif
