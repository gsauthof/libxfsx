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
/**************************************************************************/
/*
   Autogenerated by the Variant Generator.

   Call was:

     '../variant-generator/build/mkvariant' 'value.inf' '--output' 'xfsx/value.cc'
                                                                          */
/**************************************************************************/
#include "value.hh"


namespace xfsx {

Value::Value()
{
}

Value::Value(Value &&o)
{
  switch(o.tag_) {
    case 1:
      uint8_t_ = o.uint8_t_;
      tag_ = 1;
      break;
    case 2:
      int8_t_ = o.int8_t_;
      tag_ = 2;
      break;
    case 3:
      uint16_t_ = o.uint16_t_;
      tag_ = 3;
      break;
    case 4:
      int16_t_ = o.int16_t_;
      tag_ = 4;
      break;
    case 5:
      uint32_t_ = o.uint32_t_;
      tag_ = 5;
      break;
    case 6:
      int32_t_ = o.int32_t_;
      tag_ = 6;
      break;
    case 7:
      uint64_t_ = o.uint64_t_;
      tag_ = 7;
      break;
    case 8:
      int64_t_ = o.int64_t_;
      tag_ = 8;
      break;
    case 9:
      bool_ = o.bool_;
      tag_ = 9;
      break;
    case 10:
      new (&std_string_) std::string(std::move(o.std_string_));
      tag_ = 10;
      break;
    case 11:
      new (&std_pair_const_uint8_t_const_uint8_t__) std::pair<const uint8_t*, const uint8_t*>(std::move(o.std_pair_const_uint8_t_const_uint8_t__));
      tag_ = 11;
      break;
    case 12:
      new (&std_pair_const_char_const_char__) std::pair<const char*, const char*>(std::move(o.std_pair_const_char_const_char__));
      tag_ = 12;
      break;
    case 13:
      new (&xml_content_) XML_Content(std::move(o.xml_content_));
      tag_ = 13;
      break;
    case 14:
      new (&bcd_content_) BCD_Content(std::move(o.bcd_content_));
      tag_ = 14;
      break;
    case 15:
      new (&int64_content_) Int64_Content(std::move(o.int64_content_));
      tag_ = 15;
      break;
  }

}

Value::~Value()
{
  destruct();
}

void Value::destruct()
{
  switch(tag_) {
    case 10: std_string_.~basic_string(); break;
    case 11: std_pair_const_uint8_t_const_uint8_t__.~pair<const uint8_t*, const uint8_t*>(); break;
    case 12: std_pair_const_char_const_char__.~pair<const char*, const char*>(); break;
    case 13: xml_content_.~XML_Content(); break;
    case 14: bcd_content_.~BCD_Content(); break;
    case 15: int64_content_.~Int64_Content(); break;
  }
  tag_ = 0;
}

Value &Value::operator=(Value &&o)
{
  switch(o.tag_) {
    case 1:
      if (tag_ == o.tag_) {
        uint8_t_ = o.uint8_t_;
      } else {
        destruct();
        uint8_t_ = o.uint8_t_;
        tag_ = 1;
      }
      break;
    case 2:
      if (tag_ == o.tag_) {
        int8_t_ = o.int8_t_;
      } else {
        destruct();
        int8_t_ = o.int8_t_;
        tag_ = 2;
      }
      break;
    case 3:
      if (tag_ == o.tag_) {
        uint16_t_ = o.uint16_t_;
      } else {
        destruct();
        uint16_t_ = o.uint16_t_;
        tag_ = 3;
      }
      break;
    case 4:
      if (tag_ == o.tag_) {
        int16_t_ = o.int16_t_;
      } else {
        destruct();
        int16_t_ = o.int16_t_;
        tag_ = 4;
      }
      break;
    case 5:
      if (tag_ == o.tag_) {
        uint32_t_ = o.uint32_t_;
      } else {
        destruct();
        uint32_t_ = o.uint32_t_;
        tag_ = 5;
      }
      break;
    case 6:
      if (tag_ == o.tag_) {
        int32_t_ = o.int32_t_;
      } else {
        destruct();
        int32_t_ = o.int32_t_;
        tag_ = 6;
      }
      break;
    case 7:
      if (tag_ == o.tag_) {
        uint64_t_ = o.uint64_t_;
      } else {
        destruct();
        uint64_t_ = o.uint64_t_;
        tag_ = 7;
      }
      break;
    case 8:
      if (tag_ == o.tag_) {
        int64_t_ = o.int64_t_;
      } else {
        destruct();
        int64_t_ = o.int64_t_;
        tag_ = 8;
      }
      break;
    case 9:
      if (tag_ == o.tag_) {
        bool_ = o.bool_;
      } else {
        destruct();
        bool_ = o.bool_;
        tag_ = 9;
      }
      break;
    case 10:
      if (tag_ == o.tag_) {
        std_string_ = std::move(o.std_string_);
      } else {
        destruct();
        new (&std_string_) std::string(std::move(o.std_string_));
        tag_ = 10;
      }
      break;
    case 11:
      if (tag_ == o.tag_) {
        std_pair_const_uint8_t_const_uint8_t__ = std::move(o.std_pair_const_uint8_t_const_uint8_t__);
      } else {
        destruct();
        new (&std_pair_const_uint8_t_const_uint8_t__) std::pair<const uint8_t*, const uint8_t*>(std::move(o.std_pair_const_uint8_t_const_uint8_t__));
        tag_ = 11;
      }
      break;
    case 12:
      if (tag_ == o.tag_) {
        std_pair_const_char_const_char__ = std::move(o.std_pair_const_char_const_char__);
      } else {
        destruct();
        new (&std_pair_const_char_const_char__) std::pair<const char*, const char*>(std::move(o.std_pair_const_char_const_char__));
        tag_ = 12;
      }
      break;
    case 13:
      if (tag_ == o.tag_) {
        xml_content_ = std::move(o.xml_content_);
      } else {
        destruct();
        new (&xml_content_) XML_Content(std::move(o.xml_content_));
        tag_ = 13;
      }
      break;
    case 14:
      if (tag_ == o.tag_) {
        bcd_content_ = std::move(o.bcd_content_);
      } else {
        destruct();
        new (&bcd_content_) BCD_Content(std::move(o.bcd_content_));
        tag_ = 14;
      }
      break;
    case 15:
      if (tag_ == o.tag_) {
        int64_content_ = std::move(o.int64_content_);
      } else {
        destruct();
        new (&int64_content_) Int64_Content(std::move(o.int64_content_));
        tag_ = 15;
      }
      break;
  }
  return *this;
}

Value::Value(uint8_t o)
  : uint8_t_(o)
{
  tag_ = 1;
}

Value::Value(int8_t o)
  : int8_t_(o)
{
  tag_ = 2;
}

Value::Value(uint16_t o)
  : uint16_t_(o)
{
  tag_ = 3;
}

Value::Value(int16_t o)
  : int16_t_(o)
{
  tag_ = 4;
}

Value::Value(uint32_t o)
  : uint32_t_(o)
{
  tag_ = 5;
}

Value::Value(int32_t o)
  : int32_t_(o)
{
  tag_ = 6;
}

Value::Value(uint64_t o)
  : uint64_t_(o)
{
  tag_ = 7;
}

Value::Value(int64_t o)
  : int64_t_(o)
{
  tag_ = 8;
}

Value::Value(bool o)
  : bool_(o)
{
  tag_ = 9;
}

Value::Value(std::string &&o)
  : std_string_(std::move(o))
{
  tag_ = 10;
}

Value::Value(std::pair<const uint8_t*, const uint8_t*> &&o)
  : std_pair_const_uint8_t_const_uint8_t__(std::move(o))
{
  tag_ = 11;
}

Value::Value(std::pair<const char*, const char*> &&o)
  : std_pair_const_char_const_char__(std::move(o))
{
  tag_ = 12;
}

Value::Value(XML_Content &&o)
  : xml_content_(std::move(o))
{
  tag_ = 13;
}

Value::Value(BCD_Content &&o)
  : bcd_content_(std::move(o))
{
  tag_ = 14;
}

Value::Value(Int64_Content &&o)
  : int64_content_(std::move(o))
{
  tag_ = 15;
}

Value &Value::operator=(uint8_t o)
{
  if (tag_ == 1) {
    uint8_t_ = o;
  } else {
    destruct();
    uint8_t_ = o;
    tag_ = 1;
  }
  return *this;
}

Value &Value::operator=(int8_t o)
{
  if (tag_ == 2) {
    int8_t_ = o;
  } else {
    destruct();
    int8_t_ = o;
    tag_ = 2;
  }
  return *this;
}

Value &Value::operator=(uint16_t o)
{
  if (tag_ == 3) {
    uint16_t_ = o;
  } else {
    destruct();
    uint16_t_ = o;
    tag_ = 3;
  }
  return *this;
}

Value &Value::operator=(int16_t o)
{
  if (tag_ == 4) {
    int16_t_ = o;
  } else {
    destruct();
    int16_t_ = o;
    tag_ = 4;
  }
  return *this;
}

Value &Value::operator=(uint32_t o)
{
  if (tag_ == 5) {
    uint32_t_ = o;
  } else {
    destruct();
    uint32_t_ = o;
    tag_ = 5;
  }
  return *this;
}

Value &Value::operator=(int32_t o)
{
  if (tag_ == 6) {
    int32_t_ = o;
  } else {
    destruct();
    int32_t_ = o;
    tag_ = 6;
  }
  return *this;
}

Value &Value::operator=(uint64_t o)
{
  if (tag_ == 7) {
    uint64_t_ = o;
  } else {
    destruct();
    uint64_t_ = o;
    tag_ = 7;
  }
  return *this;
}

Value &Value::operator=(int64_t o)
{
  if (tag_ == 8) {
    int64_t_ = o;
  } else {
    destruct();
    int64_t_ = o;
    tag_ = 8;
  }
  return *this;
}

Value &Value::operator=(bool o)
{
  if (tag_ == 9) {
    bool_ = o;
  } else {
    destruct();
    bool_ = o;
    tag_ = 9;
  }
  return *this;
}

Value &Value::operator=(std::string &&o)
{
  if (tag_ == 10) {
    std_string_ = std::move(o);
  } else {
    destruct();
    new (&std_string_) std::string(std::move(o));
    tag_ = 10;
  }
  return *this;
}

Value &Value::operator=(std::pair<const uint8_t*, const uint8_t*> &&o)
{
  if (tag_ == 11) {
    std_pair_const_uint8_t_const_uint8_t__ = std::move(o);
  } else {
    destruct();
    new (&std_pair_const_uint8_t_const_uint8_t__) std::pair<const uint8_t*, const uint8_t*>(std::move(o));
    tag_ = 11;
  }
  return *this;
}

Value &Value::operator=(std::pair<const char*, const char*> &&o)
{
  if (tag_ == 12) {
    std_pair_const_char_const_char__ = std::move(o);
  } else {
    destruct();
    new (&std_pair_const_char_const_char__) std::pair<const char*, const char*>(std::move(o));
    tag_ = 12;
  }
  return *this;
}

Value &Value::operator=(XML_Content &&o)
{
  if (tag_ == 13) {
    xml_content_ = std::move(o);
  } else {
    destruct();
    new (&xml_content_) XML_Content(std::move(o));
    tag_ = 13;
  }
  return *this;
}

Value &Value::operator=(BCD_Content &&o)
{
  if (tag_ == 14) {
    bcd_content_ = std::move(o);
  } else {
    destruct();
    new (&bcd_content_) BCD_Content(std::move(o));
    tag_ = 14;
  }
  return *this;
}

Value &Value::operator=(Int64_Content &&o)
{
  if (tag_ == 15) {
    int64_content_ = std::move(o);
  } else {
    destruct();
    new (&int64_content_) Int64_Content(std::move(o));
    tag_ = 15;
  }
  return *this;
}

} // xfsx

