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
#ifndef XFSX_COMMENT_HH
#define XFSX_COMMENT_HH

#include <utility>

namespace xfsx {

  namespace comment {

    namespace style {

      struct XML {
        static const char open[];
        static const char close[];
      };
      struct Dash {
        static const char open[];
        static const char close[];
      };
      struct C {
        static const char open[];
        static const char close[];
      };

    }

    template <typename T>
    class Basic_Splicer {
      private:
        const char *begin_;
        const char *end_;
      public:
        Basic_Splicer(const char *begin, const char *end);

        class iterator {
          private:
            std::pair<const char*, const char*> p_;
            const char *end_;
          public:
            iterator();
            iterator(const char *begin, const char *end);
            const std::pair<const char*, const char*> &operator*() const;
            iterator &operator++();
            bool operator==(const iterator &other) const;
            bool operator!=(const iterator &other) const;
        };

        iterator begin();
        iterator end();
    };

    using XML_Splicer  = Basic_Splicer<style::XML>;
    using Dash_Splicer = Basic_Splicer<style::Dash>;
    using C_Splicer    = Basic_Splicer<style::C>;

  }


}


#endif
