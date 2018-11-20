// 2018, Georg Sauthoff <mail@gms.tf>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef XFSX_SCRATCHPAD_HH
#define XFSX_SCRATCHPAD_HH

#include "raw_vector.hh"

#include <ixxx/util.hh>
#include <assert.h>

namespace xfsx {

    namespace scratchpad {

    // Buffer class for efficient buffered reading/writing.
    //
    // When reading objects like TLV/line objects we are in the situation that we
    // to read fixed sized blocks (e.g. 128 KiB) but our objects may cross block
    // boundaries. Thus, when parsing an object  we have to take care to retrieve
    // the next block when we detect that the current object extends over the
    // current block end. Also, we have to keep as much of the current block tail
    // as it's still needed to complete the current object.
    //
    // Similarly, when writing objects, we also want to write in fixed size blocks
    // and thus have to keep the non-written tail around.
    //
    // Thus, this class maintains three positions of the buffer:
    //
    //       +----------+-----------------+
    //       |          |                 |
    //       +----------+-----------------+
    //        ^          ^                ^
    //        prelude    begin            end
    //
    // That means for the user, [prelude..begin) contains what is already processed
    // during reading or can be flushed during writing. While [begin..end) is
    // currently in active use during reading/writing.
    //
    // It uses a Raw_Vector internally such that tail extensions aren't value
    // initialized. Thus, a typical write sequence is:
    //
    // add_tail(128KiB);
    // write up to 128 KiB into end-128KiB
    // remove_tail(128Kib - actually_written);
    //
    template <typename Char>
        class Scratchpad {
            public:
                Scratchpad();
                Scratchpad(const Scratchpad &) =delete;
                Scratchpad &operator=(const Scratchpad &) =delete;
                Scratchpad(Scratchpad &&);
                Scratchpad &operator=(Scratchpad &&);

                // increment the begin position, i.e. add k existing bytes to the prelude range
                void increment_head(size_t k);
                // set prelude to begin position (i.e. prelude is empty),
                // memmove [begin..end) over the old prelude
                // and resize internal buffer to a smaller size
                void forget_prelude(size_t k);
                // increment_head + forget_prelude
                void remove_head(size_t k);

                // increment end position and resize internal buffer
                // i.e. add k bytes to the [begin..end) range
                void add_tail(size_t k);
                // decrement end position by k bytes and resize internal buffer
                void remove_tail(size_t k);

                Char *data();
                const Char *data() const;
                size_t size() const;

                const Char *prelude() const;

                Char *begin();
                const Char *begin() const;
                const Char *cbegin() const;
                Char *end();
                const Char *end() const;
                const Char *cend() const;

                void clear();
            private:
                Raw_Vector<Char> v_;
                size_t off_ {0}; // offset into v_
        };

    template <typename Char>
        class Source_File {
            public:
                Source_File();
                Source_File(const char *filename);
                Source_File(const std::string &filename);
                Source_File(ixxx::util::FD &&fd);
                Source_File(const Source_File &) =delete;
                Source_File &operator=(const Source_File &) =delete;
                Source_File(Source_File &&);
                Source_File &operator=(Source_File &&);

                std::pair<const Char*, const Char*>
                    read_more(size_t forget_cnt, size_t want_cnt);
                void set_increment(size_t inc);
                const Scratchpad<Char> &pad() const;
                bool eof() const;
            private:
                size_t inc_ {128 * 1024};
                Scratchpad<Char> pad_;
                ixxx::util::FD fd_;
                bool eof_{false};
        };


    // We use virtual inheritance such that code can construct
    // a Simple_Reader with different backends at runtime without
    // that changing its instantiation (i.e. type)
    template <typename Char>
        class Reader {
            public:
                virtual ~Reader();
                // function returns updated begin end,
                // 1st argument specifies how many prefix bytes can be discarded
                // 2nd argument specifies minimum read-size - which is
                //     respected by the function unless it encounters EOF before,
                //     which isn't an error
                virtual std::pair<const Char*, const Char*>
                    read_more(size_t forget_cnt, size_t want_cnt) = 0;
                virtual bool eof() const = 0;
        };
    template <typename Char>
        class Memory_Reader : public Reader<Char> {
            public:
                Memory_Reader();
                Memory_Reader(const Char *begin, const Char *end);
                std::pair<const Char*, const Char*>
                    read_more(size_t forget_cnt, size_t want_cnt) override;
                bool eof() const override;
            protected:
                const Char *begin_{nullptr};
                const Char *end_  {nullptr};
            private:
                bool eof_{false};
        };
    template <typename Char>
        class Mapped_Reader : public Memory_Reader<Char> {
            public:
                Mapped_Reader(const char *filename);
                Mapped_Reader(const std::string &filename);
            private:
                ixxx::util::MMap m_; 
        };
    template <typename Char>
        class File_Reader : public Reader<Char> {
            public:
                File_Reader(const File_Reader &) =delete;
                File_Reader &operator=(const File_Reader &) =delete;
                File_Reader(File_Reader &&);
                File_Reader &operator=(File_Reader &&);
                File_Reader();

                File_Reader(const char *filename);
                File_Reader(const std::string &filename);
                File_Reader(ixxx::util::FD &&fd);
                std::pair<const Char*, const Char*>
                    read_more(size_t forget_cnt, size_t want_cnt) override;
                bool eof() const override;
            private:
                Source_File<Char> source_;
        };

    template <typename Char>
        class Simple_Reader { // new Basic_Reader
            public:
                Simple_Reader(const Char *begin, const Char *end);
                Simple_Reader(std::unique_ptr<scratchpad::Reader<Char>> &&backend);
                Simple_Reader();
                Simple_Reader(const Simple_Reader &) =delete;
                Simple_Reader &operator=(const Simple_Reader &) =delete;
                Simple_Reader(Simple_Reader &&);
                Simple_Reader &operator=(Simple_Reader &&);

                // return 0: no more, 1: next one, old pointers still valid,
                // 2: next one, old window pointers are invalidated
                uint8_t  next(size_t want);
                // throw range_error if less than k bytes are part of the window
                void     check_available(size_t k);
                // i.e. window.begin is incremented
                // and those bytes are possible freed with the next next() call
                void     forget(size_t k);
                size_t   pos() const;
                void set_pos(size_t pos);
                const std::pair<const Char*, const Char*> &window() const
                {
                    return p_;
                }

            private:
                std::pair<const Char*, const Char*> p_{nullptr, nullptr};
                size_t    global_pos_ {0};
                size_t    local_pos_  {0};
                // This decouples the reader from backends like Scratchpad
                // if it's empty then the begin/end range includes the complete file
                std::unique_ptr<scratchpad::Reader<Char>> backend_;
        };

    template <typename Char>
        Simple_Reader<Char> mk_simple_reader(const std::string &filename)
        {
            return Simple_Reader<Char>(std::unique_ptr<scratchpad::Reader<Char>>(
                        new scratchpad::File_Reader<Char>(filename)));
        }
    template <typename Char>
        Simple_Reader<Char> mk_simple_reader_mapped(const std::string &filename)
        {
            return Simple_Reader<Char>(std::unique_ptr<scratchpad::Reader<Char>>(
                        new scratchpad::Mapped_Reader<Char>(filename)));
        }
    template <typename Char>
        Simple_Reader<Char> mk_simple_reader(ixxx::util::FD &&fd)
        {
            return Simple_Reader<Char>(std::unique_ptr<scratchpad::Reader<char>>(
                        new scratchpad::File_Reader<Char>(std::move(fd))));
        }


    template <typename Char>
        class Sink_File {
            public:
                Sink_File();
                ~Sink_File();
                Sink_File(const char *filename);
                Sink_File(const std::string &filename);
                Sink_File(ixxx::util::FD &&fd);
                Sink_File(Sink_File &&);
                Sink_File &operator=(Sink_File &&);
                Sink_File(const Sink_File &) =delete;
                Sink_File &operator=(const Sink_File &) =delete;

                std::pair<Char*, Char*> prepare_write(size_t forget_cnt, size_t want_cnt);
                std::pair<Char*, Char*> write_some(size_t forget_cnt);

                void flush();
                void sync();
                void set_increment(size_t inc);
                void set_sync(bool b);
            private:
                size_t inc_ {128 * 1024};
                Scratchpad<Char> pad_;
                ixxx::util::FD fd_;
                bool sync_{false};
        };

    template <typename Char>
        class Writer {
            public:
                virtual ~Writer();

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

                virtual void set_sync(bool b) = 0;
                virtual void sync() = 0;

                // only useful for scratchpad writer
                virtual void clear();
        };

    // e.g. for writing into a memory mapped file
    template <typename Char>
        class Memory_Writer : public Writer<Char> {
            public:
                Memory_Writer();
                Memory_Writer(Char *begin, Char *end);
                std::pair<Char*, Char*> prepare_write(size_t forget_cnt,
                        size_t want_cnt) override;
                std::pair<Char*, Char*> write_some(size_t forget_cnt) override;
                void flush() override;
                void sync() override;
                void set_sync(bool b) override;
            protected:
                Char *begin_{nullptr};
                Char *end_  {nullptr};
        };

    template <typename Char>
        class Mapped_Writer : public Memory_Writer<Char> {
            public:
                Mapped_Writer(ixxx::util::MMap &&m);
                Mapped_Writer(const std::string &filename, size_t size);

                void sync() override;
                void set_sync(bool b) override;
            private:
                ixxx::util::MMap m_;
                bool sync_{false};
        };

    // i.e. the [prelude..begin) pad range contains everything written
    template <typename Char>
        class Scratchpad_Writer : public Writer<Char> {
            public:
                std::pair<Char*, Char*> prepare_write(size_t forget_cnt,
                        size_t want_cnt) override;
                std::pair<Char*, Char*> write_some(size_t forget_cnt) override;
                void flush() override;
                void sync() override;
                void set_sync(bool b) override;
                void clear() override;

                Scratchpad<Char> &pad();
                void set_increment(size_t inc);

            private:
                Scratchpad<Char> pad_;
                size_t inc_ {128 * 1024};
        };

    template <typename Char>
        class File_Writer : public Writer<Char> {
            public:
                File_Writer();
                File_Writer(const std::string &filename);
                File_Writer(ixxx::util::FD &&fd);
                File_Writer(const File_Writer &) =delete;
                File_Writer &operator=(const File_Writer &) =delete;
                File_Writer(File_Writer &&);
                File_Writer &operator=(File_Writer &&);

                std::pair<Char*, Char*> prepare_write(size_t forget_cnt,
                        size_t want_cnt) override;
                std::pair<Char*, Char*> write_some(size_t forget_cnt) override;
                void flush() override;
                void sync() override;
                void set_sync(bool b) override;

                Sink_File<Char> &sink();
            private:
                Sink_File<Char> sink_;
        };

    template <typename Char>
        class Simple_Writer {
            public:
                Simple_Writer();
                // this allows to write to a large enough memory mapped file
                // that is then truncated to global_pos
                Simple_Writer(Char *begin, Char *end);
                Simple_Writer(std::unique_ptr<scratchpad::Writer<Char>> &&backend);
                Simple_Writer(Simple_Writer &&);
                Simple_Writer &operator=(Simple_Writer &&);
                Simple_Writer(const Simple_Writer &) =delete;
                Simple_Writer &operator=(const Simple_Writer &) =delete;

                ~Simple_Writer();

                void write(const Char *begin, const Char *end);
                // write string literals, i.e. assuming 0 termination
                template <size_t N>
                    void write(const Char (&s)[N])
                    {
                        assert(N);
                        this->write(s, s+N-1);
                    }

                // tell the writer to prepare a buffer large enough for k bytes
                // returns pointer to that buffer
                Char *begin_write(size_t k);
                //  must be called after the buffer returned by begin_write()
                //  is filled - with the same k
                //  a smaller k is ok, but then there is the risk that the
                //  buffer grows to the next increment
                void commit_write(size_t k);

                void flush();
                void sync();
                void set_sync(bool b);

                void clear();

                size_t   pos() const;

                scratchpad::Writer<Char> *backend();

            private:
                size_t  local_pos_  {0};
                size_t  global_pos_ {0};
                Char   *begin_      {nullptr};
                Char   *end_        {nullptr};
                bool    do_write_   {false};
                std::unique_ptr<scratchpad::Writer<Char>> backend_;
        };

        // XXX move Simple_Reader, as well

    template <typename Char>
        Simple_Writer<Char> mk_simple_writer(const std::string &filename)
        {
            return Simple_Writer<Char>(std::unique_ptr<scratchpad::Writer<Char>>(
                       new scratchpad::File_Writer<Char>(filename)
                        ));
        }
    template <typename Char>
        Simple_Writer<Char> mk_simple_writer(ixxx::util::FD &&fd)
        {
            return Simple_Writer<Char>(std::unique_ptr<scratchpad::Writer<Char>>(
                        new scratchpad::File_Writer<Char>(std::move(fd))
                        ));
        }
    template <typename Char>
        Simple_Writer<Char> mk_simple_writer_mapped(const std::string &filename, size_t size)
        {
            return Simple_Writer<Char>(std::unique_ptr<scratchpad::Writer<Char>>(
                       new scratchpad::Mapped_Writer<Char>(filename, size)
                        ));
        }

    } // namespace scratchpad

} // namespace xfsx

#endif // XFSX_SCRATCHPAD_HH
