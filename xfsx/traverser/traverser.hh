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


#ifndef XFSX_TRAVERSER_HH
#define XFSX_TRAVERSER_HH


#include <array>
#include <algorithm>

namespace xfsx {
  namespace traverser {

    enum class Hint {
      DESCEND,
      SKIP_CHILDREN,
      STOP
    };

    struct Simple_Traverse {
    template <typename F, typename T, typename Proxy>
        void operator()(Proxy &p, T &t, F &f)
        {
          while (!p.eot(t)) {
            auto r = f(p, t);
            if (r == Hint::STOP)
              return;
            p.advance(t);
          }
        }
    };

    struct Traverse {
    template <typename ...F, typename T, typename Proxy>
        void operator()(Proxy &p, T &t, F& ... f)
        {
          while (!p.eot(t)) {
            std::array<Hint, sizeof...(F)> r = { f(p, t)... };
            if (std::all_of(r.begin(), r.end(),
                  [](auto x){ return x == Hint::STOP; }))
              return;
            
            if (std::any_of(r.begin(), r.end(),
                  [](auto x){ return x == Hint::DESCEND; }))
              p.advance(t);
            else
              p.skip_children(t);
          }
        }
    };

  }
}

#endif
