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
#include "bcd.hh"

#include "bcd_impl.hh"

/* Notes on BCD encoding/decoding.


When trying to use some CPU level parallelism in
standard-conforming C/C++, two things aren't perhaps immediately
obvious:

1. Alignment requirements: for accessing non-char types, a
hardware architecture might have certain alignment requirements.
For example, that an address of an uint64_t must be 8 byte
aligned.  Alignemnt requirements might be 'soft' or 'hard'. Soft
means that unaligned accesses are still allowed, but slower. Hard
means that unaligned accesses terminate the program (think:
SIGBUS).

SPARC is one example with hard alignment requirements.  Older x86
CPUs are known to have some soft alignment requirements, where
recent ones have less/none. Even with SIMD, nowadays there are
several instructions that don't need aligned addresses, on x86.

2. Strict aliasing rules: the C++ standard specifies some rules
regarding the possible aliasing between pointers of different type.
Optimizing compilers rely on those rules to generate better code.
Ignoring those rules may introduce hard to find bugs.

For example, a char pointer may be an alias to a int64_t one.
But not the other way around. Also, a int8_t or uint8_t pointer
has different aliasing 'privileges' than a char pointer, although
they all represent a byte. Etc.

*/


namespace xfsx {

  namespace bcd {

    // XXX eliminate return statements?
    char *decode(const u8 *begin, const u8 *end,
        char *o)
    {
      impl::decode::decode(begin, end, o);
      //impl::decode::decode_lookup(begin, end, o);
      return o + (end-begin)*2;
    }

    u8 *encode(const char *begin, const char *end,
        u8 *o)
    {
      impl::encode::encode(begin, end, o);
      return o + ((end-begin)+1)/2;
    }


  }

}
