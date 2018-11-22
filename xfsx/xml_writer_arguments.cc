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
#include <xfsx/xml_writer_arguments.hh>

#include <stdexcept>

#include <grammar/grammar.hh>
#include <grammar/asn1/grammar.hh>
#include <xfsx/tap.hh>

using namespace std;

namespace xfsx {

  namespace xml {

    Writer_Arguments default_writer_arguments;
    Pretty_Writer_Arguments default_pretty_args;

    Pretty_Writer_Arguments::Pretty_Writer_Arguments()
    {
    }
    Pretty_Writer_Arguments::Pretty_Writer_Arguments(
        const std::deque<std::string> &asn_filenames)
    {
      if (asn_filenames.empty())
        throw logic_error("No ASN.1 filename given");
      grammar::Grammar g = xfsx::tap::read_asn_grammar(asn_filenames);
      translator.push(xfsx::Klasse::APPLICATION,
          grammar::tag_translations(g, 1));
      xfsx::tap::init_dereferencer(g, dereferencer);
      xfsx::tap::init_typifier(typifier);

      name_translator = xfsx::Name_Translator(
          grammar::map_name_to_shape_klasse_tag(g));
    }

  }

}
