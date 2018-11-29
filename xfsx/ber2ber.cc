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

#include "tlc_reader.hh"
#include "tlc_writer.hh"
#include "scratchpad.hh"

#include <ixxx/ixxx.hh>
#include <ixxx/util.hh>

#include <algorithm>
#include <stack>
#include <cassert>

#include <string.h>

using namespace std;

namespace xfsx {

  namespace ber {

      void write_identity(scratchpad::Simple_Reader<u8> &r,
              scratchpad::Simple_Writer<u8> &w)
      {
          TLC tlc;
          while (read_next(r, tlc)) {
              write_tag(w, tlc);
          }
      }

    void write_identity(const u8 *ibegin, const u8 *iend,
        u8 *begin, u8 *end)
    {
        auto r = scratchpad::mk_simple_reader(ibegin, iend);
        auto w = scratchpad::mk_simple_writer(begin, end);
        write_identity(r, w);
        w.flush();
    }

      void write_indefinite(scratchpad::Simple_Reader<u8> &r, scratchpad::Simple_Writer<u8> &w)
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
          TLC tlc;
          while (read_next(r, tlc)) {
              if (tlc.shape == Shape::PRIMITIVE) {
                  write_tag(w, tlc);
                  written_stack.back() += tlc.tl_size + tlc.length;
              } else { // CONSTRUCTED
                  written_stack.back() += tlc.tl_size;
                  if (!tlc.is_indefinite) {
                      length_stack.push_back(tlc.length);
                      written_stack.push_back(0);
                      tlc.init_indefinite();
                  }
                  write_tag(w, tlc);
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
      }


    u8 *write_indefinite(const u8 *ibegin, const u8 *iend,
        u8 *begin, u8 *end)
    {
        auto r = scratchpad::mk_simple_reader(ibegin, iend);
        auto w = scratchpad::mk_simple_writer(begin, end);
        write_indefinite(r, w);
        w.flush();
        return begin + w.pos();
    }

    void write_indefinite(const u8 *ibegin, const u8 *iend,
        const std::string &filename)
    {
        auto r = scratchpad::mk_simple_reader(ibegin, iend);
        auto w = scratchpad::mk_simple_writer<u8>(filename);
        write_indefinite(r, w);
        w.flush();
    }

    class Ber2Def {
        public:
            Ber2Def(scratchpad::Simple_Reader<u8> &r,
                    scratchpad::Simple_Writer<u8> &w);

            void process();
        private:
            void process_tag();
            void write_primitive();
            void write_constructed();
            void pop_constructed();

            scratchpad::Simple_Reader<u8> &r_;
            TLC tlc_;

            // stack for constructed tags - primitive tags are never
            // pushed
            std::deque<Unit> cons_stack_;
            size_t cons_stack_top_{0};
            // contains the lengths of definite constructed input tags
            std::deque<size_t> length_stack_;
            // counts against the length_stack, if top elements are equal
            // then the definite tag is finished
            std::deque<size_t> written_stack_;

            // writers are reused to avoid unnecessary object reconstructions
            // (thus no std::stack nor down-resize calls)
            // first writer directly writes to a file
            // for each definitive constructed a scratchpad writer is pushed
            std::deque<scratchpad::Simple_Writer<u8>*> writer_stack_;
            size_t writer_stack_top_{0};
            // temporary non-root writers
            std::deque<scratchpad::Simple_Writer<u8>> writers_;
    };

    Ber2Def::Ber2Def(scratchpad::Simple_Reader<u8> &r,
            scratchpad::Simple_Writer<u8> &w)
        :
            r_(r)
    {
        writer_stack_.push_back(&w);
        ++writer_stack_top_;
        length_stack_.push_back(0); // for symmetry to catch all
        written_stack_.push_back(0); // catch all such that it's never popped
    }
    void Ber2Def::process()
    {
        while (read_next(r_, tlc_)) {
            process_tag();
        }
        if (writer_stack_top_ != 1)
            throw runtime_error("unexpected writer stack - unbalanced tags?");
        if (cons_stack_top_)
            throw runtime_error("unexpected tlv stack - unbalanced tags?");

#ifndef NDEBUG
        writer_stack_[1]->flush();
        auto be = dynamic_cast<scratchpad::Scratchpad_Writer<u8>*>(
                    writer_stack_[1]->backend());
        auto b = be->pad().prelude();
        auto e = be->pad().begin();
        assert(e-b == 0);
#endif // NDEBUG

        writer_stack_[0]->flush();
    }
    void Ber2Def::pop_constructed()
    {
        cons_stack_[cons_stack_top_-1].init_length(
                writer_stack_[writer_stack_top_-1]->pos()
                );

        assert(writer_stack_top_ > 1);
        --writer_stack_top_;

        writer_stack_[writer_stack_top_]->flush();
        write_tag(*writer_stack_[writer_stack_top_-1],
                cons_stack_[cons_stack_top_-1]);
        assert(cons_stack_top_);
        --cons_stack_top_;

        auto &pad = dynamic_cast<scratchpad::Scratchpad_Writer<u8>*>(
                writer_stack_[writer_stack_top_]->backend())->pad();
        auto b = pad.prelude();
        auto e = pad.begin();
        // the Simple_Writer with File_Writer backend now supports
        // write-through with large buffers
        writer_stack_[writer_stack_top_-1]->write(b, e);
        writer_stack_[writer_stack_top_]->clear();
    }
    void Ber2Def::write_primitive()
    {
        auto &tlc = tlc_;
        written_stack_.back() += tlc.tl_size + tlc.length;
        if (tlc.is_eoc()) {
            pop_constructed();
        } else { // non-eoc primitive
            // we can write it as-is
            writer_stack_[writer_stack_top_-1]->write(tlc.begin,
                    tlc.begin+tlc.tl_size+tlc.length);
        }
    }
    void Ber2Def::write_constructed()
    {
        auto &tlc = tlc_;
        written_stack_.back() += tlc.tl_size;
        if (!tlc.is_indefinite) {
            length_stack_.push_back(tlc.length);
            written_stack_.push_back(0);
        }
        if (cons_stack_top_ >= cons_stack_.size())
            cons_stack_.push_back(tlc);
        else
            cons_stack_[cons_stack_top_] = tlc;
        ++cons_stack_top_;
        if (writer_stack_top_ >= writer_stack_.size()) {
            writers_.emplace_back(
                    std::unique_ptr<scratchpad::Writer<u8>>(
                        new scratchpad::Scratchpad_Writer<u8>()
                        ));
            writer_stack_.push_back(&writers_.back());
        }
        ++writer_stack_top_;
    }
    void Ber2Def::process_tag()
    {
        auto &tlc = tlc_;
        if (tlc.shape == Shape::PRIMITIVE) {
            write_primitive();
        } else { // Constructed
            write_constructed();
        }
        while (!length_stack_.empty()
                && length_stack_.back() == written_stack_.back()) {
            pop_constructed();
            assert(length_stack_.size() > 1);
            assert(written_stack_.size() > 1);
            written_stack_[written_stack_.size()-2] += length_stack_.back();
            written_stack_.resize(written_stack_.size()-1);
            length_stack_.resize(length_stack_.size()-1);
        }
    }

    void write_definite(scratchpad::Simple_Reader<u8> &r,
            scratchpad::Simple_Writer<u8> &w)
    {
        Ber2Def x2b(r, w);
        x2b.process();
        // already flushed in process
    }

    u8 *write_definite(const u8 *ibegin, const u8 *iend,
        u8 *begin, u8 *end)
    {
        auto r = scratchpad::mk_simple_reader(ibegin, iend);
        auto w = scratchpad::mk_simple_writer(begin, end);
        write_definite(r, w);
        return begin + w.pos();
    }

    void write_definite(const u8 *ibegin, const u8 *iend,
        const std::string &filename)
    {
        auto r = scratchpad::mk_simple_reader(ibegin, iend);
        auto w = scratchpad::mk_simple_writer<u8>(filename);
        write_definite(r, w);
    }


  }

}
