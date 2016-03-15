// Copyright 2015, Georg Sauthoff <mail@georg.so>

/* {{{ LGPLv3

    This file is part of libxfsx.

    libxfsx is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    libxfsx is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with libxfsx.  If not, see <http://www.gnu.org/licenses/>.

}}} */

#ifndef XFSX_TAP_TRAVERSER_HH
#define XFSX_TAP_TRAVERSER_HH

#include <xfsx/integer.hh>
#include <xfsx/traverser/traverser.hh>
#include <grammar/tap/tap.hh>

#include <boost/date_time/posix_time/posix_time.hpp>

#include <functional>

namespace xfsx {
  namespace byte {
    namespace writer {
      class Base;
    }
  }

  namespace tap {

    namespace traverser {
      
      using namespace xfsx::traverser;

        class CDR_Count {
          private:
            size_t counter_ {0};
            bool inside_cdrs_ {false};
          public:
            // The hint returned is just that - a hint - it can
            // be interpreted by the traverse function to speed the
            // traversal. But the traverse function should also work
            // when some hints are ignored and everything is fed into it.
            template <typename T, typename Proxy>
            xfsx::traverser::Hint operator()(const Proxy &p, const T &t)
            {
              if (p.tag(t) == grammar::tap::AUDIT_CONTROL_INFO) {
                inside_cdrs_ = false;
                return Hint::STOP;
              }
              if (!p.height(t))
                return Hint::DESCEND;
              if (p.height(t) < 2
                  && p.tag(t) != grammar::tap::CALL_EVENT_DETAIL_LIST)
                return Hint::SKIP_CHILDREN;
              if (p.tag(t) == grammar::tap::CALL_EVENT_DETAIL_LIST) {
                inside_cdrs_ = true;
                return Hint::DESCEND;
              }
              if (p.height(t) == 2 && inside_cdrs_) {
                ++counter_;
              }
              return Hint::SKIP_CHILDREN;
            }
            size_t operator()() const { return counter_; }
        };

        class Charge_Sum {
          private:
            uint64_t charge_ {0};
            std::string charge_type_ {"00"};
            bool refund_ {false};
            bool inside_cdrs_ {false};
          public:
            template <typename T, typename Proxy>
            xfsx::traverser::Hint operator()(const Proxy &p, const T &t)
            {
              if (!p.height(t))
                return Hint::DESCEND;
              if (p.tag(t) == grammar::tap::AUDIT_CONTROL_INFO) {
                inside_cdrs_ = false;
                return Hint::STOP;
              }
              if (p.tag(t) == grammar::tap::CALL_EVENT_DETAIL_LIST) {
                inside_cdrs_ = true;
                return Hint::DESCEND;
              }
              if (inside_cdrs_) {
                if (p.height(t) == 2) {
                  charge_type_ = "00";
                  refund_ = false;
                } else {
                  switch (p.tag(t)) {
                    case grammar::tap::CHARGE_TYPE:
                      p.string(t, charge_type_);
                      break;
                    case grammar::tap::CHARGE_REFUND_INDICATOR:
                      refund_ = true;
                      break;
                    case grammar::tap::CAMEL_INVOCATION_FEE:
                    case grammar::tap::CHARGE:
                      if (!refund_ && charge_type_ == "00")
                        charge_ += p.uint64(t);
                      break;
                  }
                }
                return Hint::DESCEND;
              }
              return Hint::SKIP_CHILDREN;
            }
            uint64_t operator()() const { return charge_; }
        };

        struct Less_Tag {};
        struct Greater_Tag {};

        template <typename Tag = Less_Tag>
        class Timestamp {
          private:
            std::map<uint32_t, std::string> off_map_;
            std::map<uint32_t, std::string> timestamps_;
            uint32_t code_ {0};
            std::string timestamp_;
            uint32_t completion_code_ {0};
            std::string completion_timestamp_;
            uint32_t deposit_code_ {0};
            std::string deposit_timestamp_;
            std::string cp_;
            enum State { OUTSIDE, INSIDE_NETWORK_INFO, INSIDE_CDRS,
              INSIDE_CHARGING_TIME_STAMP,
              INSIDE_COMPLETION_TIME_STAMP,
              INSIDE_DEPOSIT_TIME_STAMP
            };
            State state_ {OUTSIDE};
            std::pair<std::string, std::string> timestamp_long_;
          public:
            template <typename T, typename Proxy>
            xfsx::traverser::Hint operator()(const Proxy &p, const T &t)
            {
              // XXX implement strict TD.57 rules
              if (p.height(t) < 2)
                state_ = OUTSIDE;
              switch (state_) {
                case OUTSIDE:
                  switch (p.tag(t)) {
                    case grammar::tap::AUDIT_CONTROL_INFO:
                      finalize();
                      return Hint::STOP;
                    case grammar::tap::CALL_EVENT_DETAIL_LIST:
                      state_ = INSIDE_CDRS;
                      return Hint::DESCEND;
                    case grammar::tap::NETWORK_INFO:
                      state_ = INSIDE_NETWORK_INFO;
                      return Hint::DESCEND;
                  }
                  return p.height(t) ? Hint::SKIP_CHILDREN : Hint::DESCEND;
                case INSIDE_NETWORK_INFO:
                  switch (p.tag(t)) {
                    case grammar::tap::UTC_TIME_OFFSET_CODE:
                      code_ = p.uint32(t);
                      break;
                    case grammar::tap::UTC_TIME_OFFSET:
                      p.string(t, off_map_[code_]);
                      break;
                  }
                  return Hint::DESCEND;
                case INSIDE_CDRS:
                  switch (p.tag(t)) {
                    // The TD.57 is not absolutely clear on this; the
                    // resulting field is called 'Earliest Call Timestamp'
                    // and the description talks about the
                    // 'charging timestamp' of records or 'Call Event Details'.
                    // Thus, ignoring the Charge Detail Time Stamp for now.
                    // case grammar::tap::CHARGE_DETAIL_TIME_STAMP:
                    case grammar::tap::CHARGING_TIME_STAMP:
                    case grammar::tap::CALL_EVENT_START_TIME_STAMP:
                      state_ = INSIDE_CHARGING_TIME_STAMP;
                      break;
                    case grammar::tap::DEPOSIT_TIME_STAMP:
                      state_ = INSIDE_DEPOSIT_TIME_STAMP;
                      break;
                    case grammar::tap::COMPLETION_TIME_STAMP:
                      state_ = INSIDE_COMPLETION_TIME_STAMP;
                      break;
                    case grammar::tap::CHARGING_POINT:
                      {
                        p.string(t, cp_);
                        if (cp_.size() != 1)
                          throw std::runtime_error("Unexpected charging point size");
                        switch (cp_[0]) {
                          case 'C':
                            push(completion_timestamp_, completion_code_);
                            break;
                          case 'D':
                            push(deposit_timestamp_, deposit_code_);
                            break;
                        }
                        completion_timestamp_.clear();
                        completion_code_ = 0;
                        deposit_timestamp_.clear();
                        deposit_code_ = 0;
                      }
                      break;
                  }
                  return Hint::DESCEND;
                case INSIDE_DEPOSIT_TIME_STAMP:
                  switch (p.tag(t)) {
                    case grammar::tap::LOCAL_TIME_STAMP:
                      p.string(t, deposit_timestamp_);
                      break;
                    case grammar::tap::UTC_TIME_OFFSET_CODE:
                      deposit_code_ = p.uint32(t);
                      state_ = INSIDE_CDRS;
                      break;
                  }
                  return Hint::DESCEND;
                case INSIDE_COMPLETION_TIME_STAMP:
                  switch (p.tag(t)) {
                    case grammar::tap::LOCAL_TIME_STAMP:
                      p.string(t, completion_timestamp_);
                      break;
                    case grammar::tap::UTC_TIME_OFFSET_CODE:
                      completion_code_ = p.uint32(t);
                      state_ = INSIDE_CDRS;
                      break;
                  }
                  return Hint::DESCEND;
                case INSIDE_CHARGING_TIME_STAMP:
                  switch (p.tag(t)) {
                    case grammar::tap::LOCAL_TIME_STAMP:
                      p.string(t, timestamp_);
                      break;
                    case grammar::tap::UTC_TIME_OFFSET_CODE:
                      auto code = p.uint32(t);
                      push(timestamp_, code);
                      state_ = INSIDE_CDRS;
                      break;
                  }
                  return Hint::DESCEND;
              }
              return Hint::DESCEND;
            }
            void push(const std::string &timestamp, uint32_t code)
            {
              auto i = timestamps_.find(code);
              if (i == timestamps_.end())
                timestamps_[code] = timestamp;
              else {
                using Cmp = typename std::conditional<
                  std::is_same<Tag, Less_Tag>::value,
                    std::less<std::string>,
                    std::greater<std::string> >::type;
                if (Cmp()(timestamp, i->second))
                  i->second = timestamp;
              }
            }
            void finalize()
            {
              if (timestamps_.empty())
                return;
              using Cmp = typename std::conditional<std::is_same<Tag, Less_Tag>::value, std::less<boost::posix_time::ptime>, std::greater<boost::posix_time::ptime> >::type;
              std::map<boost::posix_time::ptime, std::pair<std::string, std::string>,  Cmp> m;
              for (auto &x : timestamps_) {
                if (x.second.size() < 14)
                  throw std::runtime_error("date string too small");
                const char *s = x.second.c_str();
                boost::posix_time::ptime pt(boost::gregorian::date(
                      integer::range_to_uint32(std::make_pair(s, s+4)),
                      integer::range_to_uint32(std::make_pair(s+4, s+6)),
                      integer::range_to_uint32(std::make_pair(s+6, s+8)) ),
                    boost::posix_time::time_duration(
                      integer::range_to_uint32(std::make_pair(s+8, s+10)),
                      integer::range_to_uint32(std::make_pair(s+10, s+12)),
                      integer::range_to_uint32(std::make_pair(s+12, s+14)) ));
                auto &off_str = off_map_.at(x.first);
                if (off_str.size() < 5)
                  throw std::runtime_error("unknown offset format");
                const char *t = off_str.c_str();
                boost::posix_time::time_duration off(
                    integer::range_to_uint32(std::make_pair(t+1, t+3)),
                    integer::range_to_uint32(std::make_pair(t+3, t+5)), 0);
                if (*t == '+')
                  pt -= off;
                else
                  pt += off;
                m[pt] = make_pair(x.second, off_str);
              }
              // or as UTC, or in the localtime of the sender ...
              timestamp_long_ = m.begin()->second;
            }
            const std::pair<std::string, std::string> &operator()() const
            { return timestamp_long_; }
        };

        class Audit_Control_Info {
          private:
          public:
            CDR_Count count;
            Charge_Sum sum;
            Timestamp<Less_Tag>    first_timestamp;
            Timestamp<Greater_Tag> last_timestamp;

            std::string comment;

            template <typename ...F, typename T, typename Proxy>
                void operator()(Proxy &p, T &t, F& ... f)
                {
                  Traverse st;
                  st(p, t, count, sum,
                      first_timestamp, last_timestamp,
                      f...);
                }

            void print(xfsx::byte::writer::Base &o, unsigned indent = 4u);

        };

    }

  } // tap
} // xfsx

#endif
