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
#include "xfsx.hh"

#include "bcd.hh"
#include "hex.hh"
#include "integer.hh"

#include <stdlib.h>
#include <stdexcept>
#include <limits>
#include <algorithm>
#include <string.h>


#if !defined(USE_BOOST_ENDIAN_FOR_INTEGER)
  #define USE_BOOST_ENDIAN_FOR_INTEGER 1
#endif
#if !defined(USE_BOOST_ENDIAN_FOR_INTEGER_WRITE)
  #define USE_BOOST_ENDIAN_FOR_INTEGER_WRITE 1
#endif
// not using boost endian for length conversion
// is slighty faster, thus, per default,
// USE_BOOST_ENDIAN_FOR_LENGTH is not defined
#if !defined(USE_BOOST_ENDIAN_FOR_LENGTH)
  #define USE_BOOST_ENDIAN_FOR_LENGTH 0
#endif

#if USE_BOOST_ENDIAN_FOR_LENGTH == 1 \
  || defined(USE_BOOST_ENDIAN_FOR_LENGTH_WRITE) \
  || defined(USE_BOOST_ENDIAN_FOR_INTEGER) \
  || defined(USE_BOOST_ENDIAN_FOR_INTEGER_WRITE)
#include <boost/endian/conversion.hpp>
#endif


using namespace std;


namespace xfsx {

  static const char klasse_universal       [] = "UNIVERSAL";
  static const char klasse_application     [] = "APPLICATION";
  static const char klasse_context_specific[] = "CONTEXT_SPECIFIC";
  static const char klasse_private         [] = "PRIVATE";

  const char *klasse_to_cstr(Klasse k)
  {
    switch (k) {
      case Klasse::UNIVERSAL       : return klasse_universal;
      case Klasse::APPLICATION     : return klasse_application;
      case Klasse::CONTEXT_SPECIFIC: return klasse_context_specific;
      case Klasse::PRIVATE         : return klasse_private;
    }
    throw range_error("invalid class value");
    return "INVALID_CLASS";
  }
  std::ostream &operator<<(std::ostream &o, Klasse k)
  {
    o << klasse_to_cstr(k);
    return o;
  }
  Klasse str_to_klasse(const std::pair<const char*, const char*> &p)
  {
    if (equal(p.first, p.second,
          klasse_universal,
          klasse_universal + sizeof(klasse_universal) - 1))
      return Klasse::UNIVERSAL;
    else if (equal(p.first, p.second,
          klasse_application,
          klasse_application + sizeof(klasse_application) - 1))
      return Klasse::APPLICATION;
    else if (equal(p.first, p.second,
          klasse_context_specific,
          klasse_context_specific + sizeof(klasse_context_specific) - 1))
      return Klasse::CONTEXT_SPECIFIC;
    else if (equal(p.first, p.second,
          klasse_private,
          klasse_private + sizeof(klasse_private) - 1))
      return Klasse::PRIVATE;
    throw range_error("unknown class value: " + string(p.first, p.second));
    return Klasse::UNIVERSAL;
  }

  uint8_t klasse_to_index(Klasse k)
  {
    uint8_t i = static_cast<uint8_t>(k);
    uint8_t r = i >> 6;
    return r;
  }
  Klasse index_to_klasse(uint8_t i)
  {
    switch (i) {
      case 0: return Klasse::UNIVERSAL;
      case 1: return Klasse::APPLICATION;
      case 2: return Klasse::CONTEXT_SPECIFIC;
      case 3: return Klasse::PRIVATE;
    }
    throw std::out_of_range("Class index is out of range");
    return Klasse::UNIVERSAL;
  }

  const char *shape_to_cstr(Shape s)
  {
    switch (s) {
      case Shape::PRIMITIVE  : return "PRIMITIVE";
      case Shape::CONSTRUCTED: return "CONSTRUCTED";
    }
    throw range_error("invalid shape value");
    return "INVALID_SHAPE";
  }
  std::ostream &operator<<(std::ostream &o, Shape s)
  {
    o << shape_to_cstr(s);
    return o;
  }

  namespace universal {

    // Mapping of universal type number to ASN.1 keywords.
    //
    // (or keyword pairs)
    const char *type_to_cstr(Tag t)
    {
      switch (t) {
        case Tag::EOC:                           return "EOC";
        case Tag::BOOLEAN:                       return "BOOLEAN";
        case Tag::INTEGER:                       return "INTEGER";
        case Tag::BIT_STRING:                    return "BIT STRING";
        case Tag::OCTET_STRING:                  return "OCTET STRING";
        case Tag::NULL_:                         return "NULL";
        case Tag::OBJECT_IDENTIFIER:             return "OBJECT IDENTIFIER";
        case Tag::OBJECT_DESCRIPTOR:             return "ObjectDescriptor";
        case Tag::EXTERNAL:                      return "EXTERNAL";
        case Tag::REAL:                          return "REAL";
        case Tag::ENUMERATED:                    return "ENUMERATED";
        case Tag::EMBEDDED_PDV:                  return "EMBEDDED PDV";
        case Tag::UTF8_STRING:                   return "UTF8String";
        case Tag::RELATIVE_OID:                  return "RELATIVE-OID";
        case Tag::TIME:                          return "TIME";
        case Tag::RESERVED_15:                   return "RESERVED-15";
                                                 // or SEQUENCE OF
        case Tag::SEQUENCE:                      return "SEQUENCE";
                                                 // or SET OF
        case Tag::SET:                           return "SET";
        case Tag::NUMERIC_STRING:                return "NumericString";
        case Tag::PRINTABLE_STRING:              return "PrintableString";
        case Tag::TELETEX_STRING:                return "TeletexString";
        case Tag::VIDEOTEX_STRING:               return "VideotexString";
        case Tag::IA5_STRING:                    return "IA5String";
        case Tag::UTC_TIME:                      return "UTCTime";
        case Tag::GENERALIZED_TIME:              return "GeneralizedTime";
        case Tag::GRAPHIC_STRING:                return "GraphicString";
        case Tag::VISIBLE_STRING:                return "VisibleString";
        case Tag::GENERAL_STRING:                return "GeneralString";
        case Tag::UNIVERSAL_STRING:              return "UniversalString";
        case Tag::UNRESTRICTED_CHARACTER_STRING: return "CHARACTER STRING";
        case Tag::BMP_STRING:                    return "BMPString";
        case Tag::DATE:                          return "DATE";
        case Tag::TIME_OF_DAY:                   return "TIME-OF-DAY";
        case Tag::DATE_TIME:                     return "DATE_-IME";
        case Tag::DURATION:                      return "DURATION";
        case Tag::OID_IRI:                       return "OID-IRI";
        case Tag::RELATIVE_OID_IRI:              return "RELATIVE-OID-IRI";
      };
      throw range_error("invalid type value");
      return "INVALID_TYPE";
    }

    // Mapping of universal tag number to 'xmlasn1typename'.
    //
    // Basically following rules are at play:
    // - upper-case type names are used as is (cf. table 4, X.680)
    // - a two-word upper-case type name is concatenated with a '_'
    // - camel case type names are used as is (cf. 12.36.3, X.680),
    //   e.g. UTF8String, GeneralizedTime, ...
    // - some constructed types are just mapped to 'SEQUENCE'
    //   e.g. (unrestricter) 'CHARACTER STRING'
    const char *type_to_xml_cstr(Tag t)
    {
      switch (t) {
        case Tag::EOC:                           return "EOC";
        case Tag::BOOLEAN:                       return "BOOLEAN";
        case Tag::INTEGER:                       return "INTEGER";
        case Tag::BIT_STRING:                    return "BIT_STRING";
        case Tag::OCTET_STRING:                  return "OCTET_STRING";
        case Tag::NULL_:                         return "NULL";
        case Tag::OBJECT_IDENTIFIER:             return "OBJECT_IDENTIFIER";
        case Tag::OBJECT_DESCRIPTOR:             return "ObjectDescriptor";
                                                 // cf. X.680, Table 4
        case Tag::EXTERNAL:                      return "SEQUENCE";
        case Tag::REAL:                          return "REAL";
        case Tag::ENUMERATED:                    return "ENUMERATED";
                                                 // cf. X.680, Table 4
        case Tag::EMBEDDED_PDV:                  return "SEQUENCE";
        case Tag::UTF8_STRING:                   return "UTF8String";
        case Tag::RELATIVE_OID:                  return "RELATIVE_OID";
        case Tag::TIME:                          return "TIME";
        case Tag::RESERVED_15:                   return "RESERVED_15";
                                                 // or SEQUENCE_OF
        case Tag::SEQUENCE:                      return "SEQUENCE";
                                                 // or SET_OF
        case Tag::SET:                           return "SET";
        case Tag::NUMERIC_STRING:                return "NumericString";
        case Tag::PRINTABLE_STRING:              return "PrintableString";
        case Tag::TELETEX_STRING:                return "TeletexString";
        case Tag::VIDEOTEX_STRING:               return "VideotexString";
        case Tag::IA5_STRING:                    return "IA5String";
        case Tag::UTC_TIME:                      return "UTCTime";
        case Tag::GENERALIZED_TIME:              return "GeneralizedTime";
        case Tag::GRAPHIC_STRING:                return "GraphicString";
        case Tag::VISIBLE_STRING:                return "VisibleString";
        case Tag::GENERAL_STRING:                return "GeneralString";
        case Tag::UNIVERSAL_STRING:              return "UniversalString";
                                                 // cf. X.680, Table 4
        case Tag::UNRESTRICTED_CHARACTER_STRING: return "SEQUENCE";
        case Tag::BMP_STRING:                    return "BMPString";
        case Tag::DATE:                          return "DATE";
        case Tag::TIME_OF_DAY:                   return "TIME_OF_DAY";
        case Tag::DATE_TIME:                     return "DATE_TIME";
        case Tag::DURATION:                      return "DURATION";
        case Tag::OID_IRI:                       return "OID_IRI";
        case Tag::RELATIVE_OID_IRI:              return "RELATIVE_OID_IRI";
      };
      throw range_error("invalid type value");
      return "INVALID_TYPE";
    }
    std::ostream &operator<<(std::ostream &o, Tag t)
    {
      o << type_to_xml_cstr(t);
      return o;
    }

  }



  Basic_Content::Basic_Content()
    :
      p_(nullptr, nullptr)
  {
  }
  Basic_Content::Basic_Content(std::pair<const char*, const char*> &&p)
    :
      p_(p)
  {
  }
  const char *Basic_Content::begin() const
  {
    return p_.first;
  }
  const char *Basic_Content::end() const
  {
    return p_.second;
  }
  size_t Basic_Content::size() const
  {
    return size_t(p_.second - p_.first);
  }

  Int64_Content::Int64_Content(const std::pair<const char*, const char*> &p)
    : i_(integer::range_to_int64(p))
  {
  }
  int64_t Int64_Content::value() const
  {
    return i_;
  }
  void Int64_Content::uint_to_int()
  {
    integer::uint_to_int(i_);
  }

  Unit::Unit()
    :
      klasse          (Klasse::UNIVERSAL),
      shape           (Shape::PRIMITIVE),
      is_long_tag     (false),
      is_indefinite   (false),
      is_long_definite(false),
      t_size          (0),
      tl_size         (0),
      tag             (0),
      length          (0)
  {
  }
  Unit::Unit(Tag_Int tag)
    :
      klasse(Klasse::UNIVERSAL)
  {
    init_constructed_from(tag);
  }
  Unit::Unit(Klasse klasse, Tag_Int tag, size_t length)
    :
      klasse(klasse)
  {
    init_constructed_from(tag, length);
  }
  Unit::Unit(Unit::EOC)
    :
      klasse          (Klasse::UNIVERSAL),
      shape           (Shape::PRIMITIVE),
      is_long_tag     (false),
      is_indefinite   (false),
      is_long_definite(false),
      t_size          (1),
      tl_size         (2),
      tag             (0),
      length          (0)
  {
  }

  bool Unit::is_eoc() const
  {
    return klasse == Klasse::UNIVERSAL && shape == Shape::PRIMITIVE
      && t_size == 1 && tl_size == 2
      && !tag && !length;
  }

  namespace {

  template <bool b, typename T> inline void shift_into_int(const u8 *&p,
      size_t n, T &r)
  {
    // work around ambiguity issue with T=size_t and boost::endian on Mac OSX
    // (where size_t neither matches uint32_t nor uint64_t
    //  and boost endian only define overloads for *int*_t types)
    // cf. https://stackoverflow.com/q/11603818/427158
#if __APPLE__ && __MACH__
    if (0) {
      (void)0;
    }
#else
    if (b) {
      memcpy(reinterpret_cast<u8 *>(&r)+(sizeof(T)-n), p, n);
      boost::endian::big_to_native_inplace(r);
      p += n;
    }
#endif
    else {
      for (uint8_t i = 0; i < n; ++i) {
        r <<= 8;
        r |= *p;
        ++p;
      }
    }
  }

  }


  /*
   * Tag-Length (TL) Unit - Basic Encoding Rules (BER)
   *
   * Identifier (1st Byte):
   *
   *                   0 <- primitive
   *                   1 <- constructed
   *                   Shape
   *                   +
   *               8 7 6 5 4 3 2 1
   *               +-+   +-------+
   *               Class |  Tag  |
   *  universal -> 0 0   1 1 1 1 1 <- long tag (> 30)
   *  applicati.-> 0 1   . . . . . <- short tag
   *  context   -> 1 0
   *  private   -> 1 1
   *
   * Long Tag (following bytes):
   *
   *    1 <- not last     1 <- not last        0 <- last tag byte
   *    Indicator         Indicator            Indicator
   *    +                 +                    +
   *    8 7 6 5 4 3 2 1   8 7 6 5 4 3 2 1 ...  8 7 6 5 4 3 2 1
   *      +-----------+     +-----------+        +-----------+
   *      | unsigned integer                                 |
   *      + most significant 7-bit     least significant bit +
   *
   *
   * Length unit (following bytes):
   *
   *                1 0 0 0 0 0 0 0 <- indefinite
   *                +-------------+
   *                8 7 6 5 4 3 2 1
   *                + +-----------+
   *        Indicator
   *  short form -> 0 . . . . . . . <- length [0..127]
   *   long form -> 1 . . . . . . . <- # of following length bytes
   *
   *
   * Long length bytes (following bytes):
   *
   *   Note: Does not have to be minimally encoded in BER
   *         (in contrast to integer values).
   *         But lengths < 128 SHOULD be encoding in short form.
   *
   *   8 7 6 5 4 3 2 1   ...   8 7 6 5 4 3 2 1
   *   +-------------+         +-------------+
   *   | unsigned integer                    |
   *   + most significant byte               + least significant bit
   *
   *
   * See also:
   *
   *   - https://en.wikipedia.org/wiki/X.690
   */

  const u8 *Unit::read(const u8 *begin,
      const u8 *end)
  {
    if (end-begin < 2)
      throw TL_Too_Small();
    const u8 *p = begin;
    klasse = static_cast<Klasse>((*p) & 0b11'000'000u);
    shape  = static_cast<Shape>((*p) & 0b00'1'00000u);
    is_long_tag = ((*p) & 0b1'11'11u) == 0b1'11'11u;
    tag = 0;
    if (is_long_tag) {
      auto start = p+1;
      for (++p; p < end; ++p) {
        tag <<= 7;
        if ((*p) & 0b1'000'0000u) {
          tag |= (*p) & 0b0'111'1111u;
        } else {
          tag |= (*p);
          ++p;
          break;
        }
      }
      if ((*(p-1)) & 0b1'000'0000u)
        throw overflow_error("truncated long tag");
      // if ( ((p-start)*7 > ssize_t(sizeof(tag))*8)
      static_assert(sizeof(tag) == 4, "assuming 4 byte tags");
      if ( !(    ((p-start) < 5)
               /* allow for some leading zero bits ... */
              || ((p-start) == 5 && ((*start) & 0b0'111'1111u)<16) ) ) {
        throw overflow_error("tag overflow");
      }
    } else {
      tag = (*p) & 0b1'11'11u;
      ++p;
    }
    t_size = p-begin;
    if (end-p < 1)
      throw overflow_error("L must be at least 1 bytes long");
    is_indefinite    = (*p) ==  0b1'000'0000u;
    is_long_definite = !is_indefinite && ((*p) & 0b1'000'0000u);
    length = 0;
    if (is_indefinite) {
      ++p;
    } else {
      if (is_long_definite) {
        uint8_t n = (*p) & 0b0'111'1111;
        if (n > sizeof(length))
          throw overflow_error("length is unrealistically long");
        ++p;
        if (end-p < n)
          throw overflow_error("length - not enough bytes");
        shift_into_int<USE_BOOST_ENDIAN_FOR_LENGTH>(p, n, length);
      } else {
        length = (*p) & 0b0'111'1111;
        ++p;
      }
    }
    tl_size = p-begin;
    if (size_t(end-p) < length)
      throw range_error("content overflows");
    if (shape == Shape::PRIMITIVE)
      return p + length;
    else
      return p;
  }

  u8 *Unit::write(u8 *begin, u8 *end) const
  {
    // if (end - begin < 2)
    //  throw overflow_error("TL write buffer must be at least 2 bytes long");
    if (end - begin < tl_size)
      throw overflow_error("TL write buffer is too small");
    uint8_t b = static_cast<uint8_t>(klasse);
    b |= static_cast<uint8_t>(shape);
    u8 *p = begin;
    if (is_long_tag) {
      b |= 0b11'11'1u;
      *p++ = b;
      for (uint8_t i = (t_size-2)*7; i > 0; i-=7) {
        uint8_t b = (tag >> i) & 0b01'11'1111u;
        b |= 0b10'00'0000;
        *p++ = b;
      }
      uint8_t b = tag & 0b01'11'1111u;
      *p++ = b;
    } else {
      b |= uint8_t(tag);
      *p++ = b;
    }
    if (is_indefinite) {
      *p++ = 0b1'000'0000;
    } else if (is_long_definite) {
      uint8_t l = tl_size - t_size - 1;
      uint8_t b = l;
      b |= 0b10'00'0000;
      *p++ = b;
      #if USE_BOOST_ENDIAN_FOR_LENGTH_WRITE
        size_t x = length;
        boost::endian::native_to_big_inplace(x);
        memcpy(p, reinterpret_cast<u8 *>(&x)+(sizeof(size_t)-l), l);
        p += l;
      #else
        for (uint8_t i = (l-1)*8; i > 0; i-=8) {
          uint8_t b = (length >> i) & 0b11'11'1111u;
          *p++ = b;
        }
        {
          uint8_t b = length & 0b11'11'1111u;
          *p++ = b;
        }
      #endif
    } else {
      uint8_t b = length & 0b01'11'1111u;
      *p++ = b;
    }
    return p;
  }

  void Unit::init_tag(Tag_Int tag)
  {
    is_long_tag = tag > 30;
    if (is_long_tag)
      t_size = 1 + minimally_encoded_tag_length(tag);
    else
      t_size = 1;
    this->tag = tag;
  }

  void Unit::init_indefinite()
  {
    is_indefinite = true;
    is_long_definite = false;
    length = 0;
    tl_size = t_size + 1;
  }
  void Unit::init_length()
  {
    is_indefinite = false;
    is_long_definite = length > 127;
    if (is_long_definite)
      tl_size = t_size + 1 + minimally_encoded_length(length);
    else
      tl_size = t_size + 1;
  }
  void Unit::init_length(size_t length)
  {
    this->length = length;
    init_length();
  }
  void Unit::init_l_size(uint8_t new_l_size)
  {
    uint8_t l_size = tl_size - t_size - 1;
    if (new_l_size < l_size)
      throw underflow_error("new length size is too small");
    tl_size += new_l_size - l_size;
    is_long_definite = true;
    is_indefinite = false;
  }

  static void Unit_init_constructed_from_shared(Unit &u, Tag_Int tag)
  {
    u.shape = Shape::CONSTRUCTED;
    u.init_tag(tag);
  }

  void Unit::init_constructed_from(Tag_Int tag)
  {
    Unit_init_constructed_from_shared(*this, tag);
    is_indefinite    = true;
    is_long_definite = false;
    tl_size          = t_size + 1;
    length           = 0;
  }
  void Unit::init_constructed_from(Tag_Int tag, size_t length)
  {
    Unit_init_constructed_from_shared(*this, tag);
    init_length(length);
  }

  const u8 *TLC::read(const u8 *begin,
      const u8 *end)
  {
    this->begin = begin;
    return Unit::read(begin, end);
  }
  u8 *TLC::write(u8 *begin, u8 *end) const
  {
    auto p = Unit::write(begin, end);
    if (shape == Shape::PRIMITIVE)
      p = copy(this->begin + tl_size, this->begin + tl_size + length, p);
    return p;
  }

  Vertical_TLC::Vertical_TLC()
  {
    stack_.resize(32);
    stack_[0].length = 0;
    stack_[0].expected_length = 0;
    stack_[0].indefinite = false;
  }
  void Vertical_TLC::push()
  {
    ++depth_;
    if (size_t(depth_) + 1 > stack_.size())
      stack_.resize(depth_ + 32);
  }
  void Vertical_TLC::pop()
  {
    if (!depth_)
      throw underflow_error("Can't pop too low");
    stack_[depth_-1].length += stack_[depth_].length;
    --depth_;
  }

  /*
   *
   * The for-loop correctly accounts for right-recursive definite-encoded
   * structures, e.g.:
   *
   * ROOT = A X; A = a B; B = b C; C = c; X = x
   *
   *                 depth  comment
   *                 -----  -----------
   * .abcx => a      1      push
   *            b    2      push
   *              c  3      pop: 2 iterations
   *          x      1
   *
   */
  void Vertical_TLC::conditional_pop()
  {
    for (;;) {
      if (stack_[depth_].indefinite) {
        break;
      } else {
        if (stack_[depth_].length > stack_[depth_].expected_length) {
          if (depth_)
            throw overflow_error("definite length cuts tag");
          else
            break; // 0-depth_ has no expected length
        }
        if (stack_[depth_].length == stack_[depth_].expected_length)
          pop();
        else
          break;
      }
    }
  }

  /*
   * Complex tags must only be pushed to the stack when
   * they are indefinite or non-empty.
   *
   * When they are empty they also may close a definite parent tag,
   * possibly over more than one level.
   *
   * Similarily an EOC may close a definite parent tag, as well.
   *
   * For computing the height of an EOC there are obviously
   * 2 choices:
   *
   *   a) same height as the constructed 'open' tag
   *   b) same height as the last primitive tag,
   *     the last enclosed one of the construction
   *
   * The function implements a) for consistency reasons:
   * For one, the size of the EOC tag counts into the constructed
   * parent tag. EOC is also a primitive. Last, but not least,
   * when visualizing the tree structure, not putting the 'close'
   * tag at the same height as the 'open' one, would be very
   * unusual.
   *
   */
  const u8 *Vertical_TLC::read(const u8 *begin,
      const u8 *end)
  {
    height = depth_;
    const u8 *r = TLC::read(begin, end);
    switch (shape) {
      case Shape::PRIMITIVE:
        if (is_eoc()) {
          if (!stack_[depth_].indefinite)
            throw Unexpected_EOC();
          pop();
          height = depth_;
        }
        stack_[depth_].length += tl_size;
        stack_[depth_].length += length;
        conditional_pop();
        break;
      case Shape::CONSTRUCTED:
        stack_[depth_].length += tl_size;
        if (is_indefinite || length) {
          push();
          stack_[depth_].length = 0;
          stack_[depth_].expected_length = length;
          stack_[depth_].indefinite = is_indefinite;
        } else {
          conditional_pop();
        }
        break;
    }
    return r;
  }

  const u8 *Vertical_TLC::skip(const u8 *begin,
      const u8 *end)
  {
    assert(begin < end);
    if (shape == Shape::CONSTRUCTED && length) {
      stack_[depth_].length = length;
      conditional_pop();
      return begin + length;
    } else {
      return begin;
    }
  }

  // We work on a Vertical_TLC and not on a Skip_EOC_Reader/Vertical_Reader
  // since we want to efficiently jump over definite constructed tags
  // (even if the first tag is indefinite)
  //
  // assumes that last operation on tlc was a read
  const u8 *Vertical_TLC::skip_children(const u8 *begin,
      const u8 *end)
  {
    if (!(begin<end))
      throw overflow_error("Not enough space for skipping children");
    auto r = begin;
    auto start_height = height;
    do {
      r = skip(r, end);
      if (r < end)
        r = this->read(r, end);
    } while (r < end && height > start_height);
    return r;
  }

  size_t minimally_encoded_tag_length(Tag_Int v)
  {
    static_assert(sizeof(unsigned int) == sizeof(Tag_Int),
        "expecting unsigned to be 4 byte because CLZ uses that type");
    static_assert(sizeof(uint32_t) == sizeof(Tag_Int),
        "expecting Tag_Int to be 4 byte");
    // clz is undefined for 0 values
    uint32_t redundant_bits = v ? clz_uint32(v) : sizeof(uint32_t)*8 - 1;
    uint32_t bits           = sizeof(uint32_t)*8 - redundant_bits;
    uint32_t parts          = (bits + 7 - 1) / 7;
    return parts;
  }

  template<> size_t minimally_encoded_length(uint8_t)
  {
    return 1;
  }
  template<> size_t minimally_encoded_length(int8_t)
  {
    return 1;
  }
  template<> size_t minimally_encoded_length(uint16_t v)
  {
    return 1 + (v > 255u);
  }
  template<> size_t minimally_encoded_length(int16_t v)
  {
    return 1 + (v < -128 || v > 255);
  }
  template<> size_t minimally_encoded_length(uint32_t v)
  {
    // clz is undefined for 0 values
    int redundant_bytes = v ? clz_uint32(v) / 8 : sizeof(uint32_t) - 1;
    return sizeof(uint32_t) - redundant_bytes;
  }
  template<> size_t minimally_encoded_length(int32_t v)
  {
    int redundant_bytes = clrsb_int32(v) / 8;
    return sizeof(int32_t) - redundant_bytes;
  }
  template<> size_t minimally_encoded_length(uint64_t v)
  {
    // clz is undefined for 0 values
    int redundant_bytes = v ? clz_uint64(v) / 8 : sizeof(uint64_t) - 1;
    return sizeof(uint64_t) - redundant_bytes;
  }
  template<> size_t minimally_encoded_length(int64_t v)
  {
    int redundant_bytes = clrsb_int64(v) / 8;
    return sizeof(int64_t) - redundant_bytes;
  }
  // work around size_t being different to uint32_t and uint64_t
  // on Mac OS X,
  // cf. http://stackoverflow.com/questions/11603818/why-is-there-ambiguity-between-uint32-t-and-uint64-t-when-using-size-t-on-mac-os
#if (defined(__APPLE__) && defined(__MACH__))
  template<> size_t minimally_encoded_length(size_t v)
  {
    static_assert(sizeof(size_t) == 4 || sizeof(size_t) == 8,
        "expecting size_t being 4 or 8 bytes big");
    if (sizeof(size_t) == 4)
      return minimally_encoded_length(uint32_t(v));
    else
      return minimally_encoded_length(uint64_t(v));
  }
#endif
  template<> size_t minimally_encoded_length(
      const std::pair<const u8 *, const u8 *> &v)
  {
    return size_t(v.second - v.first);
  }
  template<> size_t minimally_encoded_length(
      const std::pair<const char*, const char*> &v)
  {
    return size_t(v.second - v.first);
  }
  template<> size_t minimally_encoded_length(
      const std::string &v)
  {
    return v.size();
  }
  template<> size_t minimally_encoded_length(
      const XML_Content &v)
  {
    return hex::encoded_size<hex::Style::XML>(v.begin(), v.end());
  }
  template<> size_t minimally_encoded_length(
      const BCD_Content &v)
  {
    return (v.size() + 2 - 1) / 2;
  }
  template<> size_t minimally_encoded_length(
      const Int64_Content &v)
  {
    return minimally_encoded_length(v.value());
  }


  template <typename T> void decode_int(const u8 *begin, size_t length,
      T &r)
  {
    if (length > sizeof(T))
      throw overflow_error("encoded integer is too large");

    static_assert(numeric_limits<T>::is_integer,
        "Template argument must be an integer");

    if (sizeof(T) == 1) {
      r = *begin;
    } else {
      if (numeric_limits<T>::is_signed)
        r = ((*begin) & 0b1'000'0000) ? -1 : 0;
      else
        r = 0;
      const u8 *p = begin;
      shift_into_int<USE_BOOST_ENDIAN_FOR_INTEGER>(p, length, r);
    }
  }

  template<> void decode(const u8 *begin, size_t length, int64_t &r)
  {
    decode_int(begin, length, r);
  }
  template<> void decode(const u8 *begin, size_t length, uint64_t &r)
  {
    decode_int(begin, length, r);
  }
  template<> void decode(const u8 *begin, size_t length, int32_t &r)
  {
    decode_int(begin, length, r);
  }
  template<> void decode(const u8 *begin, size_t length, uint32_t &r)
  {
    decode_int(begin, length, r);
  }
  template<> void decode(const u8 *begin, size_t length, int16_t &r)
  {
    decode_int(begin, length, r);
  }
  template<> void decode(const u8 *begin, size_t length, uint16_t &r)
  {
    decode_int(begin, length, r);
  }
  template<> void decode(const u8 *begin, size_t length, int8_t &r)
  {
    decode_int(begin, length, r);
  }
  template<> void decode(const u8 *begin, size_t length, uint8_t &r)
  {
    decode_int(begin, length, r);
  }

  template<> void decode(const u8 *begin, size_t length, bool &r)
  {
    if (length > 1)
      throw overflow_error("encoded boolean is too large");
    r = *begin;
  }

  template<> void decode(const u8 *begin, size_t size,
      std::pair<const u8 *, const u8 *> &r)
  {
    r.first  = begin;
    r.second = begin + size;
  }
  template<> void decode(const u8 *begin, size_t size,
      std::pair<const char*, const char *> &r)
  {
    r.first  = reinterpret_cast<const char *>(begin);
    r.second = reinterpret_cast<const char *>(begin) + size;
  }


  template<typename T> u8 *encode_int(T t, u8 *begin, size_t size)
  {
    u8 *p = begin;
    if (sizeof(T) == 1) {
      if (size != 1)
        throw overflow_error("size too small/big for 1 byte int");
      *p++ = static_cast<uint8_t>(t);
    } else {
      if (size > sizeof(T))
        throw overflow_error("size too big for int");
      // call outside to avoid redundant calls
      //uint8_t l = minimally_encoded_length(t);
      uint8_t l = size;
     #ifdef USE_BOOST_ENDIAN_FOR_INTEGER_WRITE
        T v = boost::endian::native_to_big(t);
        #ifdef _GNU_SOURCE
          p = static_cast<u8 *>(mempcpy(p,
                reinterpret_cast<u8 *>(&v) + (sizeof(T) - l), l));
        #else
          memcpy(p, reinterpret_cast<u8 *>(&v) + (sizeof(T) - l), l);
          p += l;
        #endif
      #else
        for (uint8_t i = (l-1)*8; i>0; i-=8)
          *p++ = (t >> i) & 0b11'11'1111u;
        *p++ = t & 0b11'11'1111u;
      #endif
    }
    return p;
  }

  template<> u8 *encode(uint8_t t, u8 *begin, size_t size)
  {
    return encode_int(t, begin, size);
  }
  template<> u8 *encode(int8_t t, u8 *begin, size_t size)
  {
    return encode_int(t, begin, size);
  }
  template<> u8 *encode(uint16_t t, u8 *begin, size_t size)
  {
    return encode_int(t, begin, size);
  }
  template<> u8 *encode(int16_t t, u8 *begin, size_t size)
  {
    return encode_int(t, begin, size);
  }
  template<> u8 *encode(uint32_t t, u8 *begin, size_t size)
  {
    return encode_int(t, begin, size);
  }
  template<> u8 *encode(int32_t t, u8 *begin, size_t size)
  {
    return encode_int(t, begin, size);
  }
  template<> u8 *encode(uint64_t t, u8 *begin, size_t size)
  {
    return encode_int(t, begin, size);
  }
  template<> u8 *encode(int64_t t, u8 *begin, size_t size)
  {
    return encode_int(t, begin, size);
  }
  template<> u8 *encode(bool t, u8 *begin, size_t size)
  {
    if (size != 1)
      throw overflow_error("bool needs 1 byte");
    *begin = uint8_t(t);
    return begin + 1;
  }
  template <typename T> u8 *encode_pair(
      const std::pair<const T*, const T*> &t, u8 *begin, size_t size)
  {
    if (t.second - t.first != ssize_t(size))
      throw overflow_error("pair too small or too big");
    #ifdef _GNU_SOURCE
      return static_cast<u8 *>(mempcpy(begin, t.first, size));
    #else
      memcpy(begin, t.first, size);
      return begin + size;
    #endif
  }
  template<> u8 *encode(
      const std::pair<const u8 *, const u8 *> &t, u8 *begin,
      size_t size)
  {
    return encode_pair(t, begin, size);
  }
  template<> u8 *encode(const std::pair<const char*, const char*> &t,
      u8 *begin, size_t size)
  {
    return encode_pair(t, begin, size);
  }
  template<> u8 *encode(const string &t, u8 *begin, size_t size)
  {
    if (t.size() != size)
      throw overflow_error("string too small or too big");
    #ifdef _GNU_SOURCE
      return static_cast<u8 *>(mempcpy(begin, t.data(), size));
    #else
      memcpy(begin, t.data(), size);
      return begin + size;
    #endif
  }
  template<> u8 *encode(const XML_Content &t, u8 *begin, size_t size)
  {
    auto r = hex::encode<hex::Style::XML>(t.begin(), t.end(), begin);
    if (r !=  begin + size)
      throw overflow_error("XML_Content too long");
    return r;
  }
  template<> u8 *encode(const BCD_Content &t, u8 *begin, size_t size)
  {
    auto r = bcd::encode(t.begin(), t.end(), begin);
    if (r !=  begin + size)
      throw overflow_error("BCD_Content too long");
    return r;
  }
  template<> u8 *encode(const Int64_Content &t, u8 *begin,
      size_t size)
  {
    return encode(t.value(), begin, size);
  }



  template <typename T>
  Basic_Reader<T>::Basic_Reader(const u8 *begin, const u8 *end)
    :
      begin_(begin),
      end_(end)
  {
  }

  template <typename T>
  typename Basic_Reader<T>::iterator Basic_Reader<T>::begin()
  {
    return iterator(begin_, end_, skip_zero_);
  }
  template <typename T>
  typename Basic_Reader<T>::iterator Basic_Reader<T>::end()
  {
    return iterator(end_, end_, skip_zero_);
  }

  template <typename T>
  Basic_Reader<T>::iterator::iterator(const u8 *begin, const u8 *end,
      uint32_t skip_zero)
    :
      first_(begin),
      current_(begin),
      begin_(begin),
      end_(end),
      skip_zero_(skip_zero)
  {
    if (begin < end)
      begin_ = tlc_.read(begin, end);
  }

  template <typename T>
  const T &Basic_Reader<T>::iterator::operator*() const
  {
    return tlc_;
  }
  template <typename T>
  T &Basic_Reader<T>::iterator::operator*()
  {
    return tlc_;
  }

  const char *Unexpected_EOC::what() const noexcept
  {
    return "got EOC without matching indefinite tag";
  }

  const char *TL_Too_Small::what() const noexcept
  {
    return "TL must be at least 2 bytes long";
  }
  const char *Tag_Too_Long::what() const noexcept
  {
    return "tag too long";
  }

  template <typename T>
  typename Basic_Reader<T>::iterator &Basic_Reader<T>::iterator::operator++()
  {
    current_ = begin_;
    if (begin_ < end_) {
      for (;;) {
        try {
          begin_ = tlc_.read(begin_, end_);
        } catch (const Unexpected_EOC &) {
          if (!skip_zero_)
            throw;
          if (skip_zero_ == 1) {
            begin_ = find_if(begin_, end_, [](uint8_t c){return !!c;});
          } else {
            size_t m = end_ - first_;
            size_t skip_k = 1024;
            size_t k = ((begin_ - first_) + skip_k) / skip_k * skip_k;
            begin_ = first_ + min(m, k);
          }
          continue;
        } catch (const TL_Too_Small &) {
          if (!skip_zero_)
            throw;
          begin_ = end_;
        }
        break;
      }
    }
    return *this;
  }
  template <typename T>
  bool Basic_Reader<T>::iterator::operator==(
      const Basic_Reader<T>::iterator &other) const
  {
    return current_ == other.current_;
  }
  template <typename T>
  bool Basic_Reader<T>::iterator::operator!=(
      const Basic_Reader<T>::iterator &other) const
  {
    return !(*this == other);
  }

  template class Basic_Reader<TLC>;
  template class Basic_Reader<Vertical_TLC>;

  Skip_EOC_Reader::iterator &Skip_EOC_Reader::iterator::operator++()
  {
    do {
      Vertical_Reader::iterator::operator++();
    } while (current_ < end_ && tlc_.is_eoc());
    return *this;
  }
  Skip_EOC_Reader::iterator Skip_EOC_Reader::begin()
  {
    return iterator(begin_, end_, skip_zero_);
  }
  Skip_EOC_Reader::iterator Skip_EOC_Reader::end()
  {
    return iterator(end_, end_, skip_zero_);
  }

  Tag_Translator::Tag_Translator()
    :
      k_trans_(4u)
  {
  }
  Tag_Translator::Tag_Translator(Klasse klasse,
      std::unordered_map<uint32_t, std::string> &&m)
    :
      Tag_Translator()
  {
    push(klasse, std::move(m));
  }
  void Tag_Translator::push(Klasse klasse,
      std::unordered_map<uint32_t, std::string> &&m)
  {
    k_trans_.at(klasse_to_index(klasse)) = std::move(m);
  }
  const std::string &Tag_Translator::translate(
      Klasse klasse, Tag_Int tag) const
  {
    try {
      return k_trans_.at(klasse_to_index(klasse)).at(tag);
    } catch (const std::out_of_range &e) {
      throw range_error("Incomplete ASN.1 file - can't translate tag: "
          + std::to_string(tag));
    }
  }

  Tag_Dereferencer::Tag_Dereferencer()
    :
      v_(4u)
  {
  }
  void Tag_Dereferencer::push(Klasse klasse,
      std::unordered_set<uint32_t> &&tags,
          Klasse dest_klasse, Tag_Int dest_tag)
  {
    v_.at(klasse_to_index(klasse)).emplace_back(
        std::move(tags), make_pair(dest_klasse, dest_tag) );
  }
  std::pair<Klasse, Tag_Int> Tag_Dereferencer::dereference(
      Klasse klasse, Tag_Int tag) const
  {
    auto &d = v_.at(klasse_to_index(klasse));
    for (auto &e : d) {
      if (e.first.count(tag))
        return e.second;
    }
    return make_pair(klasse, tag);
  }
  Tag_Typifier::Tag_Typifier()
    :
      v_(4u)
  {
  }
  void Tag_Typifier::push(Klasse klasse, Tag_Int tag, Type type)
  {
    v_.at(klasse_to_index(klasse))[tag] = type;
  }
  Type Tag_Typifier::typify(Klasse klasse, Tag_Int tag) const
  {
    auto &m = v_.at(klasse_to_index(klasse));
    auto i = m.find(tag);
    if (i == m.end())
      return Type::OCTET_STRING;
    else
      return i->second;
  }
  Type Tag_Typifier::typify(const std::pair<Klasse, Tag_Int> &p) const
  {
    return typify(p.first, p.second);
  }

  Name_Translator::Name_Translator() =default;

  Name_Translator::Name_Translator(
    std::unordered_map<std::string, std::tuple<bool, uint32_t, uint32_t> > &&m)
    :
      name_map_prime_(std::move(m))
  {
    name_map_.max_load_factor(0.75);
    for (auto &e : name_map_prime_) {
      name_map_.emplace(make_pair(e.first.data(),
            e.first.data() + e.first.size()),
          make_tuple(
            std::get<0>(e.second) ? Shape::PRIMITIVE : Shape::CONSTRUCTED,
            index_to_klasse(std::get<1>(e.second)),
            std::get<2>(e.second)));
    }
  }
  std::tuple<xfsx::Shape, xfsx::Klasse, xfsx::Tag_Int>
    Name_Translator::translate(
      const std::pair<const char*, const char*> &s) const
  {
    return name_map_.at(s);
  }
  bool Name_Translator::empty() const
  {
    return name_map_.empty();
  }



}
