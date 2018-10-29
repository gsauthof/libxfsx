// 2018, Georg Sauthoff <mail@gms.tf>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "tlc_writer.hh"

#include "xfsx.hh"

#include <stdexcept>

using namespace std;

namespace xfsx {

    template <typename Char>
    Simple_Writer_Backend<Char>::~Simple_Writer_Backend() =default;

    template class Simple_Writer_Backend<u8>;

    template <typename T>
        Simple_Writer<T>::Simple_Writer()
        :
            backend_(new Memory_Writer<u8>(nullptr, nullptr))
    {
    }

    template <typename T>
        Simple_Writer<T>::Simple_Writer(u8 *begin, u8 *end)
        :
            begin_(begin),
            end_(end),
            backend_(new Memory_Writer<u8>(begin, end))
    {
    }

    template <typename T>
        Simple_Writer<T>::Simple_Writer(
                std::unique_ptr<Simple_Writer_Backend<u8>> &&backend)
        :
            backend_(std::move(backend))
    {
    }
    template <typename T>
        Simple_Writer<T>::Simple_Writer(Simple_Writer &&o)
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
    template <typename T>
        Simple_Writer<T> &Simple_Writer<T>::operator=(Simple_Writer &&o)
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

    template <typename T>
        size_t Simple_Writer<T>::pos() const
        {
            return global_pos_;
        }

    template <typename T>
        void Simple_Writer<T>::write(const T &tlc)
        {
            size_t k = tlc.tl_size;
            if (tlc.shape == Shape::PRIMITIVE)
                k += tlc.length;

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

            begin_ = tlc.write(begin_, end_);
            local_pos_ += k;
            global_pos_ += k;

            if (do_write) {
                std::tie(begin_, end_) = backend_->write_some(local_pos_);
                local_pos_ = 0;
            }
        }
    template <typename T>
        void Simple_Writer<T>::flush()
        {
            if (backend_) {
                backend_->write_some(local_pos_);
                local_pos_ = 0;
                backend_->flush();
            }
        }

    template class Simple_Writer<TLC>;
    template class Simple_Writer<TLV>;

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

    template class Memory_Writer<u8>;

    template <typename Char>
        std::pair<Char*, Char*> 
        Scratchpad_Memory_Writer<Char>::prepare_write(size_t forget_cnt, size_t want_cnt)
        {
            pad_.increment_head(forget_cnt);
            size_t k = (want_cnt + inc_ - 1)/inc_*inc_;
            pad_.add_tail(k);
            return make_pair(pad_.begin(), pad_.end());
        }
    template <typename Char>
        std::pair<Char*, Char*> 
        Scratchpad_Memory_Writer<Char>::write_some(size_t forget_cnt)
        {
            pad_.increment_head(forget_cnt);
            return make_pair(pad_.begin(), pad_.end());
        }
    template <typename Char>
        void Scratchpad_Memory_Writer<Char>::flush()
        {
        }
    template <typename Char>
        Scratchpad<Char> & Scratchpad_Memory_Writer<Char>::pad()
        {
            return pad_;
        }
    template <typename Char>
        void Scratchpad_Memory_Writer<Char>::set_increment(size_t inc)
        {
            inc_ = inc;
        }


    template class Scratchpad_Memory_Writer<u8>;

    template <typename Char>
        Scratchpad_Writer<Char>::Scratchpad_Writer(const std::string &filename)
        :
            sink_(filename)
    {
    }
    template <typename Char>
        Scratchpad_Writer<Char>::Scratchpad_Writer(ixxx::util::FD &&fd)
        :
            sink_(std::move(fd))
    {
    }
    template <typename Char>
        Scratchpad_Writer<Char>::Scratchpad_Writer(Scratchpad_Writer &&) =default;
    template <typename Char>
        Scratchpad_Writer<Char> & Scratchpad_Writer<Char>::operator=(Scratchpad_Writer &&) =default;

    template <typename Char>
        std::pair<Char*, Char*> 
        Scratchpad_Writer<Char>::prepare_write(size_t forget_cnt, size_t want_cnt)
        {
            return sink_.prepare_write(forget_cnt, want_cnt);
        }
    template <typename Char>
        std::pair<Char*, Char*> 
        Scratchpad_Writer<Char>::write_some(size_t forget_cnt)
        {
            return sink_.write_some(forget_cnt);
        }
    template <typename Char>
        void 
        Scratchpad_Writer<Char>::flush()
        {
            sink_.flush();
        }
    template <typename Char>
        Scratchpad_Sink_File<Char> & Scratchpad_Writer<Char>::sink()
        {
            return sink_;
        }

    template class Scratchpad_Writer<u8>;


} // namespace xfsx
