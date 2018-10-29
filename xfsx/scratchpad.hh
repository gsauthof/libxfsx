// 2018, Georg Sauthoff <mail@gms.tf>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef XFSX_SCRATCHPAD_HH
#define XFSX_SCRATCHPAD_HH

#include "raw_vector.hh"

#include <ixxx/util.hh>

namespace xfsx {

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
        class Scratchpad_Source_File {
            public:
                Scratchpad_Source_File();
                Scratchpad_Source_File(const char *filename);
                Scratchpad_Source_File(const std::string &filename);
                Scratchpad_Source_File(ixxx::util::FD &&fd);
                Scratchpad_Source_File(const Scratchpad_Source_File &) =delete;
                Scratchpad_Source_File &operator=(const Scratchpad_Source_File &) =delete;
                Scratchpad_Source_File(Scratchpad_Source_File &&);
                Scratchpad_Source_File &operator=(Scratchpad_Source_File &&);

                std::pair<const Char*, const Char*>
                    read_more(size_t forget_cnt, size_t want_cnt);
                void set_increment(size_t inc);
            private:
                size_t inc_ {128 * 1024};
                Scratchpad<Char> pad_;
                ixxx::util::FD fd_;
        };

    template <typename Char>
        class Scratchpad_Sink_File {
            public:
                Scratchpad_Sink_File();
                ~Scratchpad_Sink_File();
                Scratchpad_Sink_File(const char *filename);
                Scratchpad_Sink_File(const std::string &filename);
                Scratchpad_Sink_File(ixxx::util::FD &&fd);
                Scratchpad_Sink_File(Scratchpad_Sink_File &&);
                Scratchpad_Sink_File &operator=(Scratchpad_Sink_File &&);
                Scratchpad_Sink_File(const Scratchpad_Sink_File &) =delete;
                Scratchpad_Sink_File &operator=(const Scratchpad_Sink_File &) =delete;

                std::pair<Char*, Char*> prepare_write(size_t forget_cnt, size_t want_cnt);
                std::pair<Char*, Char*> write_some(size_t forget_cnt);

                void flush();
                void set_increment(size_t inc);
                void set_sync(bool b);
            private:
                size_t inc_ {128 * 1024};
                Scratchpad<Char> pad_;
                ixxx::util::FD fd_;
                bool sync_{false};
        };

} // namespace xfsx

#endif // XFSX_SCRATCHPAD_HH
