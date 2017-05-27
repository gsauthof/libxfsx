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
#ifndef XFSX_HH
#define XFSX_HH

#include <stdint.h>
#include <stddef.h>
#include <ostream>
#include <vector>
#include <stack>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include "types.hh"
#include "value.hh"
#include "s_pair.hh"

namespace xfsx {

  enum class Klasse : uint8_t {
    UNIVERSAL        = 0b00'000'000u,
    APPLICATION      = 0b01'000'000u,
    CONTEXT_SPECIFIC = 0b10'000'000u,
    PRIVATE          = 0b11'000'000u
  };
  const char *klasse_to_cstr(Klasse k);
  std::ostream &operator<<(std::ostream &o, Klasse k);
  Klasse str_to_klasse(const std::pair<const char*, const char*> &p);
  uint8_t klasse_to_index(Klasse k);
  Klasse index_to_klasse(uint8_t i);

  enum class Shape : uint8_t {
    PRIMITIVE     = 0b00'0'00000u,
    CONSTRUCTED   = 0b00'1'00000u
  };
  const char *shape_to_cstr(Shape s);
  std::ostream &operator<<(std::ostream &o, Shape s);

  namespace universal {

    enum Tag : uint8_t {
      EOC                           = 0  ,
      BOOLEAN                       = 1  ,
      INTEGER                       = 2  ,
      BIT_STRING                    = 3  ,
      OCTET_STRING                  = 4  ,
      NULL_                         = 5  ,
      OBJECT_IDENTIFIER             = 6  ,
      OBJECT_DESCRIPTOR             = 7  ,
      EXTERNAL                      = 8  ,
      REAL                          = 9  ,
      ENUMERATED                    = 10 ,
      EMBEDDED_PDV                  = 11 ,
      UTF8_STRING                   = 12 ,
      RELATIVE_OID                  = 13 ,
      TIME                          = 14 ,
      RESERVED_15                   = 15 ,
      // and SEQUENCE OF
      SEQUENCE                      = 16 ,
      // and SET OF
      SET                           = 17 ,
      NUMERIC_STRING                = 18 ,
      PRINTABLE_STRING              = 19 ,
      TELETEX_STRING                = 20 ,
      VIDEOTEX_STRING               = 21 ,
      IA5_STRING                    = 22 ,
      UTC_TIME                      = 23 ,
      GENERALIZED_TIME              = 24 ,
      GRAPHIC_STRING                = 25 ,
      VISIBLE_STRING                = 26 , // ISO646String
      GENERAL_STRING                = 27 ,
      UNIVERSAL_STRING              = 28 ,
      UNRESTRICTED_CHARACTER_STRING = 29 ,
      BMP_STRING                    = 30 ,
      DATE                          = 31 ,
      TIME_OF_DAY                   = 32 ,
      DATE_TIME                     = 33 ,
      DURATION                      = 34 ,
      OID_IRI                       = 35 , // internationalized resource identifier
      RELATIVE_OID_IRI              = 36
    };
    const char *type_to_cstr(Tag t);
    const char *type_to_xml_cstr(Tag t);
    std::ostream &operator<<(std::ostream &o, Tag t);

  }

  using Tag_Int = uint32_t;

  enum class Type {
    OCTET_STRING,
    STRING,
    INT_64,
    BCD
  };
  class Tag_Translator {
    private:
      std::vector<std::unordered_map<uint32_t, std::string> > k_trans_;
    public:
      Tag_Translator();
      Tag_Translator(Klasse klasse,
          std::unordered_map<uint32_t, std::string> &&m);
      void push(Klasse klasse, std::unordered_map<uint32_t, std::string> &&m);
      const std::string &translate(Klasse klasse, Tag_Int tag) const;
  };
  class Tag_Dereferencer {
    private:
      std::vector<
        std::deque<
          std::pair<
            std::unordered_set<uint32_t>,
            std::pair<Klasse, Tag_Int> > > > v_;
    public:
      Tag_Dereferencer();
      void push(Klasse klasse, std::unordered_set<uint32_t> &&tags,
          Klasse dest_klasse, Tag_Int dest_tag);
      std::pair<Klasse, Tag_Int> dereference(Klasse klasse, Tag_Int tag) const;
  };
  class Tag_Typifier {
    private:
      std::vector<std::unordered_map<Tag_Int, Type> > v_;
    public:
      Tag_Typifier();
      void push(Klasse klasse, Tag_Int tag, Type type);
      Type typify(Klasse klasse, Tag_Int tag) const;
      Type typify(const std::pair<Klasse, Tag_Int> &p) const;
  };

  class Name_Translator {
    private:
      std::unordered_map<std::string, std::tuple<bool, uint32_t, uint32_t> >
        name_map_prime_;

      std::unordered_map<
        std::pair<const char*, const char*>,
        std::tuple<xfsx::Shape, xfsx::Klasse, xfsx::Tag_Int>,
        s_pair::Hash,
        s_pair::Equal >
          name_map_;
    public:
      Name_Translator();
      Name_Translator(
        std::unordered_map<std::string,
        std::tuple<bool, uint32_t, uint32_t> > &&m);
      std::tuple<xfsx::Shape, xfsx::Klasse, xfsx::Tag_Int> translate(
          const std::pair<const char*, const char*> &s) const;
      bool empty() const;
  };

  struct BCD{};
  struct HEX{};
  struct HEX_XML{};

  template <typename Tag>
  class Tagged_String {
    private:
      std::string s_;
    public:
      Tagged_String();
      Tagged_String(const std::string &s);
      Tagged_String(std::string &&s);
      Tagged_String &operator=(const std::string &s);
      Tagged_String &operator=(std::string &&s);
      operator const std::string&() const;
      void clear();
      size_t size() const;
      bool empty() const;

      std::string &get();
      const std::string &get() const;
  };
  using BCD_String = Tagged_String<BCD>;
  using Hex_String = Tagged_String<HEX>;
  using Hex_XML_String = Tagged_String<HEX_XML>;


  size_t minimally_encoded_tag_length(Tag_Int v);

  template<typename T,
           typename = std::enable_if_t<std::is_fundamental<T>::value> >
      size_t minimally_encoded_length(T);
  template<> size_t minimally_encoded_length(uint8_t v);
  template<> size_t minimally_encoded_length(int8_t v);
  template<> size_t minimally_encoded_length(uint16_t v);
  template<> size_t minimally_encoded_length(int16_t v);
  template<> size_t minimally_encoded_length(uint32_t v);
  template<> size_t minimally_encoded_length(int32_t v);
  template<> size_t minimally_encoded_length(uint64_t v);
  template<> size_t minimally_encoded_length(int64_t v);
#if (defined(__APPLE__) && defined(__MACH__))
  template<> size_t minimally_encoded_length(size_t v);
#endif
  template<typename T,
           typename = std::enable_if_t<!std::is_fundamental<T>::value> >
      size_t minimally_encoded_length(const T &);
  template<> size_t minimally_encoded_length(
      const std::pair<const uint8_t*, const uint8_t *> &v);
  template<> size_t minimally_encoded_length(
      const std::pair<const char*, const char*> &v);
  template<> size_t minimally_encoded_length(
      const std::string &v);
  template<> size_t minimally_encoded_length(
      const BCD_String &v);
  template<> size_t minimally_encoded_length(
      const Hex_String &v);
  template<> size_t minimally_encoded_length(
      const Hex_XML_String &v);
  template<> size_t minimally_encoded_length(
      const XML_Content &v);
  template<> size_t minimally_encoded_length(
      const BCD_Content &v);
  template<> size_t minimally_encoded_length(
      const Int64_Content &v);

  template<typename T> void decode(const uint8_t *begin, size_t size, T &);
  template<> void decode(const uint8_t *begin, size_t size, int8_t &r);
  template<> void decode(const uint8_t *begin, size_t size, uint8_t &r);
  template<> void decode(const uint8_t *begin, size_t size, int16_t &r);
  template<> void decode(const uint8_t *begin, size_t size, uint16_t &r);
  template<> void decode(const uint8_t *begin, size_t size, int64_t &r);
  template<> void decode(const uint8_t *begin, size_t size, uint64_t &r);
  template<> void decode(const uint8_t *begin, size_t size, int32_t &r);
  template<> void decode(const uint8_t *begin, size_t size, uint32_t &r);
  template<> void decode(const uint8_t *begin, size_t size, bool &r);
  template<> void decode(const uint8_t *begin, size_t size,
      std::pair<const uint8_t*, const uint8_t *> &r);
  template<> void decode(const uint8_t *begin, size_t size,
      std::pair<const char*, const char *> &r);
  template<> void decode(const uint8_t *begin, size_t size,
      BCD_String &r);
  template<> void decode(const uint8_t *begin, size_t size,
      Hex_String &r);
  template<> void decode(const uint8_t *begin, size_t size,
      Hex_XML_String &r);

  template<typename T,
           typename = std::enable_if_t<std::is_fundamental<T>::value> >
      uint8_t *encode(T, uint8_t *begin, size_t size);
  template<> uint8_t *encode(uint8_t t, uint8_t *begin, size_t size);
  template<> uint8_t *encode(int8_t t, uint8_t *begin, size_t size);
  template<> uint8_t *encode(uint16_t t, uint8_t *begin, size_t size);
  template<> uint8_t *encode(int16_t t, uint8_t *begin, size_t size);
  template<> uint8_t *encode(uint32_t t, uint8_t *begin, size_t size);
  template<> uint8_t *encode(int32_t t, uint8_t *begin, size_t size);
  template<> uint8_t *encode(uint64_t t, uint8_t *begin, size_t size);
  template<> uint8_t *encode(int64_t t, uint8_t *begin, size_t size);
  template<> uint8_t *encode(bool t, uint8_t *begin, size_t size);
  template<typename T,
           typename = std::enable_if_t<!std::is_fundamental<T>::value> >
      uint8_t *encode(const T &, uint8_t *begin, size_t size);
  template<> uint8_t *encode(const std::pair<const uint8_t*, const uint8_t*> &t,
      uint8_t *begin, size_t size);
  template<> uint8_t *encode(const std::pair<const char*, const char*> &t,
      uint8_t *begin, size_t size);
  template<> uint8_t *encode(const std::string &t, uint8_t *begin, size_t size);
  template<> uint8_t *encode(const BCD_String &t, uint8_t *begin, size_t size);
  template<> uint8_t *encode(const Hex_String &t, uint8_t *begin, size_t size);
  template<> uint8_t *encode(const Hex_XML_String &t,
      uint8_t *begin, size_t size);
  template<> uint8_t *encode(const XML_Content &t, uint8_t *begin, size_t size);
  template<> uint8_t *encode(const BCD_Content &t, uint8_t *begin, size_t size);
  template<> uint8_t *encode(const Int64_Content &t,
      uint8_t *begin, size_t size);

  struct Encode_Visitor {
    typedef uint8_t* result_type;
    uint8_t *begin;
    size_t size;
    Encode_Visitor(uint8_t *begin, size_t size)
      :
        begin(begin),
        size(size)
    {
    }
    template <typename T,
              typename = std::enable_if_t<std::is_fundamental<T>::value> >
      uint8_t *operator()(T t) const
    {
      return encode(t, begin, size);
    }
    template <typename T,
              typename = std::enable_if_t<!std::is_fundamental<T>::value> >
      uint8_t *operator()(const T &t) const
    {
      return encode(t, begin, size);
    }
  };

  struct Length_Visitor {
    typedef size_t result_type;
    template <typename T,
              typename = std::enable_if_t<std::is_fundamental<T>::value> >
      uint8_t *operator()(T t) const
    {
      return minimally_encoded_length(t);
    }
    template <typename T,
              typename = std::enable_if_t<!std::is_fundamental<T>::value> >
      uint8_t *operator()(const T &t) const
    {
      return minimally_encoded_length(t);
    }
  };


  struct Unit {
    struct EOC {};

    Klasse klasse;
    Shape shape;
    bool is_long_tag;
    bool is_indefinite;
    bool is_long_definite;
    uint8_t t_size;
    uint8_t tl_size;
    Tag_Int tag;
    size_t length;

    bool is_eoc() const;

    const uint8_t *read(const uint8_t *begin, const uint8_t *end);
    uint8_t *write(uint8_t *begin, uint8_t *end) const;


    void init_tag(Tag_Int tag);
    void init_indefinite();
    void init_length();
    void init_length(size_t length);
    void init_l_size(uint8_t new_l_size);
    void init_constructed_from(Tag_Int tag);
    void init_constructed_from(Tag_Int tag, size_t length);
    template <typename T> void init_from(Tag_Int  tag, const T &t)
    {
      init_tag(tag);
      init_from(t);
    }
    template <typename T> void init_from(const T &t)
    {
      shape = Shape::PRIMITIVE;
      init_length(minimally_encoded_length(t));
    }

    Unit();
    Unit(Tag_Int tag);
    Unit(Klasse klasse, Tag_Int tag, size_t length);
    Unit(EOC);

    template <typename T> Unit(Tag_Int tag, const T &t)
      :
        klasse(Klasse::UNIVERSAL)
    {
      init_from(tag, t);
    }


  };

  struct TLC : public Unit {
    const uint8_t *begin {nullptr};

    const uint8_t *read(const uint8_t *begin, const uint8_t *end);
    uint8_t *write(uint8_t *begin, uint8_t *end) const;

    template <typename T> void copy_content(T &t) const
    {
      if (shape == Shape::CONSTRUCTED)
        throw std::range_error("Cannot copy from CONSTRUCTED tag");
      decode(begin + tl_size, length, t);
    }
    template <typename T> T lexical_cast()
    {
      T t;
      copy_content<T>(t);
      return t;
    }
  };

  struct Parse_Error : public std::exception {
    using std::exception::exception;
  };

  struct TL_Too_Small : public Parse_Error {
    using std::exception::exception;
    const char *what() const noexcept override;
  };

  struct Unexpected_EOC : public Parse_Error {
    using std::exception::exception;
    const char *what() const noexcept override;
  };

  struct Tag_Too_Long : public Parse_Error {
    using std::exception::exception;
    const char *what() const noexcept override;
  };

  struct Vertical_TLC : public TLC {
    public:
      struct Frame {
        size_t length;
        size_t expected_length;
        bool indefinite;
      };
      std::vector<Frame> stack_;
      uint32_t height {0};

      Vertical_TLC();
      const uint8_t *read(const uint8_t *begin, const uint8_t *end);
      const uint8_t *skip(const uint8_t *begin,
          const uint8_t *end);
      const uint8_t *skip_children(const uint8_t *begin, const uint8_t *end);

      uint32_t depth_ {0};

    private:
      void push();
      void pop();
      void conditional_pop();
  };


  struct TLV : public Unit {
    private:
      Value value_;
    public:
      const Value &value()
      {
        return value_;
      }

    inline uint8_t *write(uint8_t *begin, uint8_t *end) const
    {
      if (size_t(end-begin) < tl_size + length)
        throw std::overflow_error("write: value too large");
      auto o = Unit::write(begin, end);
      if (shape == Shape::PRIMITIVE && length)
        o = value_.accept(Encode_Visitor(o, length));
      return o;
    }

    using Unit::Unit;

    TLV()
    {
    }
    TLV(Tag_Int tag)
    {
      Unit::init_tag(tag);
    }
    template <typename T> TLV(Tag_Int tag, const T &t)
      :
        Unit(tag, t),
        value_(t)
    {
      static_assert(std::is_fundamental<T>(),
          "non-fundamental types should be moved");
      // length is correctly initialized
    }
    template <typename T> TLV(Tag_Int tag, T &&t)
      :
        Unit(tag, t),
        value_(std::move(t))
    {
      // length is correctly initialized
    }
    template <typename T> TLV &operator=(const T &t)
    {
      Unit::init_from(t);
      // length is correctly initialized
      value_ = t;
      return *this;
    }
    template <typename T> TLV &operator=(T &&t)
    {
      Unit::init_from(t);
      // length is correctly initialized
      value_ = std::move(t);
      return *this;
    }

  };



  template <typename T>
  class Basic_Reader {
    protected:
      const uint8_t *begin_;
      const uint8_t *end_;
      uint32_t skip_zero_ {0};
    public:
      Basic_Reader(const uint8_t *begin, const uint8_t *end);

      class iterator {
        protected:
          const uint8_t *first_;
          const uint8_t *current_;
          const uint8_t *begin_;
          const uint8_t *end_;
          T tlc_;
        public:
          iterator(const uint8_t *begin, const uint8_t *end, uint32_t skip_zero_);

          const T &operator*() const;
          T &operator*();
          iterator &operator++();
          bool operator==(const iterator &other) const;
          bool operator!=(const iterator &other) const;
        private:
          uint32_t skip_zero_ {0};
      };

      iterator begin();
      iterator end();

      void set_skip_zero(uint32_t b) { skip_zero_ = b; }
  };

  using Reader = Basic_Reader<TLC>;
  using Vertical_Reader = Basic_Reader<Vertical_TLC>;

  class Skip_EOC_Reader : public Vertical_Reader {
    public:
      using Vertical_Reader::Vertical_Reader;

      class iterator : public Vertical_Reader::iterator {
        public:
          using Vertical_Reader::iterator::iterator;

          iterator &operator++();
      };

      iterator begin();
      iterator end();
  };



} // xfsx



#endif
