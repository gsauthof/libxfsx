// Copyright 2015, 2018 Georg Sauthoff <mail@georg.so>

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
#ifndef XFSX_BCD_IMPL_DECODE_HH
#define XFSX_BCD_IMPL_DECODE_HH

/*
 
   This file contains functions for converting big-endian binary-coded decimals
   (BCD, cf. https://en.wikipedia.org/wiki/Binary-coded_decimal) into
   a character string.

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
   each other (also contains some example results from different architectures).

   */

#if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wswitch"
#endif

#include <assert.h>
#include <stdint.h>
#include <limits>

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

namespace xfsx { namespace bcd { namespace impl { namespace decode {

    enum class Type {
        BYTEWISE,
        // use a lookup table with 256 or 16 entries that maps
        // each byte or nibble to 2 or 1 bytes
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
    // note that not all combinations make sense, thus, not all types
    // use all values
    enum class Scatter {
        // SWAR: interleave 4 input bytes with 0s in a loop to distribute the nibbles
        // SIMD: use vector instructions (don't actually loop)
        LOOP,
        // SWAR/SIMD: Bit deposit (i.e. scatter) from a 64 bit source
        // cf. https://en.wikipedia.org/wiki/Bit_Manipulation_Instruction_Sets
        PDEP
    };
    enum class Convert {
        // Bytewise: use if-statements in the main loop
        BRANCH,
        // Bytewise: branchfree by creating a mask using normal/bit arithmetic
        // SIMD: use element-wise arithmetic for masking
        ARITH,
        // Bytewise: branchfree by creating a mask using a comparison operator
        // SIMD: use shuffle as a lookup table
        DIRECT,
        // Type Lookup: use small lookup table with only 16 entries
        SMALL
    };
    enum class Gather {
        // SWAR: shift bytes into the output
        SHIFT,
        // SWAR: memcpy + possibly bswap bytes into the output
        MEMCPY
    };

    template <
        Convert convert = Convert::DIRECT,
        char A = 'a'
        > void decode_bytewise(const u8 *begin, const u8 *end, char *o)
        {
            u8 geq_10_off = u8(A) - u8('0') - 10u; // => 0b10'01'11
            // => geq_10_off == 0b10'01'11 if A == 'a' else 0b1'11
            for (const u8 *i = begin; i != end; ++i) {
                u8 x = *i, hi, lo;
                u8 h = x >> 4;
                u8 l = x & 0xf;

                if (convert == Convert::BRANCH) {
                    if (h > 9)
                        hi = h-10 + u8(A);
                    else
                        hi = h + '0';
                    if (l > 9)
                        lo = l-10 + u8(A);
                    else
                        lo = l + u8('0');
                } else { // branchless
                    u8 h_mask;
                    switch (convert) {
                        case Convert::ARITH: {
                            // 54    = 0b0'11'01'10
                            // 54+9  = 0b0'11'11'11
                            // 54+10 = 0b1'00'00'00
                            u8 h_m = (h+54) & 0b1'00'00'00;
                            h_mask = h_m - (h_m >> 6); // => 0 or 0b11'11'11
                            } break;
                        case Convert::DIRECT:
                            h_mask = -int8_t(h>9); // => 0 or 0b11'11'11'11
                            break;
                    }

                    hi = h + u8('0') + (h_mask & geq_10_off);

                    u8 l_mask;
                    switch (convert) {
                        case Convert::ARITH: {
                            // useful variant for SAR
                            u8 l_m = (l+54) & 0b1'00'00'00;
                            l_mask = l_m - (l_m >> 6);
                            } break;
                        case Convert::DIRECT:
                            l_mask = -int8_t(l>9);
                            break;
                    }

                    lo = l + u8('0') + (l_mask & geq_10_off);
                }

                *o++ = hi;
                *o++ = lo;
            }
        }


// Generated by mk_bcd_lookup.py
// 2018, Georg Sauthoff <mail@gms.tf>
template <
    Convert convert = Convert::DIRECT,
    char A = 'a'>
void decode_lookup(const u8* begin, const u8* end, char* o)
{
    if (convert == Convert::DIRECT) {
#ifndef __BYTE_ORDER__
#error "__BYTE_ORDER__ macro not defined"
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        if (A == 'a') {
            static const uint16_t table[] = {
                /* 0x00 => */ 0x3030,
                /* 0x01 => */ 0x3130,
                /* 0x02 => */ 0x3230,
                /* 0x03 => */ 0x3330,
                /* 0x04 => */ 0x3430,
                /* 0x05 => */ 0x3530,
                /* 0x06 => */ 0x3630,
                /* 0x07 => */ 0x3730,
                /* 0x08 => */ 0x3830,
                /* 0x09 => */ 0x3930,
                /* 0x0a => */ 0x6130,
                /* 0x0b => */ 0x6230,
                /* 0x0c => */ 0x6330,
                /* 0x0d => */ 0x6430,
                /* 0x0e => */ 0x6530,
                /* 0x0f => */ 0x6630,
                /* 0x10 => */ 0x3031,
                /* 0x11 => */ 0x3131,
                /* 0x12 => */ 0x3231,
                /* 0x13 => */ 0x3331,
                /* 0x14 => */ 0x3431,
                /* 0x15 => */ 0x3531,
                /* 0x16 => */ 0x3631,
                /* 0x17 => */ 0x3731,
                /* 0x18 => */ 0x3831,
                /* 0x19 => */ 0x3931,
                /* 0x1a => */ 0x6131,
                /* 0x1b => */ 0x6231,
                /* 0x1c => */ 0x6331,
                /* 0x1d => */ 0x6431,
                /* 0x1e => */ 0x6531,
                /* 0x1f => */ 0x6631,
                /* 0x20 => */ 0x3032,
                /* 0x21 => */ 0x3132,
                /* 0x22 => */ 0x3232,
                /* 0x23 => */ 0x3332,
                /* 0x24 => */ 0x3432,
                /* 0x25 => */ 0x3532,
                /* 0x26 => */ 0x3632,
                /* 0x27 => */ 0x3732,
                /* 0x28 => */ 0x3832,
                /* 0x29 => */ 0x3932,
                /* 0x2a => */ 0x6132,
                /* 0x2b => */ 0x6232,
                /* 0x2c => */ 0x6332,
                /* 0x2d => */ 0x6432,
                /* 0x2e => */ 0x6532,
                /* 0x2f => */ 0x6632,
                /* 0x30 => */ 0x3033,
                /* 0x31 => */ 0x3133,
                /* 0x32 => */ 0x3233,
                /* 0x33 => */ 0x3333,
                /* 0x34 => */ 0x3433,
                /* 0x35 => */ 0x3533,
                /* 0x36 => */ 0x3633,
                /* 0x37 => */ 0x3733,
                /* 0x38 => */ 0x3833,
                /* 0x39 => */ 0x3933,
                /* 0x3a => */ 0x6133,
                /* 0x3b => */ 0x6233,
                /* 0x3c => */ 0x6333,
                /* 0x3d => */ 0x6433,
                /* 0x3e => */ 0x6533,
                /* 0x3f => */ 0x6633,
                /* 0x40 => */ 0x3034,
                /* 0x41 => */ 0x3134,
                /* 0x42 => */ 0x3234,
                /* 0x43 => */ 0x3334,
                /* 0x44 => */ 0x3434,
                /* 0x45 => */ 0x3534,
                /* 0x46 => */ 0x3634,
                /* 0x47 => */ 0x3734,
                /* 0x48 => */ 0x3834,
                /* 0x49 => */ 0x3934,
                /* 0x4a => */ 0x6134,
                /* 0x4b => */ 0x6234,
                /* 0x4c => */ 0x6334,
                /* 0x4d => */ 0x6434,
                /* 0x4e => */ 0x6534,
                /* 0x4f => */ 0x6634,
                /* 0x50 => */ 0x3035,
                /* 0x51 => */ 0x3135,
                /* 0x52 => */ 0x3235,
                /* 0x53 => */ 0x3335,
                /* 0x54 => */ 0x3435,
                /* 0x55 => */ 0x3535,
                /* 0x56 => */ 0x3635,
                /* 0x57 => */ 0x3735,
                /* 0x58 => */ 0x3835,
                /* 0x59 => */ 0x3935,
                /* 0x5a => */ 0x6135,
                /* 0x5b => */ 0x6235,
                /* 0x5c => */ 0x6335,
                /* 0x5d => */ 0x6435,
                /* 0x5e => */ 0x6535,
                /* 0x5f => */ 0x6635,
                /* 0x60 => */ 0x3036,
                /* 0x61 => */ 0x3136,
                /* 0x62 => */ 0x3236,
                /* 0x63 => */ 0x3336,
                /* 0x64 => */ 0x3436,
                /* 0x65 => */ 0x3536,
                /* 0x66 => */ 0x3636,
                /* 0x67 => */ 0x3736,
                /* 0x68 => */ 0x3836,
                /* 0x69 => */ 0x3936,
                /* 0x6a => */ 0x6136,
                /* 0x6b => */ 0x6236,
                /* 0x6c => */ 0x6336,
                /* 0x6d => */ 0x6436,
                /* 0x6e => */ 0x6536,
                /* 0x6f => */ 0x6636,
                /* 0x70 => */ 0x3037,
                /* 0x71 => */ 0x3137,
                /* 0x72 => */ 0x3237,
                /* 0x73 => */ 0x3337,
                /* 0x74 => */ 0x3437,
                /* 0x75 => */ 0x3537,
                /* 0x76 => */ 0x3637,
                /* 0x77 => */ 0x3737,
                /* 0x78 => */ 0x3837,
                /* 0x79 => */ 0x3937,
                /* 0x7a => */ 0x6137,
                /* 0x7b => */ 0x6237,
                /* 0x7c => */ 0x6337,
                /* 0x7d => */ 0x6437,
                /* 0x7e => */ 0x6537,
                /* 0x7f => */ 0x6637,
                /* 0x80 => */ 0x3038,
                /* 0x81 => */ 0x3138,
                /* 0x82 => */ 0x3238,
                /* 0x83 => */ 0x3338,
                /* 0x84 => */ 0x3438,
                /* 0x85 => */ 0x3538,
                /* 0x86 => */ 0x3638,
                /* 0x87 => */ 0x3738,
                /* 0x88 => */ 0x3838,
                /* 0x89 => */ 0x3938,
                /* 0x8a => */ 0x6138,
                /* 0x8b => */ 0x6238,
                /* 0x8c => */ 0x6338,
                /* 0x8d => */ 0x6438,
                /* 0x8e => */ 0x6538,
                /* 0x8f => */ 0x6638,
                /* 0x90 => */ 0x3039,
                /* 0x91 => */ 0x3139,
                /* 0x92 => */ 0x3239,
                /* 0x93 => */ 0x3339,
                /* 0x94 => */ 0x3439,
                /* 0x95 => */ 0x3539,
                /* 0x96 => */ 0x3639,
                /* 0x97 => */ 0x3739,
                /* 0x98 => */ 0x3839,
                /* 0x99 => */ 0x3939,
                /* 0x9a => */ 0x6139,
                /* 0x9b => */ 0x6239,
                /* 0x9c => */ 0x6339,
                /* 0x9d => */ 0x6439,
                /* 0x9e => */ 0x6539,
                /* 0x9f => */ 0x6639,
                /* 0xa0 => */ 0x3061,
                /* 0xa1 => */ 0x3161,
                /* 0xa2 => */ 0x3261,
                /* 0xa3 => */ 0x3361,
                /* 0xa4 => */ 0x3461,
                /* 0xa5 => */ 0x3561,
                /* 0xa6 => */ 0x3661,
                /* 0xa7 => */ 0x3761,
                /* 0xa8 => */ 0x3861,
                /* 0xa9 => */ 0x3961,
                /* 0xaa => */ 0x6161,
                /* 0xab => */ 0x6261,
                /* 0xac => */ 0x6361,
                /* 0xad => */ 0x6461,
                /* 0xae => */ 0x6561,
                /* 0xaf => */ 0x6661,
                /* 0xb0 => */ 0x3062,
                /* 0xb1 => */ 0x3162,
                /* 0xb2 => */ 0x3262,
                /* 0xb3 => */ 0x3362,
                /* 0xb4 => */ 0x3462,
                /* 0xb5 => */ 0x3562,
                /* 0xb6 => */ 0x3662,
                /* 0xb7 => */ 0x3762,
                /* 0xb8 => */ 0x3862,
                /* 0xb9 => */ 0x3962,
                /* 0xba => */ 0x6162,
                /* 0xbb => */ 0x6262,
                /* 0xbc => */ 0x6362,
                /* 0xbd => */ 0x6462,
                /* 0xbe => */ 0x6562,
                /* 0xbf => */ 0x6662,
                /* 0xc0 => */ 0x3063,
                /* 0xc1 => */ 0x3163,
                /* 0xc2 => */ 0x3263,
                /* 0xc3 => */ 0x3363,
                /* 0xc4 => */ 0x3463,
                /* 0xc5 => */ 0x3563,
                /* 0xc6 => */ 0x3663,
                /* 0xc7 => */ 0x3763,
                /* 0xc8 => */ 0x3863,
                /* 0xc9 => */ 0x3963,
                /* 0xca => */ 0x6163,
                /* 0xcb => */ 0x6263,
                /* 0xcc => */ 0x6363,
                /* 0xcd => */ 0x6463,
                /* 0xce => */ 0x6563,
                /* 0xcf => */ 0x6663,
                /* 0xd0 => */ 0x3064,
                /* 0xd1 => */ 0x3164,
                /* 0xd2 => */ 0x3264,
                /* 0xd3 => */ 0x3364,
                /* 0xd4 => */ 0x3464,
                /* 0xd5 => */ 0x3564,
                /* 0xd6 => */ 0x3664,
                /* 0xd7 => */ 0x3764,
                /* 0xd8 => */ 0x3864,
                /* 0xd9 => */ 0x3964,
                /* 0xda => */ 0x6164,
                /* 0xdb => */ 0x6264,
                /* 0xdc => */ 0x6364,
                /* 0xdd => */ 0x6464,
                /* 0xde => */ 0x6564,
                /* 0xdf => */ 0x6664,
                /* 0xe0 => */ 0x3065,
                /* 0xe1 => */ 0x3165,
                /* 0xe2 => */ 0x3265,
                /* 0xe3 => */ 0x3365,
                /* 0xe4 => */ 0x3465,
                /* 0xe5 => */ 0x3565,
                /* 0xe6 => */ 0x3665,
                /* 0xe7 => */ 0x3765,
                /* 0xe8 => */ 0x3865,
                /* 0xe9 => */ 0x3965,
                /* 0xea => */ 0x6165,
                /* 0xeb => */ 0x6265,
                /* 0xec => */ 0x6365,
                /* 0xed => */ 0x6465,
                /* 0xee => */ 0x6565,
                /* 0xef => */ 0x6665,
                /* 0xf0 => */ 0x3066,
                /* 0xf1 => */ 0x3166,
                /* 0xf2 => */ 0x3266,
                /* 0xf3 => */ 0x3366,
                /* 0xf4 => */ 0x3466,
                /* 0xf5 => */ 0x3566,
                /* 0xf6 => */ 0x3666,
                /* 0xf7 => */ 0x3766,
                /* 0xf8 => */ 0x3866,
                /* 0xf9 => */ 0x3966,
                /* 0xfa => */ 0x6166,
                /* 0xfb => */ 0x6266,
                /* 0xfc => */ 0x6366,
                /* 0xfd => */ 0x6466,
                /* 0xfe => */ 0x6566,
                /* 0xff => */ 0x6666
            };
            for (const u8* i = begin; i != end; ++i) {
                uint16_t x = table[*i];
                memcpy(o, &x, sizeof x);
                o += 2;
            }
        } else { // A == 'A'
            static const uint16_t table[] = {
                /* 0x00 => */ 0x3030,
                /* 0x01 => */ 0x3130,
                /* 0x02 => */ 0x3230,
                /* 0x03 => */ 0x3330,
                /* 0x04 => */ 0x3430,
                /* 0x05 => */ 0x3530,
                /* 0x06 => */ 0x3630,
                /* 0x07 => */ 0x3730,
                /* 0x08 => */ 0x3830,
                /* 0x09 => */ 0x3930,
                /* 0x0a => */ 0x4130,
                /* 0x0b => */ 0x4230,
                /* 0x0c => */ 0x4330,
                /* 0x0d => */ 0x4430,
                /* 0x0e => */ 0x4530,
                /* 0x0f => */ 0x4630,
                /* 0x10 => */ 0x3031,
                /* 0x11 => */ 0x3131,
                /* 0x12 => */ 0x3231,
                /* 0x13 => */ 0x3331,
                /* 0x14 => */ 0x3431,
                /* 0x15 => */ 0x3531,
                /* 0x16 => */ 0x3631,
                /* 0x17 => */ 0x3731,
                /* 0x18 => */ 0x3831,
                /* 0x19 => */ 0x3931,
                /* 0x1a => */ 0x4131,
                /* 0x1b => */ 0x4231,
                /* 0x1c => */ 0x4331,
                /* 0x1d => */ 0x4431,
                /* 0x1e => */ 0x4531,
                /* 0x1f => */ 0x4631,
                /* 0x20 => */ 0x3032,
                /* 0x21 => */ 0x3132,
                /* 0x22 => */ 0x3232,
                /* 0x23 => */ 0x3332,
                /* 0x24 => */ 0x3432,
                /* 0x25 => */ 0x3532,
                /* 0x26 => */ 0x3632,
                /* 0x27 => */ 0x3732,
                /* 0x28 => */ 0x3832,
                /* 0x29 => */ 0x3932,
                /* 0x2a => */ 0x4132,
                /* 0x2b => */ 0x4232,
                /* 0x2c => */ 0x4332,
                /* 0x2d => */ 0x4432,
                /* 0x2e => */ 0x4532,
                /* 0x2f => */ 0x4632,
                /* 0x30 => */ 0x3033,
                /* 0x31 => */ 0x3133,
                /* 0x32 => */ 0x3233,
                /* 0x33 => */ 0x3333,
                /* 0x34 => */ 0x3433,
                /* 0x35 => */ 0x3533,
                /* 0x36 => */ 0x3633,
                /* 0x37 => */ 0x3733,
                /* 0x38 => */ 0x3833,
                /* 0x39 => */ 0x3933,
                /* 0x3a => */ 0x4133,
                /* 0x3b => */ 0x4233,
                /* 0x3c => */ 0x4333,
                /* 0x3d => */ 0x4433,
                /* 0x3e => */ 0x4533,
                /* 0x3f => */ 0x4633,
                /* 0x40 => */ 0x3034,
                /* 0x41 => */ 0x3134,
                /* 0x42 => */ 0x3234,
                /* 0x43 => */ 0x3334,
                /* 0x44 => */ 0x3434,
                /* 0x45 => */ 0x3534,
                /* 0x46 => */ 0x3634,
                /* 0x47 => */ 0x3734,
                /* 0x48 => */ 0x3834,
                /* 0x49 => */ 0x3934,
                /* 0x4a => */ 0x4134,
                /* 0x4b => */ 0x4234,
                /* 0x4c => */ 0x4334,
                /* 0x4d => */ 0x4434,
                /* 0x4e => */ 0x4534,
                /* 0x4f => */ 0x4634,
                /* 0x50 => */ 0x3035,
                /* 0x51 => */ 0x3135,
                /* 0x52 => */ 0x3235,
                /* 0x53 => */ 0x3335,
                /* 0x54 => */ 0x3435,
                /* 0x55 => */ 0x3535,
                /* 0x56 => */ 0x3635,
                /* 0x57 => */ 0x3735,
                /* 0x58 => */ 0x3835,
                /* 0x59 => */ 0x3935,
                /* 0x5a => */ 0x4135,
                /* 0x5b => */ 0x4235,
                /* 0x5c => */ 0x4335,
                /* 0x5d => */ 0x4435,
                /* 0x5e => */ 0x4535,
                /* 0x5f => */ 0x4635,
                /* 0x60 => */ 0x3036,
                /* 0x61 => */ 0x3136,
                /* 0x62 => */ 0x3236,
                /* 0x63 => */ 0x3336,
                /* 0x64 => */ 0x3436,
                /* 0x65 => */ 0x3536,
                /* 0x66 => */ 0x3636,
                /* 0x67 => */ 0x3736,
                /* 0x68 => */ 0x3836,
                /* 0x69 => */ 0x3936,
                /* 0x6a => */ 0x4136,
                /* 0x6b => */ 0x4236,
                /* 0x6c => */ 0x4336,
                /* 0x6d => */ 0x4436,
                /* 0x6e => */ 0x4536,
                /* 0x6f => */ 0x4636,
                /* 0x70 => */ 0x3037,
                /* 0x71 => */ 0x3137,
                /* 0x72 => */ 0x3237,
                /* 0x73 => */ 0x3337,
                /* 0x74 => */ 0x3437,
                /* 0x75 => */ 0x3537,
                /* 0x76 => */ 0x3637,
                /* 0x77 => */ 0x3737,
                /* 0x78 => */ 0x3837,
                /* 0x79 => */ 0x3937,
                /* 0x7a => */ 0x4137,
                /* 0x7b => */ 0x4237,
                /* 0x7c => */ 0x4337,
                /* 0x7d => */ 0x4437,
                /* 0x7e => */ 0x4537,
                /* 0x7f => */ 0x4637,
                /* 0x80 => */ 0x3038,
                /* 0x81 => */ 0x3138,
                /* 0x82 => */ 0x3238,
                /* 0x83 => */ 0x3338,
                /* 0x84 => */ 0x3438,
                /* 0x85 => */ 0x3538,
                /* 0x86 => */ 0x3638,
                /* 0x87 => */ 0x3738,
                /* 0x88 => */ 0x3838,
                /* 0x89 => */ 0x3938,
                /* 0x8a => */ 0x4138,
                /* 0x8b => */ 0x4238,
                /* 0x8c => */ 0x4338,
                /* 0x8d => */ 0x4438,
                /* 0x8e => */ 0x4538,
                /* 0x8f => */ 0x4638,
                /* 0x90 => */ 0x3039,
                /* 0x91 => */ 0x3139,
                /* 0x92 => */ 0x3239,
                /* 0x93 => */ 0x3339,
                /* 0x94 => */ 0x3439,
                /* 0x95 => */ 0x3539,
                /* 0x96 => */ 0x3639,
                /* 0x97 => */ 0x3739,
                /* 0x98 => */ 0x3839,
                /* 0x99 => */ 0x3939,
                /* 0x9a => */ 0x4139,
                /* 0x9b => */ 0x4239,
                /* 0x9c => */ 0x4339,
                /* 0x9d => */ 0x4439,
                /* 0x9e => */ 0x4539,
                /* 0x9f => */ 0x4639,
                /* 0xa0 => */ 0x3041,
                /* 0xa1 => */ 0x3141,
                /* 0xa2 => */ 0x3241,
                /* 0xa3 => */ 0x3341,
                /* 0xa4 => */ 0x3441,
                /* 0xa5 => */ 0x3541,
                /* 0xa6 => */ 0x3641,
                /* 0xa7 => */ 0x3741,
                /* 0xa8 => */ 0x3841,
                /* 0xa9 => */ 0x3941,
                /* 0xaa => */ 0x4141,
                /* 0xab => */ 0x4241,
                /* 0xac => */ 0x4341,
                /* 0xad => */ 0x4441,
                /* 0xae => */ 0x4541,
                /* 0xaf => */ 0x4641,
                /* 0xb0 => */ 0x3042,
                /* 0xb1 => */ 0x3142,
                /* 0xb2 => */ 0x3242,
                /* 0xb3 => */ 0x3342,
                /* 0xb4 => */ 0x3442,
                /* 0xb5 => */ 0x3542,
                /* 0xb6 => */ 0x3642,
                /* 0xb7 => */ 0x3742,
                /* 0xb8 => */ 0x3842,
                /* 0xb9 => */ 0x3942,
                /* 0xba => */ 0x4142,
                /* 0xbb => */ 0x4242,
                /* 0xbc => */ 0x4342,
                /* 0xbd => */ 0x4442,
                /* 0xbe => */ 0x4542,
                /* 0xbf => */ 0x4642,
                /* 0xc0 => */ 0x3043,
                /* 0xc1 => */ 0x3143,
                /* 0xc2 => */ 0x3243,
                /* 0xc3 => */ 0x3343,
                /* 0xc4 => */ 0x3443,
                /* 0xc5 => */ 0x3543,
                /* 0xc6 => */ 0x3643,
                /* 0xc7 => */ 0x3743,
                /* 0xc8 => */ 0x3843,
                /* 0xc9 => */ 0x3943,
                /* 0xca => */ 0x4143,
                /* 0xcb => */ 0x4243,
                /* 0xcc => */ 0x4343,
                /* 0xcd => */ 0x4443,
                /* 0xce => */ 0x4543,
                /* 0xcf => */ 0x4643,
                /* 0xd0 => */ 0x3044,
                /* 0xd1 => */ 0x3144,
                /* 0xd2 => */ 0x3244,
                /* 0xd3 => */ 0x3344,
                /* 0xd4 => */ 0x3444,
                /* 0xd5 => */ 0x3544,
                /* 0xd6 => */ 0x3644,
                /* 0xd7 => */ 0x3744,
                /* 0xd8 => */ 0x3844,
                /* 0xd9 => */ 0x3944,
                /* 0xda => */ 0x4144,
                /* 0xdb => */ 0x4244,
                /* 0xdc => */ 0x4344,
                /* 0xdd => */ 0x4444,
                /* 0xde => */ 0x4544,
                /* 0xdf => */ 0x4644,
                /* 0xe0 => */ 0x3045,
                /* 0xe1 => */ 0x3145,
                /* 0xe2 => */ 0x3245,
                /* 0xe3 => */ 0x3345,
                /* 0xe4 => */ 0x3445,
                /* 0xe5 => */ 0x3545,
                /* 0xe6 => */ 0x3645,
                /* 0xe7 => */ 0x3745,
                /* 0xe8 => */ 0x3845,
                /* 0xe9 => */ 0x3945,
                /* 0xea => */ 0x4145,
                /* 0xeb => */ 0x4245,
                /* 0xec => */ 0x4345,
                /* 0xed => */ 0x4445,
                /* 0xee => */ 0x4545,
                /* 0xef => */ 0x4645,
                /* 0xf0 => */ 0x3046,
                /* 0xf1 => */ 0x3146,
                /* 0xf2 => */ 0x3246,
                /* 0xf3 => */ 0x3346,
                /* 0xf4 => */ 0x3446,
                /* 0xf5 => */ 0x3546,
                /* 0xf6 => */ 0x3646,
                /* 0xf7 => */ 0x3746,
                /* 0xf8 => */ 0x3846,
                /* 0xf9 => */ 0x3946,
                /* 0xfa => */ 0x4146,
                /* 0xfb => */ 0x4246,
                /* 0xfc => */ 0x4346,
                /* 0xfd => */ 0x4446,
                /* 0xfe => */ 0x4546,
                /* 0xff => */ 0x4646
            };
            for (const u8* i = begin; i != end; ++i) {
                uint16_t x = table[*i];
                memcpy(o, &x, sizeof x);
                o += 2;
            }
        }
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        if (A == 'a') {
            static const uint16_t table[] = {
                /* 0x00 => */ 0x3030,
                /* 0x01 => */ 0x3031,
                /* 0x02 => */ 0x3032,
                /* 0x03 => */ 0x3033,
                /* 0x04 => */ 0x3034,
                /* 0x05 => */ 0x3035,
                /* 0x06 => */ 0x3036,
                /* 0x07 => */ 0x3037,
                /* 0x08 => */ 0x3038,
                /* 0x09 => */ 0x3039,
                /* 0x0a => */ 0x3061,
                /* 0x0b => */ 0x3062,
                /* 0x0c => */ 0x3063,
                /* 0x0d => */ 0x3064,
                /* 0x0e => */ 0x3065,
                /* 0x0f => */ 0x3066,
                /* 0x10 => */ 0x3130,
                /* 0x11 => */ 0x3131,
                /* 0x12 => */ 0x3132,
                /* 0x13 => */ 0x3133,
                /* 0x14 => */ 0x3134,
                /* 0x15 => */ 0x3135,
                /* 0x16 => */ 0x3136,
                /* 0x17 => */ 0x3137,
                /* 0x18 => */ 0x3138,
                /* 0x19 => */ 0x3139,
                /* 0x1a => */ 0x3161,
                /* 0x1b => */ 0x3162,
                /* 0x1c => */ 0x3163,
                /* 0x1d => */ 0x3164,
                /* 0x1e => */ 0x3165,
                /* 0x1f => */ 0x3166,
                /* 0x20 => */ 0x3230,
                /* 0x21 => */ 0x3231,
                /* 0x22 => */ 0x3232,
                /* 0x23 => */ 0x3233,
                /* 0x24 => */ 0x3234,
                /* 0x25 => */ 0x3235,
                /* 0x26 => */ 0x3236,
                /* 0x27 => */ 0x3237,
                /* 0x28 => */ 0x3238,
                /* 0x29 => */ 0x3239,
                /* 0x2a => */ 0x3261,
                /* 0x2b => */ 0x3262,
                /* 0x2c => */ 0x3263,
                /* 0x2d => */ 0x3264,
                /* 0x2e => */ 0x3265,
                /* 0x2f => */ 0x3266,
                /* 0x30 => */ 0x3330,
                /* 0x31 => */ 0x3331,
                /* 0x32 => */ 0x3332,
                /* 0x33 => */ 0x3333,
                /* 0x34 => */ 0x3334,
                /* 0x35 => */ 0x3335,
                /* 0x36 => */ 0x3336,
                /* 0x37 => */ 0x3337,
                /* 0x38 => */ 0x3338,
                /* 0x39 => */ 0x3339,
                /* 0x3a => */ 0x3361,
                /* 0x3b => */ 0x3362,
                /* 0x3c => */ 0x3363,
                /* 0x3d => */ 0x3364,
                /* 0x3e => */ 0x3365,
                /* 0x3f => */ 0x3366,
                /* 0x40 => */ 0x3430,
                /* 0x41 => */ 0x3431,
                /* 0x42 => */ 0x3432,
                /* 0x43 => */ 0x3433,
                /* 0x44 => */ 0x3434,
                /* 0x45 => */ 0x3435,
                /* 0x46 => */ 0x3436,
                /* 0x47 => */ 0x3437,
                /* 0x48 => */ 0x3438,
                /* 0x49 => */ 0x3439,
                /* 0x4a => */ 0x3461,
                /* 0x4b => */ 0x3462,
                /* 0x4c => */ 0x3463,
                /* 0x4d => */ 0x3464,
                /* 0x4e => */ 0x3465,
                /* 0x4f => */ 0x3466,
                /* 0x50 => */ 0x3530,
                /* 0x51 => */ 0x3531,
                /* 0x52 => */ 0x3532,
                /* 0x53 => */ 0x3533,
                /* 0x54 => */ 0x3534,
                /* 0x55 => */ 0x3535,
                /* 0x56 => */ 0x3536,
                /* 0x57 => */ 0x3537,
                /* 0x58 => */ 0x3538,
                /* 0x59 => */ 0x3539,
                /* 0x5a => */ 0x3561,
                /* 0x5b => */ 0x3562,
                /* 0x5c => */ 0x3563,
                /* 0x5d => */ 0x3564,
                /* 0x5e => */ 0x3565,
                /* 0x5f => */ 0x3566,
                /* 0x60 => */ 0x3630,
                /* 0x61 => */ 0x3631,
                /* 0x62 => */ 0x3632,
                /* 0x63 => */ 0x3633,
                /* 0x64 => */ 0x3634,
                /* 0x65 => */ 0x3635,
                /* 0x66 => */ 0x3636,
                /* 0x67 => */ 0x3637,
                /* 0x68 => */ 0x3638,
                /* 0x69 => */ 0x3639,
                /* 0x6a => */ 0x3661,
                /* 0x6b => */ 0x3662,
                /* 0x6c => */ 0x3663,
                /* 0x6d => */ 0x3664,
                /* 0x6e => */ 0x3665,
                /* 0x6f => */ 0x3666,
                /* 0x70 => */ 0x3730,
                /* 0x71 => */ 0x3731,
                /* 0x72 => */ 0x3732,
                /* 0x73 => */ 0x3733,
                /* 0x74 => */ 0x3734,
                /* 0x75 => */ 0x3735,
                /* 0x76 => */ 0x3736,
                /* 0x77 => */ 0x3737,
                /* 0x78 => */ 0x3738,
                /* 0x79 => */ 0x3739,
                /* 0x7a => */ 0x3761,
                /* 0x7b => */ 0x3762,
                /* 0x7c => */ 0x3763,
                /* 0x7d => */ 0x3764,
                /* 0x7e => */ 0x3765,
                /* 0x7f => */ 0x3766,
                /* 0x80 => */ 0x3830,
                /* 0x81 => */ 0x3831,
                /* 0x82 => */ 0x3832,
                /* 0x83 => */ 0x3833,
                /* 0x84 => */ 0x3834,
                /* 0x85 => */ 0x3835,
                /* 0x86 => */ 0x3836,
                /* 0x87 => */ 0x3837,
                /* 0x88 => */ 0x3838,
                /* 0x89 => */ 0x3839,
                /* 0x8a => */ 0x3861,
                /* 0x8b => */ 0x3862,
                /* 0x8c => */ 0x3863,
                /* 0x8d => */ 0x3864,
                /* 0x8e => */ 0x3865,
                /* 0x8f => */ 0x3866,
                /* 0x90 => */ 0x3930,
                /* 0x91 => */ 0x3931,
                /* 0x92 => */ 0x3932,
                /* 0x93 => */ 0x3933,
                /* 0x94 => */ 0x3934,
                /* 0x95 => */ 0x3935,
                /* 0x96 => */ 0x3936,
                /* 0x97 => */ 0x3937,
                /* 0x98 => */ 0x3938,
                /* 0x99 => */ 0x3939,
                /* 0x9a => */ 0x3961,
                /* 0x9b => */ 0x3962,
                /* 0x9c => */ 0x3963,
                /* 0x9d => */ 0x3964,
                /* 0x9e => */ 0x3965,
                /* 0x9f => */ 0x3966,
                /* 0xa0 => */ 0x6130,
                /* 0xa1 => */ 0x6131,
                /* 0xa2 => */ 0x6132,
                /* 0xa3 => */ 0x6133,
                /* 0xa4 => */ 0x6134,
                /* 0xa5 => */ 0x6135,
                /* 0xa6 => */ 0x6136,
                /* 0xa7 => */ 0x6137,
                /* 0xa8 => */ 0x6138,
                /* 0xa9 => */ 0x6139,
                /* 0xaa => */ 0x6161,
                /* 0xab => */ 0x6162,
                /* 0xac => */ 0x6163,
                /* 0xad => */ 0x6164,
                /* 0xae => */ 0x6165,
                /* 0xaf => */ 0x6166,
                /* 0xb0 => */ 0x6230,
                /* 0xb1 => */ 0x6231,
                /* 0xb2 => */ 0x6232,
                /* 0xb3 => */ 0x6233,
                /* 0xb4 => */ 0x6234,
                /* 0xb5 => */ 0x6235,
                /* 0xb6 => */ 0x6236,
                /* 0xb7 => */ 0x6237,
                /* 0xb8 => */ 0x6238,
                /* 0xb9 => */ 0x6239,
                /* 0xba => */ 0x6261,
                /* 0xbb => */ 0x6262,
                /* 0xbc => */ 0x6263,
                /* 0xbd => */ 0x6264,
                /* 0xbe => */ 0x6265,
                /* 0xbf => */ 0x6266,
                /* 0xc0 => */ 0x6330,
                /* 0xc1 => */ 0x6331,
                /* 0xc2 => */ 0x6332,
                /* 0xc3 => */ 0x6333,
                /* 0xc4 => */ 0x6334,
                /* 0xc5 => */ 0x6335,
                /* 0xc6 => */ 0x6336,
                /* 0xc7 => */ 0x6337,
                /* 0xc8 => */ 0x6338,
                /* 0xc9 => */ 0x6339,
                /* 0xca => */ 0x6361,
                /* 0xcb => */ 0x6362,
                /* 0xcc => */ 0x6363,
                /* 0xcd => */ 0x6364,
                /* 0xce => */ 0x6365,
                /* 0xcf => */ 0x6366,
                /* 0xd0 => */ 0x6430,
                /* 0xd1 => */ 0x6431,
                /* 0xd2 => */ 0x6432,
                /* 0xd3 => */ 0x6433,
                /* 0xd4 => */ 0x6434,
                /* 0xd5 => */ 0x6435,
                /* 0xd6 => */ 0x6436,
                /* 0xd7 => */ 0x6437,
                /* 0xd8 => */ 0x6438,
                /* 0xd9 => */ 0x6439,
                /* 0xda => */ 0x6461,
                /* 0xdb => */ 0x6462,
                /* 0xdc => */ 0x6463,
                /* 0xdd => */ 0x6464,
                /* 0xde => */ 0x6465,
                /* 0xdf => */ 0x6466,
                /* 0xe0 => */ 0x6530,
                /* 0xe1 => */ 0x6531,
                /* 0xe2 => */ 0x6532,
                /* 0xe3 => */ 0x6533,
                /* 0xe4 => */ 0x6534,
                /* 0xe5 => */ 0x6535,
                /* 0xe6 => */ 0x6536,
                /* 0xe7 => */ 0x6537,
                /* 0xe8 => */ 0x6538,
                /* 0xe9 => */ 0x6539,
                /* 0xea => */ 0x6561,
                /* 0xeb => */ 0x6562,
                /* 0xec => */ 0x6563,
                /* 0xed => */ 0x6564,
                /* 0xee => */ 0x6565,
                /* 0xef => */ 0x6566,
                /* 0xf0 => */ 0x6630,
                /* 0xf1 => */ 0x6631,
                /* 0xf2 => */ 0x6632,
                /* 0xf3 => */ 0x6633,
                /* 0xf4 => */ 0x6634,
                /* 0xf5 => */ 0x6635,
                /* 0xf6 => */ 0x6636,
                /* 0xf7 => */ 0x6637,
                /* 0xf8 => */ 0x6638,
                /* 0xf9 => */ 0x6639,
                /* 0xfa => */ 0x6661,
                /* 0xfb => */ 0x6662,
                /* 0xfc => */ 0x6663,
                /* 0xfd => */ 0x6664,
                /* 0xfe => */ 0x6665,
                /* 0xff => */ 0x6666
            };
            for (const u8* i = begin; i != end; ++i) {
                uint16_t x = table[*i];
                memcpy(o, &x, sizeof x);
                o += 2;
            }
        } else { // A == 'A'
            static const uint16_t table[] = {
                /* 0x00 => */ 0x3030,
                /* 0x01 => */ 0x3031,
                /* 0x02 => */ 0x3032,
                /* 0x03 => */ 0x3033,
                /* 0x04 => */ 0x3034,
                /* 0x05 => */ 0x3035,
                /* 0x06 => */ 0x3036,
                /* 0x07 => */ 0x3037,
                /* 0x08 => */ 0x3038,
                /* 0x09 => */ 0x3039,
                /* 0x0a => */ 0x3041,
                /* 0x0b => */ 0x3042,
                /* 0x0c => */ 0x3043,
                /* 0x0d => */ 0x3044,
                /* 0x0e => */ 0x3045,
                /* 0x0f => */ 0x3046,
                /* 0x10 => */ 0x3130,
                /* 0x11 => */ 0x3131,
                /* 0x12 => */ 0x3132,
                /* 0x13 => */ 0x3133,
                /* 0x14 => */ 0x3134,
                /* 0x15 => */ 0x3135,
                /* 0x16 => */ 0x3136,
                /* 0x17 => */ 0x3137,
                /* 0x18 => */ 0x3138,
                /* 0x19 => */ 0x3139,
                /* 0x1a => */ 0x3141,
                /* 0x1b => */ 0x3142,
                /* 0x1c => */ 0x3143,
                /* 0x1d => */ 0x3144,
                /* 0x1e => */ 0x3145,
                /* 0x1f => */ 0x3146,
                /* 0x20 => */ 0x3230,
                /* 0x21 => */ 0x3231,
                /* 0x22 => */ 0x3232,
                /* 0x23 => */ 0x3233,
                /* 0x24 => */ 0x3234,
                /* 0x25 => */ 0x3235,
                /* 0x26 => */ 0x3236,
                /* 0x27 => */ 0x3237,
                /* 0x28 => */ 0x3238,
                /* 0x29 => */ 0x3239,
                /* 0x2a => */ 0x3241,
                /* 0x2b => */ 0x3242,
                /* 0x2c => */ 0x3243,
                /* 0x2d => */ 0x3244,
                /* 0x2e => */ 0x3245,
                /* 0x2f => */ 0x3246,
                /* 0x30 => */ 0x3330,
                /* 0x31 => */ 0x3331,
                /* 0x32 => */ 0x3332,
                /* 0x33 => */ 0x3333,
                /* 0x34 => */ 0x3334,
                /* 0x35 => */ 0x3335,
                /* 0x36 => */ 0x3336,
                /* 0x37 => */ 0x3337,
                /* 0x38 => */ 0x3338,
                /* 0x39 => */ 0x3339,
                /* 0x3a => */ 0x3341,
                /* 0x3b => */ 0x3342,
                /* 0x3c => */ 0x3343,
                /* 0x3d => */ 0x3344,
                /* 0x3e => */ 0x3345,
                /* 0x3f => */ 0x3346,
                /* 0x40 => */ 0x3430,
                /* 0x41 => */ 0x3431,
                /* 0x42 => */ 0x3432,
                /* 0x43 => */ 0x3433,
                /* 0x44 => */ 0x3434,
                /* 0x45 => */ 0x3435,
                /* 0x46 => */ 0x3436,
                /* 0x47 => */ 0x3437,
                /* 0x48 => */ 0x3438,
                /* 0x49 => */ 0x3439,
                /* 0x4a => */ 0x3441,
                /* 0x4b => */ 0x3442,
                /* 0x4c => */ 0x3443,
                /* 0x4d => */ 0x3444,
                /* 0x4e => */ 0x3445,
                /* 0x4f => */ 0x3446,
                /* 0x50 => */ 0x3530,
                /* 0x51 => */ 0x3531,
                /* 0x52 => */ 0x3532,
                /* 0x53 => */ 0x3533,
                /* 0x54 => */ 0x3534,
                /* 0x55 => */ 0x3535,
                /* 0x56 => */ 0x3536,
                /* 0x57 => */ 0x3537,
                /* 0x58 => */ 0x3538,
                /* 0x59 => */ 0x3539,
                /* 0x5a => */ 0x3541,
                /* 0x5b => */ 0x3542,
                /* 0x5c => */ 0x3543,
                /* 0x5d => */ 0x3544,
                /* 0x5e => */ 0x3545,
                /* 0x5f => */ 0x3546,
                /* 0x60 => */ 0x3630,
                /* 0x61 => */ 0x3631,
                /* 0x62 => */ 0x3632,
                /* 0x63 => */ 0x3633,
                /* 0x64 => */ 0x3634,
                /* 0x65 => */ 0x3635,
                /* 0x66 => */ 0x3636,
                /* 0x67 => */ 0x3637,
                /* 0x68 => */ 0x3638,
                /* 0x69 => */ 0x3639,
                /* 0x6a => */ 0x3641,
                /* 0x6b => */ 0x3642,
                /* 0x6c => */ 0x3643,
                /* 0x6d => */ 0x3644,
                /* 0x6e => */ 0x3645,
                /* 0x6f => */ 0x3646,
                /* 0x70 => */ 0x3730,
                /* 0x71 => */ 0x3731,
                /* 0x72 => */ 0x3732,
                /* 0x73 => */ 0x3733,
                /* 0x74 => */ 0x3734,
                /* 0x75 => */ 0x3735,
                /* 0x76 => */ 0x3736,
                /* 0x77 => */ 0x3737,
                /* 0x78 => */ 0x3738,
                /* 0x79 => */ 0x3739,
                /* 0x7a => */ 0x3741,
                /* 0x7b => */ 0x3742,
                /* 0x7c => */ 0x3743,
                /* 0x7d => */ 0x3744,
                /* 0x7e => */ 0x3745,
                /* 0x7f => */ 0x3746,
                /* 0x80 => */ 0x3830,
                /* 0x81 => */ 0x3831,
                /* 0x82 => */ 0x3832,
                /* 0x83 => */ 0x3833,
                /* 0x84 => */ 0x3834,
                /* 0x85 => */ 0x3835,
                /* 0x86 => */ 0x3836,
                /* 0x87 => */ 0x3837,
                /* 0x88 => */ 0x3838,
                /* 0x89 => */ 0x3839,
                /* 0x8a => */ 0x3841,
                /* 0x8b => */ 0x3842,
                /* 0x8c => */ 0x3843,
                /* 0x8d => */ 0x3844,
                /* 0x8e => */ 0x3845,
                /* 0x8f => */ 0x3846,
                /* 0x90 => */ 0x3930,
                /* 0x91 => */ 0x3931,
                /* 0x92 => */ 0x3932,
                /* 0x93 => */ 0x3933,
                /* 0x94 => */ 0x3934,
                /* 0x95 => */ 0x3935,
                /* 0x96 => */ 0x3936,
                /* 0x97 => */ 0x3937,
                /* 0x98 => */ 0x3938,
                /* 0x99 => */ 0x3939,
                /* 0x9a => */ 0x3941,
                /* 0x9b => */ 0x3942,
                /* 0x9c => */ 0x3943,
                /* 0x9d => */ 0x3944,
                /* 0x9e => */ 0x3945,
                /* 0x9f => */ 0x3946,
                /* 0xa0 => */ 0x4130,
                /* 0xa1 => */ 0x4131,
                /* 0xa2 => */ 0x4132,
                /* 0xa3 => */ 0x4133,
                /* 0xa4 => */ 0x4134,
                /* 0xa5 => */ 0x4135,
                /* 0xa6 => */ 0x4136,
                /* 0xa7 => */ 0x4137,
                /* 0xa8 => */ 0x4138,
                /* 0xa9 => */ 0x4139,
                /* 0xaa => */ 0x4141,
                /* 0xab => */ 0x4142,
                /* 0xac => */ 0x4143,
                /* 0xad => */ 0x4144,
                /* 0xae => */ 0x4145,
                /* 0xaf => */ 0x4146,
                /* 0xb0 => */ 0x4230,
                /* 0xb1 => */ 0x4231,
                /* 0xb2 => */ 0x4232,
                /* 0xb3 => */ 0x4233,
                /* 0xb4 => */ 0x4234,
                /* 0xb5 => */ 0x4235,
                /* 0xb6 => */ 0x4236,
                /* 0xb7 => */ 0x4237,
                /* 0xb8 => */ 0x4238,
                /* 0xb9 => */ 0x4239,
                /* 0xba => */ 0x4241,
                /* 0xbb => */ 0x4242,
                /* 0xbc => */ 0x4243,
                /* 0xbd => */ 0x4244,
                /* 0xbe => */ 0x4245,
                /* 0xbf => */ 0x4246,
                /* 0xc0 => */ 0x4330,
                /* 0xc1 => */ 0x4331,
                /* 0xc2 => */ 0x4332,
                /* 0xc3 => */ 0x4333,
                /* 0xc4 => */ 0x4334,
                /* 0xc5 => */ 0x4335,
                /* 0xc6 => */ 0x4336,
                /* 0xc7 => */ 0x4337,
                /* 0xc8 => */ 0x4338,
                /* 0xc9 => */ 0x4339,
                /* 0xca => */ 0x4341,
                /* 0xcb => */ 0x4342,
                /* 0xcc => */ 0x4343,
                /* 0xcd => */ 0x4344,
                /* 0xce => */ 0x4345,
                /* 0xcf => */ 0x4346,
                /* 0xd0 => */ 0x4430,
                /* 0xd1 => */ 0x4431,
                /* 0xd2 => */ 0x4432,
                /* 0xd3 => */ 0x4433,
                /* 0xd4 => */ 0x4434,
                /* 0xd5 => */ 0x4435,
                /* 0xd6 => */ 0x4436,
                /* 0xd7 => */ 0x4437,
                /* 0xd8 => */ 0x4438,
                /* 0xd9 => */ 0x4439,
                /* 0xda => */ 0x4441,
                /* 0xdb => */ 0x4442,
                /* 0xdc => */ 0x4443,
                /* 0xdd => */ 0x4444,
                /* 0xde => */ 0x4445,
                /* 0xdf => */ 0x4446,
                /* 0xe0 => */ 0x4530,
                /* 0xe1 => */ 0x4531,
                /* 0xe2 => */ 0x4532,
                /* 0xe3 => */ 0x4533,
                /* 0xe4 => */ 0x4534,
                /* 0xe5 => */ 0x4535,
                /* 0xe6 => */ 0x4536,
                /* 0xe7 => */ 0x4537,
                /* 0xe8 => */ 0x4538,
                /* 0xe9 => */ 0x4539,
                /* 0xea => */ 0x4541,
                /* 0xeb => */ 0x4542,
                /* 0xec => */ 0x4543,
                /* 0xed => */ 0x4544,
                /* 0xee => */ 0x4545,
                /* 0xef => */ 0x4546,
                /* 0xf0 => */ 0x4630,
                /* 0xf1 => */ 0x4631,
                /* 0xf2 => */ 0x4632,
                /* 0xf3 => */ 0x4633,
                /* 0xf4 => */ 0x4634,
                /* 0xf5 => */ 0x4635,
                /* 0xf6 => */ 0x4636,
                /* 0xf7 => */ 0x4637,
                /* 0xf8 => */ 0x4638,
                /* 0xf9 => */ 0x4639,
                /* 0xfa => */ 0x4641,
                /* 0xfb => */ 0x4642,
                /* 0xfc => */ 0x4643,
                /* 0xfd => */ 0x4644,
                /* 0xfe => */ 0x4645,
                /* 0xff => */ 0x4646
            };
            for (const u8* i = begin; i != end; ++i) {
                uint16_t x = table[*i];
                memcpy(o, &x, sizeof x);
                o += 2;
            }
        }
#else
#error "unknown byte order"
#endif
    } else { // Convert::SMALL
        if (A == 'a') {
            static const char table[] = {
                /* 0x0 => */ '0',
                /* 0x1 => */ '1',
                /* 0x2 => */ '2',
                /* 0x3 => */ '3',
                /* 0x4 => */ '4',
                /* 0x5 => */ '5',
                /* 0x6 => */ '6',
                /* 0x7 => */ '7',
                /* 0x8 => */ '8',
                /* 0x9 => */ '9',
                /* 0xa => */ 'a',
                /* 0xb => */ 'b',
                /* 0xc => */ 'c',
                /* 0xd => */ 'd',
                /* 0xe => */ 'e',
                /* 0xf => */ 'f'
            };
            for (const u8* i = begin; i != end; ++i) {
                u8 x = *i;
                u8 h = x >> 4;
                u8 l = x & 0xf;
                *o++ = table[h];
                *o++ = table[l];
            }
        } else { // A == 'A'
            static const char table[] = {
                /* 0x0 => */ '0',
                /* 0x1 => */ '1',
                /* 0x2 => */ '2',
                /* 0x3 => */ '3',
                /* 0x4 => */ '4',
                /* 0x5 => */ '5',
                /* 0x6 => */ '6',
                /* 0x7 => */ '7',
                /* 0x8 => */ '8',
                /* 0x9 => */ '9',
                /* 0xa => */ 'A',
                /* 0xb => */ 'B',
                /* 0xc => */ 'C',
                /* 0xd => */ 'D',
                /* 0xe => */ 'E',
                /* 0xf => */ 'F'
            };
            for (const u8* i = begin; i != end; ++i) {
                u8 x = *i;
                u8 h = x >> 4;
                u8 l = x & 0xf;
                *o++ = table[h];
                *o++ = table[l];
            }
        }
    }
}


    /*
     
       Decoding some BCDs in parallel using standard (bit-)arithmetic
       operations can be split into 3 phases:

       1. Scatter the BCD half-bytes (nibbles) into bytes.
       2. Convert the byte values from the integer interval [0x00..0x0f] to
          the ASCII interval ['0'..'9''a'..'f'].
       3. Gather the ASCII values into the output,
          i.e. copy the word into the output while possibly adjusting the byte order

       Scatter in ASCII art:

       BCD input (8 digits, most significant digit first, 4 bytes, 32 bits):
       
       +--------+--------+--------+--------+
       |aaaabbbb|ccccdddd|eeeeffff|gggghhhh|
       +--------+--------+--------+--------+

       Byte output (most significant bit first, 8 bytes, 64 bits):

       +--------+--------+--------+--------+--------+--------+--------+--------+
       |    aaaa|    bbbb|    cccc|    dddd|    eeee|    ffff|    gggg|    hhhh|
       +--------+--------+--------+--------+--------+--------+--------+--------+

       This can be archieved via a specialized bit scatter instruction
       (e.g. the Intel bit deposit instruction available on recent x86 CPUs) or
       in multiple steps like this:

       Byte-distribute the input into a 8 byte word and interleave with zero bytes:

       +--------+--------+--------+--------+--------+--------+--------+--------+
       |        |aaaabbbb|        |ccccdddd|        |eeeeffff|        |gggghhhh|
       +--------+--------+--------+--------+--------+--------+--------+--------+

       Copy the result, and left-shift one copy by 4 bits:

       +--------+--------+--------+--------+--------+--------+--------+--------+
       |    aaaa|bbbb    |    cccc|dddd    |    eeee|ffff    |    gggg|hhhh    |
       +--------+--------+--------+--------+--------+--------+--------+--------+

       Mask out in both copies the highest 4 bits in each byte with this mask:

       +--------+--------+--------+--------+--------+--------+--------+--------+
       |    1111|    1111|    1111|    1111|    1111|    1111|    1111|    1111|
       +--------+--------+--------+--------+--------+--------+--------+--------+

       Finally, bit-or both values.

       The challenging part about the conversion is that the ASCII intervals
       ['0'..'9'] and ['a'..'f'] aren't directly adjacent to each other.
       Thus, one has to eliminate branching via use of arithmetic/bit masking.

     */

    template <
#if defined(__BMI2__)
        Scatter scatter = Scatter::PDEP,
#else
        Scatter scatter = Scatter::LOOP,
#endif
        Gather gather = Gather::MEMCPY,
        char A = 'a',
        typename T = uint64_t
        > void decode_swar_word(const u8 *begin, char *o)
        {
            // Scatter
            T x;
            const u8 *i = begin;
            switch (scatter) {
                #ifdef __BMI2__
                case Scatter::PDEP:
                    {
                    static_assert(scatter != Scatter::PDEP || sizeof(T) == 8,
                            "PDEP scatter is only useful with T=uint64_t");
                    T t = movbe<uint32_t>(i);
                    x = _pdep_u64(t, bcast<uint64_t>(0xf));
                    }
                    break;
                #else
                case Scatter::PDEP:
                    // fall through
                #endif
                case Scatter::LOOP:
                    {
                    T t = i[0];
                    // likely target for unrolling
                    for (uint8_t k = 1; k < sizeof(T)/2; ++k) {
                        t <<= 16;
                        t |= i[k];
                    }
                    T a = (t << 4) & bcast<T>(0xf);
                    T b =  t       & bcast<T>(0xf);
                    x = a | b;
                    }
                    break;
            }

            // Convert
            T geq_10_off = bcast<T>(u8(A) - u8('0') - 10u);
            T m = (x + bcast<T>(54)) & bcast<T>(0b1'00'00'00);
            T mask = m - (m >> 6);
            T y = x + bcast<T>(u8('0')) + (mask & geq_10_off);

            // Gather
            if (gather == Gather::SHIFT) {
                // only GCC >= 8.1 optmizes such a sequence of right shifts
                // into a movbe although GCC >= 7.1 DOES optimize a sequence
                // of left shifts
                // However, GCC <= 8.1 doesn't apply this optimization on
                // an unrolled loop.
                for (uint8_t k = 56; k > 0; k-=8)
                    *o++ = y >> k;
                *o++ = y;
            } else {
                // GCC >= 7.1 optimizes the next two statements into one movbe:
                boost::endian::big_to_native_inplace(y);
                memcpy(o, &y, sizeof y);
            }
        }

    template <
#if defined(__BMI2__)
        Scatter scatter = Scatter::PDEP,
#else
        Scatter scatter = Scatter::LOOP,
#endif
        Convert convert = Convert::DIRECT,
        Gather gather = Gather::MEMCPY,
        char A = 'a',
        typename T = uint64_t
        > void decode_swar(const u8 *begin, const u8 *end, char *o)
        {
            size_t n = end-begin;
            size_t k = n / (sizeof(T)/2);
            const u8 *mid = begin + k * (sizeof(T)/2);
            for (const u8 *i = begin; i != mid; i += sizeof(T)/2) {
                decode_swar_word<scatter, gather, A, T>(i, o);
                o += sizeof(T);
            }
            decode_bytewise<convert, A>(mid, end, o);
        }

#ifdef __SSSE3__

    /*

     BCD decoding using Intel SSE2/SSE3/SSSE3 intrinsics.

     It's pretty much SSE2, except for e.g. _mm_shuffle_epi8 which is SSSE3.

     The Intel SSE instruction set contains several variants for each
     elementar (bit-)arithmetic operation, i.e. the differences
     are due to varying vector element widths and types. Not
     all operations are available in all variants, thus we
     use e.g. _mm_add_epi8 instead of an unsigned 8-bit-wise variant
     because there is none (would be named _mm_add_epu8 then).
     For our purposes this doesn't make a difference because of
     the used value domain.

     One might be tempted to directly adopt the SWAR code to make use of the
     128 bit SSE registers. After all, SWAR stands for SIMD within a register.
     But one quickly realizes that the SSE intrinsics provide much more
     versatility than what is possible with SWAR. For example, there
     is an SSE instruction for efficiently comparing 2 vectors at
     8 bit granualarity and each resulting true component is filled with 0xff.
     Thus, there is no need for elobarately creating a mask by subtracting a
     shifted value at a boundary value.

     Actually, there is even an efficient SHUFFLE instruction which not only
     allows to byte-permutate a vector in any way but also select elements
     more than once. Thus, it can be used to very efficiently implement
     a 16 entry lookup table - indexed by each element of the mask. Exactly
     what we need for decoding BCDs. Thus, there is no need to use the
     compare vector instruction, either.

     ASCII diagrams - one character symbolizes 4 bits,
     registers are written left to right from
     most to least significant byte, i.e. MSB to LSB:

     Efficient Scatter:

     input: [begin..end] = aAbBcCdDeEfFgGhH
     loadl_epi64 into 2 registers
         => a: 00 00 00 00 00 00 00 00 hH gG fF eE dD cC bB aA
     right-shift a by 4 bits, each 16 bits and store result into b:
         => b: 00 00 00 00 00 00 00 00 0h Gg  0f Fe  0d Dc  0b Ba
     interleave low, i.e. _mm_unpacklo_epi8(b, a)
         => hH  0h  gG  Gg  fF  0f  eE  Fe  dD  0d  cC  Dc  bB 0b  aA Ba
     bit-and with mask: 0xf (repeated 16 times)
     convert: ...
     gather: nothing do to, already right order!
     memcpy to out:
         => "aAbBcCdDeEfFgGhH"

     PDEP Scatter (slower on e.g. Skylake):

     input: [begin..end] = aAbBcCdDeEfFgGhH
     memcpy into 2 uint32_t:
         => i[1]: hH gG fF eE  i[0]:  dD cC bB aA  
     pdep both into j:
         => j[1]: 0h 0H 0g 0G   0f 0F 0e 0E  j[0]: 0d 0D 0c 0C   0b 0B 0a 0A
     _mm_lddqu_si128 j into x
         =>  0h 0H 0g 0G   0f 0F 0e 0E   0d 0D 0c 0C   0b 0B 0a 0A
     convert: ...
     gather:
     byte shuffle each pair with:  14 15  12 13  10 11  8 9  6 7  4 5  2 3  0 1
         =>  0H 0h 0G 0g   0F 0f 0E 0e   0D 0d 0C 0c   0B 0b 0A 0a
     memcpy to out:
         => "aAbB" "cCdD" "eEfF" "gGhH"


     */

    template <
        // on Skylake, this is actually slower
//#if defined(__BMI2__)
//        Scatter scatter = Scatter::PDEP,
//#else
        Scatter scatter = Scatter::LOOP,
        Convert convert = Convert::DIRECT,
//#endif
        char A = 'a'
        > void decode_ssse3_word(const u8 *begin, char *o)
        {
            // Scatter
            __m128i lookup_table;
            // _mm_set_epi8() arguments left to right are MSB to LSB
            if (A == 'a') {
                lookup_table = _mm_set_epi8('f', 'e', 'd', 'c', 'b', 'a',
                        '9', '8', '7', '6', '5', '4', '3', '2', '1', '0');
            } else {
                lookup_table = _mm_set_epi8('F', 'E', 'D', 'C', 'B', 'A',
                        '9', '8', '7', '6', '5', '4', '3', '2', '1', '0');
            }

            __m128i x;
            if (scatter == Scatter::LOOP
#if !defined( __BMI2__)
                    || 1
#endif
                    ) {
                // __m128i is declared as may-alias, thus no undef. behaviour
                __m128i a = _mm_loadl_epi64((__m128i const *)begin);
                // right shift (logically) by (immediate) 4 bits, each 16 bits
                __m128i b = _mm_srli_epi16(a, 4);
                // interleave first 8 b and a bytes, LSB of b is the LSB of x
                x = _mm_unpacklo_epi8(b, a);
                // broadcast 0b11'11 to all bytes
                __m128i low4_mask = _mm_set1_epi8(0xf);
                // bit-and complete vectors
                x = _mm_and_si128(x, low4_mask);
            } else { // PDEP
#if defined( __BMI2__)
                uint64_t j[2];
                uint32_t i0;
                // note: the memcpy isn't eliminated if we use an array here
                memcpy(&i0, begin, sizeof i0);
                j[0] = _pdep_u64(i0, bcast<uint64_t>(0xf));
                uint32_t i1;
                memcpy(&i1, begin+4, sizeof i1);
                j[1] = _pdep_u64(i1, bcast<uint64_t>(0xf));
                x = _mm_lddqu_si128((__m128i const*)j);
#endif
            }

            // Convert
            __m128i r;
            if (convert == Convert::DIRECT) {

                // we use x as shuffle mask,
                // i.e. as indices into the lookup table
                r = _mm_shuffle_epi8(lookup_table, x);

            } else { // Convert::ARITH
                // just for comparison, more instructions are obviously slower
                __m128i geq_10_off = _mm_set1_epi8(u8(A) - u8('0') - 10);
                __m128i zero_off   = _mm_set1_epi8(u8('0'));
                __m128i v9         = _mm_set1_epi8(9);

                // byte-wise greater-than-9 compare, set each to 0xff if true
                __m128i mask = _mm_cmpgt_epi8(x, v9);

                // add byte-wise
                __m128i y = _mm_add_epi8(x, zero_off);
                // bit-and the complete vector, i.e. zero-out some offsets
                __m128i z = _mm_and_si128(mask, geq_10_off);
                r = _mm_add_epi8(y, z);
            }

            // Gather
            // For Scatter::LOOP everything already is in the right order
            if (scatter == Scatter::PDEP) {
                // arguments left to right are MSB to LSB
                __m128i gather_mask = _mm_set_epi8(
                        14, 15, 12, 13, 10, 11, 8, 9, 6, 7, 4, 5, 2, 3, 0, 1);
                r = _mm_shuffle_epi8(r, gather_mask);
            }

            // unaliged store
            _mm_storeu_si128((__m128i*)o, r);
        }

    template <
        // on Skylake, this is actually slower
//#if defined(__BMI2__)
//        Scatter scatter = Scatter::PDEP,
//#else
        Scatter scatter = Scatter::LOOP,
//#endif
        Convert convert = Convert::DIRECT,
        char A = 'a',
        typename T = uint64_t
        > void decode_ssse3(const u8 *begin, const u8 *end, char *o)
        {
            size_t n = end-begin;
            size_t k = n / (sizeof(__m128i)/2);
            const u8 *mid = begin + k * (sizeof(__m128i)/2);
            for (const u8 *i = begin; i != mid; i += sizeof(__m128i)/2) {
                decode_ssse3_word<scatter, convert, A>(i, o);
                o += sizeof(__m128i);
            }
            // XXX call decode_swar
            decode_bytewise<convert, A>(mid, end, o);
        }
#endif // __SSSE3__


    template <
#if defined(__SSSE3__)
        Type type = Type::SIMD,
#else
        Type type = Type::SWAR,
#endif
#if defined(__BMI2__)
        Scatter scatter = Scatter::PDEP,
#else
        Scatter scatter = Scatter::LOOP,
#endif
        Convert convert = Convert::DIRECT,
        Gather gather = Gather::MEMCPY,
        char A = 'a',
#if __SIZEOF_SIZE_T__ == 4
        typename T = uint32_t
#else
        typename T = uint64_t
#endif
        > void decode(const u8 *begin, const u8 *end, char *o)
        {
            switch (type) {
                case Type::BYTEWISE:
                    decode_bytewise<convert, A>(begin, end, o);
                    break;
                case Type::LOOKUP:
                    decode_lookup<convert, A>(begin, end, o);
                    break;
                #ifdef __SSSE3__
                case Type::SIMD:
                    // Scatter::PDEP in combination with SSE3 is a bit slower
                    decode_ssse3<Scatter::LOOP, convert, A, T>(begin, end, o);
                    break;
                #else
                case Type::SIMD:
                    // fall through
                #endif
                case Type::SWAR:
                    decode_swar<scatter, convert, gather, A, T>(begin, end, o);
                    break;
            }
        }


      } // decode
    } // impl
  } // bcd
} // xfsx

#if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic pop
#endif

#endif
