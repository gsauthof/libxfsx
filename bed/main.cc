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

#include "command.hh"


#include <iostream>

using namespace std;

int main(int argc, char **argv)
{
  using namespace bed;
  try {
    Arguments args(argc, argv);
    command::execute(args);
  } catch (const std::exception &e) {
    cerr << "Error: " << e.what() << '\n';
    return 1;
  } catch (...) {
    cerr << "got non std::exception exception\n";
    return 1;
  }
  return 0;
}
