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

#include "scratchpad.hh"

#include <stdexcept>

using namespace std;

namespace xfsx {

  namespace xml {

    namespace l2 {


      class Reader {
        private:
            xml::Reader &r_;
          xxxml::doc::Ptr doc_;
          size_t count_{0};
          size_t i_{0};
          xmlNode *node_ {nullptr};

          bool  process_tag();

        public:
          Reader(xml::Reader &r, size_t count);
          xxxml::doc::Ptr read();
      };
      Reader::Reader(xml::Reader &r, size_t count)
        :
            r_(r),
          doc_(xxxml::new_doc()),
          count_(count)
      {
        xxxml::dict::Ptr dictionary = xxxml::dict::create();
        doc_->dict = dictionary.release();
      }
      xxxml::doc::Ptr Reader::read()
      {
        i_ = 0;
        if (r_.next()) {
            if (!xml::is_decl(r_.tag()))
                process_tag();
        }
        while (r_.next()) {
             bool b = process_tag();
             if (!b)
                 break;
        }
        return std::move(doc_);
      }
      bool Reader::process_tag()
      {
          auto e = r_.tag();
          if (xml::is_end_tag(e)) {
            if (!node_)
              throw runtime_error("too many closing tags");
            auto c = r_.value();
            xxxml::node_add_content(node_, c.first, s_pair::size(c));
            node_ = node_->parent;
          } else {
            if (count_ && !(i_ < count_))
              return false;
            ++i_;
            auto name = xml::element_name(e);
            if (node_) {
              // XXX eliminate mk_string?
              node_ = xxxml::new_child(node_, s_pair::mk_string(name));
            } else {
              node_ = xxxml::new_doc_node(doc_, s_pair::mk_string(name));
              xxxml::doc::set_root_element(doc_, node_);
            }
            if (xml::is_start_end_tag(e)) {
              node_ = node_->parent;
            }
            // We do not need attributes for format detection
            //Attribute_Traverser at(e, name);
            //for (; at.has_more(); ++at) { ... at.name(); at.value(); ... }
          }
          return true;
      }

      xxxml::doc::Ptr generate_tree(scratchpad::Simple_Reader<char> &in,
          size_t count)
      {
          xml::Reader x(in);
        Reader r(x, count);
        return r.read();
      }
      xxxml::doc::Ptr generate_tree(const char *begin, const char *end,
          size_t count)
      {
          auto in = scratchpad::mk_simple_reader(begin, end);
          return generate_tree(in, count);
      }

    }

  }

}

