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


#ifndef XFSX_TRAVERSER_LXML_HH
#define XFSX_TRAVERSER_LXML_HH

#include <xxxml/util.hh>
#include <xfsx/integer.hh>
#include <xfsx/s_pair.hh>

namespace xfsx {
  namespace traverser {

    class LXML_Proxy {
      private:
        const Name_Translator &translator_;
      public:
        LXML_Proxy(const Name_Translator &translator)
          : translator_(translator) {}

        Tag_Int tag(const xxxml::util::DF_Traverser &t) const
        {
          auto shape_klasse_tag = translator_.translate(
              s_pair::mk_s_pair(xxxml::name(*t)));
          return std::get<2>(shape_klasse_tag);
        }
        uint32_t height(const xxxml::util::DF_Traverser &t) const
        {
          return t.height();
        }
        void string(const xxxml::util::DF_Traverser &t, std::string &s) const
        {
          s = xxxml::content((*t)->children);
        }
        uint64_t uint64(const xxxml::util::DF_Traverser &t) const
        {
          return integer::range_to_uint64(s_pair::mk_s_pair(
                xxxml::content((*t)->children)));
        }
        uint32_t uint32(const xxxml::util::DF_Traverser &t) const
        {
          return integer::range_to_uint32(s_pair::mk_s_pair(
                xxxml::content((*t)->children)));
        }

        void advance(xxxml::util::DF_Traverser &t) { t.advance(); }
        void skip_children(xxxml::util::DF_Traverser &t) { t.skip_children(); }
        bool eot(const xxxml::util::DF_Traverser &t) const { return t.eot(); }
    };

  } // traverser
} // xfsx

#endif
