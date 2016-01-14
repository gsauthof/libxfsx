C++ library for reading, writing and maninpulating [BER][ber] and
XML encoded files.

It supports the [BER][ber] (basic encoding rules) and
transformations between BER and XML. For example, you can convert
a BER file into XML one or the other way around.

In standard documents, the BER are called a transfer syntax -
i.e. one possible transfer syntax that can be used in combination
with a [ASN.1][asn1] specification.

The repository contains also the command line frontend `bed` that
makes most library features available to the command line, e.g.:

Pretty print a BER file into XML:

    $ bed write-xml CDxyz.ber CDxyz.xml

The other way around:

    $ bed write-ber CDxyz.xml CDxyz.ber

Search using an [XPath][xpath] expression:

    $ bed search -e '/*/CallEventDetailList[1]/*[23]' CDxyz.ber

(prints the 23rd call data record of a [TAP][tap] file)

Add a comment to a TAP file:

    $ bed edit -c add '//AuditControlInfo' \
        'OperatorSpecInfoList/+OperatorSpecInformation' 'Patchdate: 20150101' \
        CDxyz.ber

Syntactically validate a TAP file (plus some constraints checking):

    $ bed validate --xsd tap_3_12_incl_const.xsd CDxyz.ber

2015, Georg Sauthoff <mail@georg.so>


## Examples

More `bed` examples are included in the help screen (`--help`) of
the program.

The unit tests and the `bed` source code can be consulted for
examples on how to use the library.

## Design

Since BER files are not self describing, some operations require
the corresponding ASN.1 specification. The parsing of ASN.1 and
the derivation of the needed information is done in
[libgrammar][libgrammar]. Low-level manipulations of a BER stream
don't require an ASN.1 specification, though.

The xfsx library provides functions for directly constructing a
libxml2 tree from a BER stream. Thus, several features of libxml2
can be leveraged (e.g.  its XPath module, XSD validation, RelaxNG
validation, etc.). The XSD/RelaxNG grammars needed for validation
can be generated via libgrammar.

The format autodetection is configurable via a [JSON][json] run
control file, where different ASN.1 files can be selected via
XPath expressions. The autodetection code is shared between the
BER and XML input mode. It is fast because only as much of the
header is read as necessary.

The low level functions for decoding and encoding values into BER
are template functions, thus one can 'plug-in' custom types into
that mechanism via adding template specializations into the same
namespace.

There is a API for reading BER units into a tagged union, where
its content can be accessed in a type safe fashion. The tagged
union is [automatically generated][variant-gen].

When writing text (e.g. XML) a light-weight buffer management is
used (cf.  `byte.cc`) instead of the STL's iostreams, yielding a
significant speedup (e.g. a speedup of ~ 7 in comparison with the
STL that comes with gcc 4.9).

A stream of BER units can be directly read from a memory mapped
file.

When writing BER units, one can influence low-level details, such
that it is even possible to generate non-conforming files (e.g.
where integers are not encoded with minimal length). Useful for
testing purposes.

## Build Instructions

The repository contains some submodules that have to be
retrieved, as well.

For example:

    $ git clone remote-url
    $ cd project
    $ git submodule update
    $ git submodule init 

Out of source builds are recommended, e.g.:

    $ mkdir build
    $ cmake ..
    $ make bed

Or to use ninja instead of make and create a release build:

    $ mkdir build-o
    $ cmake -G Ninja -D CMAKE_BUILD_TYPE=Release ..
    $ ninja-build bed

## Unittests

compile via:

    $ mkdir build
    $ cmake ..
    $ make ut

run:

    $ ./ut

or just:

    $ make check


## Dependencies

- C++11 compiler that supports some of C++14 (e.g. GCC 4.9)
- Boost >= 1.58
- cmake
- [libgrammar][libgrammar]
- [libixxx][libixxx]
- [libixxxutil][libixxxutil]
- [libxxxml][libxxxml] (C++ wrapper around libxml2)
- [cppformat][cppformat] (for converting integers to strings; its integer conversion function is very efficient)

## Usage Notes

The provided autodection run control file
(`config/detector.json`) has rules for several formats (TAP, RAP,
NRT, etc.). The referenced files are looked up via the ASN1
search path (cf. the `bed` help screen).  A quite complete ASN1
file collection (that can be used with the provided rc file)
would look like this:

    nrt_2_1.asn1
    rap_1_1.asn1
    rap_1_2.asn1
    rap_1_3.asn1
    rap_1_4.asn1
    rap_1_5.asn1
    rap_lt_tap_3_11_tail.asn1
    tap_3_1.asn1
    tap_3_2.asn1
    tap_3_3.asn1
    tap_3_4.asn1
    tap_3_9.asn1
    tap_3_10.asn1
    tap_3_10.xsd
    tap_3_11.asn1
    tap_3_12.asn1
    tap_3_12.xsd

The XSD files are needed for the validation functionality.

## Install

When installing `bed` and the library, don't forget to install
the needed XSD/ASN.1 files. See also `libgrammar` for
examples.

## License

[LGPLv3+][lgpl]


[lgpl]: https://www.gnu.org/licenses/lgpl-3.0.en.html
[ber]: https://en.wikipedia.org/wiki/X.690#BER_encoding
[asn1]: https://en.wikipedia.org/wiki/Abstract_Syntax_Notation_One
[tap]: http://www.gsma.com/newsroom/wp-content/uploads/TD.57-v32.31.pdf
[xpath]: https://en.wikipedia.org/wiki/XPath
[libgrammar]: https://github.com/gsauthof/libgrammar
[libxxxml]: https://github.com/gsauthof/libxxxml
[libixxx]: https://github.com/gsauthof/libixxx
[libixxxutil]: https://github.com/gsauthof/libixxxutil
[variant-gen]: https://github.com/gsauthof/variant-generator
[json]: https://en.wikipedia.org/wiki/JSON
[cppformat]: https://github.com/cppformat/cppformat
