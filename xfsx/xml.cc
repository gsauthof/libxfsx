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
#include "xml.hh"

#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <cassert>

#include "s_pair.hh"
#include "scratchpad.hh"

using namespace std;

namespace xfsx {

  namespace xml {

        static char empty_s[0];

        Reader::Reader(scratchpad::Simple_Reader<char> &src)
            :
                src_(src),
                p_(src_.window())
        {
        }
        bool Reader::next()
        {
            const char *x;
            for (;;) {
                x = std::find(p_.first + k_.second, p_.second, '<');
                if (x == p_.second) {
                    if (src_.eof())
                        return false;
                    src_.forget(low_);
                    src_.next(inc_);
                    k_.first -= low_;
                    k_.second -= low_;
                    low_ = 0;
                    continue;
                }
                break;
            }
            k_.first = x - p_.first + 1; // excluding the <
            const char *y;
            for (;;) {
                y = std::find(p_.first + k_.second, p_.second, '>');
                if (y == p_.second) {
                    if (src_.eof())
                        throw range_error("file ends within a tag");
                    src_.forget(low_);
                    src_.next(inc_);
                    k_.first -= low_;
                    k_.second -= low_;
                    low_ = 0;
                    continue;
                }
                break;
            }
            low_ = k_.second;
            k_.second = y - p_.first + 1; // including the >
            return true;
        }
        std::pair<const char*, const char*> Reader::tag() const
        {
            assert(k_.first < k_.second);
            return make_pair(p_.first + k_.first, p_.first + k_.second - 1);
        }
        std::pair<const char*, const char*> Reader::value() const
        {
            //assert(k_.first);
            if (k_.first)
                return make_pair(p_.first + low_, p_.first + k_.first - 1);
            else
                return make_pair(static_cast<const char*>(empty_s),
                        static_cast<const char*>(empty_s));
        }


    Attribute_Traverser::Attribute_Traverser(
        const std::pair<const char*, const char*>
        &element, const std::pair<const char*, const char*> &element_name)
    {
      assert(element.first <= element.second);
      assert(element.first <= element_name.first);
      assert(element.second >= element_name.second);

      end_ = element.second;
      if (element.first < end_ && end_[-1] == '/')
        --end_;
      assert(element_name.second <= end_);
      name_ = attribute_name(element_name.second, end_);
      if (!s_pair::empty(name_))
        value_ = attribute_value(name_.second, end_);
    }
    bool Attribute_Traverser::has_more() const
    {
      return name_.first < name_.second;
    }
    Attribute_Traverser &Attribute_Traverser::operator++()
    {
      auto r = value_.second < end_ ? value_.second + 1 : end_;
      name_ = attribute_name(r, end_);
      if (!s_pair::empty(name_))
        value_ = attribute_value(name_.second, end_);
      return *this;
    }
    const std::pair<const char*, const char*> &
      Attribute_Traverser::name() const
    {
      return name_;
    }
    const std::pair<const char*, const char*> &
      Attribute_Traverser::value() const
    {
      return value_;
    }


    bool is_start_tag(const std::pair<const char*, const char*> &p)
    {
      return p.first < p.second && *p.first != '/';
    }
    bool is_end_tag(const std::pair<const char*, const char*> &p)
    {
      return p.first < p.second && *p.first == '/';
    }
    bool is_start_end_tag(const std::pair<const char*, const char*> &p)
    {
      return p.first < p.second && p.second[-1] == '/';
    }
    bool is_decl(const std::pair<const char*, const char*> &p)
    {
        return p.first < p.second && *p.first == '?';
    }
    bool is_comment(const std::pair<const char*, const char*> &p)
    {
        // XML comments: <!--  -->
        // we assume that there are no legitimate element names that
        // start with ! and that comments don't cross other elements
        return p.first < p.second && *p.first == '!';
    }

    std::pair<const char*, const char*> element_name(
        const std::pair<const char*, const char*> &p)
    {
      if (s_pair::empty(p))
        throw runtime_error("element name is empty");
      const char *x = p.first;
      if (*x == '/')
        ++x;
      const char *r = find_if(x, p.second, [](char c){
          return c == '/' || isspace(c); });
      if (r == x)
        throw runtime_error("element name is empty");
      return make_pair(x, r);
    }

    std::pair<const char*, const char*> attribute_name(const char *begin,
        const char *end)
    {
      const char *x = find_if(begin, end, 
          [](char c){ return !isspace(c);});
      const char *y = nullptr;
      if (x < end)
        y = find_if(x + 1, end,
          [](char c){ return isspace(c) || c == '=';});
      else
        y = end;
      return make_pair(x, y);
    }

    std::pair<const char*, const char*> attribute_value(const char *begin,
        const char *end)
    {
      const char *x = find_if(begin, end, [](char c){
          return c == '"' || c == '\''; });
      const char *y = nullptr;
      if (x < end) {
        if (find(begin, x, '=') == x)
          throw runtime_error("no equal sign before attribute value");
        ++x;
        y = find(x, end, x[-1]);
        if (y == end)
          throw runtime_error("attribute quote is not closed");
      } else {
        throw runtime_error("Could not find an attribute value");
        // y = end;
      }
      return make_pair(x, y);
    }


  }


}
