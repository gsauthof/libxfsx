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
#include <bed/arguments.hh>

#include <ixxx/util.hh>
#include <fcntl.h>

using namespace std;

namespace bed {

  namespace command {

    struct Indent {
      unsigned i;
      Indent(unsigned i) : i(i) {}
      Indent operator()(unsigned k) const { return i*k; }
    };
    /*
    ostream &operator<<(ostream &o, const Indent &i)
    {
      std::fill_n(std::ostreambuf_iterator<char>(o), i.i, ' ');
      return o;
    }
    */
    xfsx::byte::writer::Base &operator<<(xfsx::byte::writer::Base &o, const Indent &i)
    {
      o.fill(i.i);
      return o;
    }

    void Compute_ACI::execute()
    {
      using namespace xfsx::tap::traverser;

      ixxx::util::Mapped_File m(args_.in_filename);
      xfsx::Vertical_TLC tlc;

      Vertical_TLC_Proxy p(m.begin(), m.end(), tlc);
      CDR_Count count;
      Charge_Sum sum;
      Timestamp<Less_Tag> first_charging;
      Timestamp<Greater_Tag> last_charging;
      Traverse t;
      t(p, tlc, count, sum, first_charging, last_charging);

      ixxx::util::FD fd;
      if (args_.out_filename.empty()) {
        fd = ixxx::util::FD(1);
        fd.set_keep_open(true);
      } else
        fd = ixxx::util::FD(args_.out_filename, O_CREAT | O_WRONLY, 0666);
      xfsx::byte::writer::File o(fd);

      Indent i(args_.indent_size);
      o << "<AuditControlInfo>\n" << i <<
"<EarliestCallTimeStamp>\n" << i(2) <<
  "<LocalTimeStamp>" << first_charging().first << "</LocalTimeStamp>\n"
    << i(2) <<
  "<UtcTimeOffset>" << first_charging().second << "</UtcTimeOffset>\n" << i <<
"</EarliestCallTimeStamp>\n" << i <<
"<LatestCallTimeStamp>\n" << i(2) <<
  "<LocalTimeStamp>" << last_charging().first << "</LocalTimeStamp>\n"
    << i(2) <<
  "<UtcTimeOffset>" << last_charging().second << "</UtcTimeOffset>\n" << i <<
"</LatestCallTimeStamp>\n" << i <<
"<TotalCharge>" << sum() << "</TotalCharge>\n" << i <<
"<TotalTaxValue>0</TotalTaxValue>\n" << i <<
"<TotalDiscountValue>0</TotalDiscountValue>\n" << i <<
"<CallEventDetailsCount>" << count() << "</CallEventDetailsCount>\n"
"</AuditControlInfo>\n"
          ;

    }

  } // command

} //bed
