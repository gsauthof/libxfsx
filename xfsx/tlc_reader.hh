// 2018, Georg Sauthoff <mail@gms.tf>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef XFSX_TLC_READER_HH
#define XFSX_TLC_READER_HH

#include <memory>
#include <utility>

#include <xfsx/octet.hh>
#include <xfsx/scratchpad.hh>

namespace xfsx {

    template <typename T>
        class Simple_Reader { // new Basic_Reader
            public:
                Simple_Reader(const u8 *begin, const u8 *end);
                Simple_Reader(std::unique_ptr<scratchpad::Reader<u8>> &&backend);
                Simple_Reader();
                Simple_Reader(const Simple_Reader &) =delete;
                Simple_Reader &operator=(const Simple_Reader &) =delete;
                Simple_Reader(Simple_Reader &&);
                Simple_Reader &operator=(Simple_Reader &&);

                bool     next();
                const T &tlc() const;
                      T &tlc();
                size_t   pos() const;
            private:
                T         tlc_;
                size_t    global_pos_ {0};
                size_t    local_pos_  {0};
                const u8 *begin_      {nullptr};
                const u8 *end_        {nullptr};
                // This decouples the reader from backends like Scratchpad
                // if it's empty then the begin/end range includes the complete file
                std::unique_ptr<scratchpad::Reader<u8>> backend_;
        };
    template <typename T>
        Simple_Reader<T> mk_tlc_reader(const std::string &filename)
        {
            return Simple_Reader<T>(std::unique_ptr<scratchpad::Reader<u8>>(
                        new scratchpad::File_Reader<u8>(filename)));
        }
    template <typename T>
        Simple_Reader<T> mk_tlc_reader_mapped(const std::string &filename)
        {
            return Simple_Reader<T>(std::unique_ptr<scratchpad::Reader<u8>>(
                        new scratchpad::Mapped_Reader<u8>(filename)));
        }
    template <typename T>
        Simple_Reader<T> mk_tlc_reader(ixxx::util::FD &&fd)
        {
            return Simple_Reader<T>(std::unique_ptr<scratchpad::Reader<u8>>(
                        new scratchpad::File_Reader<u8>(std::move(fd))));
        }


} // namespace xfsx

#endif // XFSX_TLC_READER_HH
