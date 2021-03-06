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

#ifndef XFSX_XML2LXML_HH
#define XFSX_XML2LXML_HH

#include <xxxml/xxxml.hh>

namespace xfsx {
    namespace scratchpad {
        template <typename Char> class Simple_Reader;
    }

  namespace xml {

    namespace l2 {
 
      xxxml::doc::Ptr generate_tree(scratchpad::Simple_Reader<char> &in,
          size_t count = 0);
      xxxml::doc::Ptr generate_tree(const char *begin, const char *end,
          size_t count = 0);

    }

  }

}

#endif
