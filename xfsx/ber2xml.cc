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

#include "xfsx.hh"
#include "byte.hh"
#include "hex.hh"
#include "string.hh"
#include "tlc_reader.hh"
#include "scratchpad.hh"
#include "xml_writer_arguments.hh"
#include "bcd.hh"

#include <ixxx/ixxx.hh>

// XXX
#include <iostream>

#include <algorithm>
#include <string>
#include <stack>
#include <deque>
#include <memory>

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
  // sol::string_view is an alias for std::string_view if compiling for C++17
  #include <sol/string_view.hpp>
#endif // XFSX_USE_LUA

using namespace std;

namespace xfsx {

  namespace xml {

    static void write_unber_tl(byte::writer::Base &w, const Unit &u, size_t off)
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

    void write_unber_tl(scratchpad::Simple_Reader<u8> &r,
        byte::writer::Base &w)
    {
        size_t off = 0;
        Unit u;
        while (read_next(r, u)) {
            write_unber_tl(w, u, off);
            off = r.pos();
        }
    }

    void write_unber_tl(
        const u8 *begin, const u8 *end,
        byte::writer::Base &w)
    {
        scratchpad::Simple_Reader<u8> r(begin, end);
        write_unber_tl(r, w);
        w.w.flush();
    }

    void write_unber_tl(
        const u8 *begin, const u8 *end,
        const char *filename)
    {
        scratchpad::Simple_Writer<char> x(unique_ptr<scratchpad::Writer<char>>(
                    new scratchpad::File_Writer<char>(filename)));
        byte::writer::Base w(x);

      write_unber_tl(begin, end, w);
      x.flush();
    }
    void write_unber_tl(
        const u8 *begin, const u8 *end,
        const std::string &filename)
    {
      write_unber_tl(begin, end, filename.c_str());
    }


    void write_indent_unber_tl(scratchpad::Simple_Reader<u8> &r,
        byte::writer::Base &w)
    {
        size_t off = 0;
        size_t indent = 0;
        stack<size_t> length_stack;
        length_stack.push(0);
        stack<size_t> written_stack;
        written_stack.push(0);
        Unit u;
        while (read_next(r, u)) {
            written_stack.top() += u.tl_size;
            if (u.shape == Shape::PRIMITIVE) {
                written_stack.top() += u.length;
                if (u.is_eoc()) {
                    if (indent < 4)
                        throw underflow_error("superfluous EOC?");
                    indent -= 4;
                }
            }
            w.fill(indent);
            write_unber_tl(w, u, off);
            off = r.pos();
            if (u.shape == Shape::CONSTRUCTED) {
                indent += 4;
                if (!u.is_indefinite) {
                    length_stack.push(u.length);
                    written_stack.push(0);
                }
            }
            while (!length_stack.empty()
                    && length_stack.top() == written_stack.top()) {
                auto t = length_stack.top();
                length_stack.pop();
                written_stack.pop();
                written_stack.top() += t;
                if (indent < 4)
                    throw underflow_error("indent underflow during def ending");
                indent -= 4;
            }
        }
    }

    void write_indent_unber_tl(
        const u8 *begin, const u8 *end,
        byte::writer::Base &w)
    {
        scratchpad::Simple_Reader<u8> r(begin, end);
        write_indent_unber_tl(r, w);
        w.w.flush();
    }
    void write_indent_unber_tl(
        const u8 *begin, const u8 *end,
        const char *filename)
    {
        scratchpad::Simple_Writer<char> x(unique_ptr<scratchpad::Writer<char>>(
                    new scratchpad::File_Writer<char>(filename)));
        byte::writer::Base w(x);
      write_indent_unber_tl(begin, end, w);
      x.flush();
    }
    void write_indent_unber_tl(
        const u8 *begin, const u8 *end,
        const std::string &filename)
    {
      write_indent_unber_tl(begin, end, filename.c_str());
    }


  } // xml

class Tag_Matcher {
    public:
        Tag_Matcher();
        Tag_Matcher(std::vector<std::pair<Tag_Int, Klasse>> &&path);
        Tag_Matcher(const std::vector<Tag_Int> &path);
        // push constructed and primitive tags
        // primitive tags are immediately popped
        void push(Tag_Int tag, Klasse klasse);
        void pop();
        bool matches() const;
        void set_path_str(const std::string &s) { path_str_ = s; }
        bool empty() const { return path_.empty(); }
        void set_start_anywhere(bool b) { start_anywhere_ = b; }
        bool skippable() const { return !matches() && match_pos_ != pos_; }
    private:
        std::string path_str_;
        std::vector<std::pair<Tag_Int, Klasse>> path_;
        size_t match_pos_ {0};
        size_t pos_       {0};
        bool start_anywhere_ {false};
        size_t match_off_ {0};
};
static Tag_Matcher mk_tag_matcher(const std::string &path,
        const xfsx::Name_Translator &nt)
{
    vector<pair<Tag_Int, Klasse>> v;
    v.reserve(10);
    pair<const char*, const char *> x;
    x.first = path.data();
    const char *end = path.data() + path.size();
    for (;;) {
        x.second = std::find(x.first, end, '/');
        auto r = nt.translate(x);
        v.emplace_back(get<2>(r), get<1>(r));
        if (x.second == end)
            break;
        x.first = x.second+1;
    }
    auto r = Tag_Matcher(std::move(v));
    r.set_path_str(path);
    return r;
}
Tag_Matcher::Tag_Matcher() =default;
Tag_Matcher::Tag_Matcher(std::vector<std::pair<Tag_Int, Klasse>> &&path)
    :
        path_(std::move(path))
{
}
Tag_Matcher::Tag_Matcher(const std::vector<Tag_Int> &path)
{
    path_.reserve(path.size());
    for (auto &t : path)
        path_.emplace_back(t, xfsx::Klasse::APPLICATION);
}
bool Tag_Matcher::matches() const
{
    return match_pos_ == path_.size();
}
void Tag_Matcher::push(Tag_Int tag, Klasse klasse)
{
    if (start_anywhere_) {
        // XXX don't do implicit ** matching between each tag here
        if (match_pos_ < path_.size()) {
            if ( (path_[match_pos_].first == tag
                        || !path_[match_pos_].first) // wild-card * match
                    && path_[match_pos_].second == klasse) {
                if (!match_pos_)
                    match_off_ = pos_;
                ++match_pos_;
            }
#if 0
            } else if (match_pos_) {
                // i.e. on the first mismatch, try a new one
                // XXX stash the partial match away in case
                // in can be completed later
                if (0 < path_.size()
                        && (path_[0].first == tag
                            || !path_[0].first) // wild-card * match
                        && path_[0].second == klasse) {
                    match_off_ = pos_;
                    match_pos_ = 1;
                }
            }
#endif
        }
    } else if (match_pos_ == pos_ && match_pos_ < path_.size()
            && (path_[match_pos_].first == tag
                || !path_[match_pos_].first) // wild-card * match
            && path_[match_pos_].second == klasse) {
        ++match_pos_;
    }
    ++pos_;
}
void Tag_Matcher::pop()
{
    if (match_pos_ && match_off_ + match_pos_ == pos_)
        --match_pos_;
    assert(pos_);
    --pos_;
}


class Ber2Xml {
    public:
        Ber2Xml(scratchpad::Simple_Writer<char> &w,
                const xml::Pretty_Writer_Arguments &args);
        //void process(Simple_Reader<TLC> &r);
        void process(scratchpad::Simple_Reader<u8> &r);
        void process_blocks(scratchpad::Simple_Reader<u8> &r);

        size_t open_tags();
    private:
        void print_primitive(const TLC &tlc, const string *s);
        void print_constructed(const TLC &tlc, const string *s);
        void pop_constructed(bool is_indefinite);
        void print_attributes(const TLC &tlc, const string *s);
        void indent(size_t k);

        scratchpad::Simple_Writer<char> &w_;
        byte::writer::Base o_;
        const xml::Pretty_Writer_Arguments &args_;


        deque<Unit> cons_stack_;
        size_t cons_stack_top_{0};
        deque<const std::string*> cons_str_stack_;
        // lengths of all all definite constructed tags
        stack<size_t> length_stack_;
        // counts against the same position in the length_stack_
        // to detect when a tag is 'closed'
        stack<size_t> written_stack_;
        size_t off_{0};
        size_t indent_level_{0};

        Tag_Matcher searcher_;
        size_t match_cnt_ {0};
        size_t search_ranges_pos_ {0};


        bool searcher_matches();
        void push_matcher(const TLC &tlc);
        void pop_matcher();
        void pretty_print(const TLC &tlc, Type type);
        void update_matcher(const TLC &tlc, int64_t v);
        void update_matcher(const TLC &tlc, const char *begin, const char *end);
#ifdef XFSX_USE_LUA
        void setup_lua();
        void setup_lua_functions();

        sol::state lua_;
        unordered_map<Tag_Int, sol::function> pp_fn_map_;
        Klasse pp_klasse_ {Klasse::APPLICATION};
        vector<pair<Tag_Matcher,
                    pair<sol::function, sol::function> > >
          matcher_;
        // 0 -> not matched, 1 -> matching, 2 -> 1st mismatch, after a match,
        // 3 -> done
        vector<uint8_t> matcher_state_;
        bool matcher_done_ {false};
        // reuse those helper buffer,
        // avoiding superfluous allocations/constructions
        bool pretty_printed_{false};
        Hex_String hex_str_;
        BCD_String bcd_str_;
        int64_t v_{0};
#endif // XFSX_USE_LUA
};
size_t Ber2Xml::open_tags()
{
    return cons_stack_top_;
}

Ber2Xml::Ber2Xml(scratchpad::Simple_Writer<char> &w,
        const xml::Pretty_Writer_Arguments &args)
    :
        w_(w),
        o_(w_),
        args_(args),
        searcher_(args_.search_path)
{
    length_stack_.push(0); // symmetric to catch-all
    written_stack_.push(0); // catch-all

    if (args_.search_everywhere)
        searcher_.set_start_anywhere(true);

#ifdef XFSX_USE_LUA
    if (args.pretty_print)
        setup_lua();
#endif // XFSX_USE_LUA
}

#ifdef XFSX_USE_LUA

static void setup_lua_path()
{
  string lua_path;
  try {
    string asn1_path(ixxx::ansi::getenv("ASN1_PATH"));

    try {
      lua_path = ixxx::ansi::getenv("LUA_PATH");
    } catch (const ixxx::getenv_error &e) {
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
      lua_path += "/?.luac;";
      lua_path.append((*i).begin(), (*i).end());
      lua_path += "/?.lua;";
    }
    if (!lua_path.empty())
      lua_path.resize(lua_path.size()-1);
  } catch (const ixxx::sys_error &e) {
    return;
  }
  ixxx::posix::setenv("LUA_PATH", lua_path, true);
}

void Ber2Xml::setup_lua()
{
      setup_lua_path();
      lua_.open_libraries(sol::lib::string);
      lua_.open_libraries(sol::lib::base);
      // required for require()
      lua_.open_libraries(sol::lib::package);
      // required for math.floor() - not for //
      lua_.open_libraries(sol::lib::math);
      lua_.script_file(args_.pp_filename);
      setup_lua_functions();
}
void Ber2Xml::setup_lua_functions()
{
      sol::table fns = lua_["tag_callback"];
      for (auto &fn : fns) {
        pp_fn_map_[fn.first.as<Tag_Int>()] = fn.second.as<sol::function>();
      }

      matcher_.reserve(10);
      sol::table ps = lua_["xpath_callback"];
      for (auto &p : ps) {
        matcher_.emplace_back(
                mk_tag_matcher(p.second.as<sol::table>()["path"].get<string>(),
                    args_.name_translator),
                make_pair(
                    p.second.as<sol::table>()["push"].get<sol::function>(),
                    p.second.as<sol::table>()["store"].get<sol::function>()));
      }
      matcher_state_.resize(matcher_.size());
}
#endif // XFSX_USE_LUA
void Ber2Xml::push_matcher(const TLC &tlc)
{
    if (!searcher_.empty()) {
        bool t = searcher_.matches();
        searcher_.push(tlc.tag, tlc.klasse);
        if (!t && searcher_.matches())
            ++match_cnt_;
    }
#ifdef XFSX_USE_LUA
    if (!args_.pretty_print)
        return;
    if (matcher_done_)
        return;
    size_t i = 0;
    size_t x = 0;
    for (auto &m : matcher_) {
        if (matcher_state_[i] < 3) {
            ++x;
            m.first.push(tlc.tag, tlc.klasse);
            if (matcher_state_[i] == 0 && m.first.matches())
                matcher_state_[i] = 1;
            if (matcher_state_[i] == 1 && !m.first.matches())
                matcher_state_[i] = 2;
            if (matcher_state_[i] == 2 && !m.first.matches())
                matcher_state_[i] = 3;
        }
        ++i;
    }
    if (!x)
        matcher_done_ = true;
#endif // XFSX_USE_LUA
}
void Ber2Xml::update_matcher(const TLC &tlc, int64_t v)
{
    (void)tlc;
    (void)v;
#ifdef XFSX_USE_LUA
    if (!args_.pretty_print)
        return;
    if (matcher_done_)
        return;
    size_t i = 0;
    for (auto &m : matcher_) {
        if (matcher_state_[i] == 1)
            m.second.first(tlc.tag, v);
        else if (matcher_state_[i] == 2 && m.second.second)
            m.second.second();
        ++i;
    }
#endif // XFSX_USE_LUA
}
void Ber2Xml::update_matcher(const TLC &tlc, const char *begin, const char *end)
{
    (void)tlc;
    (void)begin;
    (void)end;
#ifdef XFSX_USE_LUA
    if (!args_.pretty_print)
        return;
    if (matcher_done_)
        return;
    size_t i = 0;
    for (auto &m : matcher_) {
        if (matcher_state_[i] == 1)
            m.second.first(tlc.tag, sol::string_view(begin, end-begin));
        else if (matcher_state_[i] == 2)
            m.second.second();
        ++i;
    }
#endif // XFSX_USE_LUA
}
void Ber2Xml::pop_matcher()
{
    if (!searcher_.empty())
        searcher_.pop();
#ifdef XFSX_USE_LUA
    if (!args_.pretty_print)
        return;
    if (matcher_done_)
        return;
    size_t i = 0;
    for (auto &m : matcher_) {
        if (matcher_state_[i] < 2)
            m.first.pop();
        ++i;
    }
#endif // XFSX_USE_LUA
}

void Ber2Xml::process_blocks(scratchpad::Simple_Reader<u8> &r)
{
    auto &p = r.window();
    while (r.next(args_.block_size)) {
        //Simple_Reader<TLC> x(p.first, p.second);
        scratchpad::Simple_Reader<u8> x(p.first, p.second);
        x.set_pos(r.pos());
        try {
            process(x);
        } catch (const xfsx::Parse_Error &) {
            // pass
        }
        r.forget(args_.block_size);
    }
}
void Ber2Xml::process(scratchpad::Simple_Reader<u8> &r)
{
    off_ = r.pos();
    TLC u;
    size_t i = 0; // XXX count globally for process_blocks()
    while (read_next(r, u)) {
        if (args_.count && i >= args_.count) {
            while (cons_stack_top_)
                pop_constructed(cons_stack_[cons_stack_top_-1].is_indefinite);
            return;
        }
        ++i;
        written_stack_.top() += u.tl_size;
        bool eoc = u.is_eoc();
        if (!eoc)
            push_matcher(u);
        if (!eoc && !args_.search_everywhere && !u.is_indefinite
                && !searcher_.empty() && searcher_.skippable()) {
            r.next(u.length);
            r.forget(u.length);
            written_stack_.top() += u.length;
            pop_matcher();
        } else {
            const string *tag_str = args_.translator.empty() ?
                nullptr : args_.translator.find(u.klasse, u.tag);
            if (u.shape == Shape::PRIMITIVE) {
                written_stack_.top() += u.length;
                if (eoc) {
                    try {
                        pop_constructed(true);
                        if (args_.stop_after_first && !cons_stack_top_)
                            return;
                    } catch (const Unexpected_EOC &) {
                        if (args_.skip_zero) {
                            auto p = find_if(r.window().first, r.window().second,
                                    [](uint8_t c){return !!c;});
                            r.forget(p - r.window().first);
                        } else {
                            throw;
                        }
                    }
                }
                else {
                    print_primitive(u, tag_str);
                }
            } else { // constructed
                print_constructed(u, tag_str);
                if (!u.is_indefinite) {
                    length_stack_.push(u.length);
                    written_stack_.push(0);
                }
                if (cons_stack_top_ >= cons_stack_.size()) {
                    cons_stack_.push_back(u);
                    cons_str_stack_.push_back(tag_str);
                } else {
                    cons_stack_[cons_stack_top_] = u;
                    cons_str_stack_[cons_stack_top_] = tag_str;
                }
                ++cons_stack_top_;
            }
        }
        while (!length_stack_.empty()
                && length_stack_.top() == written_stack_.top()) {
            pop_constructed(false);
            auto t = length_stack_.top();
            length_stack_.pop();
            written_stack_.pop();
            written_stack_.top() += t;
        }
        if (args_.stop_after_first && !cons_stack_top_)
            return;
        // Bail-out early if no range can match anymore
        if (!args_.search_ranges.empty()
                && args_.search_ranges.size() == search_ranges_pos_) {
            cons_stack_top_ = 0;
            return;
        }
        off_ = r.pos();
    }
}
void Ber2Xml::pop_constructed(bool is_indefinite)
{
    if (!cons_stack_top_)
        throw xfsx::Unexpected_EOC();
    if (cons_stack_[cons_stack_top_-1].is_indefinite != is_indefinite) {
        if (is_indefinite)
            throw xfsx::TL_Too_Small();
        else
            throw xfsx::Unexpected_EOC();
    }
    if (searcher_matches()) {


    --indent_level_;
    if (cons_stack_[cons_stack_top_-1].is_indefinite
            || cons_stack_[cons_stack_top_-1].length) {
        indent(indent_level_);
        //indent(cons_stack_top_-1);
        const string *s = cons_str_stack_[cons_stack_top_-1];
        if (s) {
            w_.write("</");
            w_.write(s->data(), s->data()+s->size());
            if (cons_stack_[cons_stack_top_-1].is_indefinite)
                w_.write("> <!-- indefinite -->\n");
            else
                w_.write(">\n");
        } else {
            if (cons_stack_[cons_stack_top_-1].is_indefinite)
                w_.write("</i>\n");
            else
                w_.write("</c>\n");
        }
    }

    }
    --cons_stack_top_;
    pop_matcher();
}
bool Ber2Xml::searcher_matches()
{
    if (searcher_.empty())
        return true;
    if (searcher_.matches()) {
        if (args_.search_ranges.empty())
            return true;
        if (!match_cnt_)
            return false;
        auto k = match_cnt_ - 1;
        for (; search_ranges_pos_ < args_.search_ranges.size(); ++search_ranges_pos_) {
            if (k >= args_.search_ranges[search_ranges_pos_].first
                    && k < args_.search_ranges[search_ranges_pos_].second)
                return true;
            if (k < args_.search_ranges[search_ranges_pos_].first)
                break;
        }
        return false;
    } else {
        return false;
    }
}
void Ber2Xml::print_constructed(const TLC &tlc, const string *tag_str)
{
    if (searcher_matches()) {

    //indent(cons_stack_top_);
    indent(indent_level_);
    ++indent_level_;
    if (tag_str) {
        w_.write("<");
        w_.write(tag_str->data(), tag_str->data()+tag_str->size());
        if (tlc.is_indefinite)
            w_.write(" definite='false'");
    } else {
        if (tlc.is_indefinite)
            w_.write("<i");
        else
            w_.write("<c");
    }
    print_attributes(tlc, tag_str);
    if (!tlc.is_indefinite && !tlc.length)
        w_.write("/>\n");
    else
        w_.write(">\n");

    }
}
void Ber2Xml::print_attributes(const TLC &tlc, const string *tag_str)
{
    if (!tag_str || args_.dump_tag)
        o_ << " tag='" << tlc.tag << '\'';
    if (!tag_str || args_.dump_class)
        o_ << " class='" << klasse_to_cstr(tlc.klasse) << '\'';
    if (args_.dump_tl)
        o_ << " tl='" << tlc.tl_size << '\'';
    if (args_.dump_t)
        o_ << " t='" << tlc.t_size << '\'';
    if (args_.dump_length)
        o_ << " length='" << tlc.length << '\'';
    if (args_.dump_offset)
        o_ << " off='" << off_ << '\'';
    if (args_.hex_dump && tlc.shape == Shape::PRIMITIVE) {
        o_ << " hex='";
        size_t n = hex::decoded_size<hex::Style::Raw>(
            tlc.begin + tlc.tl_size, tlc.begin + tlc.tl_size + tlc.length);
        auto o = w_.begin_write(n);
        hex::decode<hex::Style::Raw>(
            tlc.begin + tlc.tl_size, tlc.begin + tlc.tl_size + tlc.length, o);
        w_.commit_write(n);
        o_ << "'";
    }
}
void Ber2Xml::pretty_print(const TLC &tlc, Type type)
{
    (void)tlc;
    (void)type;
#ifdef XFSX_USE_LUA
    if (!args_.pretty_print)
        return;

    if (tlc.shape != Shape::PRIMITIVE)
        return;

    pretty_printed_ = false;
    auto i = pp_fn_map_.find(tlc.tag);
    if (i == pp_fn_map_.end())
        return;

    // XXX register C++ output function that is directly called
    // from lua?
    const char *c_s = nullptr;
    switch (type) {
        case Type::INT_64:
            v_ = tlc.lexical_cast<int64_t>();
            c_s = i->second(v_).get<const char*>();
            break;
        case Type::STRING:
        case Type::OCTET_STRING:
            tlc.copy_content(hex_str_);
            c_s = i->second(sol::string_view(hex_str_.get().data(),
                        hex_str_.get().size())).get<const char*>();
            break;
        case Type::BCD:
            tlc.copy_content(bcd_str_);
            c_s = i->second(sol::string_view(bcd_str_.get().data(),
                        bcd_str_.get().size())).get<const char*>();
            break;
    }
    if (c_s) {
        o_ << " pp='" << c_s << '\'';
        pretty_printed_ = true;
    }
#endif // XFSX_USE_LUA
}
void Ber2Xml::print_primitive(const TLC &tlc, const string *tag_str)
{
    if (searcher_matches()) {

    //indent(cons_stack_top_);
    indent(indent_level_);
    if (tag_str)
        o_ << '<' << *tag_str;
    else
        o_ << "<p";
    print_attributes(tlc, tag_str);
    if (tlc.length) {
    if (args_.translator.empty()) {
        w_.write(">");
        size_t n = hex::decoded_size<hex::Style::XML>(
                tlc.begin + tlc.tl_size, tlc.begin + tlc.tl_size + tlc.length
                );
        auto x = w_.begin_write(n);
        hex::decode<hex::Style::XML>(
                tlc.begin + tlc.tl_size, tlc.begin + tlc.tl_size + tlc.length,
                x);
        w_.commit_write(n);
    } else {
	auto kt = args_.dereferencer.dereference(tlc.klasse, tlc.tag);
	auto type = args_.typifier.typify(kt);
        pretty_print(tlc, type);
        w_.write(">");
#ifdef XFSX_USE_LUA
        if (args_.pretty_print && pretty_printed_) {
            switch (type) {
            case Type::INT_64:
                o_ << v_;
                update_matcher(tlc, v_);
                break;
            case Type::BCD:
                {
                    auto x = bcd_str_.get().data();
                    auto n = bcd_str_.get().size();
                w_.write(x, x+n);
                update_matcher(tlc, x, x+n);
                }
                break;
            case Type::STRING:
            case Type::OCTET_STRING:
                {
                    auto x = hex_str_.get().data();
                    auto n = hex_str_.get().size();
                w_.write(x, x+n);
                update_matcher(tlc, x, x+n);
                }
                break;
            }
        } else 
#endif // XFSX_USE_LUA
        {
        switch (type) {
            case Type::INT_64: {
                int64_t v {0};
                xfsx::decode(tlc.begin + tlc.tl_size, tlc.length, v);
                update_matcher(tlc, v);
                o_ << v;
                } break;
            case Type::BCD: {
                size_t n = tlc.length*2;
                if (n) {
                    auto x = w_.begin_write(n);
                    bcd::decode(tlc.begin+tlc.tl_size,
                            tlc.begin+tlc.tl_size+tlc.length, x);
                    if (x[n-1] == 'f')
                        --n;
                    update_matcher(tlc, x, x+n);
                    w_.commit_write(n);
                }
                } break;
            case Type::STRING:
            case Type::OCTET_STRING:
                size_t n = hex::decoded_size<hex::Style::XML>(
                    tlc.begin + tlc.tl_size, tlc.begin + tlc.tl_size + tlc.length
                    );
                auto x = w_.begin_write(n);
                hex::decode<hex::Style::XML>(
                    tlc.begin + tlc.tl_size, tlc.begin + tlc.tl_size + tlc.length,
                    x);
                update_matcher(tlc, x, x+n);
                w_.commit_write(n);
                break;
        }
        }
    }
    if (tag_str)
        o_ << "</" << *tag_str << ">\n";
    else
        o_ << "</p>\n";
    } else {
        o_ << "/>\n";
    }
    }
    pop_matcher();
}
void Ber2Xml::indent(size_t k)
{
    size_t n = k * args_.indent_size;
    auto x = w_.begin_write(n);
    std::fill(x, x + n, ' ');
    w_.commit_write(n);
}



    namespace xml {


        void pretty_write(scratchpad::Simple_Reader<u8> &r,
                scratchpad::Simple_Writer<char> &w,
                const Pretty_Writer_Arguments &args)
        {
            if (args.skip) {
                r.next(args.skip);
                r.check_available(args.skip);
                r.forget(args.skip);
            }
            Ber2Xml b2x(w, args);
            if (args.block_size) {
                b2x.process_blocks(r);
            } else {
                b2x.process(r);
            }
            w.flush();
            if (b2x.open_tags() && !args.count)
                throw overflow_error("some tags are still open");
        }
        void pretty_write(
            const u8 *begin, const u8 *end,
            scratchpad::Simple_Writer<char> &w,
            const Pretty_Writer_Arguments &args)
        {
            auto r = scratchpad::mk_simple_reader(begin, end);
            pretty_write(r, w, args);
        }
        void pretty_write(
            const u8 *begin, const u8 *end,
            const std::string &filename,
            const Pretty_Writer_Arguments &args)
        {
            auto r = scratchpad::mk_simple_reader(begin, end);
            auto w = scratchpad::mk_simple_writer<char>(filename);
            pretty_write(r, w, args);
        }


  } // xml

} // xfsx
