// 2018, Georg Sauthoff <mail@gms.tf>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "scratchpad.hh"

#include <utility>
#include <tuple>

#include "xfsx.hh"

#include <ixxx/posix.hh>

#include <string.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

namespace xfsx {

    namespace scratchpad {

    template <typename Char>
        Scratchpad<Char>::Scratchpad() =default;
    template <typename Char>
        Scratchpad<Char>::Scratchpad(Scratchpad &&o)
        :
            v_(std::move(o.v_)),
            off_(o.off_)
    {
        o.off_ = 0;
        o.v_.resize(0); // o.v_.clear();
    }
    template <typename Char>
        Scratchpad<Char> &Scratchpad<Char>::operator=(Scratchpad &&o)
        {
            v_ = std::move(o.v_);
            off_ = o.off_;

            o.off_ = 0;
            o.v_.resize(0); // o.v_.clear();
            return *this;
        }

    template <typename Char>
        void Scratchpad<Char>::clear()
        {
            v_.clear();
            off_ = 0;
        }
    template <typename Char>
        void Scratchpad<Char>::increment_head(size_t k)
        {
            off_ += k;
            assert(off_ <= v_.size());
        }
    template <typename Char>
        void Scratchpad<Char>::forget_prelude(size_t k)
        {
            if (!k)
                return;
            // Optimally, either k == off_ (i.e. when reading) or
            // or k is just a little bit smaller than off_
            // e.g. k is at a 128k boundary and off_ a few bytes above
            // (i.e. when writing)
            assert(k <= off_);
            memmove(v_.data(), v_.data() + k, v_.size() - k);
            v_.resize(v_.size() - k);
            off_ -= k;
        }

    template <typename Char>
        void Scratchpad<Char>::remove_head(size_t k)
        {
            increment_head(k);
            forget_prelude(off_);
        }
    template <typename Char>
        void Scratchpad<Char>::add_tail(size_t k)
        {
            v_.resize(v_.size() + k);
        }
    template <typename Char>
        void Scratchpad<Char>::remove_tail(size_t k)
        {
            v_.resize(v_.size() - k);
        }
    template <typename Char>
        const Char *Scratchpad<Char>::prelude() const { return v_.data(); }
    template <typename Char>
        Char *Scratchpad<Char>::begin() { return v_.data() + off_; }
    template <typename Char>
        Char *Scratchpad<Char>::end()  { return v_.data() + v_.size(); }
    template <typename Char>
        const Char *Scratchpad<Char>::begin() const { return v_.data() + off_; }
    template <typename Char>
        const Char *Scratchpad<Char>::end() const  { return v_.data() + v_.size(); }
    template <typename Char>
        const Char *Scratchpad<Char>::cbegin() const { return v_.data() + off_; }
    template <typename Char>
        const Char *Scratchpad<Char>::cend() const  { return v_.data() + v_.size(); }

    template <typename Char>
        size_t Scratchpad<Char>::size() const { return v_.size() - off_; }


    template class Scratchpad<u8>;
    template class Scratchpad<char>;


    template <typename Char>
        Source_File<Char>::Source_File() =default;
    template <typename Char>
        Source_File<Char>::Source_File(const char *filename)
        :
            fd_(filename, O_RDONLY)
    {
    }
    template <typename Char>
        Source_File<Char>::Source_File(
                const std::string &filename)
        :
            fd_(filename, O_RDONLY)
    {
    }
    template <typename Char>
        Source_File<Char>::Source_File(ixxx::util::FD &&fd)
        :
            fd_(std::move(fd))
        {
        }
    template <typename Char>
        Source_File<Char>::Source_File(
                Source_File &&) = default;

    template <typename Char>
        Source_File<Char> &Source_File<Char>::operator=(
                Source_File &&) =default;

    template <typename Char>
        void Source_File<Char>::set_increment(size_t inc)
        {
            inc_ = inc;
        }
    template <typename Char>
        const Scratchpad<Char> &Source_File<Char>::pad() const
        {
            return pad_;
        }
    template <typename Char>
        bool Source_File<Char>::eof() const
        {
            return eof_;
        }
    template <typename Char>
        std::pair<const Char*, const Char*>
        Source_File<Char>::read_more(
                size_t forget_cnt, size_t want_cnt)
        {
            pad_.remove_head(forget_cnt);
            // reduce the number of memmove calls - doesn't make a difference
            //pad_.increment_head(forget_cnt);
            //if ((pad_.begin() - pad_.prelude()) / inc_ > 3)
            //    pad_.forget_prelude(pad_.begin() - pad_.prelude());

            size_t k = (want_cnt + inc_ - 1) / inc_;
            size_t l = k * inc_;
            pad_.add_tail(l);
            size_t m = 0;
            for (size_t i = 0; i < k; ++i) {
                size_t n = ixxx::util::read_all(fd_, pad_.end()-l+m, inc_);
                m += n;
                if (n < inc_) {
                    pad_.remove_tail(l - m);
                    eof_ = true;
                    break;
                }
            }
            return make_pair(pad_.begin(), pad_.end());
        }


    template class Source_File<u8>;
    template class Source_File<char>;


    template <typename Char>
        Sink_File<Char>::Sink_File(const char *filename)
        :
            fd_(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644)
    {
    }
    template <typename Char>
        Sink_File<Char>::Sink_File(const std::string &filename)
        :
            fd_(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644)
    {
    }
    template <typename Char>
        Sink_File<Char>::Sink_File(ixxx::util::FD &&fd)
        :
            fd_(std::move(fd))
    {
    }
    template <typename Char>
        Sink_File<Char>::~Sink_File()
        {
            try {
                flush();
            } catch (...) {
                // don't abort the program on error ...
            }
        }

    template <typename Char>
        Sink_File<Char>::Sink_File() =default;
    template <typename Char>
        Sink_File<Char>::Sink_File(Sink_File &&)
        =default;
    template <typename Char>
        Sink_File<Char>  &Sink_File<Char>::operator=(
                Sink_File &&) =default;

    template <typename Char>
        void Sink_File<Char>::set_increment(size_t inc)
        {
            inc_ = inc;
        }
    template <typename Char>
        void Sink_File<Char>::set_sync(bool b)
        {
            sync_ = b;
        }
    template <typename Char>
        std::pair<Char*, Char*>
        Sink_File<Char>::prepare_write(size_t forget_cnt, size_t want_cnt)
        {
            pad_.increment_head(forget_cnt);

            size_t k = (want_cnt + inc_ - 1)/inc_*inc_;
            pad_.add_tail(k);

            return make_pair(pad_.begin(), pad_.end());
        }

    template <typename Char>
        std::pair<Char*, Char*>
        Sink_File<Char>::write_some(size_t forget_cnt)
        {
            pad_.increment_head(forget_cnt);
            auto begin = pad_.prelude();
            auto end   = pad_.begin();
            size_t n = end-begin;
            size_t k = n / inc_;
            // reduce the number of memmove calls, write multiple in a row
            //if (k < 4)
            //    return;
            //cerr << (n-k*inc_) << '\n';
            for (size_t i = 0; i < k; ++i) {
                ixxx::util::write_all(fd_, begin, inc_);
                begin += inc_;
            }
            pad_.forget_prelude(k*inc_);

            return make_pair(pad_.begin(), pad_.end());
        }

    template <typename Char>
        std::pair<Char*, Char*>
        Sink_File<Char>::write(size_t forget_cnt,
                const Char *begin, const Char *end)
        {
            size_t n = end - begin;
            size_t m = (pad_.begin() - pad_.prelude());
            size_t rest = m % inc_;
            if (rest) {
                size_t missing = inc_ - rest;
                size_t k = min(missing, n);
                pad_.add_tail(k);
                std::copy(begin, begin + k, pad_.begin());
                begin += k;
                write_some(forget_cnt + k);
            }
            n = end - begin;
            assert((n && (pad_.begin() - pad_.prelude()) == 0) || !n);
            size_t l = n / inc_;
            for (size_t i = 0; i < l; ++i) {
                // write-through all following inc_ sized blocks
                // to avoid superfluous buffering
                ixxx::util::write_all(fd_, begin, inc_);
                begin += inc_;
            }
            n = end - begin;
            if (n) {
                pad_.add_tail(n);
                std::copy(begin, end, pad_.begin());
                pad_.increment_head(n);
            }
            return make_pair(pad_.begin(), pad_.end());
        }

    template <typename Char>
        void Sink_File<Char>::flush()
        {
            auto begin = pad_.prelude();
            auto end   = pad_.begin();
            size_t n = end-begin;
            size_t k = n / inc_;
            for (size_t i = 0; i < k; ++i) {
                ixxx::util::write_all(fd_, begin, inc_);
                begin += inc_;
            }
            if (begin != end) {
                ixxx::util::write_all(fd_, begin, end-begin);
            }
            pad_.clear();
        }
    template <typename Char>
        void Sink_File<Char>::sync()
        {
            if (sync_)
                ixxx::posix::fsync(fd_);
        }
    template <typename Char>
        size_t Sink_File<Char>::inc() const
        {
            return inc_;
        }

    template class Sink_File<u8>;
    template class Sink_File<char>;

    template <typename Char> Reader<Char>::~Reader() =default;

    template class Reader<u8>;
    template class Reader<char>;

    template <typename Char>
        Memory_Reader<Char>::Memory_Reader(const Char *begin, const Char *end)
        :
            begin_(begin),
            end_(end)
    {
    }
    template <typename Char> Memory_Reader<Char>::Memory_Reader() =default;
    template <typename Char>
        std::pair<const Char*, const Char*>
        Memory_Reader<Char>::read_more(size_t forget_cnt, size_t want_cnt)
        {
            (void)want_cnt;
            begin_ += forget_cnt;
            eof_ = true;
            return make_pair(begin_, end_);
        }
    template <typename Char>
        bool Memory_Reader<Char>::eof() const
        {
            return eof_;
        }

    template class Memory_Reader<u8>;
    template class Memory_Reader<char>;

    template <typename Char>
        Mapped_Reader<Char>::Mapped_Reader(const char *filename)
        :
            m_(ixxx::util::mmap_file(filename))
    {
        this->begin_ = reinterpret_cast<const Char*>(m_.begin());
        this->end_   = reinterpret_cast<const Char*>(m_.end());
    }
    template <typename Char>
        Mapped_Reader<Char>::Mapped_Reader(const std::string &filename)
        :
            m_(ixxx::util::mmap_file(filename))
    {
        this->begin_ = reinterpret_cast<const Char*>(m_.begin());
        this->end_   = reinterpret_cast<const Char*>(m_.end());
    }

    template class Mapped_Reader<u8>;
    template class Mapped_Reader<char>;

    template <typename Char>
        File_Reader<Char>::File_Reader(File_Reader &&) =default;
    template <typename Char>
        File_Reader<Char> &File_Reader<Char>::operator=(File_Reader &&)
        =default;
    template <typename Char>
        File_Reader<Char>::File_Reader() =default;
    template <typename Char>
        File_Reader<Char>::File_Reader(const std::string &filename)
        :
            source_(filename)
    {
    }
    template <typename Char>
        File_Reader<Char>::File_Reader(const char *filename)
        :
            source_(filename)
    {
    }
    template <typename Char>
        File_Reader<Char>::File_Reader(ixxx::util::FD &&fd)
        :
            source_(std::move(fd))
        {
        }
    template <typename Char>
        std::pair<const Char*, const Char*>
        File_Reader<Char>::read_more(size_t forget_cnt, size_t want_cnt)
        {
            return source_.read_more(forget_cnt, want_cnt);
        }
    template <typename Char> bool
        File_Reader<Char>::eof() const
        {
            return source_.eof();
        }

    template class File_Reader<u8>;
    template class File_Reader<char>;


    template <typename Char>
        Simple_Reader<Char>::Simple_Reader() =default;
    template <typename Char>
        Simple_Reader<Char>::Simple_Reader(Simple_Reader &&o)
        :
            p_(std::move(o.p_)),
            global_pos_(o.global_pos_),
            local_pos_(o.local_pos_),
            backend_(std::move(o.backend_))
    {
        o.p_.first    = nullptr;
        o.p_.second   = nullptr;
        o.global_pos_ = 0;
        o.local_pos_  = 0;
    }
    template <typename Char>
        Simple_Reader<Char> &Simple_Reader<Char>::operator=(Simple_Reader &&o)
        {
            p_            = std::move(o.p_);
            global_pos_   = o.global_pos_;
            local_pos_    = o.local_pos_;
            backend_      = std::move(o.backend_);

            o.p_.first    = nullptr;
            o.p_.second   = nullptr;
            o.global_pos_ = 0;
            o.local_pos_  = 0;
            return *this;
        }
    template <typename Char>
        size_t Simple_Reader<Char>::pos() const
        {
            return global_pos_;
        }
    template <typename Char>
        void Simple_Reader<Char>::set_pos(size_t pos)
        {
            global_pos_ = pos;
        }
    template <typename Char>
        Simple_Reader<Char>::Simple_Reader(const Char *begin, const Char *end)
        :
            p_(begin, end)
    {
    }
    template <typename Char>
        Simple_Reader<Char>::Simple_Reader(
                std::unique_ptr<scratchpad::Reader<Char>> &&backend)
        :
            backend_(std::move(backend))
    {
    }

    // moved to the header such that it can be inlined
    /*
    template <typename Char>
        const std::pair<const Char*, const Char*> &
        Simple_Reader<Char>::window() const
    {
        return p_;
    }
    */
    template <typename Char>
        void Simple_Reader<Char>::forget(size_t kk)
        {
            auto k = min(kk, size_t(p_.second - p_.first));
            local_pos_  += k;
            global_pos_ += k;
            p_.first    += k;
        }
    template <typename Char>
        void Simple_Reader<Char>::check_available(size_t k)
        {
            if (size_t(p_.second - p_.first) < k)
                throw range_error("content overflows");
        }
    template <typename Char>
        uint8_t Simple_Reader<Char>::next(size_t want_cnt)
        {
            if (backend_ && size_t(p_.second - p_.first) < want_cnt
                    && !backend_->eof()) {
                p_ = backend_->read_more(local_pos_, want_cnt - size_t(p_.second-p_.first));
                local_pos_ = 0;
                if (p_.first == p_.second)
                    return 0;
                else
                    return 2;
            }
            if (!backend_)
                eof_ = true;
            return p_.first != p_.second;
        }
    template <typename Char>
        bool Simple_Reader<Char>::eof() const
        {
            if (backend_)
                return backend_->eof();
            else
                return eof_;
        }

    template class Simple_Reader<u8>;
    template class Simple_Reader<char>;


    template <typename Char>
    Writer<Char>::~Writer() =default;

    template <typename Char>
        void Writer<Char>::clear()
        {
        }
    // default implementation, cf. File_Writer for an overwrite
    template <typename Char>
        std::pair<Char*, Char*> Writer<Char>::write(size_t forget_cnt,
            const Char *begin, const Char *end)
    {
        size_t n = end-begin;
        auto p = prepare_write(forget_cnt, n);
        std::copy(begin, end, p.first);
        return write_some(n);
    }
    template <typename Char>
        size_t Writer<Char>::inc() const
        {
            return 128 * 1024;
        }


    template class Writer<u8>;
    template class Writer<char>;

    template <typename Char> Memory_Writer<Char>::Memory_Writer() =default;
    template <typename Char>
        Memory_Writer<Char>::Memory_Writer(Char *begin, Char *end)
        :
            begin_(begin),
            end_(end)
    {
    }
    template <typename Char>
        std::pair<Char*, Char*> Memory_Writer<Char>::prepare_write(size_t forget_cnt, size_t want_cnt)
        {
            (void)want_cnt;
            begin_ += forget_cnt;
            return make_pair(begin_, end_);
        }
    template <typename Char>
        std::pair<Char*, Char*> Memory_Writer<Char>::write_some(size_t forget_cnt)
        {
            begin_ += forget_cnt;
            return make_pair(begin_, end_);
        }
    template <typename Char>
        void Memory_Writer<Char>::flush()
        {
        }
    template <typename Char>
        void Memory_Writer<Char>::sync()
        {
        }
    template <typename Char>
        void Memory_Writer<Char>::set_sync(bool)
        {
        }

    template class Memory_Writer<u8>;
    template class Memory_Writer<char>;

    template <typename Char>
        Mapped_Writer<Char>::Mapped_Writer(ixxx::util::MMap &&m)
        :
            m_(std::move(m))
    {
        this->begin_ = reinterpret_cast<Char*>(m_.begin());
        this->end_   = reinterpret_cast<Char*>(m_.end());
    }
    template <typename Char>
        Mapped_Writer<Char>::Mapped_Writer(const std::string &filename, size_t size)
        :
            m_(ixxx::util::mmap_file(filename, false, true, size))
        {
            this->begin_ = reinterpret_cast<Char*>(m_.begin());
            this->end_   = reinterpret_cast<Char*>(m_.end());
        }
    template <typename Char>
        void Mapped_Writer<Char>::sync()
        {
            if (sync_)
                m_.sync();
        }
    template <typename Char>
        void Mapped_Writer<Char>::set_sync(bool b)
        {
            sync_ = b;
        }

    template class Mapped_Writer<u8>;
    template class Mapped_Writer<char>;

    template <typename Char>
        std::pair<Char*, Char*>
        Scratchpad_Writer<Char>::prepare_write(size_t forget_cnt, size_t want_cnt)
        {
            pad_.increment_head(forget_cnt);
            size_t k = (want_cnt + inc_ - 1)/inc_*inc_;
            pad_.add_tail(k);
            return make_pair(pad_.begin(), pad_.end());
        }
    template <typename Char>
        std::pair<Char*, Char*>
        Scratchpad_Writer<Char>::write_some(size_t forget_cnt)
        {
            pad_.increment_head(forget_cnt);
            return make_pair(pad_.begin(), pad_.end());
        }
    template <typename Char>
        void Scratchpad_Writer<Char>::flush()
        {
        }
    template <typename Char>
        void Scratchpad_Writer<Char>::sync()
        {
        }
    template <typename Char>
        void Scratchpad_Writer<Char>::set_sync(bool)
        {
        }
    template <typename Char>
        Scratchpad<Char> & Scratchpad_Writer<Char>::pad()
        {
            return pad_;
        }
    template <typename Char>
        void Scratchpad_Writer<Char>::set_increment(size_t inc)
        {
            inc_ = inc;
        }

    template <typename Char>
        void Scratchpad_Writer<Char>::clear()
        {
            pad_.clear();
        }

    template class Scratchpad_Writer<u8>;
    template class Scratchpad_Writer<char>;

    template <typename Char>
        File_Writer<Char>::File_Writer(const std::string &filename)
        :
            sink_(filename)
    {
    }
    template <typename Char>
        File_Writer<Char>::File_Writer(ixxx::util::FD &&fd)
        :
            sink_(std::move(fd))
    {
    }
    template <typename Char>
        File_Writer<Char>::File_Writer(File_Writer &&) =default;
    template <typename Char>
        File_Writer<Char> & File_Writer<Char>::operator=(File_Writer &&) =default;

    template <typename Char>
        std::pair<Char*, Char*>
        File_Writer<Char>::prepare_write(size_t forget_cnt, size_t want_cnt)
        {
            return sink_.prepare_write(forget_cnt, want_cnt);
        }
    template <typename Char>
        std::pair<Char*, Char*>
        File_Writer<Char>::write_some(size_t forget_cnt)
        {
            return sink_.write_some(forget_cnt);
        }
    template <typename Char>
        std::pair<Char*, Char*>
        File_Writer<Char>::write(size_t forget_cnt,
                const Char *begin, const Char *end)
        {
            return sink_.write(forget_cnt, begin, end);
        }
    template <typename Char>
        size_t File_Writer<Char>::inc() const
        {
            return sink_.inc();
        }
    template <typename Char>
        void
        File_Writer<Char>::flush()
        {
            sink_.flush();
        }
    template <typename Char>
        void
        File_Writer<Char>::sync()
        {
            sink_.sync();
        }
    template <typename Char>
        void File_Writer<Char>::set_sync(bool b)
        {
            sink_.set_sync(b);
        }
    template <typename Char>
        Sink_File<Char> & File_Writer<Char>::sink()
        {
            return sink_;
        }

    template class File_Writer<u8>;
    template class File_Writer<char>;




    template <typename Char>
        Simple_Writer<Char>::Simple_Writer()
        :
            backend_(new scratchpad::Memory_Writer<Char>(nullptr, nullptr))
    {
    }

    template <typename Char>
        Simple_Writer<Char>::Simple_Writer(Char *begin, Char *end)
        :
            begin_(begin),
            end_(end),
            backend_(new scratchpad::Memory_Writer<Char>(begin, end))
    {
    }

    template <typename Char>
        Simple_Writer<Char>::Simple_Writer(
                std::unique_ptr<scratchpad::Writer<Char>> &&backend)
        :
            backend_(std::move(backend))
    {
    }
    template <typename Char>
        Simple_Writer<Char>::~Simple_Writer()
        {
            try {
                flush();
            } catch (...) {
                // don't abort the program on error ...
            }
        }
    template <typename Char>
        Simple_Writer<Char>::Simple_Writer(Simple_Writer &&o)
        :
            local_pos_(o.local_pos_),
            begin_(o.begin_),
            end_(o.end_),
            backend_(std::move(o.backend_))
    {
        o.local_pos_  = 0;
        o.global_pos_ = 0;
        o.begin_      = nullptr;
        o.end_        = nullptr;
    }
    template <typename Char>
        Simple_Writer<Char> &Simple_Writer<Char>::operator=(Simple_Writer &&o)
        {
            local_pos_        = o.local_pos_;
            begin_            = o.begin_;
            end_              = o.end_;
            backend_          = std::move(o.backend_);

            o.local_pos_      = 0;
            o.global_pos_     = 0;
            o.begin_          = nullptr;
            o.end_            = nullptr;
            return *this;
        }

    template <typename Char>
        size_t Simple_Writer<Char>::pos() const
        {
            return global_pos_;
        }

    template <typename Char>
        void Simple_Writer<Char>::write(const Char *begin, const Char *end)
        {
            size_t k = end-begin;
            if (k >= backend_->inc()) {
                std::tie(begin_, end_) = backend_->write(local_pos_, begin ,end);
                local_pos_   = 0;
                global_pos_ += k;
                return;
            }

            bool do_write = false;
            if (size_t(end_ - begin_) < k) {
                std::tie(begin_, end_) = backend_->prepare_write(local_pos_,
                        k - size_t(end_ - begin_));
                local_pos_ = 0;
                do_write = true;
                if (size_t(end_ - begin_) < k) {
                    throw range_error("not enough room for write");
                }
            }

            begin_ = std::copy(begin, end, begin_);
            local_pos_  += k;
            global_pos_ += k;

            if (do_write) {
                std::tie(begin_, end_) = backend_->write_some(local_pos_);
                local_pos_ = 0;
            }
        }
    template <typename Char>
        Char *Simple_Writer<Char>::begin_write(size_t k)
        {
            if (size_t(end_ - begin_) < k) {
                std::tie(begin_, end_) = backend_->prepare_write(local_pos_,
                        k - size_t(end_ - begin_));
                local_pos_ = 0;
                do_write_ = true;
                if (size_t(end_ - begin_) < k) {
                    throw range_error("not enough room for write");
                }
            }
            return begin_;
        }
    template <typename Char>
        void Simple_Writer<Char>::commit_write(size_t k)
        {
            begin_      += k;
            local_pos_  += k;
            global_pos_ += k;

            if (do_write_) {
                std::tie(begin_, end_) = backend_->write_some(local_pos_);
                local_pos_ = 0;
                do_write_ = false;
            }
        }
    template <typename Char>
        void Simple_Writer<Char>::flush()
        {
            if (backend_) {
                backend_->write_some(local_pos_);
                local_pos_ = 0;
                backend_->flush();
                begin_     = nullptr;
                end_       = nullptr;
            }
        }
    template <typename Char>
        void Simple_Writer<Char>::clear()
        {
            if (backend_) {
                local_pos_ = 0;
                global_pos_ = 0;
                begin_ = nullptr;
                end_ = nullptr;
                backend_->clear();
            }
        }
    template <typename Char>
        scratchpad::Writer<Char> *Simple_Writer<Char>::backend()
        {
            return backend_.get();
        }
    template <typename Char>
        void Simple_Writer<Char>::sync()
        {
            if (backend_) {
                backend_->sync();
            }
        }
    template <typename Char>
        void Simple_Writer<Char>::set_sync(bool b)
        {
            if (backend_) {
                backend_->set_sync(b);
            }
        }

    template class Simple_Writer<char>;
    template class Simple_Writer<u8>;

    } // namespace scratchpad

} // namespace xfsx
