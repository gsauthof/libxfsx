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

#include "octet.hh"
#include "tlc_reader.hh"
#include "tlc_writer.hh"

namespace xfsx {

    struct TLC;

  namespace ber {

      void write_identity(Simple_Reader<TLC> &r, Simple_Writer<TLC> &w);

    void write_identity(const u8 *ibegin, const u8 *iend,
        u8 *begin, u8 *end);


    u8 *write_indefinite(const u8 *ibegin, const u8 *iend,
        u8 *begin, u8 *end);
    void write_indefinite(const u8 *ibegin, const u8 *iend,
        const std::string &filename);


    u8 *write_definite(const u8 *ibegin, const u8 *iend,
        u8 *begin, u8 *end);
    void write_definite(const u8 *ibegin, const u8 *iend,
        const std::string &filename);

  }

}

#endif
