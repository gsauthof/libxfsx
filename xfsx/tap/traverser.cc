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

#include "traverser.hh"

#include <xfsx/byte.hh>

namespace xfsx {

  namespace tap {

    namespace traverser {

      void Audit_Control_Info::print(xfsx::byte::writer::Base &o,
          unsigned indent)
      {
        xfsx::byte::writer::Indent i(indent);
        o << "<AuditControlInfo>\n" << i <<
          "<EarliestCallTimeStamp>\n" << i(2) <<
          "<LocalTimeStamp>" << first_timestamp().first << "</LocalTimeStamp>\n"
          << i(2) <<
          "<UtcTimeOffset>" << first_timestamp().second << "</UtcTimeOffset>\n" << i <<
          "</EarliestCallTimeStamp>\n" << i <<
          "<LatestCallTimeStamp>\n" << i(2) <<
          "<LocalTimeStamp>" << last_timestamp().first << "</LocalTimeStamp>\n"
          << i(2) <<
          "<UtcTimeOffset>" << last_timestamp().second << "</UtcTimeOffset>\n" << i <<
          "</LatestCallTimeStamp>\n" << i <<
          "<TotalCharge>" << sum() << "</TotalCharge>\n" << i <<
          "<TotalTaxValue>" << tax_sum() << "</TotalTaxValue>\n" << i <<
          "<TotalDiscountValue>0</TotalDiscountValue>\n" << i <<
          "<CallEventDetailsCount>" << count() << "</CallEventDetailsCount>\n";

        if (!comment.empty()) {
          o << "<OperatorSpecInfoList>\n" << i(2) <<
       "<OperatorSpecInformation>" << comment << "</OperatorSpecInformation>\n" << i <<
            "</OperatorSpecInfoList>\n";
        }
          o << "</AuditControlInfo>\n"
          ;
      }

      void Audit_Control_Info::finalize()
      {
        first_timestamp.finalize();
        last_timestamp.finalize();
      }


    } // traverser

  } // tap

} // xfsx
