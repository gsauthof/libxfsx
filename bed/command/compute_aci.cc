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

#include "compute_aci.hh"

#include <xfsx/tap/traverser.hh>
#include <xfsx/traverser/tlc.hh>
#include <xfsx/xfsx.hh>
#include <xfsx/byte.hh>
#include <xfsx/scratchpad.hh>
#include <bed/arguments.hh>

#include <ixxx/util.hh>
#include <fcntl.h>

using namespace std;

namespace bed {

  namespace command {

    void Compute_ACI::execute()
    {
      using namespace xfsx::tap::traverser;

      auto m = ixxx::util::mmap_file(args_.in_filename);
      xfsx::Vertical_TLC tlc;

      Vertical_TLC_Proxy p(m.begin(), m.end(), tlc);
      Audit_Control_Info aci;
      aci(p, tlc);

      ixxx::util::FD fd;
      if (args_.out_filename.empty()) {
        fd = ixxx::util::FD(1);
        fd.set_keep_open(true);
      } else
        fd = ixxx::util::FD(args_.out_filename, O_CREAT | O_WRONLY | O_TRUNC,
            0666);
      using namespace xfsx;
      scratchpad::Simple_Writer<char> x(unique_ptr<scratchpad::Writer<char>>(
                  new scratchpad::File_Writer<char>(std::move(fd))));
      byte::writer::Base w(x);

      aci.print(w, args_.indent_size);
      x.flush();

    }

  } // command

} //bed
