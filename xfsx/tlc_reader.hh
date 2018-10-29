// 2018, Georg Sauthoff <mail@gms.tf>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef XFSX_TLC_READER_HH
#define XFSX_TLC_READER_HH

#include <memory>
#include <utility>

#include <xfsx/octet.hh>
#include <xfsx/scratchpad.hh>

namespace xfsx {

    // We use virtual inheritance such that code can construct
    // a Simple_Reader with different backends at runtime without
    // that changing its instantiation (i.e. type)
    template <typename Char>
        class Simple_Reader_Backend {
            public:
                virtual ~Simple_Reader_Backend();
                // function returns updated begin end,
                // 1st argument specifies how many prefix bytes can be discarded
                // 2nd argument specifies minimum read-size - which is
                //     respected by the function unless it encounters EOF before,
                //     which isn't an error
                virtual std::pair<const Char*, const Char*>
                    read_more(size_t forget_cnt, size_t want_cnt) = 0;
        };
    template <typename T>
        class Simple_Reader { // new Basic_Reader
            public:
                Simple_Reader(const u8 *begin, const u8 *end);
                Simple_Reader(std::unique_ptr<Simple_Reader_Backend<u8>> &&backend);
                Simple_Reader();
                Simple_Reader(const Simple_Reader &) =delete;
                Simple_Reader &operator=(const Simple_Reader &) =delete;
                Simple_Reader(Simple_Reader &&);
                Simple_Reader &operator=(Simple_Reader &&);

                bool     next();
                const T &tlc() const;
                size_t   pos() const;
            private:
                T         tlc_;
                size_t    global_pos_ {0};
                size_t    local_pos_  {0};
                const u8 *begin_      {nullptr};
                const u8 *end_        {nullptr};
                // This decouples the reader from backends like Scratchpad
                // if it's empty then the begin/end range includes the complete file
                std::unique_ptr<Simple_Reader_Backend<u8>> backend_;
        };
    template <typename Char>
        class Memory_Reader : public Simple_Reader_Backend<Char> {
            public:
                Memory_Reader(const Char *begin, const Char *end);
                std::pair<const Char*, const Char*>
                    read_more(size_t forget_cnt, size_t want_cnt) override;
            private:
                const Char *begin_{nullptr};
                const Char *end_{nullptr};
        };
    template <typename Char>
        class Scratchpad_Reader : public Simple_Reader_Backend<Char> {
            public:
                Scratchpad_Reader(const Scratchpad_Reader &) =delete;
                Scratchpad_Reader &operator=(const Scratchpad_Reader &) =delete;
                Scratchpad_Reader(Scratchpad_Reader &&);
                Scratchpad_Reader &operator=(Scratchpad_Reader &&);
                Scratchpad_Reader();

                Scratchpad_Reader(const std::string &filename);
                Scratchpad_Reader(ixxx::util::FD &&fd);
                std::pair<const Char*, const Char*>
                    read_more(size_t forget_cnt, size_t want_cnt) override;
            private:
                Scratchpad_Source_File<Char> source_;
        };

    template <typename T>
        Simple_Reader<T> mk_tlc_reader(const std::string &filename)
        {
            return Simple_Reader<T>(std::unique_ptr<Scratchpad_Reader<u8>>(
                        new Scratchpad_Reader<u8>(filename)));
        }
    template <typename T>
        Simple_Reader<T> mk_tlc_reader(ixxx::util::FD &&fd)
        {
            return Simple_Reader<T>(std::unique_ptr<Scratchpad_Reader<u8>>(
                        new Scratchpad_Reader<u8>(std::move(fd))));
        }

} // namespace xfsx

#endif // XFSX_TLC_READER_HH
