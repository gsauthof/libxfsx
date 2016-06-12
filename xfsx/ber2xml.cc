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

#include <xfsx_config.hh>


#include <stack>
#include <fcntl.h>
// XXX remove
#include <iostream>

#include <boost/algorithm/string.hpp>

#ifdef XFSX_USE_LUA
  #ifdef XFSX_USE_LUAJIT
    // since some luajit versions don't come with lua.hpp but sol2 does
    // include it, we have to include the headers before such that
    // not the ones _relative_ to lua.hpp are included
    extern "C" {
      #include <lua.h>
      #include <lualib.h>
      #include <lauxlib.h>
    }
    #define SOL_LUAJIT
  #endif // XFSX_USE_LUAJIT
  #include <sol.hpp>
#endif // XFSX_USE_LUA

#include "xfsx.hh"
#include "byte.hh"
#include "hex.hh"
#include "traverser/matcher.hh"
#include "traverser/tlc.hh"
#include "path.hh"

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
      ixxx::util::FD fd(filename, O_CREAT | O_WRONLY | O_TRUNC, 0666);
      byte::writer::File w{fd};
      write_unber_tl(begin, end, w);
      w.flush();
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
      ixxx::util::FD fd(filename, O_CREAT | O_WRONLY | O_TRUNC, 0666);
      byte::writer::File w{fd};
      write_indent_unber_tl(begin, end, w);
      w.flush();
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
        const uint8_t *end_   {nullptr};
        Skip_EOC_Reader r;
        byte::writer::Base &w;
        stack<bool> indefinite_stack;
        Vertical_TLC *tlc {nullptr};

        virtual void write_all();
        virtual void write_element_tail();
        virtual void write_element();
        void write_primitive();
        virtual void write_primitive_tag_open();
        virtual void write_primitive_tag_close();
        virtual void write_content();
        void write_hex_dump();
        virtual void write_attributes();
        virtual void write_class();
        virtual void write_tag();
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
        begin_(begin),
        end_(end),
        r(begin_ + args.skip, end),
        w(w)
    {
    }

    void Writer::write_element_tail()
    {
      if (tlc->shape == Shape::PRIMITIVE) {
        write_primitive();
      } else {
        write_constructed();
      }
    }
    void Writer::write_element()
    {
      write_element_tail();
    }
    void Writer::write_class()
    {
      w << " class='" << klasse_to_cstr(tlc->klasse) << '\'';
    }
    void Writer::write_tag()
    {
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
      write_tag();
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

    void Writer::write_all()
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

    void Writer::write()
    {
      if (args_.search_path.empty()) {
        write_all();
      } else {
        using namespace xfsx::traverser;
        Vertical_TLC t;
        Vertical_TLC_Proxy p(begin_, end_, t);
        Basic_Matcher<Vertical_TLC_Proxy, Vertical_TLC> pf(
            args_.search_path, args_.search_everywhere);
        auto ret = advance_first_match(p, t, pf);

        auto i = args_.search_ranges.begin();
        auto j = args_.search_ranges.end();
        if (i == j)
          return;
        size_t k = 0;

        while (!p.eot(t)) {
          if (k >= (*i).first) {
            r = Skip_EOC_Reader(t.begin, end_);
            write_all();
          }
          ++k;
          if (k >= (*i).second) {
            ++i;
            if (i == j)
              break;
          }
          ret = advance_next_match(ret, p, t, pf);
        }
      }
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
      ixxx::util::FD fd(filename, O_CREAT | O_WRONLY | O_TRUNC, 0666);
      byte::writer::File w{fd};
      write(begin, end, w, args);
      w.flush();
    }
    void write(
        const uint8_t *begin, const uint8_t *end,
        const std::string &filename,
        const Writer_Arguments &args)
    {
      write(begin, end, filename.c_str(), args);
    }


    class Pretty_Writer : public Writer {
      protected:
        const Pretty_Writer_Arguments &args_;
      private:
        stack<const std::string *> tag_stack_;
        const std::string *tag_str_ {nullptr};
        xfsx::BCD_String bcd_;

      protected:
        Type type_ {Type::OCTET_STRING};

        void write_element() override;
        void write_primitive_tag_open() override;
        void write_primitive_tag_close() override;
        void write_class() override;
        void write_tag() override;
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
    void Pretty_Writer::write_element()
    {
      if (tlc->shape == Shape::PRIMITIVE) {
	auto kt = args_.dereferencer.dereference(tlc->klasse, tlc->tag);
	type_ = args_.typifier.typify(kt);
      }
      Writer::write_element();
    }
    void Pretty_Writer::write_class()
    {
      if (!tag_str_ || args_.dump_class)
        Writer::write_class();
    }
    void Pretty_Writer::write_tag()
    {
      if (tag_str_ && args_.dump_tag)
        w << " tag='" << tlc->tag << '\'';
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
      switch (type_) {
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

#ifdef XFSX_USE_LUA

    static void setup_lua_path()
    {
      string lua_path;
      try {
        string asn1_path(ixxx::ansi::getenv("ASN1_PATH"));

        try {
          lua_path = ixxx::ansi::getenv("LUA_PATH");
        } catch (const ixxx::runtime_error &e) {
        }
        if (!lua_path.empty())
          lua_path += ';';
        using string_iterator = boost::algorithm::split_iterator<const char*>;
        pair<const char *, const char*> inp(asn1_path.data(),
            asn1_path.data() + asn1_path.size());
        for (string_iterator i = boost::algorithm::make_split_iterator(inp,
              boost::algorithm::token_finder([](auto c){return c==':';}));
            i != string_iterator(); ++i) {
          lua_path.append((*i).begin(), (*i).end());
          lua_path += "/?.lua;";
          lua_path.append((*i).begin(), (*i).end());
          lua_path += "/?;";
        }
        if (!lua_path.empty())
          lua_path.resize(lua_path.size());
      } catch (const ixxx::runtime_error &e) {
        return;
      }
      ixxx::posix::setenv("LUA_PATH", lua_path, true);
    }

    class Lua_Pretty_Writer : public Pretty_Writer {
      private:
        sol::state lua_;

        traverser::Vertical_TLC_Proxy proxy_;

        unordered_map<Tag_Int, sol::function> pp_fn_map_;
        Klasse pp_klasse_ = Klasse::APPLICATION;
        vector<pair<traverser::Basic_Matcher<
                      traverser::Vertical_TLC_Proxy, Vertical_TLC>,
                    pair<sol::function, sol::function> > >
          matcher_;
        bool matcher_finalized_ {false};

        void read_functions();
        void write_pp();
      public:
        Lua_Pretty_Writer(
            const uint8_t *begin, const uint8_t *end,
            byte::writer::Base &w, const Pretty_Writer_Arguments &args);

        void write_attributes() override;
        void write_element_tail() override;
    };

    Lua_Pretty_Writer::Lua_Pretty_Writer(
        const uint8_t *begin, const uint8_t *end,
        byte::writer::Base &w, const Pretty_Writer_Arguments &args)
      :
        Pretty_Writer(begin, end, w, args)
    {
      setup_lua_path();
      lua_.open_libraries(sol::lib::string);
      lua_.open_libraries(sol::lib::base);
      // required for require()
      lua_.open_libraries(sol::lib::package);
      // required for math.floor() - not for //
      lua_.open_libraries(sol::lib::math);
      lua_.script_file(args_.pp_filename);
      read_functions();
    }


    void Lua_Pretty_Writer::read_functions()
    {
      sol::table fns = lua_["tag_callback"];
      for (auto &fn : fns) {
        pp_fn_map_[fn.first.as<Tag_Int>()] = fn.second.as<sol::function>();
      }

      matcher_.reserve(10);
      sol::table ps = lua_["xpath_callback"];
      for (auto &p : ps) {
        matcher_.push_back(make_pair(
              traverser::Basic_Matcher<
                  traverser::Vertical_TLC_Proxy, Vertical_TLC>(
                    path::parse(p.second.as<sol::table>()["path"].get<string>(), args_.name_translator).first),
              make_pair(p.second.as<sol::table>()["push"].get<sol::function>(), p.second.as<sol::table>()["store"].get<sol::function>())));
      }
    }

    void Lua_Pretty_Writer::write_element_tail()
    {
      if (!matcher_finalized_) {
        for (auto &m : matcher_) {
          if (m.first.result_ == traverser::Matcher_Result::FINALIZE) {
            matcher_finalized_ = true;
            continue;
          }
          matcher_finalized_ = false;
          m.first(proxy_, *tlc);
          if (m.first.result_ == traverser::Matcher_Result::APPLY) {
            if (tlc->shape != Shape::PRIMITIVE)
              return;

            switch (type_) {
              case Type::INT_64:
                m.second.first(tlc->tag, tlc->lexical_cast<int64_t>());
                break;
              case Type::STRING:
              case Type::OCTET_STRING:
                {
                  Hex_String x;
                  tlc->copy_content(x);
                  m.second.first(tlc->tag, x.get());
                }
                break;
              default:
                break;
            }

          } else if (m.first.result_ == traverser::Matcher_Result::FINALIZE) {
            if (m.second.second)
              m.second.second();
          }
        }
      }
      Pretty_Writer::write_element_tail();
    }
    void Lua_Pretty_Writer::write_attributes()
    {
      Pretty_Writer::write_attributes();
      write_pp();
    }
    void Lua_Pretty_Writer::write_pp()
    {
      if (tlc->klasse != pp_klasse_)
        return;
      // XXX  overwrite new write_primitive_attributes() instead
      if (tlc->shape != Shape::PRIMITIVE)
        return;

      auto i = pp_fn_map_.find(tlc->tag);
      if (i == pp_fn_map_.end())
        return;

      // XXX register C++ output function that is directly called
      // from lua?
      //string s;
      const char *c_s = nullptr;
      switch (type_) {
        case Type::INT_64:
          //s = i->second(tlc->lexical_cast<int64_t>());
          c_s = i->second(tlc->lexical_cast<int64_t>()).get<const char*>();
          break;
        case Type::STRING:
        case Type::OCTET_STRING:
          {
            Hex_String x;
            tlc->copy_content(x);
            //s = i->second(x.get());
            c_s = i->second(x.get()).get<const char*>();
          }
          break;
        case Type::BCD:
          {
            BCD_String x;
            tlc->copy_content(x);
            //s = i->second(x.get());
            c_s = i->second(x.get()).get<const char*>();
          }
          break;
      }
      if (c_s)
        w << " pp='" << c_s << '\'';
    }
#endif // XFSX_USE_LUA

    void pretty_write(
        const uint8_t *begin, const uint8_t *end,
        byte::writer::Base &w,
        const Pretty_Writer_Arguments &args)
    {
      if (args.pretty_print) {
#ifdef XFSX_USE_LUA
        Lua_Pretty_Writer writer(begin, end, w, args);
        writer.write();
#else
        throw logic_error("not compiled with Lua support");
#endif // XFSX_USE_LUA
      } else {
        Pretty_Writer writer(begin, end, w, args);
        writer.write();
      }
    }

    void pretty_write(
        const uint8_t *begin, const uint8_t *end,
        const std::string &filename,
        const Pretty_Writer_Arguments &args)
    {
        ixxx::util::FD fd(filename, O_CREAT | O_WRONLY | O_TRUNC, 0666);
        xfsx::byte::writer::File w{fd};
        pretty_write(begin, end, w, args);
        w.flush();
    }

  }

}
