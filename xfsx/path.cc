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

#include "path.hh"

#include <limits>
#include <algorithm>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/icl/interval_set.hpp>
#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/algorithm/string/finder.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <xfsx/xfsx.hh>
#include <xfsx/integer.hh>

using namespace std;

template <typename T>
  using Closed_Interval_Set = boost::icl::interval_set<T,
            ICL_COMPARE_INSTANCE(ICL_COMPARE_DEFAULT, T),
            boost::icl::closed_interval<T>
        >;
// -> insert boost::icl::closed_interval<T> elements ...
template <typename T>
  using Right_Open_Interval_Set = boost::icl::interval_set<T,
            ICL_COMPARE_INSTANCE(ICL_COMPARE_DEFAULT, T),
            boost::icl::right_open_interval<T>
        >;
// -> insert boost::icl::right_open_interval<T> elements ...

using string_iterator = boost::algorithm::split_iterator<const char*>;

namespace xfsx {

  namespace path {

    /*
      // first_finder alternative:
      for (string_iterator i = boost::algorithm::make_split_iterator(inp,
            boost::algorithm::first_finder(",", boost::algorithm::is_equal()));
          i != string_iterator(); ++i) {
      }

      // token_finder classification alternatives:
      for (string_iterator i = boost::algorithm::make_split_iterator(inp,
            boost::algorithm::token_finder(
              boost::algorithm::is_from_range(',', ',')));
          i != string_iterator(); ++i) {
      }
      for (string_iterator i = boost::algorithm::make_split_iterator(inp,
            boost::algorithm::token_finder(boost::algorithm::is_any_of(",")));
          i != string_iterator(); ++i) {
      }

      // cf. http://www.boost.org/doc/libs/1_60_0/doc/html/string_algo/reference.html#header.boost.algorithm.string.classification_hpp
     */

    /*
       We parse the closed position intervals into right open ones
         -> to be consistent with all the other intervals.
       */
    std::vector<std::pair<size_t, size_t> > parse_range_predicate(
        const std::string &s)
    {
      auto first = s.find('[');
      auto last = s.find(']');
      pair<const char *, const char*> inp(
        first == string::npos ?  s.data() : s.data() + first + 1,
        last  == string::npos ?  s.data() + s.size() : s.data() + last);
      if (boost::empty(inp))
        throw range_error("path range predicate is empty");
      Right_Open_Interval_Set<size_t> iset;
      for (string_iterator i = boost::algorithm::make_split_iterator(inp,
            boost::algorithm::token_finder([](auto c){return c==',';}));
          i != string_iterator(); ++i) {
        auto mid = boost::find_first(*i, "..");
        auto left = make_pair((*i).begin(), mid.begin());
        auto right = make_pair(mid.end(), (*i).end());
        pair<size_t, size_t> p;
        p.first = integer::range_to_uint64(left);
        if (p.first)
          --p.first;
        if (boost::empty(mid))
          p.second = p.first + 1;
        else if (boost::empty(right))
          p.second = std::numeric_limits<size_t>::max();
        else
          p.second = integer::range_to_uint64(right);
        iset.insert(boost::icl::right_open_interval<size_t>(p.first, p.second));
      }
      size_t k = 0;
      vector<std::pair<size_t, size_t> > r(iset.iterative_size());
      for (auto &i : iset) {
        r[k].first = i.lower();
        r[k].second = i.upper();
        // boost interval set silently drops empty left-open intervals
        // on insert - thus, we don't have to check for them here
        // i.e. boost::empty(r[k]) holds
        // cf. interval_set_empty_after_empty_insert() in test/path.cc
        ++k;
      }
      return r;
    }
    std::vector<std::pair<size_t, size_t> > ranges(const std::string &s)
    {
      auto first = s.find('[');
      if (first == string::npos)
        return {make_pair(0, 1)};
      else
        return parse_range_predicate(s);
    }

    template <typename F>
      static
      std::pair<std::vector<xfsx::Tag_Int>, bool>
      foreach_part(const string &search_path_str, F f)
      {
        std::vector<xfsx::Tag_Int> search_path;
        bool search_everywhere = false;
        if (!search_path_str.empty()) {
          pair<const char *, const char*> inp(
              search_path_str.data(),
              search_path_str.data()
                + std::min(search_path_str.find('['), search_path_str.size()));
          if (search_path_str[0] == '/')
            ++inp.first;
          else
            search_everywhere = true;

          if (!boost::empty(inp)) {
            for (string_iterator i = boost::algorithm::make_split_iterator(inp,
                  boost::algorithm::token_finder([](auto c){return c=='/';}));
                i != string_iterator(); ++i) {
              if ((*i).size() == 1 && *(*i).begin() == '*')
                search_path.push_back(0);
              else
                f(search_path, *i);
            }
          }
        }
        return make_pair(std::move(search_path), search_everywhere);
      }

    std::pair<std::vector<xfsx::Tag_Int>, bool> parse(
        const std::string &search_path_str)
    {
      boost::regex re("[/*0-9]+");
      if (boost::regex_match(search_path_str, re))
        return foreach_part(search_path_str, [](auto &v, auto &part) {
            v.push_back(
                integer::range_to_uint64(make_pair(part.begin(), part.end())));
            });
      else
        return make_pair(vector<xfsx::Tag_Int>(), false);
    }
    std::pair<std::vector<xfsx::Tag_Int>, bool> parse(
        const std::string &search_path_str,
        const xfsx::Name_Translator &name_translator)
    {
      if (name_translator.empty())
        return parse(search_path_str);
      return foreach_part(search_path_str,
          [&name_translator](auto &v, auto &part) {
            try {
              v.push_back(std::get<2>(name_translator.translate(
                      make_pair(part.begin(), part.end()))));
            } catch (const std::out_of_range &) {
              if (!std::all_of(
                    part.begin(), part.end(), boost::algorithm::is_digit()))
                throw range_error("search path contains invalid element");
              v.push_back(
                  integer::range_to_uint64(
                      make_pair(part.begin(), part.end())));
            }
          });
    }

  }

}
