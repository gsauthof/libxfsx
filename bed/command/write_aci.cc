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

#include "write_aci.hh"

#include <xfsx/tap/traverser.hh>
#include <xfsx/traverser/tlc.hh>
#include <xfsx/xfsx.hh>
#include <xfsx/byte.hh>
#include <xfsx/lxml2ber.hh>
#include <xfsx/tap.hh>
#include <bed/arguments.hh>
#include <xxxml/xxxml.hh>

#include <ixxx/util.hh>
#include <fcntl.h>

using namespace std;

using u8 = xfsx::u8;

namespace bed {

  namespace command {

    void Write_ACI::execute()
    {
      using namespace xfsx::tap::traverser;

      auto m = ixxx::util::mmap_file(args_.in_filename);
      xfsx::Vertical_TLC tlc;

      Vertical_TLC_Proxy p(m.begin(), m.end(), tlc);

      if (p.tag(tlc) != grammar::tap::TRANSFER_BATCH)
        throw runtime_error("Format currently not supported");
      p.advance(tlc);
      const u8 *begin = tlc.begin;

      Audit_Control_Info aci;
      aci(p, tlc);

      const u8 *end = m.end();
      if (p.tag(tlc) == grammar::tap::AUDIT_CONTROL_INFO)
        end = tlc.begin;

      xfsx::byte::writer::Memory b;
      aci.comment = "Generated by libxfsx";
      aci.finalize();
      aci.print(b);
      auto doc = xxxml::read_memory(b.begin(), b.end(), nullptr, nullptr, 0);

      xfsx::BER_Writer_Arguments args;
      xfsx::tap::apply_grammar(args_.asn_filenames, args);
      xfsx::xml::l2::BER_Writer bw(doc, args);
      bw.write();

      size_t n = (end-begin) + bw.size();

      xfsx::Unit tb(xfsx::Klasse::APPLICATION, grammar::tap::TRANSFER_BATCH, n);

      ixxx::util::FD fd(ixxx::util::FD(
            args_.out_filename, O_CREAT | O_WRONLY | O_TRUNC, 0666));
      xfsx::byte::writer::File o(fd);

      auto x = reinterpret_cast<u8 *>(o.obtain_chunk(tb.tl_size));
      x = tb.write(x, x + tb.tl_size);
      o.write(reinterpret_cast<const char*>(begin),
          reinterpret_cast<const char*>(end));
      x = reinterpret_cast<u8 *>(o.obtain_chunk(bw.size()));
      bw.store(x, x + bw.size());

      o.flush();


    }

  } // command

} //bed
