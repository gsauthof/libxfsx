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
#ifndef XFSX_XML2BER_HH
#define XFSX_XML2BER_HH

#include "xfsx.hh"
#include "ber_writer_arguments.hh"

#include <string>
#include <unordered_map>

//#include <byte.hh>

namespace xfsx {

  namespace xml {


    //void write_ber(const char *begin, const char *end,
    //    byte::writer::Base &w);

    void write_ber(const char *begin, const char *end,
        const std::string &filename,
        const BER_Writer_Arguments &args = default_ber_writer_arguments);

    void write_ber(const char *begin, const char *end,
        std::vector<uint8_t> &v,
        const BER_Writer_Arguments &args
        );

  }

}


#endif
