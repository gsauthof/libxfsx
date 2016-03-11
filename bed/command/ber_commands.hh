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
#ifndef BED_COMMAND_BER_COMMANDS_HH
#define BED_COMMAND_BER_COMMANDS_HH

#include <bed/command.hh>

#include <bed/command/compute_aci.hh>

namespace bed {

  namespace command {

    class Write_Identity : public Base {
      public:
        using Base::Base;
        void execute() override;
    };

    class Write_Definite : public Base {
      public:
        using Base::Base;
        void execute() override;
    };

    class Write_Indefinite : public Base {
      public:
        using Base::Base;
        void execute() override;
    };

    class Write_XML : public Base {
      public:
        using Base::Base;
        void execute() override;
    };

    class Pretty_Write_XML : public Base {
      public:
        using Base::Base;
        void execute() override;
    };

    class Search_XPath : public Base {
      public:
        using Base::Base;
        void execute() override;
    };

    class Write_BER : public Base {
      public:
        using Base::Base;
        void execute() override;
    };

    class Validate_XSD : public Base {
      public:
        using Base::Base;
        void execute() override;
    };

    class Edit : public Base {
      public:
        using Base::Base;
        void execute() override;
    };

  }

}

#endif
