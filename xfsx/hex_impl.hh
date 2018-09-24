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
#ifndef XFSX_HEX_IMPL_HH
#define XFSX_HEX_IMPL_HH

#include "hex.hh"

#include <algorithm>
#include <stdint.h>
#include <boost/regex.hpp>

#include "bcd_impl.hh"
#include "octet.hh"

namespace xfsx {

  namespace hex {

    namespace impl {


      struct Is_Control {
        bool operator()(uint8_t b) const { return (b < 32u || b > 126u); }
      };
      struct Is_Printable {
        bool operator()(uint8_t b) const { return (b > 31u && b < 127u); }
      };


      namespace Is_Special {
        template <typename Style_Tag> struct Base;
        template <> struct Base<Style::XML> {
          bool operator()(uint8_t b) const {
            return char(b) == '&' || char(b) == '<' || char(b) == '>';
          }
        };
        template <> struct Base<Style::C> {
          bool operator()(uint8_t b) const { return char(b) == '\\'; }
        };
        template <> struct Base<Style::Raw> {
          bool operator()(uint8_t b) const { return false; }
        };
      }

      template <typename Style_Tag> struct Is_Normal {
        bool operator()(uint8_t b) const
        {
          return Is_Printable()(b) && !Is_Special::Base<Style_Tag>()(b);
        }
      };
      template <> struct Is_Normal<Style::Raw> {
        bool operator()(uint8_t b) const
        {
          return false;
        }
      };

      template <typename Style_Tag> struct Is_Not_Normal {
        bool operator()(uint8_t b) const
        {
          return Is_Control()(b) || Is_Special::Base<Style_Tag>()(b);
        }
      };
      template <> struct Is_Not_Normal<Style::Raw> {
        bool operator()(uint8_t b) const
        {
          return true;
        }
      };

      namespace Surround {
        template <typename Style_Tag> struct Base;
        template <> struct Base<Style::XML> {
          char *prefix(char *o) const
          {
            *o++ = '&';
            *o++ = '#';
            *o++ = 'x';
            return o;
          }
          constexpr size_t prefix_size() const { return 3u; }
          char *suffix(char *o) const
          {
            *o++ = ';';
            return o;
          }
          constexpr size_t suffix_size() const { return 1u; }
        };
        template <> struct Base<Style::C> {
          char *prefix(char *o) const
          {
            *o++ = '\\';
            *o++ = 'x';
            return o;
          }
          constexpr size_t prefix_size() const { return 2u; }
          char *suffix(char *o) const
          {
            return o;
          }
          constexpr size_t suffix_size() const { return 0u; }
        };

        template <> struct Base<Style::Raw> {
          char *prefix(char *o) const
          {
            return o;
          }
          constexpr size_t prefix_size() const { return 0u; }
          char *suffix(char *o) const
          {
            return o;
          }
          constexpr size_t suffix_size() const { return 0u; }
        };
      }

      template <typename Style_Tag> struct Overhead {
        constexpr size_t operator()() const {
          return Surround::Base<Style_Tag>().prefix_size()
            + Surround::Base<Style_Tag>().suffix_size() + 1u;
        }
      };

      template <typename Style_Tag>
        struct Escape {
          char *operator()(uint8_t b, char *o) const
          {
            o = Surround::Base<Style_Tag>().prefix(o);
            o = bcd::impl::decode::Basic_Decode<char*, uint16_t>()(&b, &b+1, o);
            o = Surround::Base<Style_Tag>().suffix(o);
            return o;
          }
        };

      template <typename Style_Tag>
        size_t count_decode_overhead(const u8 *begin, const u8 *end)
        {
          return std::count_if(begin, end, Is_Not_Normal<Style_Tag>())
            * Overhead<Style_Tag>()();
        }
      template <typename Style_Tag>
      size_t decoded_size(const u8 *begin, const u8 *end)
      {
        return size_t(end-begin)
          + impl::count_decode_overhead<Style_Tag>(begin, end);
      }
      template <>
      inline size_t decoded_size<Style::Raw>(
          const u8 *begin, const u8 *end)
      {
        size_t n = end - begin;
        return n * 2u;
      }

      template <typename I, typename O, typename Unary_Predicate >
        std::pair<I, O> copy_while(I begin, I end, O o_begin,
            Unary_Predicate pred)
        {
          auto i = std::find_if_not(begin, end, pred);
          auto o = std::copy(begin, i, o_begin);
          return std::make_pair(i, o);
        }

      template <typename I, typename O, typename Translate_Op,
               typename Unary_Predicate>
                 std::pair<I, O> translate_while(I begin, I end, O o_begin, 
                     Translate_Op translate_op, Unary_Predicate pred)
                 {
                   while (begin != end) {
                     if (pred(*begin))
                       o_begin = translate_op(*begin++, o_begin);
                     else
                       break;
                   }
                   return std::make_pair(begin, o_begin);
                 }

      template <typename Style_Tag>
        char *decode(const u8 *begin, const u8 *end, char *o)
        {
          const u8 *i = begin;
          auto io = std::make_pair(i, o);
          do {
            io = copy_while(io.first, end, io.second, Is_Normal<Style_Tag>());
            io = translate_while(io.first, end, io.second,
                Escape<Style_Tag>(), Is_Not_Normal<Style_Tag>());
          } while (io.first < end);
          return io.second;
        }

      namespace Next_Quoted {
        namespace Tag {
          struct Regex{};
          struct Search{};
        }

        template <typename Style_Tag, typename Policy = Tag::Search>
          struct Base;

        template <> struct Base<Style::XML, Tag::Regex> {
          const char *operator()(const char *begin, const char *end)
          {
            static const boost::regex re("&#x..;", boost::regex::extended);
            boost::match_results<const char*> m;
            if (boost::regex_search(begin, end, m, re)) {
              return m[0].first;
            } else {
              return end;
            }
         
          }
        };
        template <> struct Base<Style::C, Tag::Regex> {
          const char *operator()(const char *begin, const char *end)
          {
            static const boost::regex re("\\\\x..", boost::regex::extended);
            boost::match_results<const char*> m;
            if (boost::regex_search(begin, end, m, re)) {
              return m[0].first;
            } else {
              return end;
            }
         
          }
        };

        // Using the search() algorithm is twice as fast
        // in comparison to boost regex.

        template <> struct Base<Style::XML, Tag::Search> {
          const char *operator()(const char *begin, const char *end)
          {
            auto p = begin;
            while (p < end) {
              const char start[] = "&#x";
              p = std::search(p, end, start, start + sizeof(start) - 1);
              if (end-p < 6)
                return end;
              if (p[5] == ';')
                return p;
              p += 3;
            }
            return end;
          }
        };
        template <> struct Base<Style::C, Tag::Search> {
          const char *operator()(const char *begin, const char *end)
          {
            auto p = begin;
              const char start[] = "\\x";
              p = std::search(p, end, start, start + sizeof(start) - 1);
              if (end-p < 4)
                return end;
            return p;
          }
        };
        template <> struct Base<Style::Raw, Tag::Search> {
          const char *operator()(const char *begin, const char *end)
          {
            return begin;
          }
        };
      }
      namespace Is_Quoted {
        template <typename Style_Tag> struct Base;
        template <> struct Base<Style::XML> {
          bool operator()(const char *begin, const char *end)
          {
            if (end-begin < 6)
              return false;
            return begin[0] == '&' && begin[1] == '#' && begin[2] == 'x'
              && begin[5] == ';';
          }
        };
        template <> struct Base<Style::C> {
          bool operator()(const char *begin, const char *end)
          {
            if (end-begin < 3)
              return false;
            return begin[0] == '\\';
          }
        };
        template <> struct Base<Style::Raw> {
          bool operator()(const char *begin, const char *end)
          {
            if (end-begin < 2)
              return false;
            return true;
          }
        };
      }
      template <typename Style_Tag>
        struct Un_Escape {
          std::pair<const char*, u8*>
            operator()(const char *begin, const char *end, u8 *o)
          {
            const char *a = begin + Surround::Base<Style_Tag>().prefix_size();
            const char *b =
              begin + Surround::Base<Style_Tag>().prefix_size() + 2u;
            o = bcd::impl::encode::Basic_Encode<u8*, uint16_t,
              // to avoid alignment issues
              bcd::impl::encode::Scatter::Memcpy >()( a, b, o);
            auto i = begin
              + Surround::Base<Style_Tag>().prefix_size() + 2u
              + Surround::Base<Style_Tag>().suffix_size();
            return std::make_pair(i, o);
          }
        };

      template <typename I, typename O, typename Next_Op>
        std::pair<I, O> copy_it_while(I begin, I end, O o_begin,
            Next_Op next_op)
        {
          auto i = next_op(begin, end);
          auto o = std::copy(begin, i, o_begin);
          return std::make_pair(i, o);
        }

      template <typename I, typename O, typename Translate_Op,
               typename Unary_Predicate>
                 std::pair<I, O> translate_it_while(I begin, I end, O o_begin, 
                     Translate_Op translate_op, Unary_Predicate pred)
                 {
                   auto io = std::make_pair(begin, o_begin);
                   while (begin < end) {
                     if (pred(io.first, end))
                       io = translate_op(io.first, end, io.second);
                     else
                       break;
                   }
                   return io;
                 }

      template <typename Style_Tag>
        size_t count_encode_savings(const char *begin, const char *end)
        {
          const char *i = begin;
          size_t n = 0;
          for (;;) {
            i = Next_Quoted::Base<Style_Tag>()(i, end);
            if (i < end) {
              ++n;
              i += Surround::Base<Style_Tag>().prefix_size() + 2u
                  + Surround::Base<Style_Tag>().suffix_size();
            } else
              break;
          }
          return n * (Surround::Base<Style_Tag>().prefix_size() + 1u
              + Surround::Base<Style_Tag>().suffix_size());
        }

      template <typename Style_Tag>
        size_t encoded_size(const char *begin, const char *end)
        {
          return size_t(end-begin)
            - impl::count_encode_savings<Style_Tag>(begin, end);
        }
      template <>
        inline size_t encoded_size<Style::Raw>(
            const char *begin, const char *end)
        {
          size_t n = end - begin;
          return n / 2u;
        }

      template <typename Style_Tag>
        u8 *encode(const char *begin, const char *end, u8 *o)
        {
          const char *i = begin;
          auto io = std::make_pair(i, o);
          do {
            io = copy_it_while(io.first, end, io.second,
                Next_Quoted::Base<Style_Tag>());
            io = translate_it_while(io.first, end, io.second,
                Un_Escape<Style_Tag>(), Is_Quoted::Base<Style_Tag>());
          } while (io.first < end);
          return io.second;
        }

    }

  }

}

#endif
