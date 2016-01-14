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
#ifndef XFSX_S_PAIR_HH
#define XFSX_S_PAIR_HH

#include <utility>
#include <string>


namespace xfsx {

  namespace s_pair {

    struct Equal {
      bool operator()(
          const std::pair<const char *, const char*> &a,
          const std::pair<const char *, const char*> &b) const;
    };
    struct Hash {
      size_t operator()(
          const std::pair<const char *, const char*> &a) const;
    };


    template <typename T>
    inline bool empty(const std::pair<T, T> &p)
    {
      return p.first == p.second;
    }

    bool equal(const std::pair<const char *, const char*> &l, const char *r,
        size_t n);
    inline size_t size(const std::pair<const char *, const char*> &p)
    {
      return p.second - p.first;
    }

    std::string mk_string(const std::pair<const char*, const char*> &p);

    std::pair<const char*, const char*> mk_s_pair(const char *s);

  }

}


#endif
