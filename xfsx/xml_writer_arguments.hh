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
#ifndef XFSX_XML_WRITER_ARGUMENTS_HH
#define XFSX_XML_WRITER_ARGUMENTS_HH

#include <deque>
#include <string>
#include <vector>

#include <xfsx/xfsx.hh>

namespace xfsx {

  namespace xml {

    struct Writer_Arguments {
      unsigned indent_size      {4};
      bool     hex_dump         {false};
      bool     dump_tag         {false};
      bool     dump_class       {false};
      bool     dump_tl          {false};
      bool     dump_t           {false};
      bool     dump_length      {false};
      bool     dump_offset      {false};
      bool     dump_rank        {false};
      bool     dump_indefinite  {true};
      size_t   skip             {0};
      bool     stop_after_first {false};
      size_t   count            {0};
      std::vector<Tag_Int> search_path;
      bool     search_everywhere {false};
      std::vector<std::pair<size_t, size_t> > search_ranges;
    };

    extern Writer_Arguments default_writer_arguments;

    struct Pretty_Writer_Arguments : public Writer_Arguments {
      Pretty_Writer_Arguments();
      Pretty_Writer_Arguments(const std::deque<std::string> &asn_filenames);
      Tag_Translator translator;
      Tag_Dereferencer dereferencer;
      Tag_Typifier typifier;

      Name_Translator name_translator;
    };

  }
}


#endif
