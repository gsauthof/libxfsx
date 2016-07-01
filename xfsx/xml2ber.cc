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
#include "xml2ber.hh"

#include "xml.hh"
#include "xfsx.hh"
#include "ber_writer.hh"

#include <stdlib.h>
#include <stack>
#include <algorithm>



using namespace std;

namespace xfsx {

  namespace xml {


    class BER_Writer : public BER_Writer_Base {
      private:
      protected:
        Element_Traverser t;
        std::pair<const char*, const char*> e;
        std::pair<const char*, const char*> name;

        void ignore_xml_decl();
        void read_tag();
        void read_attributes();
        void write_primitive_tag();
        void write_constructed_tag();
      public:
        BER_Writer(const char *begin, const char *end,
            const BER_Writer_Arguments &args
            );
        void write();
        void write(const string &filename);
        void write(std::vector<uint8_t> &v);
    };

    BER_Writer::BER_Writer(const char *begin, const char *end,
        const BER_Writer_Arguments &args
        )
      :
        BER_Writer_Base(args),
        t(begin, end)
    {
    }

    void BER_Writer::ignore_xml_decl()
    {
      if (t.has_more()) {
        if (*(*t).first == '?')
          ++t;
      }
    }

    void BER_Writer::write()
    {
      ignore_xml_decl();
      for (; t.has_more(); ++t) {
        e = *t;
        tlv = TLV();
        if (xml::is_end_tag(e)) {
          write_end_tag();
          continue;
        }
        read_tag();
        read_attributes();
        switch (tlv.shape) {
          case Shape::PRIMITIVE:
            write_primitive_tag();
            break;
          case Shape::CONSTRUCTED:
            write_constructed_tag();
            break;
        }
      }
    }

    void BER_Writer::write(const std::string &filename)
    {
      write();
      store(filename);
    }

    void BER_Writer::write(std::vector<uint8_t> &v)
    {
      write();
      store(v);
    }


    void BER_Writer::read_tag()
    {
      name = xml::element_name(e);
      BER_Writer_Base::read_tag(name);
    }

    void BER_Writer::read_attributes()
    {
      Attribute_Traverser at(e, name);
      for (; at.has_more(); ++at) {
        read_attribute(at.name(), at.value());
      }
    }


    void BER_Writer::write_primitive_tag()
    {
      if (xml::is_start_end_tag(e)) {
        std::pair<const char*, const char*> empty_content(e.second, e.second);
        BER_Writer_Base::write_primitive_tag(empty_content);
      } else {
        auto p = *t;
        ++t;
        if (!t.has_more())
          throw runtime_error("document can't end with a primitive tag");
        BER_Writer_Base::write_primitive_tag(xml::content(p, *t));
      }
    }


    void BER_Writer::write_constructed_tag()
    {
      BER_Writer_Base::write_constructed_tag(xml::is_start_end_tag(e));
    }


    void write_ber(const char *begin, const char *end,
        const std::string &filename,
        const BER_Writer_Arguments &args
        )
    {
      BER_Writer w(begin, end, args);
      w.write(filename);
    }

    void write_ber(const char *begin, const char *end,
        vector<uint8_t> &v,
        const BER_Writer_Arguments &args
        )
    {
      BER_Writer w(begin, end, args);
      w.write(v);
    }




  }

}
