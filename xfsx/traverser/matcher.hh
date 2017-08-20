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

#ifndef XFSX_TRAVERSER_MATCHER_HH
#define XFSX_TRAVERSER_MATCHER_HH

#include <string>
#include <vector>
#include <deque>
#include <stack>
#include <unordered_map>
#include <ostream>
#include <functional>

// #include <iostream>

#include <xfsx/xfsx.hh>
#include <xfsx/traverser/traverser.hh>

namespace xfsx {

  namespace traverser {

    enum class Matcher_Result { NONE, INIT, APPLY, FINALIZE };


    template <typename Proxy, typename T>
    struct Basic_Matcher {
      std::vector<Tag_Int> path_;
      bool anywhere_ {false};
      // XXX also test this during matching
      Klasse klasse_ {Klasse::APPLICATION};

      enum State { INITIAL, MATCHING, SUSPENDED, INSIDE, FINISHED };
      State state_ { INITIAL };
      uint32_t off_ {0};
      uint32_t mis_off_ {0};
      uint32_t mat_off_ {0};
      Matcher_Result result_ {Matcher_Result::NONE};
      std::function<xfsx::traverser::Hint(xfsx::traverser::Hint r,
            Matcher_Result res, const Proxy &p, const T &t)> callback_;

      Basic_Matcher(const std::vector<Tag_Int> &path,
          bool anywhere = false, Klasse klasse = Klasse::APPLICATION)
        : path_(path),
          anywhere_(anywhere),
          klasse_(klasse)
      {
      }

      xfsx::traverser::Hint operator()(const Proxy &p, const T &t)
      {
        result_ = Matcher_Result::NONE;
        //std::cerr << "State: " << state_ << ", tag: " << p.tag(t) << ", height: " << p.height(t) << ", off: " << off_ << ", mis_off: " << mis_off_ << ", mat_off: " << mat_off_ << '\n';
        auto r = Hint::DESCEND;
        if (state_ != INITIAL) {
          if (p.height(t) <= mis_off_) {
            state_ = MATCHING;
            mis_off_ = 0;
          }
          if (p.height(t) <= mat_off_) {
            result_ = Matcher_Result::FINALIZE;
            state_ = MATCHING;
            mat_off_ = 0;
          }
        }
        if (p.height(t) <= off_) {
          state_ = INITIAL;
          off_ = 0;
        }
        switch (state_) {
          case FINISHED:
            return Hint::STOP;
          case INITIAL:
            if (!path_.empty() && (!p.height(t) || anywhere_)
                && (!path_[0] || path_[0] == p.tag(t))) {
              off_ = p.height(t);
              if (path_.size() == 1) {
                result_ = Matcher_Result::INIT;
                state_ = INSIDE;
                mat_off_ = p.height(t);
              } else {
                state_ = MATCHING;
              }
            }
            break;
          case MATCHING:
            if (path_.at(p.height(t) - off_) == p.tag(t)
                || !path_.at(p.height(t) - off_) ) {
              if (path_.size() == p.height(t) + 1 - off_) {
                result_ = Matcher_Result::INIT;
                state_ = INSIDE;
                mat_off_ = p.height(t);
              }
            } else {
              mis_off_ = p.height(t);
              state_ = SUSPENDED;
              r = Hint::SKIP_CHILDREN;
            }
            break;
          case INSIDE:
            result_ = Matcher_Result::APPLY;
            break;
          case SUSPENDED:
            r = Hint::SKIP_CHILDREN;
            break;
        }
        if (callback_)
          r = callback_(r, result_, p, t);
        return r;
      }
    };

    template <typename P, typename T, typename M>
    xfsx::traverser::Hint advance_first_match(P &p, T &t, M &m)
    {
      while (!p.eot(t)) {
        auto r = m(p, t);
        if (m.result_ == traverser::Matcher_Result::INIT)
          return r;
        if (r == traverser::Hint::SKIP_CHILDREN)
          p.skip_children(t);
        else
          p.advance(t);
      }
      return xfsx::traverser::Hint::STOP;
    }
    template <typename P, typename T, typename M>
    xfsx::traverser::Hint advance_next_match(xfsx::traverser::Hint r,
        P &p, T &t, M &m)
    {
      for (;;) {
        if (r == traverser::Hint::SKIP_CHILDREN)
          p.skip_children(t);
        else
          p.advance(t);
        if (p.eot(t))
          break;
        auto r = m(p, t);
        if (m.result_ == traverser::Matcher_Result::INIT)
          return r;
      }
      return xfsx::traverser::Hint::STOP;
    }

    inline std::ostream &operator<<(std::ostream &o, Matcher_Result r)
    {
      switch (r) {
        case Matcher_Result::NONE: o << "NONE"; break;
        case Matcher_Result::INIT: o << "INIT"; break;
        case Matcher_Result::APPLY: o << "APPLY"; break;
        case Matcher_Result::FINALIZE: o << "FINALIZE"; break;
      };
      return o;
    }

  } // traverser

} // xfsx

#endif // XFSX_TRAVERSER_MATCHER_HH

