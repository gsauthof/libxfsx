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

#if XFSX_HAVE_FROM_CHARS
    #include <charconv>
#else
    #include <boost/spirit/include/qi_parse.hpp>
    #include <boost/spirit/include/qi_auto.hpp>
    #include <boost/spirit/include/qi_numeric.hpp>
#endif

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
//  3) Boost qi-parse
//
//  efficient, because no extra buffer, but some compile time overhead
//
//  4) C++ std::from_chars()
//
//  only available since C++17, also allows for efficient implementations
//
//  Conclusion: use from_chars() where available, qi-parse otherwise.

namespace xfsx {

  namespace integer {

    uint32_t range_to_uint32(const std::pair<const char*, const char*> &p)
    {
      uint32_t t = 0;
#if XFSX_HAVE_FROM_CHARS
      std::from_chars(p.first, p.second, t);
#else
      boost::spirit::qi::parse(p.first, p.second, t);
#endif

      return t;
    }

    uint64_t range_to_uint64(const std::pair<const char*, const char*> &p)
    {
      uint64_t t = 0;
#if XFSX_HAVE_FROM_CHARS
      std::from_chars(p.first, p.second, t);
#else
      boost::spirit::qi::parse(p.first, p.second, t);
#endif
      return t;
    }

    int64_t range_to_int64(const std::pair<const char*, const char*> &p)
    {
      int64_t t = 0;
#if XFSX_HAVE_FROM_CHARS
      std::from_chars(p.first, p.second, t);
#else
      boost::spirit::qi::parse(p.first, p.second, t);
#endif
      return t;
    }


    void uint_to_int(int64_t &a)
    {
      if (a < 0) {
        a &= 0x00000000ffffffffl;
      }
    }


	static const uint64_t powers_of_10[] = {
	  0LLU,
	  10LLU,
	  100LLU,
	  1000LLU,
	  10000LLU,
	  100000LLU,
	  1000000LLU,
	  10000000LLU,
	  100000000LLU,
	  1000000000LLU,
	  10000000000LLU,
	  100000000000LLU,
	  1000000000000LLU,
	  10000000000000LLU,
	  100000000000000LLU,
	  1000000000000000LLU,
	  10000000000000000LLU,
	  100000000000000000LLU,
	  1000000000000000000LLU,
	  10000000000000000000LLU
	};

	static const uint32_t powers_of_10_u32[] = {
	  0,
	  10,
	  100,
	  1000,
	  10000,
	  100000,
	  1000000,
	  10000000,
	  100000000,
	  1000000000
	};


        // public-domain from:
        // http://graphics.stanford.edu/~seander/bithacks.html#IntegerLog10
        // via:
        // https://github.com/localvoid/cxx-benchmark-count-digits/blob/master/src/bth-clz.cpp
	uint32_t dec_digits(uint64_t n)
        {
	  uint64_t t = (64 - clz_uint64(n | 1)) * 1233 >> 12;
	  return t - (n < powers_of_10[t]) + 1;
	}

	uint32_t dec_digits(uint32_t n)
        {
	  uint32_t t = (32 - clz_uint32(n | 1)) * 1233 >> 12;
	  return t - (n < powers_of_10_u32[t]) + 1;
	}
	uint32_t dec_digits(unsigned char n)
        {
            return dec_digits(uint32_t(n));
        }
#if __APPLE__ && __MACH__
        // On Mac OSX, Apple defines size_t in the most annoying way
        // elsewhere its either the same as uint64_t or uint32_t
        // cf. https://stackoverflow.com/q/11603818/427158
	uint32_t dec_digits(size_t n)
        {
            if (sizeof(size_t) == sizeof(uint64_t))
                return dec_digits(uint64_t(n));
            else
                return dec_digits(uint32_t(n));
        }
#endif


  }

}
