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
#ifndef XFSX_SEARCH_HH
#define XFSX_SEARCH_HH

#include <stdint.h>
#include <vector>

#include <xfsx/xfsx.hh>

namespace xfsx {

  const uint8_t *search(const uint8_t *begin, const uint8_t *end,
      const std::vector<Tag_Int> &path, bool everywhere = false,
      Klasse klasse = Klasse::APPLICATION);

} // xfsx


#endif
