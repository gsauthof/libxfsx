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
#ifndef XFSX_LXML2BER_HH
#define XFSX_LXML2BER_HH

#include <string>

#include "ber_writer_arguments.hh"
#include <xxxml/xxxml.hh>

namespace xfsx {
  namespace xml {
    namespace l2 {

    void write_ber(const xxxml::doc::Ptr &doc,
        const std::string &filename,
        const BER_Writer_Arguments &args = default_ber_writer_arguments);

    }
  }
}

#endif
