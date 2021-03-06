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
#include "string.hh"
#include "scratchpad.hh"
#include "tlc_reader.hh"

#include <xxxml/xxxml.hh>

#include "xml_writer_arguments.hh"


using namespace std;

namespace xfsx {

  namespace xml {

    namespace l2 {


      class Tree_Generator {
        private:
          const Pretty_Writer_Arguments &args_;
          stack<xmlNode*> node_stack_;
          stack<size_t> rank_stack_;
          TLC tlc;
          xxxml::doc::Ptr doc_;
          xfsx::BCD_String bcd_;
          scratchpad::Simple_Writer<char> memw_;

            // lengths of all all definite constructed tags
            stack<size_t> length_stack_;
            // counts against the same position in the length_stack_
            // to detect when a tag is 'closed'
            stack<size_t> written_stack_;

          void gen_node();
          void gen_rank(xmlNode *node);
          void gen_hex(xmlNode *node);
          void gen_indefinite(xmlNode *node);
          void gen_primitive();
          void gen_constructed();
          void pop();
        public:
          Tree_Generator(
              xxxml::doc::Ptr &&doc,
              const Pretty_Writer_Arguments &args
              );
          xxxml::doc::Ptr generate(scratchpad::Simple_Reader<u8> &r);
      };

      Tree_Generator::Tree_Generator(
          xxxml::doc::Ptr &&doc,
          const Pretty_Writer_Arguments &args)
        :
          args_(args),
          doc_(std::move(doc)),
          memw_(unique_ptr<scratchpad::Writer<char>>(
                    new scratchpad::Scratchpad_Writer<char>()))
      {
        rank_stack_.push(0);
        length_stack_.push(0); // symmetric to catch-all
        written_stack_.push(0); // catch-all
      }

      void Tree_Generator::gen_node()
      {
        assert(!rank_stack_.empty());
        rank_stack_.top() += 1;
        if (tlc.shape == Shape::PRIMITIVE) {
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
        byte::writer::Base w(memw_);
        w << rank_stack_.top() << '\0';
        memw_.flush();
        const auto &pad = dynamic_cast<scratchpad::Scratchpad_Writer<char>*>(
                memw_.backend())->pad();
        xxxml::new_prop(node, "rank", pad.prelude());
      }

      void Tree_Generator::gen_indefinite(xmlNode *node)
      {
        if (!args_.dump_indefinite || !tlc.is_indefinite)
          return;
        xxxml::new_prop(node, "definite", "false");
      }

      void Tree_Generator::gen_hex(xmlNode *node)
      {
        if (!args_.hex_dump)
          return;
        memw_.clear();
        byte::writer::Base w(memw_);
        size_t n = hex::decoded_size<hex::Style::Raw>(
            tlc.begin + tlc.tl_size, tlc.begin + tlc.tl_size + tlc.length
            );
        auto o = memw_.begin_write(n);
        auto r = hex::decode<hex::Style::Raw>(
            tlc.begin + tlc.tl_size, tlc.begin + tlc.tl_size + tlc.length,
            o);
        (void)r;
        memw_.commit_write(n);
        w << '\0';
        memw_.flush();
        const auto &pad = dynamic_cast<scratchpad::Scratchpad_Writer<char>*>(
                memw_.backend())->pad();
        xxxml::new_prop(node, "hex", pad.prelude());
      }

      void Tree_Generator::gen_primitive()
      {
        if (node_stack_.empty())
          throw runtime_error("no parent available for primitive tag");
        auto kt = args_.dereferencer.dereference(tlc.klasse, tlc.tag);
        Type t = args_.typifier.typify(kt);
        const string &name = args_.translator.translate(tlc.klasse, tlc.tag);

        xmlNode *node = xxxml::new_doc_node(doc_, name);
        xxxml::add_child(node_stack_.top(), node);
        gen_rank(node);
        gen_hex(node);

        switch (t) {
          case Type::INT_64:
            {
              int64_t v {0};
              xfsx::decode(tlc.begin + tlc.tl_size, tlc.length, v);
              memw_.clear();
              byte::writer::Base w(memw_);
              w << v;
              memw_.flush();
                const auto &pad = dynamic_cast<scratchpad::Scratchpad_Writer<char>*>(
                        memw_.backend())->pad();
              xxxml::node_add_content(node,
                  pad.prelude(), pad.begin()-pad.prelude());
            }
            break;
          case Type::BCD:
            {
              xfsx::decode(tlc.begin + tlc.tl_size, tlc.length, bcd_);
              xxxml::node_add_content(node,
                  bcd_.get().data(), bcd_.get().size());
            }
            break;
          case Type::STRING:
          case Type::OCTET_STRING:
              xxxml::node_add_content(node,
                  reinterpret_cast<const char*>(tlc.begin) + tlc.tl_size,
                  tlc.length);
            break;
        }
      }

      void Tree_Generator::gen_constructed()
      {
        const string &name = args_.translator.translate(tlc.klasse, tlc.tag);
        xmlNode *node = nullptr;
        if (node_stack_.empty()) {
          node = xxxml::new_doc_node(doc_, name);
          auto t = xxxml::doc::set_root_element(doc_, node);
          if (t) {
              xmlFreeNode(t);
              throw runtime_error("multiple roots aren't supported with lxml");
          }
        } else {
          node = xxxml::new_child(node_stack_.top(), name);
        }
        gen_rank(node);
        gen_indefinite(node);
        if (tlc.is_indefinite || tlc.length) {
          node_stack_.push(node);
          rank_stack_.push(0);
        }
      }

      xxxml::doc::Ptr Tree_Generator::generate(scratchpad::Simple_Reader<u8> &r)
      {
          size_t i = 0;
          while (read_next(r, tlc)) {
              if (args_.count && !(i<args_.count))
                  break;
              written_stack_.top() += tlc.tl_size;
              if (tlc.shape == Shape::PRIMITIVE) {
                  written_stack_.top() += tlc.length;
              } else if (!tlc.is_indefinite && tlc.length) {
                  length_stack_.push(tlc.length);
                  written_stack_.push(0);
              }
              if (tlc.is_eoc()) {
                  pop();
              } else {
                  gen_node();
              }
              while (!length_stack_.empty()
                      && length_stack_.top() == written_stack_.top()) {
                  pop();
                  auto t = length_stack_.top();
                  length_stack_.pop();
                  written_stack_.pop();
                  written_stack_.top() += t;
              }
              if (args_.stop_after_first && node_stack_.empty())
                  break;
              ++i;
          }
          return std::move(doc_);
      }

      void Tree_Generator::pop()
      {
          if (node_stack_.empty())
              throw xfsx::Unexpected_EOC();
        node_stack_.pop();
        assert(rank_stack_.size() > 1);
        rank_stack_.pop();
      }

      xxxml::doc::Ptr generate_tree(
              scratchpad::Simple_Reader<u8> &r,
          const Pretty_Writer_Arguments &args)
      {
        xxxml::doc::Ptr doc = xxxml::new_doc();
        xxxml::dict::Ptr dictionary = xxxml::dict::create();
        doc->dict = dictionary.release();

        Tree_Generator g(std::move(doc), args);
        if (args.skip) {
            r.next(args.skip);
            r.check_available(args.skip);
            r.forget(args.skip);
        }
        return g.generate(r);
      }

      xxxml::doc::Ptr generate_tree(
          const xfsx::u8 *begin,
          const xfsx::u8 *end,
          const Pretty_Writer_Arguments &args)
      {
        auto r = scratchpad::mk_simple_reader(begin, end);
        return generate_tree(r, args);
      }


    }

  }

}
