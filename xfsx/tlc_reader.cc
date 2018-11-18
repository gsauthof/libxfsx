// 2018, Georg Sauthoff <mail@gms.tf>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "tlc_reader.hh"

#include "xfsx.hh"

#include <stdexcept>

using namespace std;

namespace xfsx {


    template <typename T>
        Simple_Reader<T>::Simple_Reader()
        :
            backend_(new scratchpad::Memory_Reader<u8>(begin_, end_))
    {
    }
    template <typename T>
        Simple_Reader<T>::Simple_Reader(Simple_Reader &&o)
        :
            tlc_(std::move(o.tlc_)),
            global_pos_(o.global_pos_),
            local_pos_(o.local_pos_),
            begin_(o.begin_),
            end_(o.end_),
            backend_(std::move(o.backend_))
    {
        o.tlc_        = T();
        o.global_pos_ = 0;
        o.local_pos_  = 0;
        o.begin_      = nullptr;
        o.end_        = nullptr;
    }
    template <typename T>
        Simple_Reader<T> &Simple_Reader<T>::operator=(Simple_Reader &&o)
        {
            tlc_          = std::move(o.tlc_);
            global_pos_   = o.global_pos_;
            local_pos_    = o.local_pos_;
            begin_        = o.begin_;
            end_          = o.end_;
            backend_      = std::move(o.backend_);

            o.tlc_        = T();
            o.global_pos_ = 0;
            o.local_pos_  = 0;
            o.begin_      = nullptr;
            o.end_        = nullptr;
            return *this;
        }
    template <typename T>
        size_t Simple_Reader<T>::pos() const
        {
            return global_pos_;
        }
    template <typename T>
        Simple_Reader<T>::Simple_Reader(const u8 *begin, const u8 *end)
        :
            begin_(begin),
            end_(end),
            backend_(new scratchpad::Memory_Reader<u8>(begin_, end_))
    {
    }
    template <typename T>
        Simple_Reader<T>::Simple_Reader(
                std::unique_ptr<scratchpad::Reader<u8>> &&backend)
        :
            begin_(nullptr),
            end_(nullptr),
            backend_(std::move(backend))
    {
    }

    template <typename T>
        const T &Simple_Reader<T>::tlc() const
        {
            return tlc_;
        }
    template <typename T>
        T &Simple_Reader<T>::tlc()
        {
            return tlc_;
        }
    template <typename T>
        bool Simple_Reader<T>::next()
        {

            if (backend_ && (end_ - begin_) < 20 && !backend_->eof()) {
                std::tie(begin_, end_) = backend_->read_more(local_pos_, 20);
                local_pos_ = 0;
            }
            if (begin_ == end_)
                return false;
            tlc_.read(begin_, end_);

            size_t k = tlc_.tl_size;
            if (tlc_.shape == Shape::PRIMITIVE)
                k += tlc_.length;

            if (tlc_.shape == Shape::PRIMITIVE) {
                if (size_t(end_ - begin_) < k) {
                    std::tie(begin_, end_) = backend_->read_more(local_pos_,
                            k - size_t(end_ - begin_));
                    local_pos_ = 0;
                    // XXX fix this when T=Unit
                    tlc_.begin = begin_;
                }
                if (size_t(end_ - begin_) < k) {
                    throw range_error("content overflows");
                }
            }
            local_pos_  += k;
            global_pos_ += k;
            begin_ += k;
            return true;
        }

    template class Simple_Reader<TLC>;
    //template class Simple_Reader<Unit>;



} // namespace xfsx
