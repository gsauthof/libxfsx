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
#ifndef BED_COMMAND_ARGUMENTS_HH
#define BED_COMMAND_ARGUMENTS_HH

namespace xfsx {
  namespace xml {
    struct Writer_Arguments;
    struct Pretty_Writer_Arguments;
  }
}

namespace bed {

  class Arguments;

  namespace command {

    void apply_arguments(const Arguments &a,
        xfsx::xml::Writer_Arguments &b);

    void apply_arguments(const Arguments &a,
        xfsx::xml::Pretty_Writer_Arguments &b);

  }

}

#endif


