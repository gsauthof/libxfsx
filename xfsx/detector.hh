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
#ifndef XFSX_DETECTOR_HH
#define XFSX_DETECTOR_HH

#include <xxxml/xxxml.hh>
#include <grammar/grammar.hh>

#include <string>
#include <deque>
#include <functional>

namespace xfsx {

  namespace detector {


    xxxml::doc::Ptr read_xml_header(
        const std::string &filename,
        size_t count,
        const std::deque<std::string> &asn_filenames);

    xxxml::doc::Ptr read_ber_header(
        const std::string &filename,
        size_t count,
        const std::deque<std::string> &asn_filenames);

    struct Result {
      std::deque<std::string> asn_filenames;
      std::deque<std::string> constraint_filenames;
      std::string pp_filename;
      std::string name;
      std::string long_name;
      size_t major{0};
      size_t minor{0};
    };

    using Read_Function = std::function<xxxml::doc::Ptr(const std::string &,
        size_t, const std::deque<std::string>&)>;

    Result detect(
        const std::string &filename,
        const std::string &config_filename,
        const Read_Function &read_fn,
        const std::deque<std::string> &asn_search_path
        );

    Result detect_ber(
        const std::string &filename,
        const std::string &config_filename,
        const std::deque<std::string> &asn_search_path
        );

    Result detect_xml(
        const std::string &filename,
        const std::string &config_filename,
        const std::deque<std::string> &asn_search_path
        );

    std::deque<std::string> default_asn_search_path();
    std::string default_config_filename(const std::deque<std::string> &asn_search_path);

  }

}

#endif
