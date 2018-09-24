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
#include "hex_impl.hh" // also includes hex.hh

#include <algorithm>
#include <stdlib.h>
#include "octet.hh"

using namespace std;

namespace xfsx {

  namespace hex {


    template <typename Style_Tag>
    size_t decoded_size(const u8 *begin, const u8 *end)
    {
      return impl::decoded_size<Style_Tag>(begin, end);
    }
    template size_t decoded_size<Style::XML>(
        const u8 *begin, const u8 *end);
    template size_t decoded_size<Style::C>(
        const u8 *begin, const u8 *end);
    template size_t decoded_size<Style::Raw>(
        const u8 *begin, const u8 *end);

    template <typename Style_Tag>
    char *decode(const u8 *begin, const u8 *end, char *o)
    {
      return impl::decode<Style_Tag>(begin, end, o);
    }
    template char *decode<Style::XML>(
        const u8 *begin, const u8 *end, char *o);
    template char *decode<Style::C>(
        const u8 *begin, const u8 *end, char *o);
    template char *decode<Style::Raw>(
        const u8 *begin, const u8 *end, char *o);

    template <typename Style_Tag>
      u8 *encode(const char *begin, const char *end, u8 *o)
    {
      return impl::encode<Style_Tag>(begin, end, o);
    }
    template u8 *encode<Style::XML>(
        const char *begin, const char *end, u8 *o);
    template u8 *encode<Style::C>(
        const char *begin, const char *end, u8 *o);
    template u8 *encode<Style::Raw>(
        const char *begin, const char *end, u8 *o);

    template <typename Style_Tag>
      size_t encoded_size(const char *begin, const char *end)
      {
        return impl::encoded_size<Style_Tag>(begin, end);
      }
    template size_t encoded_size<Style::XML>(
        const char *begin, const char *end);
    template size_t encoded_size<Style::C>(const char *begin, const char *end);
    template size_t encoded_size<Style::Raw>(
        const char *begin, const char *end);

  }

}

