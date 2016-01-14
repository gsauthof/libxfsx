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
#ifndef XFSX_TYPES_HH
#define XFSX_TYPES_HH

#include <utility>
#include <stdint.h>

namespace xfsx {

  class Basic_Content {
    private:
      std::pair<const char *, const char *> p_;
    public:
      Basic_Content();
      Basic_Content(std::pair<const char*, const char*> &&p);
      const char *begin() const;
      const char *end() const;
      size_t size() const;
  };

  // may include XML entities (&#x..;)
  class XML_Content : public Basic_Content {
    public:
      using Basic_Content::Basic_Content;
  };

  class BCD_Content : public Basic_Content {
    public:
      using Basic_Content::Basic_Content;
  };

  class Int64_Content {
    private:
      int64_t i_;
    public:
      Int64_Content(const std::pair<const char*, const char*> &p);
      int64_t value() const;
      void uint_to_int();
  };


}

#endif
