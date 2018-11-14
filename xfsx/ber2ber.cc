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

      void write_indefinite(Simple_Reader<TLC> &r, Simple_Writer<TLC> &w)
      {
          // we only put the length of each definite constructed tag on it
          // primitive and indefinite constructed tags can be written
          // immediately
          deque<size_t> length_stack;
          length_stack.push_back(0); // such that catch-all root isn't popped
          // this counts against the length_stack
          // when both top elements are equal than the constructed tag
          // is done and we have to emit an EOC
          deque<size_t> written_stack;
          written_stack.push_back(0); // catch-all root
          array<u8, 2> eoc = {0, 0};
          while (r.next()) {
              TLC &tlc = r.tlc();
              if (tlc.shape == Shape::PRIMITIVE) {
                  w.write(tlc);
                  written_stack.back() += tlc.tl_size + tlc.length;
              } else { // CONSTRUCTED
                  written_stack.back() += tlc.tl_size;
                  if (!tlc.is_indefinite) {
                      length_stack.push_back(tlc.length);
                      written_stack.push_back(0);
                      tlc.init_indefinite();
                  }
                  w.write(tlc);
              }
              while (!length_stack.empty()
                      && length_stack.back() == written_stack.back()) {
                  w.write(eoc.begin(), eoc.end());
                  assert(length_stack.size() > 1);
                  assert(written_stack.size() > 1);
                  written_stack[written_stack.size()-2] += length_stack.back();
                  length_stack.resize(length_stack.size()-1);
                  written_stack.resize(written_stack.size()-1);
              }
          }
          w.flush();
      }


    u8 *write_indefinite(const u8 *ibegin, const u8 *iend,
        u8 *begin, u8 *end)
    {
        Simple_Reader<TLC> r(ibegin, iend);
        Simple_Writer<TLC> w(begin, end);
        write_indefinite(r, w);
        return begin + w.pos();
    }

    void write_indefinite(const u8 *ibegin, const u8 *iend,
        const std::string &filename)
    {
        Simple_Reader<TLC> r(ibegin, iend);
        auto w = mk_tlc_writer<TLC>(filename);
        write_indefinite(r, w);
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
