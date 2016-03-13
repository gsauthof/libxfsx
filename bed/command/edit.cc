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

#include "edit.hh"

#include <xfsx/ber2lxml.hh>
#include <xfsx/lxml2ber.hh>
#include <ixxx/util.hh>
#include <xxxml/util.hh>
#include <bed/arguments.hh>
#include "arguments.hh"
#include <xfsx/xml_writer_arguments.hh>
#include <xfsx/tap.hh>

#include <boost/lexical_cast.hpp>

#include <stdexcept>

using namespace std;

namespace bed {

  namespace command {

    void Edit::execute()
    {
      ixxx::util::Mapped_File in(args_.in_filename);

      xfsx::xml::Pretty_Writer_Arguments pretty_args(args_.asn_filenames);
      apply_arguments(args_, pretty_args);

      xxxml::doc::Ptr doc = xfsx::xml::l2::generate_tree(in.begin(), in.end(),
          pretty_args);

      for (auto &edit_op : args_.edit_ops) {
        switch (edit_op.command) {
          case Edit_Command::REMOVE:
            xxxml::util::remove(doc, edit_op.argv.at(0));
            break;
          case Edit_Command::REPLACE:
            xxxml::util::replace(doc, edit_op.argv.at(0),
                edit_op.argv.at(1), edit_op.argv.at(2));
            break;
          case Edit_Command::ADD:
            xxxml::util::add(doc, edit_op.argv.at(0),
                edit_op.argv.at(1), edit_op.argv.at(2));
            break;
          case Edit_Command::SET_ATT:
            xxxml::util::set_attribute(doc, edit_op.argv.at(0),
                edit_op.argv.at(1), edit_op.argv.at(2));
            break;
          case Edit_Command::INSERT:
            {
              if (!edit_op.argv.at(1).empty() && edit_op.argv.at(1)[0] == '@') {
                ixxx::util::Mapped_File m(edit_op.argv.at(1).substr(1));
                xxxml::util::insert(doc, edit_op.argv.at(0),
                    m.s_begin(), m.s_end(),
                    boost::lexical_cast<int>(edit_op.argv.at(2)));
              } else {
                xxxml::util::insert(doc, edit_op.argv.at(0),
                    edit_op.argv.at(1).data(),
                    edit_op.argv.at(1).data() + edit_op.argv.at(1).size(),
                    boost::lexical_cast<int>(edit_op.argv.at(2)));
              }
            }
            break;
          default:
            throw logic_error("Edit command not implemented yet");
        }
      }

      xfsx::BER_Writer_Arguments args;
      xfsx::tap::apply_grammar(args_.asn_filenames, args);

      xfsx::xml::l2::write_ber(doc, args_.out_filename, args);
    }

  } // command

} //bed
