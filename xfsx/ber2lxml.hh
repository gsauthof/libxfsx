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
#ifndef XFSX_BER2LXML_HH
#define XFSX_BER2LXML_HH

#include <xxxml/xxxml.hh>
#include <stdint.h>
#include "octet.hh"

namespace xfsx {

    namespace scratchpad {
        template<typename Char> class Simple_Reader;
    }

  namespace xml {

    struct Pretty_Writer_Arguments;

    namespace l2 {

      xxxml::doc::Ptr generate_tree(
              scratchpad::Simple_Reader<u8> &r,
              const Pretty_Writer_Arguments &args);
      xxxml::doc::Ptr generate_tree(const u8 *begin, const u8 *end,
          const Pretty_Writer_Arguments &args);
    }
  }
}

#endif
