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
        Scratchpad_Source_File<Char>::Scratchpad_Source_File() =default;
    template <typename Char>
        Scratchpad_Source_File<Char>::Scratchpad_Source_File(const char *filename)
        :
            fd_(filename, O_RDONLY)
    {
    }
    template <typename Char>
        Scratchpad_Source_File<Char>::Scratchpad_Source_File(
                const std::string &filename)
        :
            fd_(filename, O_RDONLY)
    {
    }
    template <typename Char>
        Scratchpad_Source_File<Char>::Scratchpad_Source_File(ixxx::util::FD &&fd)
        :
            fd_(std::move(fd))
        {
        }
    template <typename Char>
        Scratchpad_Source_File<Char>::Scratchpad_Source_File(Scratchpad_Source_File &&)
        =default;

    template <typename Char>
        Scratchpad_Source_File<Char> &Scratchpad_Source_File<Char>::operator=(
                Scratchpad_Source_File &&) =default;

    template <typename Char>
        void Scratchpad_Source_File<Char>::set_increment(size_t inc)
        {
            inc_ = inc;
        }
    template <typename Char>
        std::pair<const Char*, const Char*>
        Scratchpad_Source_File<Char>::read_more(
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
                    break;
                }
            }
            return make_pair(pad_.begin(), pad_.end());
        }


    template class Scratchpad_Source_File<u8>;
    template class Scratchpad_Source_File<char>;


    template <typename Char>
        Scratchpad_Sink_File<Char>::Scratchpad_Sink_File(const char *filename)
        :
            fd_(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644)
    {
    }
    template <typename Char>
        Scratchpad_Sink_File<Char>::Scratchpad_Sink_File(const std::string &filename)
        :
            fd_(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644)
    {
    }
    template <typename Char>
        Scratchpad_Sink_File<Char>::Scratchpad_Sink_File(ixxx::util::FD &&fd)
        :
            fd_(std::move(fd))
    {
    }
    template <typename Char>
        Scratchpad_Sink_File<Char>::~Scratchpad_Sink_File()
        {
            try {
                flush();
            } catch (...) {
                // don't abort the program on error ...
            }
        }

    template <typename Char>
        Scratchpad_Sink_File<Char>::Scratchpad_Sink_File() =default;
    template <typename Char>
        Scratchpad_Sink_File<Char>::Scratchpad_Sink_File(Scratchpad_Sink_File &&)
        =default;
    template <typename Char>
        Scratchpad_Sink_File<Char>  &Scratchpad_Sink_File<Char>::operator=(
                Scratchpad_Sink_File &&) =default;

    template <typename Char>
        void Scratchpad_Sink_File<Char>::set_increment(size_t inc)
        {
            inc_ = inc;
        }
    template <typename Char>
        void Scratchpad_Sink_File<Char>::set_sync(bool b)
        {
            sync_ = b;
        }
    template <typename Char>
        std::pair<Char*, Char*>
        Scratchpad_Sink_File<Char>::prepare_write(size_t forget_cnt, size_t want_cnt)
        {
            pad_.increment_head(forget_cnt);

            size_t k = (want_cnt + inc_ - 1)/inc_*inc_;
            pad_.add_tail(k);

            return make_pair(pad_.begin(), pad_.end());
        }

    template <typename Char>
        std::pair<Char*, Char*>
        Scratchpad_Sink_File<Char>::write_some(size_t forget_cnt)
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
        void Scratchpad_Sink_File<Char>::flush()
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
            if (sync_)
                ixxx::posix::fsync(fd_);
            pad_.clear();
        }

    template class Scratchpad_Sink_File<u8>;
    template class Scratchpad_Sink_File<char>;

} // namespace xfsx
