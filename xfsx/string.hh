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

#ifndef XFSX_STRING_HH
#define XFSX_STRING_HH

#include "raw_vector.hh"
#include "xfsx.hh"

namespace xfsx {

    struct BCD{};
    struct HEX{};
    struct HEX_XML{};

    template <typename Tag>
        class Tagged_String {
            private:
                Raw_Vector<char> s_;
            public:
                Tagged_String();
                Tagged_String(const char *s);
                Tagged_String(const Raw_Vector<char> &s);
                Tagged_String(Raw_Vector<char> &&s);
                Tagged_String &operator=(const Raw_Vector<char> &s);
                Tagged_String &operator=(Raw_Vector<char> &&s);
                operator const Raw_Vector<char>&() const;
                void clear();
                size_t size() const;
                bool empty() const;

                Raw_Vector<char> &get();
                const Raw_Vector<char> &get() const;
        };
    using BCD_String = Tagged_String<BCD>;
    using Hex_String = Tagged_String<HEX>;
    using Hex_XML_String = Tagged_String<HEX_XML>;

    template<> size_t minimally_encoded_length(
            const BCD_String &v);
    template<> size_t minimally_encoded_length(
            const Hex_String &v);
    template<> size_t minimally_encoded_length(
            const Hex_XML_String &v);

    template<> void decode(const u8 *begin, size_t size,
            BCD_String &r);
    template<> void decode(const u8 *begin, size_t size,
            Hex_String &r);
    template<> void decode(const u8 *begin, size_t size,
            Hex_XML_String &r);
    template<> u8 *encode(const BCD_String &t, u8 *begin, size_t size);
    template<> u8 *encode(const Hex_String &t, u8 *begin, size_t size);
    template<> u8 *encode(const Hex_XML_String &t,
            u8 *begin, size_t size);

}

#endif // XFSX_STRING_HH
