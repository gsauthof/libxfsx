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
#include "ber2lxml.hh"

#include <stack>
#include <cassert>

#include "xfsx.hh"
#include "byte.hh"
#include "hex.hh"

#include <xxxml/xxxml.hh>

#include "xml_writer_arguments.hh"

using namespace std;

namespace xfsx {

  namespace xml {

    namespace l2 {


      class Tree_Generator {
        private:
          const Pretty_Writer_Arguments &args_;
          Skip_EOC_Reader reader_;
          stack<xmlNode*> node_stack_;
          stack<size_t> rank_stack_;
          TLC *tlc {nullptr};
          xxxml::doc::Ptr doc_;
          xfsx::BCD_String bcd_;
          byte::writer::Memory memw_;


          void pop_until(size_t h);
          void gen_node();
          void gen_rank(xmlNode *node);
          void gen_hex(xmlNode *node);
          void gen_indefinite(xmlNode *node);
          void gen_primitive();
          void gen_constructed();
          void pop();
        public:
          Tree_Generator(
              const xfsx::u8 *begin, const xfsx::u8 *end,
              xxxml::doc::Ptr &&doc,
              const Pretty_Writer_Arguments &args
              );
          xxxml::doc::Ptr generate();
      };

      Tree_Generator::Tree_Generator(
          const xfsx::u8 *begin, const xfsx::u8 *end,
          xxxml::doc::Ptr &&doc,
          const Pretty_Writer_Arguments &args)
        :
          args_(args),
          reader_(begin + args.skip, end),
          doc_(std::move(doc))
      {
        rank_stack_.push(0);
      }

      void Tree_Generator::gen_node()
      {
        assert(!rank_stack_.empty());
        rank_stack_.top() += 1;
        if (tlc->shape == Shape::PRIMITIVE) {
          gen_primitive();
        } else {
          gen_constructed();
        }
      }

      void Tree_Generator::gen_rank(xmlNode *node)
      {
        if (!args_.dump_rank)
          return;
        memw_.clear();
        memw_ << rank_stack_.top() << '\0';
        xxxml::new_prop(node, "rank", memw_.begin());
      }

      void Tree_Generator::gen_indefinite(xmlNode *node)
      {
        if (!args_.dump_indefinite || !tlc->is_indefinite)
          return;
        xxxml::new_prop(node, "definite", "false");
      }

      void Tree_Generator::gen_hex(xmlNode *node)
      {
        if (!args_.hex_dump)
          return;
        memw_.clear();
        size_t n = hex::decoded_size<hex::Style::Raw>(
            tlc->begin + tlc->tl_size, tlc->begin + tlc->tl_size + tlc->length
            );
        auto o = memw_.obtain_chunk(n);
        auto r = hex::decode<hex::Style::Raw>(
            tlc->begin + tlc->tl_size, tlc->begin + tlc->tl_size + tlc->length,
            o);
        (void)r;
        memw_ << '\0';
        xxxml::new_prop(node, "hex", memw_.begin());
      }

      void Tree_Generator::gen_primitive()
      {
        if (node_stack_.empty())
          throw runtime_error("no parent available for primitive tag");
        auto kt = args_.dereferencer.dereference(tlc->klasse, tlc->tag);
        Type t = args_.typifier.typify(kt);
        const string &name = args_.translator.translate(tlc->klasse, tlc->tag);

        xmlNode *node = xxxml::new_doc_node(doc_, name);
        xxxml::add_child(node_stack_.top(), node);
        gen_rank(node);
        gen_hex(node);

        switch (t) {
          case Type::INT_64:
            {
              int64_t v {0};
              xfsx::decode(tlc->begin + tlc->tl_size, tlc->length, v);
              memw_.clear();
              memw_ << v;
              xxxml::node_add_content(node,
                  memw_.begin(), memw_.written());
            }
            break;
          case Type::BCD:
            {
              xfsx::decode(tlc->begin + tlc->tl_size, tlc->length, bcd_);
              xxxml::node_add_content(node,
                  bcd_.get().data(), bcd_.get().size());
            }
            break;
          case Type::STRING:
          case Type::OCTET_STRING:
              xxxml::node_add_content(node,
                  reinterpret_cast<const char*>(tlc->begin) + tlc->tl_size,
                  tlc->length);
            break;
        }
      }

      void Tree_Generator::gen_constructed()
      {
        const string &name = args_.translator.translate(tlc->klasse, tlc->tag);
        xmlNode *node = nullptr;
        if (node_stack_.empty()) {
          node = xxxml::new_doc_node(doc_, name);
          xxxml::doc::set_root_element(doc_, node);
        } else {
          node = xxxml::new_child(node_stack_.top(), name);
        }
        gen_rank(node);
        gen_indefinite(node);
        if (tlc->is_indefinite || tlc->length) {
          node_stack_.push(node);
          rank_stack_.push(0);
        }
      }

      xxxml::doc::Ptr Tree_Generator::generate()
      {
        size_t i = 0;
        for (auto &tlc : reader_) {
          if (args_.count && !(i<args_.count))
            break;
          this->tlc = &tlc;
          pop_until(tlc.height);
          gen_node();
          if ((args_.skip || args_.stop_after_first) && !tlc.depth_)
            break;
          ++i;
        }
        return std::move(doc_);
      }

      void Tree_Generator::pop_until(size_t h)
      {
        while (h < node_stack_.size()) {
          pop();
        }
      }

      void Tree_Generator::pop()
      {
        node_stack_.pop();
        rank_stack_.pop();
      }

      xxxml::doc::Ptr generate_tree(
          const xfsx::u8 *begin,
          const xfsx::u8 *end,
          const Pretty_Writer_Arguments &args)
      {
        xxxml::doc::Ptr doc = xxxml::new_doc();
        xxxml::dict::Ptr dictionary = xxxml::dict::create();
        doc->dict = dictionary.release();

        Tree_Generator g(begin, end, std::move(doc), args);
        return g.generate();
      }


    }

  }

}
