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
#include "command.hh"

#include <bed/command/ber_commands.hh>
#include <bed/arguments.hh>

#include <stdexcept>
#include <memory>

using namespace std;

namespace bed {

  namespace command {

    Base::Base(const Arguments &args)
      :
        args_(args)
    {
    }

    void execute(const Arguments &args)
    {
      unique_ptr<Base> c;
      switch (args.command) {
        case Command::WRITE_IDENTITY:
          c = make_unique<Write_Identity>(args);
          break;
        case Command::WRITE_DEFINITE:
          c = make_unique<Write_Definite>(args);
          break;
        case Command::WRITE_INDEFINITE:
          c = make_unique<Write_Indefinite>(args);
          break;
        case Command::WRITE_XML:
          c = make_unique<Write_XML>(args);
          break;
        case Command::PRETTY_WRITE_XML:
          c = make_unique<Pretty_Write_XML>(args);
          break;
        case Command::SEARCH_XPATH:
          c = make_unique<Search_XPath>(args);
          break;
        case Command::WRITE_BER:
          c = make_unique<Write_BER>(args);
          break;
        case Command::VALIDATE_XSD:
          c = make_unique<Validate_XSD>(args);
          break;
        case Command::EDIT:
          c = make_unique<Edit>(args);
          break;
        case Command::COMPUTE_ACI:
          c = make_unique<Compute_ACI>(args);
          break;
        case Command::WRITE_ACI:
          c = make_unique<Write_ACI>(args);
          break;
        default:
          throw logic_error("Command not implemented yet");
      }
      c->execute();

    }

  }

}
