// 2018, Georg Sauthoff <mail@gms.tf>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef XFSX_TLC_WRITER_HH
#define XFSX_TLC_WRITER_HH

#include <utility>
#include <memory>

#include "scratchpad.hh"
#include "octet.hh"

namespace xfsx {

    template <typename Char>
        class Simple_Writer_Backend {
            public:
                virtual ~Simple_Writer_Backend();

                // we split this into prepare_write() and write_some() because the Simple_Writer
                // writes just after prepare_write() returns over the block boundary,
                // thus, very likely we just have to move a few bytes when write_some is called

                // function returns updated begin end
                // 1st argument specifies head bytes that can be written
                // 2nd argument specifies minimum bytes needed at the back,
                //     function may provide more
                virtual std::pair<Char*, Char*> prepare_write(
                        size_t forget_cnt, size_t want_cnt) = 0;

                // function returns updated begin end
                virtual std::pair<Char*, Char*> write_some(size_t forget_cnt) = 0;


                // completely flush the buffer
                virtual void flush() = 0;
        };

    template <typename T>
        class Simple_Writer {
            public:
                Simple_Writer();
                // this allows to write to a large enough memory mapped file
                // that is then truncated to global_pos
                Simple_Writer(u8 *begin, u8 *end);
                Simple_Writer(std::unique_ptr<Simple_Writer_Backend<u8>> &&backend);
                Simple_Writer(Simple_Writer &&);
                Simple_Writer &operator=(Simple_Writer &&);
                Simple_Writer(const Simple_Writer &) =delete;
                Simple_Writer &operator=(const Simple_Writer &) =delete;

                void write(const T &tlc);

                void flush();

                size_t   pos() const;

            private:
                size_t  local_pos_  {0};
                size_t  global_pos_ {0};
                u8     *begin_      {nullptr};
                u8     *end_        {nullptr};
                std::unique_ptr<Simple_Writer_Backend<u8>> backend_;
        };

    // e.g. for writing into a memory mapped file
    template <typename Char>
        class Memory_Writer : public Simple_Writer_Backend<Char> {
            public:
                Memory_Writer(Char *begin, Char *end);
                std::pair<Char*, Char*> prepare_write(size_t forget_cnt,
                        size_t want_cnt) override;
                std::pair<Char*, Char*> write_some(size_t forget_cnt) override;
                void flush() override;
            private:
                Char *begin_{nullptr};
                Char *end_{nullptr};
        };

    // i.e. the [prelude..begin) pad range contains everything written
    template <typename Char>
        class Scratchpad_Memory_Writer : public Simple_Writer_Backend<Char> {
            public:
                std::pair<Char*, Char*> prepare_write(size_t forget_cnt,
                        size_t want_cnt) override;
                std::pair<Char*, Char*> write_some(size_t forget_cnt) override;
                void flush() override;
                Scratchpad<Char> &pad();
                void set_increment(size_t inc);
            private:
                Scratchpad<Char> pad_;
                size_t inc_ {128 * 1024};
        };

    template <typename Char>
        class Scratchpad_Writer : public Simple_Writer_Backend<Char> {
            public:
                Scratchpad_Writer();
                Scratchpad_Writer(const std::string &filename);
                Scratchpad_Writer(ixxx::util::FD &&fd);
                Scratchpad_Writer(const Scratchpad_Writer &) =delete;
                Scratchpad_Writer &operator=(const Scratchpad_Writer &) =delete;
                Scratchpad_Writer(Scratchpad_Writer &&);
                Scratchpad_Writer &operator=(Scratchpad_Writer &&);

                std::pair<Char*, Char*> prepare_write(size_t forget_cnt,
                        size_t want_cnt) override;
                std::pair<Char*, Char*> write_some(size_t forget_cnt) override;
                void flush() override;
                Scratchpad_Sink_File<Char> &sink();
            private:
                Scratchpad_Sink_File<Char> sink_;
        };

    template <typename T>
        Simple_Writer<T> mk_tlc_writer(const std::string &filename, bool sync=false)
        {
            auto p = new Scratchpad_Writer<u8>(filename);
            p->sink().set_sync(sync);
            return Simple_Writer<T>(std::unique_ptr<Scratchpad_Writer<u8>>(p));
        }
    template <typename T>
        Simple_Writer<T> mk_tlc_writer(ixxx::util::FD &&fd, bool sync=false)
        {
            auto p = new Scratchpad_Writer<u8>(std::move(fd));
            p->sink().set_sync(sync);
            return Simple_Writer<T>(std::unique_ptr<Scratchpad_Writer<u8>>(p));
        }


} // namespace xfsx

#endif // XFSX_TLC_WRITER_HH
