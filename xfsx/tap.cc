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
#include "tap.hh"

#include "xfsx.hh"
#include <grammar/asn1/grammar.hh>
#include <grammar/asn1/mini_parser.hh>

using namespace std;

namespace xfsx {

  namespace tap {

    grammar::Grammar read_asn_grammar(
        const std::deque<std::string> &asn_filenames)
    {
      grammar::Grammar g = grammar::asn1::mini::parse_files(asn_filenames);
      grammar::asn1::add_terminals(g);
      g.init_links();
      grammar::erase_unreachable_symbols(g);
      grammar::derive_tags(g);
      return g;
    }

    void init_dereferencer(const grammar::Grammar &g,
        xfsx::Tag_Dereferencer &dereferencer)
    {
      dereferencer.push(xfsx::Klasse::APPLICATION,
          grammar::tag_closure(g, "BCDString", 1),
          xfsx::Klasse::UNIVERSAL, 128u
          );
      dereferencer.push(xfsx::Klasse::APPLICATION,
          grammar::tag_closure(g, "OCTET STRING", 1),
          xfsx::Klasse::UNIVERSAL, xfsx::universal::OCTET_STRING);
      dereferencer.push(xfsx::Klasse::APPLICATION,
          grammar::tag_closure(g, "INTEGER", 1),
          xfsx::Klasse::UNIVERSAL, xfsx::universal::INTEGER);
    }

    void init_typifier(xfsx::Tag_Typifier &typifier)
    {
      typifier.push(xfsx::Klasse::UNIVERSAL, 128u, xfsx::Type::BCD);
      typifier.push(xfsx::Klasse::UNIVERSAL, xfsx::universal::OCTET_STRING,
          xfsx::Type::OCTET_STRING);
      typifier.push(xfsx::Klasse::UNIVERSAL, xfsx::universal::INTEGER,
          xfsx::Type::INT_64);
    }

  }
}
