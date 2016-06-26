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

#include "xml2lxml.hh"
#include "xml.hh"
#include "s_pair.hh"

#include <stdexcept>

using namespace std;

namespace xfsx {

  namespace xml {

    namespace l2 {


      class Reader {
        private:
          xxxml::doc::Ptr doc_;
          Element_Traverser trav_;
          size_t count_;

          void ignore_xml_decl();
        public:
          Reader(const char *begin, const char *end, size_t count);
          xxxml::doc::Ptr read();
      };
      Reader::Reader(const char *begin, const char *end, size_t count)
        :
          doc_(xxxml::new_doc()),
          trav_(begin, end),
          count_(count)
      {
        xxxml::dict::Ptr dictionary = xxxml::dict::create();
        doc_->dict = dictionary.release();
      }
      void Reader::ignore_xml_decl()
      {
        if (trav_.has_more()) {
          if (*(*trav_).first == '?')
            ++trav_;
        }
      }
      xxxml::doc::Ptr Reader::read()
      {
        ignore_xml_decl();
        xmlNode *node = nullptr;
        size_t i = 0;
        pair<const char*, const char*> last(nullptr, nullptr);
        for (; trav_.has_more(); ++trav_) {
          auto e = *trav_;
          if (xml::is_end_tag(e)) {
            if (!node)
              throw runtime_error("too many closing tags");
            auto c = xml::content(last, e);
            xxxml::node_add_content(node, c.first, s_pair::size(c));
            node = node->parent;
          } else {
            if (count_ && !(i < count_))
              break;
            ++i;
            auto name = xml::element_name(e);
            if (node) {
              // XXX eliminate mk_string?
              node = xxxml::new_child(node, s_pair::mk_string(name));
            } else {
              node = xxxml::new_doc_node(doc_, s_pair::mk_string(name));
              xxxml::doc::set_root_element(doc_, node);
            }
            if (xml::is_start_end_tag(e)) {
              node = node->parent;
            }
            // We do not need attributes for format detection
            //Attribute_Traverser at(e, name);
            //for (; at.has_more(); ++at) { ... at.name(); at.value(); ... }
          }
          last = e;
        }
        return std::move(doc_);
      }

      xxxml::doc::Ptr generate_tree(const char *begin, const char *end,
          size_t count)
      {
        Reader r(begin, end, count);
        return r.read();
      }

    }

  }

}

