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
#ifndef XFSX_TAP_HH
#define XFSX_TAP_HH

#include <deque>
#include <string>

namespace grammar {
  class Grammar;
}
namespace xfsx {
  class Tag_Dereferencer;
  class Tag_Typifier;
}

namespace xfsx {

  namespace tap {

    grammar::Grammar read_asn_grammar(
        const std::deque<std::string> &asn_filenames);
    void init_dereferencer(const grammar::Grammar &g,
        xfsx::Tag_Dereferencer &dereferencer);
    void init_typifier(xfsx::Tag_Typifier &typifier);

  }

}

#endif
