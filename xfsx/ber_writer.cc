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
#include "ber_writer.hh"

#include "xfsx.hh"
#include "integer.hh"
#include "s_pair.hh"

#include <ixxx/util.hh>

using namespace std;

namespace xfsx {




    BER_Writer_Base::BER_Writer_Base(const BER_Writer_Arguments &args)
      :
        args_(args)
    {
      TLV tlv_root;
      tlv_root.shape = Shape::CONSTRUCTED;
      root = ber::TLV_Node(std::move(tlv_root));
      node_stack.push(&root);
    }

    void BER_Writer_Base::write_end_tag()
    {
      if (node_stack.size() < 2)
        throw runtime_error("document is not well formed");
      auto old_length = node_stack.top()->length();
      auto l = node_stack.top()->init_length();
      bool was_indefinite = node_stack.top()->is_indefinite();
      // XXX recognize when all ancestors are indefinite -> write directly
      node_stack.top()->mk_vector(old_length);
      node_stack.pop();
      node_stack.top()->add_to_length(l);
      if (was_indefinite) {
        TLV eoc{Unit::EOC()};
        node_stack.top()->push(make_unique<ber::TLV_Node>(std::move(eoc)));
      }
    }

    void BER_Writer_Base::read_tag(const std::pair<const char*, const char*> &name)
    {
      l_size_ = 0;
      uint2int_ = false;
      klasse_present = false;
      tag_present = false;
      symbol_present_ = false;
      if (!args_.translator.empty()) {
        try {
          auto shape_klasse_tag = args_.translator.translate(name);
          tlv.shape = std::get<0>(shape_klasse_tag);
          tlv.klasse = std::get<1>(shape_klasse_tag);
          tlv.init_tag(std::get<2>(shape_klasse_tag));
          tag_present = true;
          symbol_present_ = true;
          return;
        } catch (const std::out_of_range &e) {
        }
      }
      if (s_pair::size(name) == 1) {
        switch (*name.first) {
          case 'i': tlv.shape = Shape::CONSTRUCTED;
                    tlv.init_indefinite();
                    break;
          case 'c': tlv.shape = Shape::CONSTRUCTED; break;
          case 'p': tlv.shape = Shape::PRIMITIVE; break;
          default:
            throw runtime_error("Unknown tag name: " + s_pair::mk_string(name));
        }
      } else {
        throw runtime_error("Unknown tag name: " + s_pair::mk_string(name));
      }
    }

    void BER_Writer_Base::read_attribute(
        const std::pair<const char*, const char*> &name,
        const std::pair<const char*, const char*> &value)
    {
      if (s_pair::equal(name, "tag", 3)) {
        tlv.init_tag(integer::range_to_uint32(value));
        tag_present = true;
      } else if (s_pair::equal(name, "class", 5)) {
        tlv.klasse = str_to_klasse(value);
        klasse_present = true;
      } else if (s_pair::equal(name, "indefinite", 10)) {
        if (tlv.shape == Shape::PRIMITIVE)
          throw runtime_error("a primitive tag must not be indefinite");
        if (s_pair::equal(value, "true", 4))
          tlv.init_indefinite();
      } else if (s_pair::equal(name, "definite", 8)) {
        if (tlv.shape == Shape::PRIMITIVE)
          throw runtime_error("a primitive tag must not be indefinite");
        if (s_pair::equal(value, "false", 5))
          tlv.init_indefinite();
      } else if (s_pair::equal(name, "l_size", 6)) {
        l_size_ = integer::range_to_uint32(value);
      } else if (s_pair::equal(name, "uint2int", 8)) {
        uint2int_ = s_pair::equal(value, "true", 4);
      }
    }

    void BER_Writer_Base::write_primitive_tag(
        const std::pair<const char*, const char*> &content_a)
    {
      if (!tag_present)
        throw runtime_error("element is missing mandatory tag attribute");
      auto content = content_a;
      if (!tag_present)
        throw runtime_error("element is missing mandatory tag attribute");
      if (s_pair::empty(content)) {
        tlv = XML_Content();
      } else {
        if (args_.translator.empty() || !symbol_present_)
          tlv = XML_Content(std::move(content));
        else {
          auto kt = args_.dereferencer.dereference(tlv.klasse, tlv.tag);
          Type type = args_.typifier.typify(kt);
          switch (type) {
            case Type::INT_64:
              if (uint2int_) {
                Int64_Content c(std::move(content));
                c.uint_to_int();
                tlv = c;
              } else
                tlv = Int64_Content(std::move(content));
              break;
            case Type::BCD:
              tlv = BCD_Content(std::move(content));
              break;
            case Type::STRING:
            case Type::OCTET_STRING:
              tlv = XML_Content(std::move(content));
              break;
          }
        }
      }
      node_stack.top()->push(make_unique<ber::TLV_Node>(std::move(tlv)));
    }

    void BER_Writer_Base::write_constructed_tag(bool is_empty)
    {
      if (is_empty) {
        if (!tlv.is_indefinite)
          tlv.init_length();
        node_stack.top()->add_to_length(tlv.tl_size);
        node_stack.top()->push(make_unique<ber::TLV_Node>(std::move(tlv)));
        if (tlv.is_indefinite) {
          TLV eoc{Unit::EOC()};
          node_stack.top()->push(make_unique<ber::TLV_Node>(std::move(eoc)));
        }
      } else {
        if (l_size_)
          tlv.tl_size = tlv.t_size + 1 + l_size_;
        node_stack.push(
            node_stack.top()->push(
              make_unique<ber::TLV_Node>(std::move(tlv))));
      }
    }

    size_t BER_Writer_Base::size() const
    {
      return root.length();
    }

    void BER_Writer_Base::store(const string &filename)
    {
      if (node_stack.size() > 1)
        throw runtime_error("some tags still open at the end of the document");

      auto f = ixxx::util::mmap_file(filename, false, true, root.length());
      store(f.begin(), f.end());
    }
    void BER_Writer_Base::store(std::vector<u8> &v)
    {
      if (node_stack.size() > 1)
        throw runtime_error("some tags still open at the end of the document");

      v.resize(root.length());
      store(v.data(), v.data()+v.size());
    }

    void BER_Writer_Base::store(u8 *begin, u8 *end)
    {
      if (node_stack.size() > 1)
        throw runtime_error("some tags still open at the end of the document");

      auto n = root.length();
      if (end-begin < ssize_t(n))
        throw underflow_error("ber_writer_base: destination buffer too small");
      auto p = begin;
      for (auto &child : root.children()) {
        p = child->write(p, end);
      }
      if (p < begin + n)
        throw runtime_error("written less than computed before");
      if (p > begin + n)
        throw runtime_error("written more than computed before");
    }


}
