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
#include "s_pair.hh"

#include <string.h>
#include <boost/functional/hash.hpp>

using namespace std;

namespace xfsx {

  namespace s_pair {

    bool equal(const std::pair<const char *, const char*> &l, const char *r,
        size_t n)
    {
      return std::equal(l.first, l.second, r, r + n);
    }

    string mk_string(const std::pair<const char*, const char*> &p)
    {
      return string(p.first, p.second);
    }

    bool Equal::operator() (
        const std::pair<const char *, const char*> &a,
        const std::pair<const char *, const char*> &b) const
    {
      return std::equal(a.first, a.second, b.first, b.second);
    }
    size_t Hash::operator()(
        const std::pair<const char *, const char*> &a) const
    {
      return boost::hash_range(a.first, a.second);
    }

    std::pair<const char*, const char*> mk_s_pair(const char *s)
    {
      pair<const char*, const char*> r(s, s + strlen(s));
      return r;
    }

  }

}
