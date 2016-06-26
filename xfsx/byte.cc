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

#include <vector>
#include <stdexcept>
#include <algorithm>
#include <string.h>

#include <ixxx/ixxx.h>

#include <cppformat/format.h>

using namespace std;

namespace xfsx {

  namespace byte {

    namespace writer {

      template <> size_t encoded_length(const char *s)
      {
        return strlen(s);
      }
      template <typename T>
      size_t encoded_length_int(T v)
      {
        typename fmt::internal::IntTraits<T>::MainType unsigned_v = v;
        if (fmt::internal::is_negative(v)) {
          unsigned_v = 0 - unsigned_v;
          return 1 + size_t(fmt::internal::count_digits(unsigned_v));
        } else
          return fmt::internal::count_digits(unsigned_v);
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
      template <> size_t encoded_length(char v)
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
      char *encode_int(T v, char *o, size_t n)
      {
        typename fmt::internal::IntTraits<T>::MainType unsigned_v = v;
        if (fmt::internal::is_negative(v)) {
          *o++ = '-';
          --n;
          unsigned_v = 0 - unsigned_v;
        }
        fmt::internal::format_decimal(o, unsigned_v, n);
        return o + n;
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
      template <> char *encode(char v, char *o, size_t n)
      {
        *o++ = v;
        return o;
      }
      template <> char *encode(unsigned char v, char *o, size_t n)
      {
        return encode_int(uint8_t(v), o, n);
      }
      template <> char *encode(const std::string &s, char *o, size_t n)
      {
        return copy(s.begin(), s.end(), o);
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

      Base::Base(Base &&o)
        :
          cur_(o.cur_)
      {
        o.cur_ = nullptr;
      }

      Base::Base()  =default;
      Base::~Base() =default;

      size_t Base::written() const
      {
        return written_;
      }

      char *Base::obtain_chunk(size_t n)
      {
        make_room_for(n);
        auto r = cur_;
        cur_ += n;
        written_ += n;
        return r;
      }

      void Base::write(const char *begin, const char *end)
      {
        auto o = obtain_chunk(end-begin);
        copy(begin, end, o);
      }
      void Base::fill(size_t n, char value)
      {
        auto o = obtain_chunk(n);
        fill_n(o, n, value);
      }

      void Base::flush()
      {
        flush_it();
      }


      Memory::Memory(size_t n, size_t increment)
        :
          v_(n),
          increment_(increment)
      {
        cur_ = v_.data();
      }
      void Memory::make_room_for(size_t n)
      {
        size_t occupied = cur_ - v_.data();
        size_t available = v_.size() - occupied;
        if (n <= available)
          return;
        size_t delta = n - available;
        v_.resize(occupied + ((delta+increment_-1u)/increment_)*increment_  );
        cur_ = v_.data() + occupied;
      }
      void Memory::rewind(size_t n)
      {
        size_t occupied = cur_ - v_.data();
        if (n > occupied)
          throw runtime_error("Cannot rewind over the beginning");
        cur_ -= n;
        written_ -= n;
      }
      void Memory::clear()
      {
        cur_ = v_.data();
        written_ = 0u;
      }
      void Memory::flush_it()
      {
      }
      const char *Memory::begin() const
      {
        return v_.data();
      }
      const char *Memory::end() const
      {
        return cur_;
      }

      File::File(ixxx::util::FD &fd, size_t n)
        :
          Memory(n, n),
          fd_(fd)
      {
      }
      File::~File()
      {
        try {
          // we explicitly call this object's flush_it() because
          // otherwise we could end up call the version from a
          // derived class, which already is destructed, thus
          // would be undefined
          File::flush_it();
        } catch (...) {
        }
      }
      void File::make_room_for(size_t n)
      {
        size_t m = cur_ - v_.data();
        if (m >= increment_) {
          size_t i = 0;
          for (i = 0; i < m / increment_ * increment_; i += increment_) {
            auto r = ixxx::posix::write(fd_, v_.data() + i, increment_);
            if (size_t(r) != increment_)
              throw runtime_error("less bytes written than expected");
          }
          copy(v_.data() + i, cur_, v_.data());
          cur_ -= i;
        }
        Memory::make_room_for(n);
      }
      void File::write(const char *begin, const char *end)
      {
        size_t n = end-begin;
        size_t m = cur_ - v_.data();

        if (increment_ - m % increment_ > n) {
          Base::write(begin, end);
        } else {
          const char *next_begin = begin +
            (m % increment_ ?  increment_ - m % increment_ : 0);
          cur_ = copy(begin, next_begin, cur_);
          for (const char *x = v_.data(); x < cur_; x += increment_) {
            auto r = ixxx::posix::write(fd_, x, increment_);
            if (size_t(r) != increment_)
              throw runtime_error("less bytes written than expected");
          }
          written_ += next_begin - begin;;
          cur_ = v_.data();
          for (const char *x = next_begin; x + increment_ <= end; x += increment_) {
            auto r = ixxx::posix::write(fd_, x, increment_);
            if (size_t(r) != increment_)
              throw runtime_error("less bytes written than expected");
          }
          size_t a = end - next_begin;
          written_ += a / increment_ * increment_;
          size_t rest = a % increment_;
          Base::write(end-rest, end);
        }
      }
      void File::flush_it()
      {
        size_t m = cur_ - v_.data();
        auto r = ixxx::posix::write(fd_, v_.data(), m);
        if (size_t(r) != m)
          throw runtime_error("less bytes written than expected");
        cur_ = v_.data();
      }


    }


  }

}


