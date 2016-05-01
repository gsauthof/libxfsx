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
#include "search.hh"


#include "traverser/matcher.hh"
#include "traverser/tlc.hh"

namespace xfsx {

  // matches a tag path
  // tag == 0 -> wildcard (think: '*')
  // everywhere -> can match everywhere, doesn't have to start at the root
  //               (think: no '/' in the front a path expression)
  // Example:
  // /foo/bar/baz -> path = {{ to_tag(foo), to_tag(bar), to_tag(baz) }},
  //                 everywhere = false
  // foo/bar/baz  -> path = {{ to_tag(foo), to_tag(bar), to_tag(baz) }},
  //                 everywhere = true

  const uint8_t *search(const uint8_t *begin, const uint8_t *end,
      const std::vector<Tag_Int> &path, bool everywhere, Klasse klasse)
  {
    using namespace xfsx::traverser;
    Vertical_TLC t;
    Vertical_TLC_Proxy p(begin, end, t);

    Basic_Matcher<Vertical_TLC_Proxy, Vertical_TLC> f(path, everywhere, klasse);
    while (!p.eot(t)) {
      auto r = f(p, t);
      if (f.result_ == Matcher_Result::INIT)
        return t.begin;
      if (r == xfsx::traverser::Hint::SKIP_CHILDREN)
        p.skip_children(t);
      else
        p.advance(t);
    }
    return end;
  }

} // xfsx
