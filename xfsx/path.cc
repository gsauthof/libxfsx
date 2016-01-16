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

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

#include <xfsx/xfsx.hh>

using namespace std;

namespace xfsx {

  namespace path {

    std::pair<std::vector<xfsx::Tag_Int>, bool> parse(
        const std::string &search_path_str)
    {
      std::vector<xfsx::Tag_Int> search_path;
      bool search_everywhere = false;
      boost::regex re("[/*0-9]+");
      if (boost::regex_match(search_path_str, re)) {
        vector<string> parts;
        string s;
        if (search_path_str[0] == '/') {
          s = search_path_str.substr(1);
        } else {
          s = search_path_str;
          search_everywhere = true;
        }
        boost::split(parts, s, boost::is_any_of("/"));
        for (auto &part : parts) {
          if (part == "*")
            search_path.push_back(0);
          else
            search_path.push_back(
                boost::lexical_cast<xfsx::Tag_Int>(part));
        }
      }
      return make_pair(std::move(search_path), search_everywhere);
    }
    std::pair<std::vector<xfsx::Tag_Int>, bool> parse(
        const std::string &search_path_str,
        const xfsx::Name_Translator &name_translator)
    {
      std::vector<xfsx::Tag_Int> search_path;
      bool search_everywhere = false;
      if (!search_path_str.empty()) {
        vector<string> parts;
        string s;
        if (search_path_str[0] == '/') {
          s = search_path_str.substr(1);
        } else {
          s = search_path_str;
          search_everywhere = true;
        }
        boost::split(parts, s, boost::is_any_of("/"));
        try {
          for (auto &part : parts) {
            if (part == "*") {
              search_path.push_back(0);
            } else {
              try {
                search_path.push_back(
                    std::get<2>(name_translator.translate(
                        make_pair(part.data(), part.data() + part.size()))));
              } catch (const std::out_of_range &) {
                search_path.push_back(
                    boost::lexical_cast<xfsx::Tag_Int>(part));
              }
            }
          }
        } catch (const boost::bad_lexical_cast &e) {
          throw range_error("search path contains invalid element");
        }
      }
      return make_pair(std::move(search_path), search_everywhere);
    }

  }

}
