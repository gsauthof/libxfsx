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
#include "integer.hh"

#include <boost/spirit/include/qi_parse.hpp>
#include <boost/spirit/include/qi_auto.hpp>
#include <boost/spirit/include/qi_numeric.hpp>


// Alternatives are:
//
// 1) boost lexical cast
//
// For example (where even an additonal temporary string object is created):
//
//     tlv.init_tag(boost::lexical_cast<unsigned>(mk_string(at.value())));
//
// too slow for inner loops, because it constructs a temporary stream.
//
// 2) strtol
//
// For example:
//
//     char tag_buffer[11] = {0};
//     // ^ outside the loop ...
//     if (size(at.value()) > 10)
//       throw runtime_error("tag integer is too large");
//     *copy(at.value().first, at.value().second, tag_buffer) = 0;
//     tlv.init_tag(strtol(tag_buffer, nullptr, 10));
//
//  is faster than 1), but since strtol need zero terminated strings, a (small)
//  copy into a temporary buffer is needed.
//
//  Conclusion: qi-parse is the fastest method.

namespace xfsx {

  namespace integer {

    uint32_t range_to_uint32(const std::pair<const char*, const char*> &p)
    {
      uint32_t t = 0;
      boost::spirit::qi::parse(p.first, p.second, t);
      return t;
    }

    uint64_t range_to_uint64(const std::pair<const char*, const char*> &p)
    {
      uint64_t t = 0;
      boost::spirit::qi::parse(p.first, p.second, t);
      return t;
    }

    int64_t range_to_int64(const std::pair<const char*, const char*> &p)
    {
      int64_t t = 0;
      boost::spirit::qi::parse(p.first, p.second, t);
      return t;
    }


    void uint_to_int(int64_t &a)
    {
      if (a < 0) {
        a &= 0x00000000ffffffffl;
      }
    }

  }

}
