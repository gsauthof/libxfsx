// 2018, Georg Sauthoff <mail@gms.tf>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef XFSX_TLC_WRITER_HH
#define XFSX_TLC_WRITER_HH

#include <utility>
#include <memory>

#include "scratchpad.hh"
#include "octet.hh"

namespace xfsx {


    // XXX derive from scratchpad::Simple_Writer
    // or just provide a generic write function, e.g.:
    // template<typename TLC>
    //     void write_tag(scratchpad::Simple_Writer<u8> &w, const TLC &tlc);
    template <typename T>
        class Simple_Writer {
            public:
                Simple_Writer();
                // this allows to write to a large enough memory mapped file
                // that is then truncated to global_pos
                Simple_Writer(u8 *begin, u8 *end);
                Simple_Writer(std::unique_ptr<scratchpad::Writer<u8>> &&backend);
                Simple_Writer(Simple_Writer &&);
                Simple_Writer &operator=(Simple_Writer &&);
                Simple_Writer(const Simple_Writer &) =delete;
                Simple_Writer &operator=(const Simple_Writer &) =delete;

                void write(const T &tlc);
                void write(const u8 *begin, const u8 *end);

                void flush();
                void sync();
                void set_sync(bool b);

                void clear();

                size_t   pos() const;

                scratchpad::Writer<u8> *backend();

            private:
                size_t  local_pos_  {0};
                size_t  global_pos_ {0};
                u8     *begin_      {nullptr};
                u8     *end_        {nullptr};
                std::unique_ptr<scratchpad::Writer<u8>> backend_;
        };

    template <typename T>
        Simple_Writer<T> mk_tlc_writer(const std::string &filename)
        {
            return Simple_Writer<T>(std::unique_ptr<scratchpad::Writer<u8>>(
                       new scratchpad::File_Writer<u8>(filename)
                        ));
        }
    template <typename T>
        Simple_Writer<T> mk_tlc_writer(ixxx::util::FD &&fd)
        {
            return Simple_Writer<T>(std::unique_ptr<scratchpad::Writer<u8>>(
                        new scratchpad::File_Writer<u8>(std::move(fd))
                        ));
        }
    template <typename T>
        Simple_Writer<T> mk_tlc_writer_mapped(const std::string &filename, size_t size)
        {
            return Simple_Writer<T>(std::unique_ptr<scratchpad::Writer<u8>>(
                       new scratchpad::Mapped_Writer<u8>(filename, size)
                        ));
        }


} // namespace xfsx

#endif // XFSX_TLC_WRITER_HH
