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
#include "byte.hh"

#include "integer.hh"

#include <type_traits>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <string.h>

#include <ixxx/ixxx.hh>

#if XFSX_HAVE_FROM_CHARS
    #include <charconv>
#else
    #include <fmt/format.h>
#endif

using namespace std;

namespace xfsx {

  namespace byte {

    namespace writer {

      template <> size_t encoded_length(const char *s)
      {
        return strlen(s);
      }

      template <typename T>
      size_t encoded_length_int(T value)
      {
          if (value < 0) {
              // make_unsigned_t<T> v = -value; // C++ 14
              typename make_unsigned<T>::type v = -value; // C++ 14
              return 1 + integer::dec_digits(v);
          } else {
              // make_unsigned_t<T> v = value; // C++ 14
              typename make_unsigned<T>::type v = value;
              return integer::dec_digits(v);
          }
      }
      template <> size_t encoded_length(uint32_t v)
      {
        return encoded_length_int(v);
      }
      template <> size_t encoded_length(uint64_t v)
      {
        return encoded_length_int(v);
      }
      template <> size_t encoded_length(int32_t v)
      {
        return encoded_length_int(v);
      }
      template <> size_t encoded_length(int64_t v)
      {
        return encoded_length_int(v);
      }
  // work around size_t being different to uint32_t and uint64_t
  // on Mac OS X,
  // cf. http://stackoverflow.com/questions/11603818/why-is-there-ambiguity-between-uint32-t-and-uint64-t-when-using-size-t-on-mac-os
#if (defined(__APPLE__) && defined(__MACH__))
      template <> size_t encoded_length(size_t v)
      {
        return encoded_length_int(v);
      }
      template <> size_t encoded_length(long v)
      {
        return encoded_length_int(v);
      }
#endif
      template <> size_t encoded_length(char)
      {
        return 1;
      }
      template <> size_t encoded_length(unsigned char v)
      {
        return encoded_length_int(uint8_t(v));
      }
      template <> size_t encoded_length(const std::string &s)
      {
        return s.size();
      }
      template <> size_t encoded_length(const Raw_Vector<char> &v)
      {
        return v.size();
      }
      template <> size_t encoded_length(
          const std::pair<const char*, size_t> &v)
      {
        return v.second;
      }
      template <> size_t encoded_length(
          const std::pair<const char*, int> &v)
      {
        return v.second;
      }
      template <> size_t encoded_length(
          const std::pair<const char*, const char*> &v)
      {
        return v.second - v.first;
      }

      template <> char *encode(const char *s, char *o, size_t n)
      //char *encode(const char *s, char *o, size_t n)
      {
        return copy(s, s+n, o);
      }

      template <typename T>
      char *encode_int(T value, char *buffer, size_t n)
      {
#if XFSX_HAVE_FROM_CHARS
          auto r = std::to_chars(buffer, buffer+n, value);
          return r.ptr;
#else
          // copied and slightly adapted this encoding part from
          // fmt/fmt/format/format.h:3425, format_decimal()
          // https://github.com/fmtlib/fmt
          // BSD 2-clause "Simplified" License
        typedef typename fmt::internal::IntTraits<T>::MainType MainType;
        MainType abs_value = static_cast<MainType>(value);
        if (fmt::internal::is_negative(value)) {
          *buffer++ = '-';
          --n;
          abs_value = 0 - abs_value;
        }
        if (abs_value < 100) {
          if (abs_value < 10) {
            *buffer++ = static_cast<char>('0' + abs_value);
            return buffer;
          }
          unsigned index = static_cast<unsigned>(abs_value * 2);
          *buffer++ = fmt::internal::Data::DIGITS[index];
          *buffer++ = fmt::internal::Data::DIGITS[index + 1];
          return buffer;
        }
        unsigned num_digits = n;
        fmt::internal::format_decimal(buffer, abs_value, num_digits);
        buffer += num_digits;
        return buffer;
#endif
      }
      template <> char *encode(uint32_t v, char *o, size_t n)
      {
        return encode_int(v, o, n);
      }
      template <> char *encode(uint64_t v, char *o, size_t n)
      {
        return encode_int(v, o, n);
      }
      template <> char *encode(int32_t v, char *o, size_t n)
      {
        return encode_int(v, o, n);
      }
      template <> char *encode(int64_t v, char *o, size_t n)
      {
        return encode_int(v, o, n);
      }
  // work around size_t being different to uint32_t and uint64_t
  // on Mac OS X,
  // cf. http://stackoverflow.com/questions/11603818/why-is-there-ambiguity-between-uint32-t-and-uint64-t-when-using-size-t-on-mac-os
#if (defined(__APPLE__) && defined(__MACH__))
      template <> char *encode(size_t v, char *o, size_t n)
      {
        return encode_int(v, o, n);
      }
      template <> char *encode(long v, char *o, size_t n)
      {
        return encode_int(v, o, n);
      }
#endif
      template <> char *encode(char v, char *o, size_t /*n*/)
      {
        *o++ = v;
        return o;
      }
      template <> char *encode(unsigned char v, char *o, size_t n)
      {
        return encode_int(uint8_t(v), o, n);
      }
      template <> char *encode(const std::string &s, char *o, size_t /*n*/)
      {
        return copy(s.begin(), s.end(), o);
      }
      template <> char *encode(const Raw_Vector<char> &v, char *o, size_t /*n*/)
      {
        return copy(v.begin(), v.end(), o);
      }
      template <> char *encode(const std::pair<const char*, size_t> &v,
          char *o, size_t n)
      {
        return copy(v.first, v.first + n, o);
      }
      template <> char *encode(const std::pair<const char*, int> &v,
          char *o, size_t n)
      {
        return copy(v.first, v.first + n, o);
      }
      template <> char *encode(const std::pair<const char*, const char*> &v,
          char *o, size_t n)
      {
        return copy(v.first, v.first + n, o);
      }


      Base &operator<<(Base &b, const Indent &i)
      {
          char  *o = b.w.begin_write(i.i);
          fill(o, o + i.i, ' ');
          b.w.commit_write(i.i);
          return b;
      }
      void Base::fill(size_t n)
      {
          char  *o = w.begin_write(n);
          std::fill(o, o + n, ' ');
          w.commit_write(n);
      }


    }


  }

}


