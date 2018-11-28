// 2018, Georg Sauthoff <mail@gms.tf>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef XFSX_TLC_READER_HH
#define XFSX_TLC_READER_HH

#include <memory>
#include <utility>

#include <xfsx/octet.hh>
#include <xfsx/scratchpad.hh>

namespace xfsx {
    class TLC;
    class Unit;

    template<typename T> bool read_next(scratchpad::Simple_Reader<u8> &r,
            T &tlc);
    template<> bool read_next(scratchpad::Simple_Reader<u8> &r, TLC &tlc);
    template<> bool read_next(scratchpad::Simple_Reader<u8> &r, Unit &tlc);


} // namespace xfsx

#endif // XFSX_TLC_READER_HH
