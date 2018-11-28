// 2018, Georg Sauthoff <mail@gms.tf>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "tlc_reader.hh"

#include "xfsx.hh"

#include <stdexcept>

using namespace std;

namespace xfsx {

    template<> bool read_next(scratchpad::Simple_Reader<u8> &r, TLC &tlc)
    {
        bool t = r.next(20);
        if (!t)
            return false;

        tlc.read(r.window().first, r.window().second);

        size_t k = tlc.tl_size;
        if (tlc.shape == Shape::PRIMITIVE) {
            k += tlc.length;
            auto i = r.next(k);
            r.check_available(k);
            if (i == 2)
                tlc.begin = r.window().first;
        }
        r.forget(k);
        return true;
    }
    template<> bool read_next(scratchpad::Simple_Reader<u8> &r, Unit &tlc)
    {
        bool t = r.next(20);
        if (!t)
            return false;

        tlc.read(r.window().first, r.window().second);

        size_t k = tlc.tl_size;
        if (tlc.shape == Shape::PRIMITIVE) {
            k += tlc.length;
            r.next(k);
            r.check_available(k);
        }
        r.forget(k);
        return true;
    }

} // namespace xfsx
