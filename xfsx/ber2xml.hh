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
#ifndef BER2XML_HH
#define BER2XML_HH

#include <stdint.h>

#include <xfsx/byte.hh>
#include <xfsx/xml_writer_arguments.hh>

namespace xfsx {

  namespace xml {

    class Writer_Arguments;
    class Pretty_Writer_Arguments;

    void write_unber_tl(
        const uint8_t *begin, const uint8_t *end,
        byte::writer::Base &w
        );
    void write_unber_tl(
        const uint8_t *begin, const uint8_t *end,
        const std::string &filename);
    void write_unber_tl(
        const uint8_t *begin, const uint8_t *end,
        const char *filename);

    void write_indent_unber_tl(
        const uint8_t *begin, const uint8_t *end,
        byte::writer::Base &w);
    void write_indent_unber_tl(
        const uint8_t *begin, const uint8_t *end,
        const std::string &filename);
    void write_indent_unber_tl(
        const uint8_t *begin, const uint8_t *end,
        const char *filename);


    void write(
        const uint8_t *begin, const uint8_t *end,
        byte::writer::Base &w,
        const Writer_Arguments &args = default_writer_arguments
        );
    void write(
        const uint8_t *begin, const uint8_t *end,
        const std::string &filename,
        const Writer_Arguments &args = default_writer_arguments
        );
    void write(
        const uint8_t *begin, const uint8_t *end,
        const char *filename,
        const Writer_Arguments &args = default_writer_arguments
        );

    void pretty_write(
        const uint8_t *begin, const uint8_t *end,
        byte::writer::Base &w,
        const Pretty_Writer_Arguments &args);
    void pretty_write(
        const uint8_t *begin, const uint8_t *end,
        const std::string &filename,
        const Pretty_Writer_Arguments &args);
  }

}

#endif
