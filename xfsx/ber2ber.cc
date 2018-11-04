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
#include "ber2ber.hh"

#include "xfsx.hh"
#include "ber_node.hh"

#include <ixxx/ixxx.hh>
#include <ixxx/util.hh>

#include <algorithm>
#include <stack>
#include <cassert>

#include <string.h>

using namespace std;

namespace xfsx {

  namespace ber {

      void write_identity(Simple_Reader<TLC> &r, Simple_Writer<TLC> &w)
      {
          while (r.next()) {
              const TLC &tlc = r.tlc();
              w.write(tlc);
          }
          w.flush();
      }

    void write_identity(const u8 *ibegin, const u8 *iend,
        u8 *begin, u8 *end)
    {
        Simple_Reader<TLC> r(ibegin, iend);
        Simple_Writer<TLC> w(begin, end);
        write_identity(r, w);
    }


    u8 *write_indefinite(const u8 *ibegin, const u8 *iend,
        u8 *begin, u8 *end)
    {
      u8 *p = begin;
      Vertical_Reader r(ibegin, iend);
      uint32_t last_height = 0;
      stack<bool> marker_stack;
      for (auto &tlc : r) {
        Unit &u = tlc;
        if (tlc.height < last_height) {
          Unit eoc{Unit::EOC()};
          assert(marker_stack.size() >= last_height - tlc.height);
          for (unsigned i = 0; i < last_height - tlc.height; ++i) {
            if (marker_stack.top())
              p = eoc.write(p, end);
            marker_stack.pop();
          }
        }
        last_height = tlc.height;
        switch (u.shape) {
          case Shape::PRIMITIVE:
            p = u.write(p, end);
            p = copy(tlc.begin + u.tl_size, tlc.begin + u.tl_size + u.length,
                p);
            break;
          case Shape::CONSTRUCTED:
            if (u.is_indefinite) {
              p = u.write(p, end);
              marker_stack.push(false);
            } else {
              if (u.length) {
                u.init_indefinite();
                p = u.write(p, end);
                marker_stack.push(true);
              } else {
                u.init_indefinite();
                p = u.write(p, end);
                Unit eoc{Unit::EOC()};
                p = eoc.write(p, end);
              }
            }
            break;
        }
      }
      Unit eoc{Unit::EOC()};
      assert(marker_stack.size() >= last_height);
      for (unsigned i = 0; i < last_height; ++i) {
        if (marker_stack.top())
          p = eoc.write(p, end);
        marker_stack.pop();
      }
      return p;
    }

    void write_indefinite(const u8 *ibegin, const u8 *iend,
        const std::string &filename)
    {
      ixxx::util::FD fd(filename, O_CREAT | O_RDWR, 0666);
      size_t n = (iend-ibegin)*2;
      ixxx::posix::ftruncate(fd, n);
      {
          auto f = ixxx::util::mmap_file(fd, false, true, n);
          auto r = write_indefinite(ibegin, iend, f.begin(), f.end());
          n = r - f.begin();
      }
      // apparently, under newer wine versions (e.g. >= 3.17), ftruncate fails
      // with Invalid Argument if the mapping is still established
      ixxx::posix::ftruncate(fd, n);
      fd.close();
    }


    u8 *write_definite(const u8 *ibegin, const u8 *iend,
        u8 *begin, u8 *end)
    {
      Skip_EOC_Reader r(ibegin, iend);
      uint32_t last_height = 0;

      TLC root_tlc;
      root_tlc.shape = Shape::CONSTRUCTED;
      TLC_Node root(std::move(root_tlc));
      stack<TLC_Node*> node_stack;
      node_stack.push(&root);
      for (auto &tlc : r) {
        if (tlc.height < last_height) {
          for (unsigned i = 0; i < last_height - tlc.height; ++i) {
            assert(!node_stack.empty());
            auto l = node_stack.top()->init_length();
            node_stack.top()->mk_vector();
            node_stack.pop();
            node_stack.top()->add_to_length(l);
          }
        }
        last_height = tlc.height;
        switch (tlc.shape) {
          case Shape::PRIMITIVE:
            node_stack.top()->push(make_unique<TLC_Node>(std::move(tlc)));
            break;
          case Shape::CONSTRUCTED:
            if (tlc.is_indefinite) {
              tlc.is_indefinite = false;
              node_stack.push(
                  node_stack.top()->push(make_unique<TLC_Node>(std::move(tlc))));
            } else {
              if (tlc.length) {
                node_stack.push(
                    node_stack.top()->push(
                      make_unique<TLC_Node>(std::move(tlc))));
              } else {
                node_stack.top()->add_to_length(tlc.tl_size);
                node_stack.top()->push(make_unique<TLC_Node>(std::move(tlc)));
              }
            }
            break;
        }
      }
      for (unsigned i = 0; i < last_height; ++i) {
        assert(!node_stack.empty());
        auto l = node_stack.top()->init_length();
        node_stack.top()->mk_vector();
        node_stack.pop();
        node_stack.top()->add_to_length(l);
      }
      assert(!node_stack.empty());
      u8 *p = begin;
      for (auto &child : root.children())
        p = child->write(p, end);
      return p;
    }

    void write_definite(const u8 *ibegin, const u8 *iend,
        const std::string &filename)
    {
      ixxx::util::FD fd(filename, O_CREAT | O_RDWR, 0666);
      size_t n = (iend-ibegin)*2;
      ixxx::posix::ftruncate(fd, n);
      {
          auto f = ixxx::util::mmap_file(fd, false, true, n);
          auto r = write_definite(ibegin, iend, f.begin(), f.end());
          n = r - f.begin();
      }
      // apparently, under newer wine versions (e.g. >= 3.17), ftruncate fails
      // with Invalid Argument if the mapping is still established
      ixxx::posix::ftruncate(fd, n);
      fd.close();
    }


  }

}
