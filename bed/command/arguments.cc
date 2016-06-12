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

#include "arguments.hh"

#include <bed/arguments.hh>
#include <xfsx/xml_writer_arguments.hh>
#include <xfsx/tap.hh>
#include <xfsx/path.hh>

namespace bed {

  namespace command {

    void apply_search_args(const Arguments &a,
        xfsx::xml::Writer_Arguments &b,
        const xfsx::Tag_Translator &translator,
        const xfsx::Name_Translator &name_translator)
    {
      if (!b.search_path.empty())
        return;
      if (!a.kth_cdr.empty()) {
        b.search_path = xfsx::tap::kth_cdr_path(translator);
        b.search_ranges = xfsx::path::parse_range_predicate(a.kth_cdr);
      } else if (a.skip_to_aci) {
        b.search_path = xfsx::tap::aci_path(translator);
        b.search_ranges.emplace_back(0, 1);
      } else {
        auto x = xfsx::path::parse(a.search_path, name_translator);
        b.search_path = std::move(x.first);
        b.search_everywhere = x.second;
        b.search_ranges = xfsx::path::ranges(a.search_path);
      }
    }

    void apply_arguments(const Arguments &a,
        xfsx::xml::Writer_Arguments &b)
    {
      b.indent_size      = a.indent_size;
      b.hex_dump         = a.hex_dump;
      b.dump_tag         = a.dump_tag;
      b.dump_class       = a.dump_class;
      b.dump_tl          = a.dump_tl;
      b.dump_t           = a.dump_t;
      b.dump_length      = a.dump_length;
      b.dump_offset      = a.dump_offset;
      b.skip             = a.skip;
      b.stop_after_first = a.stop_after_first;
      b.count            = a.count;

      apply_search_args(a, b, xfsx::tap::mini_tap_translator(),
          xfsx::Name_Translator());
    }

    void apply_arguments(const Arguments &a,
        xfsx::xml::Pretty_Writer_Arguments &b)
    {
      apply_search_args(a, b, b.translator, b.name_translator);
      b.pretty_print     = a.pretty_print;
      b.pp_filename      = a.pp_filename;

      apply_arguments(a, *static_cast<xfsx::xml::Writer_Arguments*>(&b));
    }

  }

}
