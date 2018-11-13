// 2018, Georg Sauthoff <mail@gms.tf>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "tlc_writer.hh"

#include "xfsx.hh"

#include <stdexcept>

using namespace std;

namespace xfsx {

    template <typename T>
        Simple_Writer<T>::Simple_Writer()
        :
            backend_(new scratchpad::Memory_Writer<u8>(nullptr, nullptr))
    {
    }

    template <typename T>
        Simple_Writer<T>::Simple_Writer(u8 *begin, u8 *end)
        :
            begin_(begin),
            end_(end),
            backend_(new scratchpad::Memory_Writer<u8>(begin, end))
    {
    }

    template <typename T>
        Simple_Writer<T>::Simple_Writer(
                std::unique_ptr<scratchpad::Writer<u8>> &&backend)
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
        void Simple_Writer<T>::write(const u8 *begin, const u8 *end)
        {
            size_t k = end-begin;

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
    template <typename T>
        void Simple_Writer<T>::clear()
        {
            if (backend_) {
                local_pos_ = 0;
                global_pos_ = 0;
                begin_ = nullptr;
                end_ = nullptr;
                backend_->clear();
            }
        }
    template <typename T>
        scratchpad::Writer<u8> *Simple_Writer<T>::backend()
        {
            return backend_.get();
        }
    template <typename T>
        void Simple_Writer<T>::sync()
        {
            if (backend_) {
                backend_->sync();
            }
        }
    template <typename T>
        void Simple_Writer<T>::set_sync(bool b)
        {
            if (backend_) {
                backend_->set_sync(b);
            }
        }

    template class Simple_Writer<TLC>;
    template class Simple_Writer<TLV>;


} // namespace xfsx
