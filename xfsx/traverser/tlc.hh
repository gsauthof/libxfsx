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


#ifndef XFSX_TRAVERSER_TLC_HH
#define XFSX_TRAVERSER_TLC_HH

#include <xfsx/xfsx.hh>

namespace xfsx {
  namespace traverser {

    class Vertical_TLC_Proxy {
      private:
        std::pair<const uint8_t *, const uint8_t *> p_;

      public:
      Vertical_TLC_Proxy()
        : p_(nullptr, nullptr)
      {
      }
      Vertical_TLC_Proxy(const uint8_t *begin, const uint8_t *end,
          Vertical_TLC &t )
        : p_(begin, end) { advance(t); }

      Tag_Int tag(const Vertical_TLC &t) const { return t.tag; }
      Klasse klasse(const Vertical_TLC &t) const { return t.klasse; }
      uint32_t height(const Vertical_TLC &t) const { return t.height; }
      void string(const Vertical_TLC &t, std::string &s) const
      {
        std::pair<const char *, const char*> p;
        t.copy_content(p);
        s.clear();
        s.insert(s.end(), p.first, p.second);
      }
      uint64_t uint64(const Vertical_TLC &t) const
      {
        uint64_t r;
        t.copy_content(r);
        return r;
      }
      uint32_t uint32(const Vertical_TLC &t) const
      {
        uint32_t r;
        t.copy_content(r);
        return r;
      }

      void advance(Vertical_TLC &t)
      {
        if (!p_.first || p_.first == p_.second)
          p_.first = nullptr;
        else
          p_.first = t.read(p_.first, p_.second);
      }

      void skip_children(Vertical_TLC &t)
      {
        if (!p_.first || p_.first == p_.second)
          p_.first = nullptr;
        else {
          p_.first = t.skip_children(p_.first, p_.second);
          if (p_.first == p_.second)
            p_.first = nullptr;
        }
      }

      bool eot(const Vertical_TLC &t) const
      {
        return !p_.first;
      }
    };

  }
}

#endif
