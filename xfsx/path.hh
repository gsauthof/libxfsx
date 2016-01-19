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
#ifndef XFSX_PATH_HH
#define XFSX_PATH_HH

#include <vector>
#include <utility>
#include <string>

namespace xfsx {

  class Name_Translator;
  using Tag_Int = uint32_t;

  namespace path {

    std::pair<std::vector<xfsx::Tag_Int>, bool> parse(
        const std::string &search_path_str);
    std::pair<std::vector<xfsx::Tag_Int>, bool> parse(
        const std::string &search_path_str,
        const xfsx::Name_Translator &name_translator);

    std::vector<std::pair<size_t, size_t> > parse_range_predicate(
        const std::string &s);
    std::vector<std::pair<size_t, size_t> > ranges(const std::string &s);
  }

}


#endif
