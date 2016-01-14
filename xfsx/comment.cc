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
#include "comment.hh"

#include <algorithm>

using namespace std;


namespace xfsx {

  namespace comment {

    namespace style {


      const char XML::open[]   = "<!--";
      const char XML::close[]  = "-->";
      const char Dash::open[]  = "--";
      const char Dash::close[] = "--";
      const char C::open[]     = "/*";
      const char C::close[]    = "*/";


    }

    template <typename T>
      Basic_Splicer<T>::Basic_Splicer(const char *begin, const char *end)
      :
        begin_(begin),
        end_(end)
      {
      }

    template <typename T>
    typename Basic_Splicer<T>::iterator Basic_Splicer<T>::begin()
    {
      return iterator(begin_, end_);
    }
    template <typename T>
    typename Basic_Splicer<T>::iterator Basic_Splicer<T>::end()
    {
      return iterator(end_, end_);
    }

    template <typename T>
    Basic_Splicer<T>::iterator::iterator()
      :
        p_(nullptr, nullptr),
        end_(nullptr)
    {
    }

    template <typename T>
    Basic_Splicer<T>::iterator::iterator(const char *begin, const char *end)
      :
        end_(end)
    {
      p_.first = begin;
      p_.second = search(p_.first, end_, T::open, T::open + sizeof(T::open)-1);
    }

    template <typename T>
    const std::pair<const char*, const char*> &
    Basic_Splicer<T>::iterator::operator*() const
    {
      return p_;
    }
    template <typename T>
      typename Basic_Splicer<T>::iterator
        &Basic_Splicer<T>::iterator::operator++()
    {
      p_.first = search(p_.second + sizeof(T::open)-1, end_,
          T::close, T::close + sizeof(T::close)-1);
      if (p_.first != end_)
        p_.first += sizeof(T::close)-1;
      p_.second = search(p_.first, end_, T::open, T::open + sizeof(T::open)-1);
      return *this;
    }

    template <typename T>
    bool Basic_Splicer<T>::iterator::operator==(
        const Basic_Splicer<T>::iterator &other) const
    {
      return p_ == other.p_;
    }
    template <typename T>
    bool Basic_Splicer<T>::iterator::operator!=(const Basic_Splicer<T>::iterator &other) const
    {
      return !(*this == other);
    }


    template class Basic_Splicer<style::XML>;
    template class Basic_Splicer<style::Dash>;
    template class Basic_Splicer<style::C>;



  }

}
