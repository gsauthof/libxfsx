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
#include "character.hh"

#include <algorithm>
#include <stdexcept>

using namespace std;

namespace xfsx {

  namespace character {

      void verify_filename_part(const char *begin, const char *end)
      {
        auto b = reinterpret_cast<const unsigned char*>(begin);
        auto e = reinterpret_cast<const unsigned char*>(end);
        bool r = std::none_of(b, e, [](auto c) {
            return (c < 32 || c > 126 || c == '/'); });
        if (!r)
          throw range_error(
                "Filename must not contain slashes nor control characters");
      }
      void verify_filename_part(const std::string &s)
      {
        verify_filename_part(s.data(), s.data() + s.size());
      }

  }

}
