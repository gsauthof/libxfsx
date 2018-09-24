// Copyright 2017, Georg Sauthoff <mail@georg.so>

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
#ifndef XFSX_OCTET_HH
#define XFSX_OCTET_HH

namespace xfsx {

  using u8 = unsigned char;

/* We use u8 and not u8_t because the _t suffix is reserved for
   standard/system usage.

   We basically have 3 choices for a byte type we want to use
   for byte manipulations and byte memory management:

   - char - depending on the architecture might be signed or not,
     however, signed doesn't make much sense for a byte represenation
   - unsigned char (uchar) - the preferred choice - on architectures where
     char is signed you get type errors when you accidently
     assign a uchar pointer to a char one
   - std::byte - only available since C++17, a bit inconvenient
     for arithmetic operations since some explicit conversion is involved

   Under strict aliasing rules, all three types may alias anything.

   In fact, these 3 types are the only ones with this property (besides
   void). Especially uint8_t doesn't have this property, although
   it also fits in one byte (on modern architectures).

   The following table summarizes the properties of the different types:

   Type                uchar    char    std::byte   uint8_t    void
   ----------------------------------------------------------------
   Value type          yes      yes     yes         yes        no
   Signed              no       maybe   no          no         n/a
   Arithmetic          yes      yes     no          yes        no
   Bit Arithmetic      yes      yes     yes         yes        n/a
   Pointer Usability   yes      yes     yes         yes        yes
   May alias           yes      yes     yes         no         yes
   Pointer Arithmetic  yes      yes     yes         yes        no
   Printf specifier    yes      yes     no          yes        n/a
   ostream operator    yes      yes     no          yes        n/a
   impl. cast to int   yes      yes     no          yes        n/a

   */

} // xfsx

#endif // XFSX_OCTET_HH
