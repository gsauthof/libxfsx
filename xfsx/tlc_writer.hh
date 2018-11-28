// 2018, Georg Sauthoff <mail@gms.tf>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef XFSX_TLC_WRITER_HH
#define XFSX_TLC_WRITER_HH

#include <utility>
#include <memory>

#include "scratchpad.hh"
#include "octet.hh"

namespace xfsx {

    struct Unit;

    // instantiated for TLC, TLV
    template<typename T>
        void write_tag(scratchpad::Simple_Writer<u8> &w, const T &tlc);
    template<>
        void write_tag(scratchpad::Simple_Writer<u8> &w, const Unit &u);


} // namespace xfsx

#endif // XFSX_TLC_WRITER_HH
