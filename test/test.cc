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

#include "test.hh"

#include <ixxx/ixxx.h>

using namespace std;

namespace test {

  namespace path {

    const std::string &in()
    {
      static string p;
      if (p.empty()) {
        try {
          p = ixxx::ansi::getenv("TEST_IN_BASE");
        } catch (...) {
          p = "../test";
        }
        p += "/in";
      }
      return p;
    }
    const std::string &ref()
    {
      static string p;
      if (p.empty()) {
        try {
          p = ixxx::ansi::getenv("TEST_IN_BASE");
        } catch (...) {
          p = "../test";
        }
        p += "/ref";
      }
      return p;
    }
    const std::string &out()
    {
      static string p;
      if (p.empty()) {
        try {
          p = ixxx::ansi::getenv("TEST_OUT");
        } catch (...) {
          p = "out";
        }
      }
      return p;
    }

  }

}
