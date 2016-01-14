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
#ifndef XFSX_BER_NODE_HH
#define XFSX_BER_NODE_HH

#include <vector>
#include <deque>
#include <memory>
#include <stdint.h>

#include "xfsx.hh"

namespace xfsx {

  namespace ber {

    template <typename T>
    class Node {
      private:
        T tlc_;
        // XXX replace with boost intrusive slist
        std::deque<std::unique_ptr<Node> > children_;
        std::vector<uint8_t> v_;
      public:
        Node(const Node &) =delete;
        Node &operator=(const Node&) =delete;
        Node();
        Node(T &&tlc);
        Node(Node &&o);
        Node &operator=(Node &&o);
        Node<T> *push(std::unique_ptr<Node> e);
        void add_to_length(size_t l);
        size_t init_length();
        uint8_t *write(uint8_t *begin, uint8_t *end) const;
        const std::deque<std::unique_ptr<Node<T> > > &children() const;

        void mk_vector(size_t l = 0);

        bool is_indefinite() const;
        size_t length() const;
    };

    using TLC_Node = Node<TLC>;
    using TLV_Node = Node<TLV>;


  }

}

#endif
