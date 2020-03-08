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

#include <ixxx/posix.hh>
#include <ixxx/util.hh>

#include <xfsx/bcd.hh>
#include <xfsx/bcd_impl.hh>

using namespace std;

static void Assert(bool b, const string &msg)
{
  if (!b)
    throw std::runtime_error(msg);
}
using u8 = xfsx::u8;

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
  size_t inp_increment(size_t digits) const
  {
    return digits/2u;
  }
  size_t out_increment(size_t digits) const
  {
    return digits;
  }
  void init(vector<char> &) const
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
  size_t inp_increment(size_t digits) const
  {
    return digits;
  }
  size_t out_increment(size_t digits) const
  {
    return digits/2;
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
      const size_t n = helper.inp_increment(args.digits);
      const size_t m = helper.out_increment(args.digits);
      auto start = chrono::high_resolution_clock::now();
      for (size_t i = 0; i < args.blocks; ++i) {
        some_fn(x, x + n, o);
        x += n;
        o += m;
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

  bench("Naive branch decode", args,
      [](const u8 *b, const u8 *e, char *o) {
        using namespace xfsx::bcd::impl::decode;
        decode_bytewise<Convert::BRANCH>(b, e, o); }, Decode_Helper());
  bench("Naive cmp decode", args,
      [](const u8 *b, const u8 *e, char *o) {
        using namespace xfsx::bcd::impl::decode;
        decode_bytewise<Convert::DIRECT>(b, e, o); }, Decode_Helper());
  bench("Naive subtract decode", args,
      [](const u8 *b, const u8 *e, char *o) {
        using namespace xfsx::bcd::impl::decode;
        decode_bytewise<Convert::ARITH>(b, e, o); }, Decode_Helper());
  bench("Lookup decode", args,
      [](const u8 *b, const u8 *e, char *o) {
        using namespace xfsx::bcd::impl::decode;
        decode_lookup(b, e, o); }, Decode_Helper());
  bench("Lookup small decode", args,
      [](const u8 *b, const u8 *e, char *o) {
        using namespace xfsx::bcd::impl::decode;
        decode_lookup<Convert::SMALL>(b, e, o); }, Decode_Helper());

  bench("SWAR decode", args,
      [](const u8 *b, const u8 *e, char *o) {
        using namespace xfsx::bcd::impl::decode;
        decode_swar<Scatter::LOOP>(b, e, o); }, Decode_Helper());

#if defined(__BMI2__)
  bench("SWAR PDEP decode", args,
      [](const u8 *b, const u8 *e, char *o) {
        using namespace xfsx::bcd::impl::decode;
        decode_swar<Scatter::PDEP>(b, e, o); }, Decode_Helper());
#endif
#ifdef __SSSE3__
  bench("SIMD SSE3 decode", args,
      [](const u8 *b, const u8 *e, char *o) {
        using namespace xfsx::bcd::impl::decode;
        decode_ssse3<Scatter::LOOP>(b, e, o); }, Decode_Helper());
  #if defined(__BMI2__)
  bench("SIMD SSE3 PDEP decode", args,
      [](const u8 *b, const u8 *e, char *o) {
        using namespace xfsx::bcd::impl::decode;
        decode_ssse3<Scatter::PDEP>(b, e, o); }, Decode_Helper());
  #endif
#endif
  bench("default decode", args,
      [](const u8 *b, const u8 *e, char *o) {
        using namespace xfsx::bcd;
        decode(b, e, o); }, Decode_Helper());
  cerr << '\n';

  bench("Default encode", args,
      [](const char *b, const char *e, u8 *o) {
        xfsx::bcd::encode(b, e, o); }, Encode_Helper());
  bench("Bytewise branch", args,
      [](const char *b, const char *e, u8 *o) {
        using namespace xfsx::bcd::impl::encode;
        encode_bytewise<Convert::BRANCH>(b, e, o); },
        Encode_Helper());
  bench("Bytewise cmp", args,
      [](const char *b, const char *e, u8 *o) {
        using namespace xfsx::bcd::impl::encode;
        encode_bytewise<Convert::DIRECT>(b, e, o); },
        Encode_Helper());
  bench("Lookup", args,
      [](const char *b, const char *e, u8 *o) {
        using namespace xfsx::bcd::impl::encode;
        encode_lookup(b, e, o); },
        Encode_Helper());
  bench("SWAR", args,
      [](const char *b, const char *e, u8 *o) {
        using namespace xfsx::bcd::impl::encode;
        encode_swar<Convert::DIRECT, Gather::LOOP>(b, e, o); },
        Encode_Helper());
#if defined(__BMI2__)
  bench("SWAR PEXT", args,
      [](const char *b, const char *e, u8 *o) {
        using namespace xfsx::bcd::impl::encode;
        encode_swar<Convert::DIRECT, Gather::PEXT>(b, e, o); },
        Encode_Helper());
#endif
#ifdef __SSSE3__
  bench("SIMD SSSE3", args,
      [](const char *b, const char *e, u8 *o) {
        using namespace xfsx::bcd::impl::encode;
        encode_ssse3<Convert::DIRECT, Gather::LOOP>(b, e, o); },
        Encode_Helper());
  bench("SIMD SSSE3 shift/and gather", args,
      [](const char *b, const char *e, u8 *o) {
        using namespace xfsx::bcd::impl::encode;
        encode_ssse3<Convert::DIRECT, Gather::SHIFT_AND>(b, e, o); },
        Encode_Helper());
  bench("SIMD SSSE3 saturate shuffle convert", args,
      [](const char *b, const char *e, u8 *o) {
        using namespace xfsx::bcd::impl::encode;
        encode_ssse3<Convert::SATURATE_SHUFFLE, Gather::LOOP>(b, e, o); },
        Encode_Helper());
#if defined(__BMI2__)
  bench("SIMD SSSE3 PEXT", args,
      [](const char *b, const char *e, u8 *o) {
        using namespace xfsx::bcd::impl::encode;
        encode_ssse3<Convert::DIRECT, Gather::PEXT>(b, e, o); },
        Encode_Helper());
#endif
#endif //__SSSE3__
  return 0;
}

/*

## example output, Xeon Platinum 8168 CPU @ 2.70GHz

i.e. Skylake Xeon with AVX512 in Digitalocean datacenter, i.e. turbo boost
likely off and possibly shared with AVX512 heavy guests which definitely may
decrease the frequency.

Naive branch decode: decoded 5654.23 MiB in 10 s at 565.423 MiB/s using 1000 times 16 digits per iteration
Naive cmp decode: decoded 4817.44 MiB in 10 s at 481.744 MiB/s using 1000 times 16 digits per iteration
Naive subtract decode: decoded 4616.62 MiB in 10 s at 461.662 MiB/s using 1000 times 16 digits per iteration
Lookup decode: decoded 17749.1 MiB in 10 s at 1774.91 MiB/s using 1000 times 16 digits per iteration
Lookup small decode: decoded 9508.84 MiB in 10 s at 950.884 MiB/s using 1000 times 16 digits per iteration
SWAR decode: decoded 7932.26 MiB in 10 s at 793.226 MiB/s using 1000 times 16 digits per iteration
SWAR PDEP decode: decoded 13289.9 MiB in 10 s at 1328.99 MiB/s using 1000 times 16 digits per iteration
SIMD SSE3 decode: decoded 41749.2 MiB in 10 s at 4174.92 MiB/s using 1000 times 16 digits per iteration
SIMD SSE3 PDEP decode: decoded 11000.5 MiB in 10 s at 1100.05 MiB/s using 1000 times 16 digits per iteration
default decode: decoded 21889.5 MiB in 10 s at 2188.95 MiB/s using 1000 times 16 digits per iteration

Default encode: encoded 22916 MiB in 10 s at 2291.6 MiB/s using 1000 times 16 digits per iteration
Bytewise branch: encoded 9020.16 MiB in 10 s at 902.016 MiB/s using 1000 times 16 digits per iteration
Bytewise cmp: encoded 8814.44 MiB in 10 s at 881.444 MiB/s using 1000 times 16 digits per iteration
Lookup: encoded 18998.3 MiB in 10 s at 1899.83 MiB/s using 1000 times 16 digits per iteration
SWAR: encoded 16959.9 MiB in 10 s at 1695.99 MiB/s using 1000 times 16 digits per iteration
SWAR PEXT: encoded 22176.1 MiB in 10 s at 2217.61 MiB/s using 1000 times 16 digits per iteration
SIMD SSSE3: encoded 35960 MiB in 10 s at 3596 MiB/s using 1000 times 16 digits per iteration
SIMD SSSE3 PEXT: encoded 35924.7 MiB in 10 s at 3592.47 MiB/s using 1000 times 16 digits per iteration


Encoding without AVX512 support disabled:

SIMD SSSE3: encoded 31555.5 MiB in 10 s at 3155.55 MiB/s using 1000 times 16 digits per iteration
SIMD SSSE3 PEXT: encoded 36059.3 MiB in 10 s at 3605.93 MiB/s using 1000 times 16 digits per iteration


## New encode example output (2018-10-11), Skylake i7-6600U CPU @ 2.60GHz

Default encode: encoded 23133.8 MiB in 10 s at 2313.38 MiB/s using 1000 times 16 digits per iteration
Bytewise branch: encoded 9290.95 MiB in 10 s at 929.095 MiB/s using 1000 times 16 digits per iteration
Bytewise cmp: encoded 9587.14 MiB in 10 s at 958.714 MiB/s using 1000 times 16 digits per iteration
Lookup: encoded 18808.4 MiB in 10 s at 1880.84 MiB/s using 1000 times 16 digits per iteration
SWAR: encoded 19447.2 MiB in 10 s at 1944.72 MiB/s using 1000 times 16 digits per iteration
SWAR PEXT: encoded 26709.5 MiB in 10 s at 2670.95 MiB/s using 1000 times 16 digits per iteration
SIMD SSSE3: encoded 42142.7 MiB in 10 s at 4214.27 MiB/s using 1000 times 16 digits per iteration
SIMD SSSE3 PEXT: encoded 42163.9 MiB in 10 s at 4216.39 MiB/s using 1000 times 16 digits per iteration

## Example encode output (2018-10-07), Intel Atom CPU C3758 @ 2.20GHz

Default encode: encoded 9261.15 MiB in 10 s at 926.115 MiB/s using 1000 times 16 digits per iteration
Bytewise branch: encoded 3233.93 MiB in 10 s at 323.393 MiB/s using 1000 times 16 digits per iteration
Bytewise cmp: encoded 3324.31 MiB in 10 s at 332.431 MiB/s using 1000 times 16 digits per iteration
Lookup: encoded 6021.41 MiB in 10 s at 602.141 MiB/s using 1000 times 16 digits per iteration
SWAR: encoded 6281.23 MiB in 10 s at 628.123 MiB/s using 1000 times 16 digits per iteration
SIMD SSSE3: encoded 15165.4 MiB in 10 s at 1516.54 MiB/s using 1000 times 16 digits per iteration

## New decode example output (2018-10-07), Skylake i7-6600U CPU @ 2.60GHz

Naive branch decode: decoded 5575.43 MiB in 10 s at 557.543 MiB/s using 1000 times 16 digits per iteration
Naive cmp decode: decoded 4794.57 MiB in 10 s at 479.457 MiB/s using 1000 times 16 digits per iteration
Naive subtract decode: decoded 4539.64 MiB in 10 s at 453.964 MiB/s using 1000 times 16 digits per iteration
Lookup decode: decoded 14348.7 MiB in 10 s at 1434.87 MiB/s using 1000 times 16 digits per iteration
Lookup small decode: decoded 9259.23 MiB in 10 s at 925.923 MiB/s using 1000 times 16 digits per iteration
SWAR decode: decoded 9763.04 MiB in 10 s at 976.304 MiB/s using 1000 times 16 digits per iteration
SWAR PDEP decode: decoded 17100.6 MiB in 10 s at 1710.06 MiB/s using 1000 times 16 digits per iteration
SIMD SSE3 decode: decoded 43511.9 MiB in 10 s at 4351.19 MiB/s using 1000 times 16 digits per iteration
SIMD SSE3 PDEP decode: decoded 10635.6 MiB in 10 s at 1063.56 MiB/s using 1000 times 16 digits per iteration
default decode: decoded 15485.1 MiB in 10 s at 1548.51 MiB/s using 1000 times 16 digits per iteration

## Example decode output (2018-10-07), Intel Atom CPU C3758 @ 2.20GHz

Naive branch decode: decoded 1945.64 MiB in 10 s at 194.564 MiB/s using 1000 times 16 digits per iteration
Naive cmp decode: decoded 1715.36 MiB in 10 s at 171.536 MiB/s using 1000 times 16 digits per iteration
Naive subtract decode: decoded 1520.94 MiB in 10 s at 152.094 MiB/s using 1000 times 16 digits per iteration
Lookup decode: decoded 4717.04 MiB in 10 s at 471.704 MiB/s using 1000 times 16 digits per iteration
Lookup small decode: decoded 2999.39 MiB in 10 s at 299.939 MiB/s using 1000 times 16 digits per iteration
SWAR decode: decoded 3416.11 MiB in 10 s at 341.611 MiB/s using 1000 times 16 digits per iteration
SIMD SSE3 decode: decoded 12041.5 MiB in 10 s at 1204.15 MiB/s using 1000 times 16 digits per iteration
default decode: decoded 7354.67 MiB in 10 s at 735.467 MiB/s using 1000 times 16 digits per iteration
   
## Old example output on a 2nd gen iCore7 (i7-2640M).

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

