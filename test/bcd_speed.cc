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

#include <string>
#include <stdexcept>
#include <vector>
#include <chrono>
#include <iostream>

#include <boost/lexical_cast.hpp>

#include <ixxx/posix.h>
#include <ixxx/util.hh>

#include <xfsx/bcd.hh>
#include <xfsx/bcd_impl.hh>

using namespace std;

void Assert(bool b, const string &msg)
{
  if (!b)
    throw std::runtime_error(msg);
}

struct Arguments {
  size_t blocks       {1000}; // blocks per iteration
  size_t digits       {16  }; // digits per block
  size_t seconds      {10  }; // run iterations for x seconds
  size_t skip_seconds {2   }; // warmup

  string urandom {"/dev/urandom"};


  Arguments() {}
  Arguments(int argc, char **argv)
  {
    for (int i = 1; i < argc; ++i) {
      if (!strcmp(argv[i], "-b") || !strcmp(argv[i], "--blocks")) {
        ++i;
        Assert(i<argc, "-b argument is missing");
        blocks = boost::lexical_cast<size_t>(argv[i]);
      } else if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--digits")) {
        ++i;
        Assert(i<argc, "-d argument is missing");
        digits = boost::lexical_cast<size_t>(argv[i]);
      } else if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--seconds")) {
        ++i;
        Assert(i<argc, "-s argument is missing");
        seconds = boost::lexical_cast<size_t>(argv[i]);
      } else if (!strcmp(argv[i], "-k") || !strcmp(argv[i], "--skip")) {
        ++i;
        Assert(i<argc, "-k argument is missing");
        skip_seconds = boost::lexical_cast<size_t>(argv[i]);
      }
      //if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--digits")) {
    }
  }
};

static void random_init(const string &file, vector<char> &v)
{
  ixxx::util::FD fd(file, O_RDONLY);
  size_t i = 0;
  const size_t read_size = 4u * 1024u;
  for (; i + read_size < v.size(); i += read_size)
    ixxx::posix::read(fd, v.data() + i, read_size);
  if (i < v.size())
    ixxx::posix::read(fd, v.data() + i, v.size() - i);
}

struct Decode_Helper
{
  const u8 *cast_input(const char *c) const
  {
    return reinterpret_cast<const u8*>(c);
  }
  char *cast_output(char *c) const
  {
    return c;
  }
  size_t increment_size(size_t digits) const
  {
    return digits/2u;
  }
  void init(vector<char> &v) const
  {
  }
  const char *action_str{"decoded"};
};

struct Encode_Helper
{
  const char *cast_input(const char *c) const
  {
    return c;
  }
  u8 *cast_output(char *c) const
  {
    return reinterpret_cast<u8*>(c);
  }
  size_t increment_size(size_t digits) const
  {
    return digits;
  }
  void init(vector<char> &v) const
  {
    Assert(v.size()%2 == 0, "block size must be even");
    std::copy(v.begin(), v.begin()+v.size()/2,
        v.begin()+v.size()/2);
    std::transform(v.begin(), v.end(), v.begin(),
        [](char c) { unsigned i = c%16; return i < 10 ? '0' + i : 'a' + (i-10); } );
  }
  const char *action_str{"encoded"};
};

template <typename F, typename H>
static void bench(const string &name,
    const Arguments &args, F some_fn, H helper)
{
  vector<char> v(args.blocks * args.digits);
  random_init(args.urandom, v);
  vector<char> w(args.blocks * args.digits);

  chrono::high_resolution_clock::duration da = chrono::seconds(0);
  size_t bytes = 0;
  for (unsigned k = 0; k<2; ++k) {
    size_t seconds = 0;
    if (!k)
      seconds = args.skip_seconds;
    else
      seconds = args.seconds;
    da = chrono::seconds(0);
    bytes = 0;
    while (size_t(chrono::duration_cast<chrono::seconds>(da).count())
            < seconds) {
      helper.init(v);
      auto x = helper.cast_input(v.data());
      auto o = helper.cast_output(w.data());
      const size_t n = helper.increment_size(args.digits);
      auto start = chrono::high_resolution_clock::now();
      for (size_t i = 0; i < args.blocks; ++i) {
        some_fn(x, x + n, o);
        x += n;
        o += args.digits;
        bytes += n;
      }
      auto stop = chrono::high_resolution_clock::now();
      auto d(stop - start);
      da += d;
      swap(v, w);
    }
  }
  //double seconds = chrono::duration_cast<chrono::nanoseconds>(da).count()
  double seconds = chrono::duration_cast<chrono::milliseconds>(da).count()
    / 1000.0;
  double mibs = double(bytes)/1024.0/1024.0;
  cerr << name << ": " << helper.action_str << ' ' << mibs << " MiB in "
    << seconds << " s at "
    << mibs/seconds << " MiB/s using " << args.blocks << " times "
    << args.digits << " digits per iteration\n";
}


int main(int argc, char **argv)
{
  Arguments args(argc, argv);
  //bench_decode(args);
  bench("Default decode", args,
      [](const u8 *b, const u8 *e, char *o) {
        xfsx::bcd::decode(b, e, o); }, Decode_Helper());
  bench("Default functor", args,
      [](const u8 *b, const u8 *e, char *o) {
        xfsx::bcd::impl::decode::Decode<char*>()(b, e, o); }, Decode_Helper());
  bench("Two half decode", args,
      [](const u8 *b, const u8 *e, char *o) {
        xfsx::bcd::impl::decode::Two_Half_Decode<char*>()(b, e, o); },
        Decode_Helper() );
  bench("Two half decode cmp", args,
      [](const u8 *b, const u8 *e, char *o) {
        xfsx::bcd::impl::decode::Two_Half_Decode<char*,
          xfsx::bcd::impl::decode::Half_To_Char_Cmp<> >()(b, e, o); },
          Decode_Helper() );
  bench("Default encode", args,
      [](const char *b, const char *e, u8 *o) {
        xfsx::bcd::encode(b, e, o); }, Encode_Helper());
  bench("Two char encode", args,
      [](const char *b, const char *e, u8 *o) {
        xfsx::bcd::impl::encode::Two_Char_Encode<u8 *>()(b, e, o); },
        Encode_Helper());
  return 0;
}

/* Example output on a 2nd gen iCore7 (i7-2640M).

Compiled with -O3, gcc (GCC) 5.3.1 20151207 (Red Hat 5.3.1-2), Fedora 23.

$ for i in 16 32 64; do ./bcd_speed -s 10 -d $i; done
Default decode: decoded 7608.54 MiB in 10 s at 760.854 MiB/s using 1000 times 16 digits per iteration
Default functor: decoded 9903.98 MiB in 10 s at 990.398 MiB/s using 1000 times 16 digits per iteration
Two half decode: decoded 3418.14 MiB in 10 s at 341.814 MiB/s using 1000 times 16 digits per iteration
Two half decode cmp: decoded 3177.13 MiB in 10 s at 317.713 MiB/s using 1000 times 16 digits per iteration
Default encode: encoded 14510.3 MiB in 10 s at 1451.03 MiB/s using 1000 times 16 digits per iteration
Two char encode: encoded 7216.61 MiB in 10 s at 721.661 MiB/s using 1000 times 16 digits per iteration

Default decode: decoded 9871.35 MiB in 10 s at 987.135 MiB/s using 1000 times 32 digits per iteration
Default functor: decoded 11573.7 MiB in 10 s at 1157.37 MiB/s using 1000 times 32 digits per iteration
Two half decode: decoded 17211.7 MiB in 10 s at 1721.17 MiB/s using 1000 times 32 digits per iteration
Two half decode cmp: decoded 13895.2 MiB in 10 s at 1389.52 MiB/s using 1000 times 32 digits per iteration
Default encode: encoded 18475.8 MiB in 10 s at 1847.58 MiB/s using 1000 times 32 digits per iteration
Two char encode: encoded 32884.4 MiB in 10 s at 3288.44 MiB/s using 1000 times 32 digits per iteration

Default decode: decoded 11565.9 MiB in 10 s at 1156.59 MiB/s using 1000 times 64 digits per iteration
Default functor: decoded 12136.8 MiB in 10 s at 1213.68 MiB/s using 1000 times 64 digits per iteration
Two half decode: decoded 20775.5 MiB in 10 s at 2077.55 MiB/s using 1000 times 64 digits per iteration
Two half decode cmp: decoded 16448.2 MiB in 10 s at 1644.82 MiB/s using 1000 times 64 digits per iteration
Default encode: encoded 21872.3 MiB in 10 s at 2187.23 MiB/s using 1000 times 64 digits per iteration
Two char encode: encoded 42555.7 MiB in 10 s at 4255.57 MiB/s using 1000 times 64 digits per iteration

## PPC970FX, 1.8 GHz, -O3, gcc (Debian 4.9.2-10) 4.9.2, Debian 8

Default decode: decoded 1936.44 MiB in 10 s at 193.644 MiB/s using 1000 times 16 digits per iteration
Default functor: decoded 2363.55 MiB in 10 s at 236.355 MiB/s using 1000 times 16 digits per iteration
Two half decode: decoded 1087.17 MiB in 10 s at 108.717 MiB/s using 1000 times 16 digits per iteration
Two half decode cmp: decoded 762.871 MiB in 10 s at 76.2871 MiB/s using 1000 times 16 digits per iteration
Default encode: encoded 4676.61 MiB in 10 s at 467.661 MiB/s using 1000 times 16 digits per iteration
Two char encode: encoded 1965.74 MiB in 10 s at 196.574 MiB/s using 1000 times 16 digits per iteration

Default decode: decoded 2614.2 MiB in 10 s at 261.42 MiB/s using 1000 times 32 digits per iteration
Default functor: decoded 2957.86 MiB in 10 s at 295.786 MiB/s using 1000 times 32 digits per iteration
Two half decode: decoded 1188.35 MiB in 10 s at 118.835 MiB/s using 1000 times 32 digits per iteration
Two half decode cmp: decoded 810.059 MiB in 10 s at 81.0059 MiB/s using 1000 times 32 digits per iteration
Default encode: encoded 6669.1 MiB in 10 s at 666.91 MiB/s using 1000 times 32 digits per iteration
Two char encode: encoded 2169.07 MiB in 10 s at 216.907 MiB/s using 1000 times 32 digits per iteration

Default decode: decoded 2998.14 MiB in 10 s at 299.814 MiB/s using 1000 times 64 digits per iteration
Default functor: decoded 3373.78 MiB in 10 s at 337.378 MiB/s using 1000 times 64 digits per iteration
Two half decode: decoded 1251.25 MiB in 10 s at 125.125 MiB/s using 1000 times 64 digits per iteration
Two half decode cmp: decoded 837.433 MiB in 10 s at 83.7433 MiB/s using 1000 times 64 digits per iteration
Default encode: encoded 7940.43 MiB in 10 s at 794.043 MiB/s using 1000 times 64 digits per iteration
Two char encode: encoded 2288.39 MiB in 10 s at 228.839 MiB/s using 1000 times 64 digits per iteration

   */

