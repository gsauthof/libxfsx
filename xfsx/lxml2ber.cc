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
#include "lxml2ber.hh"

#include <stack>

#include "ber_writer.hh"

using namespace std;

namespace xfsx {

  namespace xml {

    namespace l2 {


      BER_Writer::BER_Writer(const xxxml::doc::Ptr &doc,
          const BER_Writer_Arguments &args
          )
        :
          BER_Writer_Base(args),
          doc_(doc)
      {
      }
      void BER_Writer::write()
      {
        stack<const xmlNode*> element_stack;
        const xmlNode *root = xxxml::doc::get_root_element(doc_);
        element_stack.push(root);
        while (!element_stack.empty()) {
          tlv = TLV();
          const xmlNode *e = element_stack.top();
          element_stack.pop();
          if (!e) {
            // since the last null-next pointer of the last child
            // triggers the write_end_tag(), we have to break before the
            // null-next pointer of the root element
            if (element_stack.empty())
              break;
            write_end_tag();
            continue;
          }
          write_element(e);
          element_stack.push(xxxml::next_element_sibling(e));
          const xmlNode *child = xxxml::first_element_child(e);
          if (child) {
            element_stack.push(child);
          }
        }
      }
      void BER_Writer::write(const std::string &filename)
      {
        write();
        store(filename);
      }
      void BER_Writer::write_element(const xmlNode *element)
      {
        pair<const char*, const char*> name(
            s_pair::mk_s_pair(xxxml::name(element)));
        BER_Writer_Base::read_tag(name);
        read_attributes(element);

        switch (tlv.shape) {
          case Shape::PRIMITIVE:
            {
              if (!element->children || element->children->type != XML_TEXT_NODE
                  || element->children != element->last)
                throw logic_error("primitive element must have one text_node");
              pair<const char*, const char*> content(s_pair::mk_s_pair(
                  xxxml::content(element->children)));
              BER_Writer_Base::write_primitive_tag(content);
            }
            break;
          case Shape::CONSTRUCTED:
            BER_Writer_Base::write_constructed_tag(
                !xxxml::first_element_child(element));
            break;
        }
      }
      void BER_Writer::read_attributes(const xmlNode *element)
      {
        for (const xmlAttr *a = element->properties; a; a = a->next) {
          if (!a->children || a->children->type != XML_TEXT_NODE)
            throw logic_error("unexpected attribute child");
          pair<const char*, const char*> name(
              s_pair::mk_s_pair(xxxml::name(a)));
          pair<const char*, const char*> value(s_pair::mk_s_pair(
                xxxml::content(a->children)));
          read_attribute(name, value);
        }
      }

      void write_ber(const xxxml::doc::Ptr &doc,
          const std::string &filename,
          const BER_Writer_Arguments &args)
      {
        BER_Writer w(doc, args);
        w.write(filename);
      }

    }
  }

}
