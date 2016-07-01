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
#ifndef XFSX_BER_WRITER_HH
#define XFSX_BER_WRITER_HH

#include <stack>
#include <string>

#include <xfsx/xfsx.hh>
#include <xfsx/ber_node.hh>
#include <xfsx/ber_writer_arguments.hh>

namespace xfsx {


    class BER_Writer_Base {
      private:
      protected:
        const BER_Writer_Arguments &args_;
        ber::TLV_Node root;
        std::stack<ber::TLV_Node*> node_stack;
        TLV tlv;
        bool    tag_present     {false};
        bool    symbol_present_ {false};
        bool    klasse_present  {false};
        uint8_t l_size_         {0    };
        bool    uint2int_       {false};

        void write_end_tag();
        void read_tag(const std::pair<const char*, const char*> &name);
        void read_attribute(const std::pair<const char*, const char*> &name,
            const std::pair<const char*, const char*> &value);
        void write_primitive_tag(
            const std::pair<const char*, const char*> &content);
        void write_constructed_tag(bool is_empty);
        void store(const std::string &filename);
      public:
        BER_Writer_Base(const BER_Writer_Arguments &args);
        size_t size() const;
        void store(uint8_t *begin, uint8_t *end);
        void store(std::vector<uint8_t> &v);
    };

}

#endif
