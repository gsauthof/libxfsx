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
#ifndef XFSX_BYTE_HH
#define XFSX_BYTE_HH

#include <vector>
#include "raw_vector.hh"
#include <ixxx/util.hh>

#include "scratchpad.hh"

namespace xfsx {

  namespace byte {


    namespace writer {

      template <typename T,
           typename = std::enable_if_t<
             std::is_fundamental<T>::value || std::is_pointer<T>::value
           > >
      size_t encoded_length(T v);
      template <> size_t encoded_length(const char *s);
      template <> size_t encoded_length(uint32_t v);
      template <> size_t encoded_length(uint64_t v);
      template <> size_t encoded_length(int32_t v);
      template <> size_t encoded_length(int64_t v);
#if (defined(__APPLE__) && defined(__MACH__))
      template <> size_t encoded_length(size_t v);
      template <> size_t encoded_length(long v);
      template <> size_t encoded_length(unsigned long v);
#endif
      template <> size_t encoded_length(char v);
      template <> size_t encoded_length(unsigned char v);
      template <typename T,
           typename = std::enable_if_t<
             !(std::is_fundamental<T>::value || std::is_pointer<T>::value)
           > >
      size_t encoded_length(const T &v);
      template <> size_t encoded_length(const std::string &s);
      template <> size_t encoded_length(const Raw_Vector<char> &v);
      template <> size_t encoded_length(
          const std::pair<const char*, size_t> &v);
      template <> size_t encoded_length(
          const std::pair<const char*, int> &v);
      template <> size_t encoded_length(
          const std::pair<const char*, const char*> &v);
      template <typename T,
           typename = std::enable_if_t<
             std::is_fundamental<T>::value || std::is_pointer<T>::value
           > >
      char *encode(T v, char *o, size_t n);
      template <> char *encode(const char *s, char *o, size_t n);
      template <> char *encode(uint32_t v, char *o, size_t n);
      template <> char *encode(uint64_t v, char *o, size_t n);
      template <> char *encode(int32_t v, char *o, size_t n);
      template <> char *encode(int64_t v, char *o, size_t n);
#if (defined(__APPLE__) && defined(__MACH__))
      template <> char *encode(size_t v, char *o, size_t n);
      template <> char *encode(long v, char *o, size_t n);
      template <> char *encode(unsigned long v, char *o, size_t n);
#endif
      template <> char *encode(char v, char *o, size_t n);
      template <> char *encode(unsigned char v, char *o, size_t n);
      template <typename T,
           typename = std::enable_if_t<
             !(std::is_fundamental<T>::value || std::is_pointer<T>::value)
           > >
      char *encode(const T &v, char *o, size_t n);
      template <> char *encode(const std::pair<const char*, size_t> &v,
          char *o, size_t n);
      template <> char *encode(const std::pair<const char*, int> &v,
          char *o, size_t n);
      template <> char *encode(const std::pair<const char*, const char*> &v,
          char *o, size_t n);
      template <> char *encode(const std::string &s, char *o, size_t n);
      template <> char *encode(const Raw_Vector<char> &v, char *o, size_t n);


      struct Base {
          scratchpad::Simple_Writer<char> &w;
          Base(scratchpad::Simple_Writer<char> &w) : w(w) {}
          void fill(size_t n);
      };
      template <typename T>
        inline Base &operator<<(Base &b, T t)
        {
          size_t n = encoded_length(t);
          char  *o = b.w.begin_write(n);
          encode(t, o, n);
          b.w.commit_write(n);
          return b;
        }
      struct Indent {
        unsigned i;
        Indent(unsigned i) : i(i) {}
        Indent operator()(unsigned k) const { return i*k; }
      };
      Base &operator<<(Base &b, const Indent &i);


    } // writer

  } // byte

} // xfsx


#endif
