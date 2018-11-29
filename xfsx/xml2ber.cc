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

#include <algorithm>


#include <assert.h>
#include "ber_writer_arguments.hh"
#include <stdexcept>
#include <xfsx/scratchpad.hh>
#include <xfsx/xml.hh>
#include <xfsx/s_pair.hh>
#include <xfsx/tap.hh>
#include <xfsx/ber_writer_arguments.hh>
#include <xfsx/tlc_writer.hh>
#include <xfsx/integer.hh>

using namespace xfsx;



using namespace std;

namespace xfsx {

  namespace xml {



class Xml2Ber {
    public:
        Xml2Ber(scratchpad::Simple_Reader<char> &in,
                scratchpad::Simple_Writer<u8> &out,
                const BER_Writer_Arguments &args
                );

        void process();
    private:
        void process_tag();
        // if constructed and indefinite: write TL part
        // otherwise: write nothing
        void write_start();
        // if primitive: write to the top buffer
        // if constructed definite: write TL part and copy top+1 buffer
        // if constructed indefinite: copy top+1 buffer
        void write_end(bool is_empty);

        xml::Reader r_;
        Attributes attributes_;
        const BER_Writer_Arguments &args_;

        // top of the stack is the current tag
        // either primitive or constructed
        // the active rest of the stack are constructed tags
        std::deque<TLV> tlv_stack_;
        size_t tlv_stack_top_{0};

        // writers are reused to avoid unnecessary object reconstructions
        // (thus no std::stack nor down-resize calls)
        // first writer directly writes to a file
        // for each definitive constructed a scratchpad writer is pushed
        std::deque<scratchpad::Simple_Writer<u8>*> writer_stack_;
        std::deque<scratchpad::Simple_Writer<u8>> writers_;
        size_t writer_stack_top_{0};

        std::array<u8, 2> eoc_{0, 0};
};

Xml2Ber::Xml2Ber(scratchpad::Simple_Reader<char> &in,
        scratchpad::Simple_Writer<u8> &out,
        const BER_Writer_Arguments &args
        )
    :
        r_(in),
        args_(args)
{
    writer_stack_.push_back(&out);
    ++writer_stack_top_;
}
void Xml2Ber::process()
{
    if (r_.next()) {
        if (!xml::is_decl(r_.tag()))
            process_tag();
    }
    while (r_.next()) {
        process_tag();
    }
    if (writer_stack_top_ != 1)
        throw runtime_error("unexpected writer stack - unbalanced tags?");
    if (tlv_stack_top_)
        throw runtime_error("unexpected tlv stack - unbalanced tags?");

#ifndef NDEBUG
    //assert(writer_stack_.size() > 1);
    if (writer_stack_.size() > 1) {
        writer_stack_[1]->flush();
        auto be = dynamic_cast<scratchpad::Scratchpad_Writer<u8>*>(
                    writer_stack_[1]->backend());
        auto b = be->pad().prelude();
        auto e = be->pad().begin();
        assert(e-b == 0);
    }
#endif // NDEBUG

    // writer_stack_[0] is already written out in last write_end() call
    // we just need to flush
    writer_stack_[0]->flush();
}
// return: full-initialized
bool read_tag(const std::pair<const char*, const char*> &name,
        TLV &tlv, const BER_Writer_Arguments &args)
{
    if (!args.translator.empty()) {
        try {
            auto shape_klasse_tag = args.translator.translate(name);
            tlv.shape = std::get<0>(shape_klasse_tag);
            tlv.klasse = std::get<1>(shape_klasse_tag);
            tlv.init_tag(std::get<2>(shape_klasse_tag));
            return true;
        } catch (const std::out_of_range &e) {
            // pass
        }
    }
    if (s_pair::size(name) == 1) {
        switch (*name.first) {
            case 'i': tlv.shape = Shape::CONSTRUCTED;
                      tlv.is_indefinite = true;
                      break;
            case 'c': tlv.shape = Shape::CONSTRUCTED; break;
            case 'p': tlv.shape = Shape::PRIMITIVE; break;
            default:
                      throw runtime_error("Unknown tag name: " + s_pair::mk_string(name));
        }
    } else {
        throw runtime_error("Unknown tag name: " + s_pair::mk_string(name));
    }
    return false;
}
void read_attribute(
        const std::pair<const char*, const char*> &name,
        const std::pair<const char*, const char*> &value,
        TLV &tlv, Attributes &a)
{
    if (s_pair::equal(name, "tag", 3)) {
        tlv.init_tag(integer::range_to_uint32(value));
        a.tag_present = true;
    } else if (s_pair::equal(name, "class", 5)) {
        tlv.klasse = str_to_klasse(value);
    } else if (s_pair::equal(name, "indefinite", 10)) {
        if (tlv.shape == Shape::PRIMITIVE)
            throw runtime_error("a primitive tag must not be indefinite");
        if (s_pair::equal(value, "true", 4))
            tlv.is_indefinite = true;
    } else if (s_pair::equal(name, "definite", 8)) {
        if (tlv.shape == Shape::PRIMITIVE)
            throw runtime_error("a primitive tag must not be indefinite");
        if (s_pair::equal(value, "false", 5))
            tlv.is_indefinite = true;
    } else if (s_pair::equal(name, "l_size", 6)) {
        a.l_size = integer::range_to_uint32(value);
    } else if (s_pair::equal(name, "uint2int", 8)) {
        a.uint2int = s_pair::equal(value, "true", 4);
    }
}
static Attributes read_attributes(
        const std::pair<const char*, const char*> &tag,
        const std::pair<const char*, const char*> &name,
        TLV &tlv)
{
    Attributes a;
    xml::Attribute_Traverser at(tag, name);
    for (; at.has_more(); ++at) {
        auto k = at.name();
        auto v = at.value();
        read_attribute(k, v, tlv, a);
    }
    return a;
}

void Xml2Ber::process_tag()
{
    using xfsx::s_pair::mk_string;

    auto t = r_.tag();
    if (xml::is_comment(t))
        return;
    if (xml::is_start_tag(t)) {
        TLV tlv;
        auto name = xml::element_name(t);
        bool full_tag = read_tag(name, tlv, args_);
        attributes_ = read_attributes(t, name, tlv);
        attributes_.full_tag = full_tag;
        // we have to init_indefinite() after a possible tag change
        if (tlv.is_indefinite)
            tlv.init_indefinite();
        if (attributes_.l_size)
            tlv.init_l_size(attributes_.l_size);
        if (!full_tag && !attributes_.tag_present)
            throw runtime_error("element is missing mandatory tag attribute");
        if (tlv_stack_top_ >= tlv_stack_.size())
            tlv_stack_.push_back(std::move(tlv));
        else
            tlv_stack_[tlv_stack_top_] = std::move(tlv);
        ++tlv_stack_top_;
        write_start();
        if (xml::is_start_end_tag(t))
            write_end(true);
    } else if (xml::is_end_tag(t)) {
        write_end(false);
    }
}
void Xml2Ber::write_start()
{
    TLV &tlv = tlv_stack_[tlv_stack_top_-1];
    if (tlv.shape == Shape::CONSTRUCTED) {
        if (tlv.is_indefinite) {
            write_tag(*writer_stack_[writer_stack_top_-1], tlv);
        } else {

            if (writer_stack_top_ >= writer_stack_.size()) {
                writers_.emplace_back(
                        std::unique_ptr<scratchpad::Writer<u8>>(
                            new scratchpad::Scratchpad_Writer<u8>()
                            )
                        );
                writer_stack_.push_back(&writers_.back());
            } else {
                auto b  = dynamic_cast<scratchpad::Scratchpad_Writer<u8>*>(
                        writer_stack_[writer_stack_top_]->backend());
                (void)b;
                assert(b->pad().begin() - b->pad().prelude() == 0);
            }
            ++writer_stack_top_;
        }
    }
}
void add_content(const std::pair<const char*, const char*> &value,
        TLV &tlv, const Attributes &as, const BER_Writer_Arguments &args)
{
    auto content = value;
    if (!as.full_tag)
        tlv = XML_Content(std::move(content));
    else {
        auto kt = args.dereferencer.dereference(tlv.klasse, tlv.tag);
        Type type = args.typifier.typify(kt);
        switch (type) {
            case Type::INT_64:
                if (as.uint2int) {
                    Int64_Content c(value);
                    c.uint_to_int();
                    tlv = c;
                } else
                    tlv = Int64_Content(value);
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
void Xml2Ber::write_end(bool is_empty)
{
    if (!tlv_stack_top_)
        throw underflow_error("tlv stack underflow - unbalanced tags?");
    TLV &tlv = tlv_stack_[tlv_stack_top_-1];


    if (tlv.shape == Shape::CONSTRUCTED) {
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
    } else { // Shape::PRIMITIVE
        if (is_empty)
            tlv = XML_Content();
        else {
            auto v = r_.value();
            add_content(v, tlv, attributes_, args_);
        }
        write_tag(*writer_stack_[writer_stack_top_-1], tlv);
    }

    assert(tlv_stack_top_);
    --tlv_stack_top_;
}

    void write_ber(scratchpad::Simple_Reader<char> &in,
                scratchpad::Simple_Writer<u8> &out,
                const BER_Writer_Arguments &args
            )
    {
        Xml2Ber x2b(in, out, args);
        x2b.process();
    }

  }

}
