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
#ifndef XFSX_HEX_HH
#define XFSX_HEX_HH

#include <stdint.h>
#include <stddef.h>

namespace xfsx {

  namespace hex {

      namespace Style {
        struct XML {};

        // The C style escape sequences are always assumed to be
        // two hex-digit long. This is different from the
        // C++ rules:
        //
        // 'Hexadecimal escape sequences have no length limit and terminate
        // at the first character that is not a valid hexadecimal digit.'
        // http://en.cppreference.com/w/cpp/language/escape
        //
        // Especially, something like "\xDEad" would mean here
        //
        // { 0xde, uint8_t('a'), uint8_t('d') }
        //
        // where a standard conforming C++ compiler interprets it as:
        //
        // { uint8_t(0xdead) }
        //
        struct C {};

        struct Raw {};
      };


    template <typename Style_Tag>
      size_t decoded_size(const uint8_t *begin, const uint8_t *end);
    template <typename Style_Tag>
      char *decode(const uint8_t *begin, const uint8_t *end, char *o);
    template <typename Style_Tag>
      uint8_t *encode(const char *begin, const char *end, uint8_t *o);
    template <typename Style_Tag>
      size_t encoded_size(const char *begin, const char *end);

  }

}


#endif
