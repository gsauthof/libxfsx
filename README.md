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

## XER

The XML writer relies on libgrammar for translation BER tag
numbers to XML element names. Libgrammar can also transform an
ASN.1 specification into an XSD (or RelaxNG) schema that can be
used to validate the XML writer output.

The transformation done by libgrammar is not equivalent to the
[ASN.1 XML encoding rules (XER)][xer].

The reason for this is: the XER have severe disadvantages. When
transforming a BER file to XML using the XER, following cases may
occur:

A new parent element must be introduced, e.g.:

    -- ASN.1 snippet
    DataInterChange ::= CHOICE
    {
        transferBatch TransferBatch,
        notification  Notification,
    }
    -- BER stream: [Tag=1, ...]

    <!-- XER result -->
    <DataInterChange> <!-- not present in BER -->
       <transferBatch>...</transferBatch>
    </DataInterChange>

    <!-- libXFSX/bed XML result -->
    <TransferBatch>...</TransferBatch>



An existing parent element must be removed, e.g.:

    -- ASN.1 snippet
    ... ::= ... SEQUENCE { ..., ... BasicServiceCodeList, ... }
    BasicServiceCodeList ::= [APPLICATION 37] SEQUENCE OF BasicServiceCode
    TeleServiceCode      ::= [APPLICATION 218] HexString (SIZE(2))
    BasicServiceCode     ::= [APPLICATION 426] CHOICE
    {
        teleServiceCode      TeleServiceCode,
        bearerServiceCode    BearerServiceCode,
    }
    -- BER stream: [..., Tag=37, Tag=426, Tag=426, ...]

    <!-- XER result -->
    <basicServiceCodeList><teleServiceCode>...</teleServiceCode></...>
    <!-- => BasicServiceCode (tag=426) is gone -->

    <!-- XFSX XML result -->
    <BasicServiceCodeList>
      <BasicServiceCode>
        <TeleServiceCode>...</TeleServiceCode>
      </BasicServiceCode>
    </BasicServiceCodeList>

A different element name for the same BER tag must be used
because it's locally used differently (used in another type or
named differently), e.g.:

    -- ASN.1 snippet
    ... ::= ... SEQUENCE { ..., ... RecEntityCodeList, ... }
    ... ::= ... SEQUENCE { ..., ... SomeRec, ...}
    RecEntityCode     ::= [APPLICATION 184] Code
    RecEntityCodeList ::= [APPLICATION 185] SEQUENCE OF RecEntityCode
    SomeRec           ::= [APPLICATION 1234] SEQUENCE {
       recEntityCode        RecEntityCode,
       ...
    }
    -- BER stream: [ ..., Tag=185, Tag=184, ..., Tag=1234, Tag=184, ...]

    <!-- XER result -->
    <recEntityCodeList><RecEntityCode>...</RecEntityCode>recEntityCodeList>
    ... <someRec><recEntityCode>...</recEntityCode></someRec>
    <!-- => although it's the same tag in the BER file -->

    <!-- XFSX XML result -->
    <RecEntityCodeList><RecEntityCode>...</RecEntityCode></RecEntityCodeList>
    ... <SomeRec><RecEntityCode>...</RecEntityCodeList></SomeRec>

The same element name does not mean that the BER tags are the
same:

    -- ASN.1 snippet
    ... ::= ... SEQUENCE { ..., ... A, ... }
    Bar ::= [Application 4] OCTET STRING
    Blub::= [Application 5] INTEGER
    A   ::= [Application 1] SEQUENCE { foo Foo, fuu Fuu }
    Foo ::= [Application 2] SEQUENCE { blah Bar }
    Fuu ::= [Application 3] SEQUENCE { blah Blub }
    -- BER stream: [ ..., Tag=1, Tag=2, Tag=4, Tag=3, Tag=5, ...]

    <!-- XER result -->
    <a><foo><blah>...</blah></foo><fuu><blah>...</blah></fuu></a>
    <!-- => in the BER file the blah elements have different tags

    <!-- XFSX XML result -->
    <A><Foo><Bar>...</Bar></Foo><Fuu><Blub>...</Blub></Fuu></A>


In general this means: you have to use a stack for BER to XER
transformation.

In contrast the libgrammar ASN.1 to XSD/RelaxNG transformation is
very straight forward: each BER tag is directly mapped to the
ASN.1 type name (using its tag number and class number), i.e. the
mapping is bijective. Thus, no stack is necessary for
transforming BER to XML. A state machine is sufficient. Meaning,
the transformation is more efficient.  Analogously, the
transformation from XML to BER also only needs a state machine.

In addition to that - the resulting XML is also more
comprehensible because one knows that every XML element has a
representation in the BER file, two XML elements with the same
name also have the same BER tag representation and there is no
BER tag that is hidden in the XML file.

## TAP 3.12 Test Scenarios

The GSMA has published some TAP 3.12 test scenarios - they
are described in TD.60 and the specified test call data records are also
available as BER file (TD.62) and XML file (TD.61).

The BER test file can also be converted to XML with `bed write-xml`.
For comparing the result with the TD.61 XML file, the TD.61 XML
file can be converted to the XFSX/bed XML format. For
example via following shell 'one'-liner:

    xmlstarlet sel -t -c '//transferBatch' TD.61\ v30.7.xml \
      | sed 's@\(</\?[a-z]\)@\U\1@g' \
      | sed -e 's@\(</\?\)EquipmentIdentifier>@\1ImeiOrEsn>@' \
            -e 's@\(</\?\)ServiceCode>@\1BasicServiceCode>@' \
            -e 's@\(</\?\)CurrencyConversionInfo>@\1CurrencyConversionList>@'
            -e 's@\(</\?\)RecEntityInfo>@\1RecEntityInfoList>@'
            -e 's@\(</\?\)MessageDescriptionInfo>@\1MessageDescriptionInfoList>@' \
            -e 's@\(</\?\)CallEventDetails>@\1CallEventDetailList>@' \
      | xmlstarlet ed -r '/*/*/Taxation' -v TaxationList
      | xmlstarlet ed -r '/*/*/Discounting' -v DiscountingList \
      | xmlstarlet ed -r '/*/*/UtcTimeOffsetInfo' -v UtcTimeOffsetInfoList \
      | xmlstarlet ed -r '//OperatorSpecInformation/OperatorSpecInformation/parent::*' -v OperatorSpecInfoList \
      | xmlstarlet ed -r '//TaxInformation/TaxInformation/parent::*' -v TaxInformationList \
      | xmlstarlet ed -r '//MobileTerminatedCall/BasicCallInformation' -v MtBasicCallInformation \
      | xmlstarlet ed -r '//MobileOriginatedCall/BasicCallInformation' -v MoBasicCallInformation \
      | xmlstarlet ed -r '//SupplServiceUsed/BasicServiceCodeList/TeleServiceCode' -v 'TeleServiceCodeXXX' \
      | sed 's@<TeleServiceCodeXXX>\([^<]\+\)</TeleServiceCodeXXX>@<BasicServiceCode><TeleServiceCode>\1</TeleServiceCode></BasicServiceCode>@' \
      | XMLLINT_INDENT='    ' xmllint --format --recover - \
      | tail -n +2  > td61_sane.xml

After that transformation there are 2 differences in the values:

- the GSMA prints the trailing fill digit (F) of decoded BCD
  numbers, where XFSX/bed does not
- the GSMA prints CallRereference values as hexadecimal digit
  string, where XFSX/bed prints it as string and escapes characters
  to XML rules

Those differences are also due to the TD.61 XML file being XER
encoded.

Of course, transforming the TD.62 BER file with `bed` to XML and
then back to BER yields the original file:

    bed write-xml TD.61\ v30.7.asn1 td61_bed.xml
    bed write-ber td61_bed.xml t61_bed.ber
    cmp TD.61\ v30.7.asn1 t61_bed.ber
    echo $?    # <- prints 0, i.e. no difference

(note that the TD.62 BER file is also included with the TD.61 zip
archive and has 'asn1' as extension even though it is a BER file;
and not an abstract syntax notation schema)


## Install

When installing `bed` and the library, don't forget to install
the needed XSD/ASN.1 files. See also `libgrammar` for
examples.

## Platforms

Tested on:

- Fedora Linux 23, x86-64
- Solaris 10, SPARC (GCC 4.9)

The low-level components are written in a portable fashion, i.e.
the C++ code is portable between little- and big-endian architectures,
it runs on architecture with strict alignment requirements, etc.

Thus, in general, it should run on platforms where a C++14 compiler is
available.



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
[td60]: http://www.gsma.com/newsroom/wp-content/uploads/TD.60-v30.5.pdf
[xer]: https://en.wikipedia.org/wiki/XML_Encoding_Rules
