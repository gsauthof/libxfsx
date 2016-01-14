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
#include "ber2xml.hh"

#include <stack>
#include <fcntl.h>

#include "xfsx.hh"
#include "byte.hh"
#include "hex.hh"

#include <ixxx/ixxx.h>

using namespace std;

namespace xfsx {

  namespace xml {

    void write_unber_tl(byte::writer::Base &w, const Unit &u, size_t off)
    {
      w
        << '<'
        << (u.shape == Shape::PRIMITIVE ? 'P' : ( u.is_indefinite ? 'I' : 'C') )
        << " O=\"" << off << '"'
        << " T=\"[" << klasse_to_cstr(u.klasse) << ' ' << u.tag << "]\""
        << " TL=\"" << unsigned(u.tl_size) << '"'
        << " V=\"";
      if (u.is_indefinite)
        w << "Indefinite";
      else
        w << u.length;
      w  << "\">\n";
    }

    void write_unber_tl(
        const uint8_t *begin, const uint8_t *end,
        byte::writer::Base &w)
    {
      Reader r(begin, end);
      auto i = r.begin();
      for (; i!= r.end(); ++i) {
        const TLC &tlc = *i;
        const Unit &u = tlc;
        write_unber_tl(w, u, tlc.begin - begin);
      }
      w.flush();
    }

    void write_unber_tl(
        const uint8_t *begin, const uint8_t *end,
        const char *filename)
    {
      ixxx::util::FD fd(filename, O_CREAT | O_WRONLY, 0666);
      byte::writer::File w{fd};
      write_unber_tl(begin, end, w);
      w.flush();
      ixxx::posix::ftruncate(fd, w.written());
    }
    void write_unber_tl(
        const uint8_t *begin, const uint8_t *end,
        const std::string &filename)
    {
      write_unber_tl(begin, end, filename.c_str());
    }


    void write_indent_unber_tl(
        const uint8_t *begin, const uint8_t *end,
        byte::writer::Base &w)
    {
      Vertical_Reader r(begin, end);
      for (auto &tlc : r) {
        const Unit &u = tlc;
        w.fill(tlc.height * 4);
        write_unber_tl(w, u, tlc.begin - begin);
      }
    }
    void write_indent_unber_tl(
        const uint8_t *begin, const uint8_t *end,
        const char *filename)
    {
      ixxx::util::FD fd(filename, O_CREAT | O_WRONLY, 0666);
      byte::writer::File w{fd};
      write_indent_unber_tl(begin, end, w);
      w.flush();
      ixxx::posix::ftruncate(fd, w.written());
    }
    void write_indent_unber_tl(
        const uint8_t *begin, const uint8_t *end,
        const std::string &filename)
    {
      write_indent_unber_tl(begin, end, filename.c_str());
    }


    class Writer {
      private:
        const Writer_Arguments &args_;
      protected:
        const uint8_t *begin_ {nullptr};
        Skip_EOC_Reader r;
        byte::writer::Base &w;
        stack<bool> indefinite_stack;
        TLC *tlc {nullptr};

        void write_element();
        void write_primitive();
        virtual void write_primitive_tag_open();
        virtual void write_primitive_tag_close();
        virtual void write_content();
        void write_hex_dump();
        void write_attributes();
        virtual void write_class();
        void write_tl();
        void write_t();
        void write_length();
        void write_offset();
        void write_constructed();
        virtual void write_constructed_tag_open();
        virtual void write_constructed_tag_close();
        void write_closing(size_t h);
        virtual void push();
        virtual void pop();
      public:
        Writer(
            const uint8_t *begin, const uint8_t *end,
            byte::writer::Base &w, const Writer_Arguments &args);
        void write();

    };
    Writer::Writer(
        const uint8_t *begin, const uint8_t *end,
        byte::writer::Base &w, const Writer_Arguments &args)
      :
        args_(args),
        begin_(begin + args.skip),
        r(begin_, end),
        w(w)
    {
    }

    void Writer::write_element()
    {
      if (tlc->shape == Shape::PRIMITIVE) {
        write_primitive();
      } else {
        write_constructed();
      }
    }
    void Writer::write_class()
    {
      w << " class='" << klasse_to_cstr(tlc->klasse) << '\'';
    }
    void Writer::write_tl()
    {
      w << " tl='" << tlc->tl_size << '\'';
    }
    void Writer::write_t()
    {
      w << " t='" << tlc->t_size << '\'';
    }
    void Writer::write_length()
    {
      w << " length='" << tlc->length << '\'';
    }
    void Writer::write_offset()
    {
      w << " off='" << (tlc->begin - begin_) << '\'';
    }
    void Writer::write_content()
    {
      size_t n = hex::decoded_size<hex::Style::XML>(
          tlc->begin + tlc->tl_size, tlc->begin + tlc->tl_size + tlc->length
          );
      auto o = w.obtain_chunk(n);
      auto r = hex::decode<hex::Style::XML>(
          tlc->begin + tlc->tl_size, tlc->begin + tlc->tl_size + tlc->length,
          o);
      (void)r;
    }
    void Writer::write_hex_dump()
    {
      if (!args_.hex_dump)
        return;
      w << " hex='";
      size_t n = hex::decoded_size<hex::Style::Raw>(
          tlc->begin + tlc->tl_size, tlc->begin + tlc->tl_size + tlc->length
          );
      auto o = w.obtain_chunk(n);
      auto r = hex::decode<hex::Style::Raw>(
          tlc->begin + tlc->tl_size, tlc->begin + tlc->tl_size + tlc->length,
          o);
      (void)r;
      w << "'";
    }
    void Writer::write_attributes()
    {
      write_class();
      if (args_.dump_tl)
        write_tl();
      if (args_.dump_t)
        write_t();
      if (args_.dump_length)
        write_length();
      if (args_.dump_offset)
        write_offset();
    }
    void Writer::write_primitive()
    {
      write_primitive_tag_open();
      write_attributes();
      if (tlc->length) {
        write_hex_dump();
        w << '>';
        write_content();
        write_primitive_tag_close();
      } else {
        w << "/>\n";
      }
    }
    void Writer::write_primitive_tag_open()
    {
      w << "<p tag='" << tlc->tag << '\'';
    }
    void Writer::write_primitive_tag_close()
    {
      w << "</p>\n";
    }
    void Writer::push()
    {
      if (tlc->is_indefinite || tlc->length)
        indefinite_stack.push(tlc->is_indefinite);
    }
    void Writer::write_constructed()
    {
      push();
      write_constructed_tag_open();
      write_attributes();
      if (tlc->is_indefinite) {
        w << ">\n";
      } else {
        if (tlc->length) {
          w << ">\n";
        } else {
          w << "/>\n";
        }
      }
    }
    void Writer::write_constructed_tag_open()
    {
      if (tlc->is_indefinite)
        w << "<i tag='" << tlc->tag << '\'';
      else
        w << "<c tag='" << tlc->tag << '\'';
    }

    void Writer::write_closing(size_t h)
    {
        while (h < indefinite_stack.size()) {
          w.fill((indefinite_stack.size()-1) * args_.indent_size);
          write_constructed_tag_close();
          pop();
        }
    }
    void Writer::pop()
    {
      indefinite_stack.pop();
    }
    void Writer::write_constructed_tag_close()
    {
          if (indefinite_stack.top())
            w << "</i>\n";
          else
            w << "</c>\n";
    }

    void Writer::write()
    {
      size_t i = 0;
      for (auto &tlc : r) {
        if (args_.count && !(i<args_.count))
          break;
        this->tlc = &tlc;
        write_closing(tlc.height);
        w.fill(tlc.height * args_.indent_size);
        write_element();
        if ((args_.skip || args_.stop_after_first) && !tlc.depth_)
          break;
        ++i;
      }
      write_closing(0);
    }


    void write(
        const uint8_t *begin, const uint8_t *end,
        byte::writer::Base &w,
        const Writer_Arguments &args)
    {
      Writer writer(begin, end, w, args);
      writer.write();
    }
    void write(
        const uint8_t *begin, const uint8_t *end,
        const char *filename,
        const Writer_Arguments &args)
    {
      ixxx::util::FD fd(filename, O_CREAT | O_WRONLY, 0666);
      byte::writer::File w{fd};
      write(begin, end, w, args);
      w.flush();
      ixxx::posix::ftruncate(fd, w.written());
    }
    void write(
        const uint8_t *begin, const uint8_t *end,
        const std::string &filename,
        const Writer_Arguments &args)
    {
      write(begin, end, filename.c_str(), args);
    }


    class Pretty_Writer : public Writer {
      private:
        const Pretty_Writer_Arguments &args_;
        stack<const std::string *> tag_stack_;
        const std::string *tag_str_ {nullptr};
        xfsx::BCD_String bcd_;

      protected:
        void write_primitive_tag_open() override;
        void write_primitive_tag_close() override;
        void write_class() override;
        void write_content() override;
        void write_constructed_tag_open() override;
        void write_constructed_tag_close() override;
        void push() override;
        void pop() override;
      public:
        Pretty_Writer(
            const uint8_t *begin, const uint8_t *end,
            byte::writer::Base &w, const Pretty_Writer_Arguments &args);
    };
    Pretty_Writer::Pretty_Writer(
        const uint8_t *begin, const uint8_t *end,
        byte::writer::Base &w, const Pretty_Writer_Arguments &args)
      :
        Writer(begin, end, w, args),
        args_(args)
    {
    }
    void Pretty_Writer::write_class()
    {
      if (!tag_str_)
        Writer::write_class();
    }
    void Pretty_Writer::write_primitive_tag_open()
    {
      try {
        tag_str_ = &args_.translator.translate(tlc->klasse, tlc->tag);
        w << '<' << *tag_str_;
      } catch (const runtime_error &e) {
        tag_str_ = nullptr;
        Writer::write_primitive_tag_open();
      }
    }
    void Pretty_Writer::write_primitive_tag_close()
    {
      if (tag_str_)
        w << "</" << *tag_str_ << ">\n";
      else
        Writer::write_primitive_tag_close();
    }
    void Pretty_Writer::write_content()
    {
      auto kt = args_.dereferencer.dereference(tlc->klasse, tlc->tag);
      Type t = args_.typifier.typify(kt);
      switch (t) {
        case Type::INT_64:
          {
            int64_t v {0};
            xfsx::decode(tlc->begin + tlc->tl_size, tlc->length, v);
            w << v;
          }
          break;
        case Type::BCD:
          {
            xfsx::decode(tlc->begin + tlc->tl_size, tlc->length, bcd_);
            w << bcd_.get();
          }
          break;
        case Type::STRING:
        case Type::OCTET_STRING:
          Writer::write_content();
          break;
      }
    }
    void Pretty_Writer::push()
    {
      Writer::push();
      try {
        tag_str_ = &args_.translator.translate(tlc->klasse, tlc->tag);
      } catch (const runtime_error &e) {
        tag_str_ = nullptr;
      }
      if (tlc->is_indefinite || tlc->length)
        tag_stack_.push(tag_str_);
    }
    void Pretty_Writer::write_constructed_tag_open()
    {
      if (tag_str_) {
        w << '<' << *tag_str_;
        if (tlc->is_indefinite)
          w << " definite='false'";
      } else
        Writer::write_constructed_tag_open();
    }
    void Pretty_Writer::pop()
    {
      Writer::pop();
      tag_stack_.pop();
    }
    void Pretty_Writer::write_constructed_tag_close()
    {
      if (tag_stack_.top()) {
        w << "</" << *tag_stack_.top() << '>';
        if (indefinite_stack.top())
          w << " <!-- indefinite -->";
        w << '\n';
      } else
        Writer::write_constructed_tag_close();
    }

    void pretty_write(
        const uint8_t *begin, const uint8_t *end,
        byte::writer::Base &w,
        const Pretty_Writer_Arguments &args)
    {
      Pretty_Writer writer(begin, end, w, args);
      writer.write();
    }

    void pretty_write(
        const uint8_t *begin, const uint8_t *end,
        const std::string &filename,
        const Pretty_Writer_Arguments &args)
    {
        ixxx::util::FD fd(filename, O_CREAT | O_WRONLY, 0666);
        xfsx::byte::writer::File w{fd};
        pretty_write(begin, end, w, args);
        w.flush();
        ixxx::posix::ftruncate(fd, w.written());
    }

  }

}
