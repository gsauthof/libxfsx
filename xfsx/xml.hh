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
#ifndef XFSX_XML_HH
#define XFSX_XML_HH

#include <utility>
#include <string>
#include <memory>

namespace xfsx {
    namespace scratchpad {
        template <typename Char> class Simple_Reader;
    }
  namespace xml {

    // Limitatons:
    // does not allow '>' in attribute values
    // where the XML spec does allow it.
    // The XML spec forbids '<', though:
    //
    // http://www.w3.org/TR/REC-xml/#NT-AttValue
    //
    // Comments must not be placed between primitve open/close tags
    class Reader {
        public:
            Reader(scratchpad::Simple_Reader<char> &src);

            bool next();
            std::pair<const char*, const char*> tag() const;
            std::pair<const char*, const char*> value() const;
        private:
            scratchpad::Simple_Reader<char> &src_;
            const std::pair<const char *, const char *> &p_;
            size_t inc_ {128 * 1024};
            std::pair<size_t, size_t> k_ {0, 0};
            size_t low_{0};
    };

    class Attribute_Traverser {
      private:
        std::pair<const char*, const char*> name_;
        std::pair<const char*, const char*> value_;
        const char *end_;
      public:
        Attribute_Traverser(const std::pair<const char*, const char*>
            &element, const std::pair<const char*, const char*> &element_name);
        bool has_more() const;
        Attribute_Traverser &operator++();
        const std::pair<const char*, const char*> &name() const;
        const std::pair<const char*, const char*> &value() const;
    };


    bool is_start_tag(const std::pair<const char*, const char*> &p);
    bool is_end_tag(const std::pair<const char*, const char*> &p);
    bool is_start_end_tag(const std::pair<const char*, const char*> &p);
    bool is_decl(const std::pair<const char*, const char*> &p);
    bool is_comment(const std::pair<const char*, const char*> &p);

    std::pair<const char*, const char*> element_name(
        const std::pair<const char*, const char*> &p);

    std::pair<const char*, const char*> attribute_name(const char *begin,
        const char *end);

    std::pair<const char*, const char*> attribute_value(const char *begin,
        const char *end);

    

  }

}

#endif
