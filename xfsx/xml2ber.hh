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


#include <memory>
#include <utility>

#include "octet.hh"

namespace xfsx {
    namespace scratchpad {
        template <typename Char> class Simple_Reader;
        template <typename Char> class Simple_Writer;
    }
    class BER_Writer_Arguments;
    class TLV;

    namespace xml {


        void write_ber(scratchpad::Simple_Reader<char> &in,
                scratchpad::Simple_Writer<u8> &out,
                const BER_Writer_Arguments &args
                );

        // also used by lxml2ber.cc
        //
        struct Attributes {
                uint8_t l_size         {0    };
                bool    uint2int       {false};
                bool    tag_present    {false};
                bool    full_tag       {false};
        };
        bool read_tag(const std::pair<const char*, const char*> &name,
                TLV &tlv, const BER_Writer_Arguments &args);
        void read_attribute(
                const std::pair<const char*, const char*> &name,
                const std::pair<const char*, const char*> &value,
                TLV &tlv, Attributes &a);
        void add_content(const std::pair<const char*, const char*> &value,
                TLV &tlv, const Attributes &as, const BER_Writer_Arguments &args);

    }

}


#endif
