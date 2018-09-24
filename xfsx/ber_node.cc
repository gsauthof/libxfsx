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
#include "ber_node.hh"

#include "xfsx.hh"

#include <cassert>

using namespace std;


namespace xfsx {

  namespace ber {

    template <typename T>
    Node<T>::Node(Node &&o)
      :
        tlc_(std::move(o.tlc_)),
        children_(std::move(o.children_)),
        v_(std::move(o.v_))
    {
    }
    template <typename T>
    Node<T> &Node<T>::operator=(Node &&o)
    {
      tlc_ = std::move(o.tlc_);
      children_ = std::move(o.children_);
      v_ = std::move(o.v_);
      return *this;
    }

    template <typename T>
    Node<T>::Node() =default;
    template <typename T>
    Node<T>::Node(T &&tlc)
      :
        tlc_(std::move(tlc))
    {
      if (tlc_.shape == Shape::CONSTRUCTED)
        tlc_.length = 0;
    }
    template <typename T>
    Node<T> *Node<T>::push(unique_ptr<Node> e)
    {
      if (    (e->tlc_.shape == Shape::PRIMITIVE && !e->tlc_.tl_size)
           || (e->tlc_.shape == Shape::PRIMITIVE && !e->tlc_.t_size) )
        throw logic_error("tl_size must be initialized before (node push)");
      if (e->tlc_.shape == Shape::PRIMITIVE)
        tlc_.length += e->tlc_.tl_size + e->tlc_.length;
      children_.push_back(std::move(e));
      return children_.back().get();
    }
    template <typename T>
    void Node<T>::add_to_length(size_t l)
    {
      if (tlc_.shape != Shape::CONSTRUCTED)
        throw logic_error("node: length must not be added to a primitive tag");
      tlc_.length += l;
    }
    template <typename T>
    size_t Node<T>::init_length()
    {
      if (tlc_.shape != Shape::CONSTRUCTED)
        throw logic_error("node: length of primitive tag can't be initialized");
      size_t l = tlc_.length;
      if (tlc_.is_indefinite)
        tlc_.init_indefinite(); // resets tlc_length to 0
      else {
        uint8_t old_tl_size = tlc_.tl_size;
        tlc_.init_length();
        if (old_tl_size > tlc_.tl_size)
          tlc_.tl_size = old_tl_size;
      }
      return tlc_.tl_size + l;
    }
    template <typename T>
    u8 *Node<T>::write(u8 *begin, u8 *end) const
    {
      auto p = tlc_.write(begin, end);
      if (!v_.empty()) {
        p = copy(v_.begin(), v_.end(), p);
      }
      for (auto &child : children_)
        p = child->write(p, end);
      return p;
    }
    template <typename T>
    void Node<T>::mk_vector(size_t l)
    {
      v_.resize(l ? l : tlc_.length);
      auto p = v_.data();
      auto end = p + v_.size();
      for (auto &child : children_)
        p = child->write(p, end);
      children_ = deque<unique_ptr<Node> >();
    }
    template <typename T>
    const deque<unique_ptr<Node<T>> > &Node<T>::children() const
    {
      return children_;
    }
    template <typename T>
    bool Node<T>::is_indefinite() const
    {
      return tlc_.is_indefinite;
    }
    template <typename T>
    size_t Node<T>::length() const
    {
      return tlc_.length;
    }

    template class Node<TLC>;
    template class Node<TLV>;

  }
}
