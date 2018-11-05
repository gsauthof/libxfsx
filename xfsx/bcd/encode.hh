// Copyright 2015-2018, Georg Sauthoff <mail@georg.so>

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
#ifndef XFSX_BCD_IMPL_ENCODE_HH
#define XFSX_BCD_IMPL_ENCODE_HH

/*

   This file contains functions for converting a string of
   hexadecimal characters to big-endian binary-coded decimals
   (BCD, cf. https://en.wikipedia.org/wiki/Binary-coded_decimal).

   The functions are parametrized much to optimally target target different
   architectures and being able to easily benchmark different implementation
   choices.

   The code is layed out from easy/naive to more complicated. Thus, for
   didactic purposes it makes sense to read the code sequentially from the top
   as the variations on the same theme get more complicated.

   Template functions with default template arguments are used
   to keep thing generic where sensible. Implementation choices are
   guarded by if/switch-statements thus when instantiated the compiler
   eliminates some dead code. Some implementation choices can also be
   considered as executable documentation, i.e. when a naive implementation
   describes the effect of a more complicated one.

   There are some comments inline, but see also xfsx/bcd.cc for some
   comments regarding some generic pitfalls.

   See also test/bcd_speed.cc for running the different variants against
   each other (also contains some example results from different
   architectures).

   */

#include <stdint.h>

#include <stdlib.h>
#include <boost/endian/conversion.hpp>

#include <xfsx/bcd/util.hh>

#ifdef __BMI2__
    #include <x86intrin.h> // _pdep_u64
#endif

#ifdef __SSSE3__
    //#include <emmintrin.h> // SSE2
    #include <tmmintrin.h> // SSSE3  _mm_shuffle_epi8
#endif

namespace xfsx { namespace bcd { namespace impl { namespace encode {

    enum class Type {
        BYTEWISE,
        // use a lookup table with 32 entries that maps each character
        // to a nibble
        LOOKUP,
        // SIMD within a register
        // https://en.wikipedia.org/wiki/SWAR
        // e.g. 8 digits in parallel on a 64 bit machine
        SWAR,
        // Single Instruction Multiple Data
        // https://en.wikipedia.org/wiki/SIMD
        // e.g. 16 digits in parallel with SSE2
        SIMD
    };
    enum class Convert {
        // Bytewise: use if-statements in the main loop
        BRANCH,
        // Bytewise: branchfree by creating a mask using normal/bit arithmetic
        ARITH,
        // Bytewise: branchfree by creating a mask using a comparison operator
        // SIMD: masked second subtraction
        DIRECT,
        // SIMD: shuffle the two possible offsets for one subtraction
        SATURATE_SHUFFLE,
        // SIMD: shuffle the three possible offsets for one subtraction
        SHIFT_SHUFFLE
    };
    enum class Gather {
        // SWAR: copy each even byte
        // SIMD: use vertical multiply and add to pack in one step - don't actually loop
        LOOP,
        // SIMD: use right-shift/and vector instructions
        SHIFT_AND,
        // SWAR, SIMD: Bit extract (gather) from a 64 bit source
        // cf. https://en.wikipedia.org/wiki/Bit_Manipulation_Instruction_Sets
        PEXT
    };

    template <Convert convert = Convert::DIRECT>
        void encode_bytewise(const char *begin, const char *end, u8 *o)
        {
            size_t n = end-begin;
            const char *mid = begin + n/2*2;
            u8 lower_case_mask = 0b10'00'00;
            u8 gt9_off = u8('a')-u8('0')-10; // == 0b11'10'11
            for (const char *i = begin; i != mid; i+=2) {
                u8 hi = i[0];
                if (convert == Convert::BRANCH) {
                    if (hi >= u8('a'))
                        hi -= u8('a')-10;
                    else if (hi >= u8('A'))
                        hi -= u8('A')-10;
                    else
                        hi -= u8('0');
                } else {
                    u8 gt9_mask_hi;
                    if (convert == Convert::ARITH) {
                        // hi >> 6 == 0 if hi in ['0'..'9']; otherwise: 1
                        // mask either this or 0b11'11'11
                        gt9_mask_hi = 0b1'00'00'00 - (hi >> 6);
                    } else { // Convert::DIRECT
                        gt9_mask_hi = -int8_t(hi > u8('9')); // 0 or 0b11'11'11'11
                    }
                    // difference between lower/upper case is just the 6th bit
                    hi |= lower_case_mask;
                    hi -= u8('0');
                    hi -= gt9_off & gt9_mask_hi;
                }

                u8 lo = i[1];
                if (convert == Convert::BRANCH) {
                    if (lo >= u8('a'))
                        lo -= u8('a')-10;
                    else if (lo >= u8('A'))
                        lo -= u8('A')-10;
                    else
                        lo -= u8('0');
                } else {
                    u8 gt9_mask_lo;
                    if (convert == Convert::ARITH) {
                        gt9_mask_lo = 0b1'00'00'00 - (lo >> 6);
                    } else { // Convert::DIRECT
                        gt9_mask_lo = -int8_t(lo > u8('9')); // 0 or 0b11'11'11'11
                    }
                    lo |= lower_case_mask;
                    lo -= u8('0');
                    lo -= gt9_off & gt9_mask_lo;
                }
                u8 x = (hi << 4) | lo;
                *o++ = x;
            }
            if (mid != end) {
                u8 hi = *mid;
                if (convert == Convert::BRANCH) {
                    if (hi >= u8('a'))
                        hi -= u8('a')-10;
                    else if (hi >= u8('A'))
                        hi -= u8('A')-10;
                    else
                        hi -= u8('0');
                } else {
                    u8 gt9_mask_hi;
                    if (convert == Convert::ARITH) {
                        // hi >> 6 == 0 if hi in ['0'..'9']; otherwise: 1
                        // mask either this or 0b11'11'11
                        gt9_mask_hi = 0b1'00'00'00 - (hi >> 6);
                    } else { // Convert::DIRECT
                        gt9_mask_hi = -int8_t(hi > u8('9')); // 0 or 0b11'11'11'11
                    }
                    // difference between lower/upper case is just the 6th bit
                    hi |= lower_case_mask;
                    hi -= u8('0');
                    hi -= gt9_off & gt9_mask_hi;
                }
                u8 x = (hi << 4) | u8(0xf);
                *o++ = x;
            }
        }
    inline void encode_lookup(const char *begin, const char *end, u8 *o)
    {
        static const uint8_t table[] = {
            /* 0       => */  0,
            /* 1, 'a'  => */ 10,
            /* 2, 'b'  => */ 11,
            /* 3, 'c'  => */ 12,
            /* 4, 'd'  => */ 13,
            /* 5, 'e'  => */ 14,
            /* 6, 'f'  => */ 15,
            /* 7       => */  0,
            /* 8       => */  0,
            /* 9       => */  0,
            /* 10      => */  0,
            /* 11      => */  0,
            /* 12      => */  0,
            /* 13      => */  0,
            /* 14      => */  0,
            /* 15      => */  0,
            /* 16, '0' => */  0,
            /* 17, '1' => */  1,
            /* 18, '2' => */  2,
            /* 19, '3' => */  3,
            /* 20, '4' => */  4,
            /* 21, '5' => */  5,
            /* 22, '6' => */  6,
            /* 23, '7' => */  7,
            /* 24, '8' => */  8,
            /* 25, '9' => */  9,
            /* 26      => */  0,
            /* 27      => */  0,
            /* 28      => */  0,
            /* 29      => */  0,
            /* 30      => */  0,
            /* 31      => */  0
        };
        size_t n = end-begin;
        const char *mid = begin + n/2*2;
        for (const char *i = begin; i != mid; i+=2) {
            u8 hi = table[u8(i[0])%32];
            u8 lo = table[u8(i[1])%32];
            u8 x = (hi << 4) | lo;
            *o++ = x;
        }
        if (mid != end) {
            u8 hi = table[u8(*mid)%32];
            u8 x = (hi << 4) | u8(0xf);
            *o++ = x;
        }
    }

    /*
       Encode multiple bytes at once with some arithmetic.

       Gather diagram:

       input: abcd
     
       registers read left to right: MSB -> LSB
       for brevity the registers in the diagrams are just 32 bit wide

       memcpy (and byte-swap on little endian architectures)
       +--------+--------+--------+--------+
       |    aaaa|    bbbb|    cccc|    dddd|
       +--------+--------+--------+--------+

       right-shift by 4 a copy:
       +--------+--------+--------+--------+
       |        |aaaa    |bbbb    |cccc    |
       +--------+--------+--------+--------+
       bit-and both copies:
       +--------+--------+--------+--------+
       |    aaaa|aaaabbbb|bbbbcccc|ccccdddd|
       +--------+--------+--------+--------+
       take:
       +--------+--------+--------+--------+
       |        |aaaabbbb|        |ccccdddd|
       +--------+--------+--------+--------+

       */

    template <
#ifdef __BMI2__
        Gather gather = Gather::PEXT,
#else
        Gather gather = Gather::LOOP,
#endif
#if __SIZEOF_SIZE_T__ == 4
        typename T = uint32_t
#else
        typename T = uint64_t
#endif
        >
        void encode_swar_word(const char *begin, u8 *o)
    {
        // Scatter
        T x = movbe<T>(begin);

        // Convert
        T lower_case_mask = bcast<T>(0b10'00'00);
        T gt9_off = bcast<T>(u8('a')-u8('0')-u8(10));

        T m = (x>>6) & bcast<T>(1);
        T gt9_mask = bcast<T>(0b1'00'00'00) - m;

        x |= lower_case_mask;
        x -= bcast<T>('0');
        x -= gt9_off & gt9_mask;

        // Gather
#if defined(__BMI2__)
        if (gather == Gather::LOOP) {
#endif

            T y = x | (x>>4);
            for (unsigned i = sizeof(T)*8-2*8; i > 0; i -= 16)
                *o++ = (y >> i) & 0xff;
            *o++ = y & 0xff;
#if defined(__BMI2__)
        } else { // Scatter::PEXT
            static_assert(sizeof(T) == 8,
                    "PEXT gather is only useful with T=uint64_t");
            // extract the low 4 bits for each byte, i.e. with 0b00'00'11'11
            uint32_t y =_pext_u64(x, bcast<T>(0xf));
            boost::endian::big_to_native_inplace(y);
            memcpy(o, &y, sizeof y);
        }
#endif
    }

    template <
        Convert convert = Convert::DIRECT,
#ifdef __BMI2__
        Gather gather = Gather::PEXT,
#else
        Gather gather = Gather::LOOP,
#endif
#if __SIZEOF_SIZE_T__ == 4
        typename T = uint32_t
#else
        typename T = uint64_t
#endif
    >
    void encode_swar(const char *begin, const char *end, u8 *o)
    {
        size_t n = end-begin;
        const char *mid = begin + n/8*8;
        for (const char *i = begin; i != mid; i+=8) {
            encode_swar_word<gather, T>(i, o);
            o += 4;
        }
        encode_lookup(mid, end, o);
    }

#ifdef __SSSE3__
    /*
       Encode decimal characters into a big-endian BCD string
       using SSSE3 instructions.

       The conversion is pretty similar to the non-branch/cmp bytewise
       and the SWAR variant. In contrast to SWAR we have an efficient
       and convenient bytewise compare instruction (_mm_cmpgt_epi8()),
       thus we use that to create a mask instead of doing some arithmetic
       around a boundary value.

       One notable SSSE3 instinsic is the shuffle for efficiently
       moving bytes around in a register. Most other stuff is then
       SSE3 or earlier.

       In contrast to the decoding, the shuffle can't be directly
       used with a lookup table, because we would need a 32 element
       table, whereas shuffle only support 16 byte vectors, even
       with AVX. However, we can map the input ASCII characters
       to a smaller range (by a saturating subtraction or a right-shift)
       and then use shuffle to lookup the right offsets. That means
       we map the ASCII ranges '0'..'9', 'A'..'F', 'a'..'f' to
       a subset of 0..15 - which are then used as indexes into a
       offset lookup table. Although, this saves us one or two
       instructions, it's not necessarily faster as just masking a second
       subtraction (e.g. not on i7 Skylake).

       The AVX extensions increase the register sizes to 256 and
       512 Bit, which alone isn't that useful for many BCD
       use cases where we often just have to deal with 16 digits/8 bytes.
       But AVX512 (!) also generalizes some new instructions to
       128 bit registers with 8 bit lanes. Particularly interesting
       are the maskable instructions, e.g. _mm_mask_sub_epi8().
       With that instruction we can eliminate another mask adjustment SIMD
       instruction and free an SIMD register.

       Some ASCII diagrams:
       register content is written left to right 
       most significant byte (MSB) to least signifcant byte (LSB)
       when a byte contains 2 characters, each symbolizes 4 bits
       conversion details are left out for brevity

       input: a b c d  e f g h    i j k l  m n o p
       unaligned load, lddqu
           => x:  p  o  n  m   l  k  j  i     h  g  f  e   d  c  b  a
       swap for each 16 bits:
           => x:  o  p  m  n   k  l  i  j     g  h  e  f   c  d  a  b   
       convert each byte to 4 bits:
           => x: 0o 0p 0m 0n  0k 0l 0i 0j    0g 0h 0e 0f  0c 0d 0a 0b   
       right-shift complete vector by 4 bits:
           => y: 00 o0 p0 m0  n0 k0 l0 i0    j0 g0 h0 e0  f0 c0 d0 a0   
       bit-or x and y:
           => z: 0o op pm mn  nk kl li ij    jg gh he ef  fc cd da ab   
       shuffle every 2nd byte to the lower half with mask:
           0x80 (8 times), 14, 12, 10, 8, 6, 4, 2, 0
           => x: 00 00 00 00  00 00 00 00    op mn kl ij  gh ef cd ab
       store lower half into the output string:
           => ab cd ef gh ij kl mn op

       PEXT (bit extract alternative)
       input: a b c d  e f g h    i j k l  m n o p
       unaligned load, lddqu
           => x:  p  o  n  m   l  k  j  i     h  g  f  e   d  c  b  a
       swap for each 16 bits:
           => x:  o  p  m  n   k  l  i  j     g  h  e  f   c  d  a  b   
       convert each byte to 4 bits:
           => x: 0o 0p 0m 0n  0k 0l 0i 0j    0g 0h 0e 0f  0c 0d 0a 0b   
       bit extract (PEXT) high and low (into two 32 bit integers):
           => j[1]: op mn kl ij  j[0]: gh ef cd ab       
       store those 8 bytes into the output string:
           => ab cd ef gh ij kl mn op

       Packed scatter alternative:
       convert each byte to 4 bits:
           => x: 0o 0p 0m 0n  0k 0l 0i 0j    0g 0h 0e 0f  0c 0d 0a 0b   
       vertically multiply each byte with y and add the result into 16 bit components
       with _mm_maddubs_epi16(), i.e. multiply each odd element by 2*4, i.e. left shift by 4:
           => y: 0x10 0x01 0x10 0x01 ... 0x10 0x01
           => x: 00 op 00 mn  00 kl 00 ij    00 gh 00 ef  00 cd 00 ab
       shuffle every 2nd byte to the lower half with mask:
           0x80 (8 times), 14, 12, 10, 8, 6, 4, 2, 0
           => x: 00 00 00 00  00 00 00 00    op mn kl ij  gh ef cd ab
       store lower half into the output string:
           => ab cd ef gh ij kl mn op

     */
    template <
        Convert convert = Convert::DIRECT,
#ifdef __BMI2__
        // mixing SSE with  BMI slows things down
        Gather gather = Gather::LOOP
#else
        Gather gather = Gather::LOOP
#endif
        >
        void encode_ssse3_word(const char *begin, u8 *o)
    {
        // scatter
        // unaligned load 16 bytes
        __m128i x = _mm_lddqu_si128((const __m128i*)begin);

        // pair-wise swap neighboring bytes
        // arguments left to right are MSB to LSB
        __m128i scatter_mask = _mm_set_epi8(14, 15, 12, 13, 10, 11, 8, 9, 6, 7, 4, 5, 2, 3, 0, 1);
        x = _mm_shuffle_epi8(x, scatter_mask);

        // Convert
        switch (convert) {
            case Convert::DIRECT: // fastest on Skylake
            {
            // broadcast argument to all 16 byte positions
            __m128i lower_case_mask = _mm_set1_epi8(0b10'00'00);
            __m128i gt9_off = _mm_set1_epi8(u8('a')-u8('0')-u8(10));

            __m128i v9 = _mm_set1_epi8(u8('9'));
#if !(defined(__AVX512VL__) && defined(__AVX512BW__))
            // byte-wise compare each byte: 0xff if greather than, 0 otherwise
            // this mask is for zeroing out offsets on some byte positions
            __m128i gt9_mask = _mm_cmpgt_epi8(x, v9);
#else
            // /proc/cpuinfo avx512vl avx512bw, -mavx512vl -mavx512bw
            // byte-wise compare each byte: result is a 16 bit mask,
            // i.e. one bit 0/1 per byte
            // _MM_CMPINT_NLE is the same operator ...
            __mmask16 gt9_mask = _mm_cmp_epi8_mask(x, v9, _MM_CMPINT_GT);
#endif

            // unconditionally make characters lower-case, null-op for '0'..'9'
            x = _mm_or_si128(x, lower_case_mask);
            __m128i v0 = _mm_set1_epi8(u8('0'));
            x = _mm_sub_epi8(x, v0);
#if !(defined(__AVX512VL__) && defined(__AVX512BW__))
            __m128i m = _mm_and_si128(gt9_off, gt9_mask);
            x = _mm_sub_epi8(x, m);
#else
            // only subtract if corresponding bit in the mask is 1
            x = _mm_mask_sub_epi8(x, gt9_mask, x, gt9_off);
#endif
            }
            break;
            case Convert::SHIFT_SHUFFLE: // slowest on Skylake (not much though)
            {
            // we want to right-shift each byte, but SSE/AVX don't provide
            // such an instruction, the next best thing is to emulate it
            // shifting each 16 bits and masking the spill from the highest 8 bits 
            __m128i l2_mask = _mm_set1_epi8(0b11);
            // shift right by 5 bits, each 16 bit element
            __m128i off_choice = _mm_srli_epi16(x, 5);
            // mask the low 2 bits
            off_choice = _mm_and_si128(off_choice, l2_mask);
            // arguments left to right are MSB to LSB
            __m128i off_lut = _mm_set_epi8(0x80, 0x80, 0x80, 0x80,
                    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 
                    u8('a')-10, u8('A')-10 , u8('0') , 0x80);
            __m128i off = _mm_shuffle_epi8(off_lut, off_choice);
            x = _mm_sub_epi8(x, off);
            }
            break;
            case Convert::SATURATE_SHUFFLE: // a bit slower on Skylake (not much)
            {
            // the lower-case masking allows us to use the saturating
            // subtraction
            //
            // broadcast argument to all 16 byte positions
            __m128i lower_case_mask = _mm_set1_epi8(0b10'00'00);
            // unconditionally make characters lower-case, null-op for '0'..'9'
            x = _mm_or_si128(x, lower_case_mask);

            __m128i cutoff = _mm_set1_epi8(u8('a')-1);
            // saturating subtract
            // i.e. each byte is either 0 (for '0'..'9') or 1..6 (for 'a'..'f')
            __m128i off_choice = _mm_subs_epu8(x, cutoff);
            // arguments left to right are MSB to LSB
            __m128i off_lut = _mm_set_epi8(0x80, 0x80, 0x80, 0x80,
                    0x80, 0x80, 0x80, 0x80,
                    0x80, u8('a')-10, u8('a')-10, u8('a')-10,
                          u8('a')-10, u8('a')-10, u8('a')-10, u8('0'));
            __m128i off = _mm_shuffle_epi8(off_lut, off_choice);
            x = _mm_sub_epi8(x, off);
            }
            break;
        }

        // Gather
        switch (gather) {
#if !defined( __BMI2__)
            case Gather::PEXT:
#endif
            case Gather::LOOP:
            {
            __m128i pack_mask = _mm_set_epi8(0x10, 0x01, 0x10, 0x01,
                    0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01,
                    0x10, 0x01, 0x10, 0x01); 
            // vertically multiply (i.e. left shift by 4 or 0) and add
            // for each 2 8-bit elements - store result in 16 bits
            // has higher latency and shift/and, but just one instruction
            __m128i z = _mm_maddubs_epi16(x, pack_mask);
            // move bytes on even positions to the lower half
            // for easier unloading
            // 0x80 inserts a 0, arguments left to right from MSB to LSB
            __m128i gather_mask = _mm_set_epi8(
                    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
                    14, 12, 10, 8, 6, 4, 2, 0);
            x = _mm_shuffle_epi8(z, gather_mask);
            // (unaligned) store the lower 8 bytes
            // same effect as _mm_storeu_si64(),
            // but GCC <= 8.2, clang < 8 don't provide it
            _mm_storel_epi64((__m128i*)o, x);
            }
            break;
            case Gather::SHIFT_AND:
            {
            // shift right logically  by (immediate) 4 bits, for each 2 bytes
            __m128i y = _mm_srli_epi16(x, 4);
            // bit-or both vectors
            __m128i z = _mm_or_si128(x, y);
            // move bytes on even positions to the lower half
            // for easier unloading
            // 0x80 inserts a 0, arguments left to right from MSB to LSB
            __m128i gather_mask = _mm_set_epi8(
                    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
                    14, 12, 10, 8, 6, 4, 2, 0);
            x = _mm_shuffle_epi8(z, gather_mask);
            // (unaligned) store the lower 8 bytes
            // same effect as _mm_storeu_si64(),
            // but GCC <= 8.2, clang < 8 don't provide it
            _mm_storel_epi64((__m128i*)o, x);
            }
            break;
#if defined( __BMI2__)
            case Gather::PEXT:
            {
            uint64_t i[2];
            // (unaligned store) all 16 bytes
            _mm_storeu_si128((__m128i*)i, x);
            uint32_t j[2];
            // extract the lowest 4 bits of each byte
            // and store in consecutive bit positions
            j[0] =_pext_u64(i[0], bcast<uint64_t>(0xf));
            j[1] =_pext_u64(i[1], bcast<uint64_t>(0xf));
            memcpy(o, j, sizeof j);
            }
            break;
#endif
        }
    }


    template <
        Convert convert = Convert::DIRECT,
#ifdef __BMI2__
        // Mixing SSE with BMI slows things down
        Gather gather = Gather::LOOP
#else
        Gather gather = Gather::LOOP
#endif
        >
    void encode_ssse3(const char *begin, const char *end, u8 *o)
    {
        size_t n = end-begin;
        const char *mid = begin + n/16*16;
        for (const char *i = begin; i != mid; i+=16) {
            encode_ssse3_word<convert, gather>(i, o);
            o += 8;
        }
        encode_lookup(mid, end, o);
    }
#endif // __SSSE3__


    template <
#if defined(__SSSE3__)
        Type type = Type::SIMD,
#else
        Type type = Type::LOOKUP,
#endif
        Convert convert = Convert::DIRECT,
#if defined( __BMI2__)
        Gather gather = Gather::PEXT,
#else
        Gather gather = Gather::LOOP,
#endif
#if __SIZEOF_SIZE_T__ == 4
        typename T = uint32_t
#else
        typename T = uint64_t
#endif
        > void encode(const char *begin, const char *end, u8 *o)
        {
            switch (type) {
                case Type::BYTEWISE:
                    encode_bytewise<convert>(begin, end, o);
                    break;
                case Type::LOOKUP:
                    encode_lookup(begin, end, o);
                    break;
                case Type::SIMD:
#ifdef __SSSE3__
                    // mixing PEXT with SSE slows things down
                    encode_ssse3<convert, Gather::LOOP>(begin, end, o);
                    break;
#else
                    // fall through
#endif
                case Type::SWAR:
                    encode_swar<convert, gather, T>(begin, end, o);
                    break;
            }
        }


      } // encode
    } // impl
  } // bcd
} // xfsx

#endif
