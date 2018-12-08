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
#include <deque>
#include <utility>
#include <stdexcept>

#include "scratchpad.hh"
#include "xml2lxml.hh"
#include "lxml2ber.hh"
#include "xml_writer_arguments.hh"
#include "tap.hh"
#include "xml2ber.hh"
#include "tlc_writer.hh"


using namespace std;

namespace xfsx {

  namespace xml {

    namespace l2 {

class Lxml2Ber {
    public:
        Lxml2Ber(const xxxml::doc::Ptr &doc,
                scratchpad::Simple_Writer<u8> &out,
                const BER_Writer_Arguments &args
                );

        void process();
    private:
        void write_primitive(const xmlNode *e, TLV &tlv);
        void process_element(const xmlNode *e);
        void process_constructed(const Unit &u);
        void pop_constructed();

        const xxxml::doc::Ptr &doc_;
        xml::Attributes attributes_;
        const BER_Writer_Arguments &args_;

        // top of the stack is the current constructed tag
        // the rest its ancestors - this allows to keep track of
        // definite vs. indefinite
        std::deque<Unit> tlv_stack_;
        size_t tlv_stack_top_{0};

        // writers are reused to avoid unnecessary object reconstructions
        // (thus no std::stack nor down-resize calls)
        // first writer directly writes to a file
        // for each definitive constructed a scratchpad writer is pushed
        std::deque<scratchpad::Simple_Writer<u8>*> writer_stack_;
        std::deque<scratchpad::Simple_Writer<u8>> writers_;
        size_t writer_stack_top_{0};

        std::array<u8, 2> eoc_{{0, 0}};

        size_t inc_{128*1024};
};

Lxml2Ber::Lxml2Ber(const xxxml::doc::Ptr &doc,
        scratchpad::Simple_Writer<u8> &out,
        const BER_Writer_Arguments &args
        )
    :
        doc_(doc),
        args_(args)
{
    writer_stack_.push_back(&out);
    ++writer_stack_top_;
}
static xml::Attributes read_attributes(
    const xmlNode *element,
        TLV &tlv)
{
    xml::Attributes as;
    for (const xmlAttr *a = element->properties; a; a = a->next) {
        if (!a->children || a->children->type != XML_TEXT_NODE)
            throw logic_error("unexpected attribute child");
        pair<const char*, const char*> name(
                s_pair::mk_s_pair(xxxml::name(a)));
        pair<const char*, const char*> value(s_pair::mk_s_pair(
                    xxxml::content(a->children)));
        xml::read_attribute(name, value, tlv, as);
    }
    return as;
}
void Lxml2Ber::process()
{
    stack<const xmlNode*> stack;
    const xmlNode *root = xxxml::doc::get_root_element(doc_);
    stack.push(root);
    while (!stack.empty()) {
        const xmlNode *e = stack.top();
        stack.pop();

        if (e) {
            process_element(e);
            // might be null - signalizes end then - see our else branch
            stack.push(xxxml::next_element_sibling(e));
            const xmlNode *child = xxxml::first_element_child(e);
            if (child)
                stack.push(child);
        } else { // constructed end
            // since the last null-next pointer of the last child
            // triggers the write_end_tag(), we have to break before the
            // null-next pointer of the root element
            if (stack.empty())
                break;
            pop_constructed();
        }
    }
}
void Lxml2Ber::process_element(const xmlNode *e)
{
    TLV tlv;
    pair<const char*, const char*> name(
            s_pair::mk_s_pair(xxxml::name(e)));
    bool full_tag = xml::read_tag(name, tlv, args_);
    attributes_ = read_attributes(e, tlv);
    attributes_.full_tag = full_tag;
    // we have to init_indefinite() after a possible tag change
    if (tlv.is_indefinite)
        tlv.init_indefinite();
    if (attributes_.l_size)
        tlv.init_l_size(attributes_.l_size);
    if (!full_tag && !attributes_.tag_present)
        throw runtime_error("element is missing mandatory tag attribute");

    if (tlv.shape == Shape::PRIMITIVE) {
        write_primitive(e, tlv);
    } else { // constructed
        process_constructed(tlv);
    }
}
void Lxml2Ber::process_constructed(const Unit &tlv)
{
    if (tlv.is_indefinite) {
        write_tag(*writer_stack_[writer_stack_top_-1], tlv);
    } else { // definite
        if (writer_stack_top_ >= writer_stack_.size()) {
            writers_.emplace_back(
                    std::unique_ptr<scratchpad::Writer<u8>>(
                        new scratchpad::Scratchpad_Writer<u8>()
                        )
                    );
            writer_stack_.push_back(&writers_.back());
        }
#ifndef NDEBUG
        else {
            auto b  = dynamic_cast<scratchpad::Scratchpad_Writer<u8>*>(
                    writer_stack_[writer_stack_top_]->backend());
            assert(b->pad().begin() - b->pad().prelude() == 0);
        }
#endif
        ++writer_stack_top_;
    }
    if (tlv_stack_top_ >= tlv_stack_.size())
        tlv_stack_.push_back(std::move(tlv));
    else
        tlv_stack_[tlv_stack_top_] = std::move(tlv);
    ++tlv_stack_top_;
}
void Lxml2Ber::pop_constructed()
{
    assert(tlv_stack_top_);
    Unit &tlv = tlv_stack_[tlv_stack_top_-1];
    if (tlv.is_indefinite) {
        writer_stack_[writer_stack_top_-1]->write(eoc_.begin(), eoc_.end());
    } else {
        --writer_stack_top_;
        assert(tlv_stack_top_);

        size_t n = writer_stack_[writer_stack_top_]->pos();

        auto old_tl_size = tlv.tl_size;
        tlv.init_length(n);
        if (old_tl_size > tlv.tl_size) { // if l_size was specified
            tlv.tl_size = old_tl_size;
            tlv.is_long_definite = true;
        }

        assert(tlv.tl_size);

        write_tag(*writer_stack_[writer_stack_top_-1], tlv);

        writer_stack_[writer_stack_top_]->flush();
        auto &pad = dynamic_cast<scratchpad::Scratchpad_Writer<u8>*>(
                writer_stack_[writer_stack_top_]->backend())->pad();
        auto b = pad.prelude();
        auto e = pad.begin();
        // the Simple_Writer with File_Writer backend now supports
        // write-through with large buffers
        writer_stack_[writer_stack_top_-1]->write(b, e);
        writer_stack_[writer_stack_top_]->clear();
    }
    --tlv_stack_top_;
}
void Lxml2Ber::write_primitive(
        const xmlNode *e,
        TLV &tlv)
{
    if (!e->children || e->children->type != XML_TEXT_NODE
            || e->children != e->last)
        throw logic_error("primitive element must have one text_node");
    pair<const char*, const char*> v(s_pair::mk_s_pair(
                xxxml::content(e->children)));
    xml::add_content(v, tlv, attributes_, args_);
    write_tag(*writer_stack_[writer_stack_top_-1], tlv);
}


      void write_ber(const xxxml::doc::Ptr &doc,
          scratchpad::Simple_Writer<u8> &out,
          const BER_Writer_Arguments &args)
      {
            Lxml2Ber l2b(doc, out, args);
            l2b.process();
      }
      void write_ber(const xxxml::doc::Ptr &doc,
          const std::string &filename,
          const BER_Writer_Arguments &args)
      {
          auto out = scratchpad::mk_simple_writer<u8>(filename);
          write_ber(doc, out, args);
          out.flush();
      }

    }
  }

}
