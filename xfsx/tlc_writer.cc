// 2018, Georg Sauthoff <mail@gms.tf>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "tlc_writer.hh"

#include "xfsx.hh"

#include <stdexcept>

using namespace std;

namespace xfsx {

    template<typename T>
        void write_tag(scratchpad::Simple_Writer<u8> &w, const T &tlc)
        {
            size_t k = tlc.tl_size;
            if (tlc.shape == Shape::PRIMITIVE)
                k += tlc.length;
            auto o = w.begin_write(k);
            tlc.write(o, o+k);
            w.commit_write(k);
        }
    template
        void write_tag<TLC>(scratchpad::Simple_Writer<u8> &w, const TLC &tlc);
    template
        void write_tag<TLV>(scratchpad::Simple_Writer<u8> &w, const TLV &tlc);

    template<>
        void write_tag(scratchpad::Simple_Writer<u8> &w, const Unit &tlc)
        {
            size_t k = tlc.tl_size;
            auto o = w.begin_write(k);
            tlc.write(o, o+k);
            w.commit_write(k);
        }



} // namespace xfsx
