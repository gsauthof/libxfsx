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
#ifndef BED_ARGUMENTS_IMPL_HH
#define BED_ARGUMENTS_IMPL_HH

namespace bed {

  enum class Edit_Command {
    NONE,
    REMOVE,
    REPLACE,
    ADD,
    SET_ATT,
    INSERT,
    WRITE_ACI,
    LAST_
  };

  enum class Option {
    VERBOSE,
    HELP,
    VERSION,
    INDENT,
    ASN,
    ASN_PATH,
    ASN_CFG,
    NO_DETECT,
    HEX,
    TAG,
    KLASSE,
    TL,
    T_SIZE,
    LENGTH,
    OFFSET,
    SKIP,
    BCI,
    SEARCH,
    ACI,
    CDR,
    FIRST,
    COUNT,
    EXPR,
    XSD,
    COMMAND,
    OUTPUT,
    PRETTY_PRINT,
    PP_FILE
  };

} // bed

#endif // BED_ARGUMENTS_IMPL_HH

