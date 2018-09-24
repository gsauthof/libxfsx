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

#include "string.hh"
#include "bcd.hh"
#include "hex.hh"
#include "xfsx.hh"

#include <stdexcept>
#include <string.h>

namespace xfsx {

    template <typename Tag> Tagged_String<Tag>::Tagged_String() =default;
    template <typename Tag>
        Tagged_String<Tag>::Tagged_String(const Raw_Vector<char> &s)
        :
            s_(s)
    {
    }
    template <typename Tag>
        Tagged_String<Tag>::Tagged_String(const char *s)
        :
            s_(s, s + strlen(s))
    {
    }
    template <typename Tag>
        Tagged_String<Tag>::Tagged_String(Raw_Vector<char> &&s)
        :
            s_(std::move(s))
    {
    }
    template <typename Tag>
        Tagged_String<Tag> &Tagged_String<Tag>::operator=(const Raw_Vector<char> &s)
        {
            s_ = s;
            return *this;
        }
    template <typename Tag>
        Tagged_String<Tag> &Tagged_String<Tag>::operator=(Raw_Vector<char> &&s)
        {
            s_ = std::move(s);
            return *this;
        }
    template <typename Tag>
        Tagged_String<Tag>::operator const Raw_Vector<char>&() const
        {
            return s_;
        }
    template <typename Tag>
        void Tagged_String<Tag>::clear()
        {
            s_.clear();
        }
    template <typename Tag>
        size_t Tagged_String<Tag>::size() const
        {
            return s_.size();
        }
    template <typename Tag>
        bool Tagged_String<Tag>::empty() const
        {
            return s_.empty();
        }
    template <typename Tag>
        Raw_Vector<char> &Tagged_String<Tag>::get()
        {
            return s_;
        }
    template <typename Tag>
        const Raw_Vector<char> &Tagged_String<Tag>::get() const
        {
            return s_;
        }

    template class Tagged_String<BCD>;
    template class Tagged_String<HEX>;
    template class Tagged_String<HEX_XML>;



    template<> size_t minimally_encoded_length(
            const BCD_String &v)
    {
        return (v.size() + 2 - 1) / 2;
    }
    template <typename Style_Tag, typename H> size_t minimally_encoded_length_hex(
            const H &v)
    {
        return hex::encoded_size<Style_Tag>(
                v.get().data(), v.get().data() + v.get().size());
    }
    template<> size_t minimally_encoded_length(
            const Hex_String &v)
    {
        return minimally_encoded_length_hex<hex::Style::C>(v);
    }
    template<> size_t minimally_encoded_length(
            const Hex_XML_String &v)
    {
        return minimally_encoded_length_hex<hex::Style::XML>(v);
    }

    template<> void decode(const u8 *begin, size_t size,
            BCD_String &r)
    {
        if (!size) {
            r.clear();
            return;
        }
        Raw_Vector<char> s(r.get());
        s.resize(size*2);
        const u8 *end = begin + size;
        char *x = &s[0];
        bcd::decode(begin, end, x);
        if (s.back() == 'f')
            s.resize(s.size()-1);
        r = std::move(s);
    }
    template <typename Style_Tag, typename H>
        void decode_hex(const u8 *begin, size_t size,
                H &r)
        {
            if (!size) {
                r.clear();
                return;
            }
            Raw_Vector<char> s(std::move(r.get()));
            const u8 *end = begin + size;
            s.resize(hex::decoded_size<Style_Tag>(begin, end));
            char *x = &s[0];
            hex::decode<Style_Tag>(begin, end, x);
            r = std::move(s);
        }
    template<> void decode(const u8 *begin, size_t size,
            Hex_String &r)
    {
        decode_hex<hex::Style::C>(begin, size, r);
    }
    template<> void decode(const u8 *begin, size_t size,
            Hex_XML_String &r)
    {
        decode_hex<hex::Style::XML>(begin, size, r);
    }

    template<> u8 *encode(const BCD_String &t, u8 *begin, size_t size)
    {
        if ((t.size() + 2 - 1) / 2 != size)
            throw std::overflow_error("BCD_String too small or too big");
        return bcd::encode(t.get().data(), t.get().data() + t.get().size(), begin);
    }

    template <typename Style_Tag, typename H>
        u8 *encode_hex(const H &t, u8 *o_begin, size_t size)
        {
            const char *begin = t.get().data();
            const char *end   = t.get().data() + t.get().size();
            if (hex::encoded_size<Style_Tag>(begin, end) != size)
                throw std::overflow_error("Hex_String too small or too big");
            return hex::encode<Style_Tag>(begin, end, o_begin);
        }
    template<> u8 *encode(const Hex_String &t, u8 *o_begin, size_t size)
    {
        return encode_hex<hex::Style::C>(t, o_begin, size);
    }
    template<> u8 *encode(const Hex_XML_String &t, u8 *o_begin,
            size_t size)
    {
        return encode_hex<hex::Style::XML>(t, o_begin, size);
    }
}
