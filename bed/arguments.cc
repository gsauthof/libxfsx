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
#include "arguments.hh"
#include "arguments_impl.hh"
static const char help_text[] =
R"( COMMAND [OPTIONS] INPUT [OUTPUT]

Commands:

  write-id      Decode a BER file and encode it without changes.
                Mainly for testing purposes, i.e. to detect errors in the input
                file or in the decoding/encoding machinery.
                This operation has constant memory usage.
                Supports reading from/writing to stdin/stdout (use - as filename).

  write-def     Convert a BER file into one where all constructed tags are
                definite.
                This operation has linear memory usage, since the size of the
                length information depends on the children of a constructed tag.

  write-indef   Convert a BER file into one where all constructed tags are
                indefinite.
                This operation has constant memory usage.

  write-xml     Convert a BER file into XML.
                This operation has constant memory usage.

  write-ber     Convert a XML file into BER.
                This operation has linear memory usage.

  search XPATH  Convert a BER file into an in-memory XML node tree
                and apply an XPath 1.0 expression to it.
                Nodeset-results are printed as XML.

                Example - Print the 23rd call data record:

                    search '/*/CallEventDetailList[1]/*[23]' --asn tap312.asn1

  validate      Syntactically validate a BER file against
                an XSD schema.
                For that, the BER file is loaded into memory.
                A proper XSD file can be generated via the
                `ged` command of `libgrammar`.

                Example:

                    validate --xsd tap312.xsd --asn tap312.asn1 input.ber

  edit          Apply edit operations to a BER file.
                For that, the BER file is loaded into memory.

  compute-aci   Compute the Audit Control Info of a TAP file and print
                the result as XML.

  write-aci     Compute the Audit Control Info of a TAP file and rewrite
                it in the output file. This operation has constant
                memory usage, but otherwise yields the same result
                as `edit -c write-aci`.

  mk-bash-comp  Print Bash command completion
                Activate it via: `. <(bed mk-bash-comp)`
                (or create a file in your bash completion directory)

Files:

  The OUTPUT argument is mandatory for most commands. For the xml command
  it is optional and when it is omitted, the XML is written to stdout.

Arguments:

  General:

    -v              increase the verbosity
    -h,--help       display this text
    --version       version string

  write-id:

    --mmap          Memory-map input file
    --mmap-out      Memory-map output file
    --no-fsync      skip fsync/msync call after the last write

  write-def:

    --mmap          Memory-map input file
    --no-fsync      skip fsync/msync call after the last write

  write-indef:

    --mmap          Memory-map input file
    --no-fsync      skip fsync/msync call after the last write

  write-xml:

    --mmap          Memory-map input file
    --no-fsync      skip fsync/msync call after the last write
    --indent N      Indentation step size (default: 4)
    -a,--asn FILE   Use ASN.1 grammar for pretty printing (can be specified
                    multiple times)
    --asn-path DIR  Search path when auto-detecting the ASN.1 grammar
                    (can be specified multiple times)
                    (default: $ASN1_PATH, $HOME/.config/xfsx/asn1,
                     /etc/xfsx/asn1, PREFIX/share/xfsx/asn1)
    --asn-cfg FILE  JSON file that configures the auto-detection
                    (default: first detector.json in the asn search path)
    --no-detect     Disable autodetect
    -x,--hex        Also hexadecimal dump the content of each primitive tag
    --tag           Include tag number
    --class         Include class number
    --tl            Include size of the encoded tag-length-length (TL)
    --t_size        Include size of the encoded tag
    --length        Include length of the content
    --off           Include byte offset of the tag
    --skip BYTES    Skip BYTES of input file
    -0,--skip0 [BYTES] Skip trailing zero bytes
                    If BYTES are specified it is skipped to the next block boundary.
    --block BYTES   Read the files in blocks of BYTES size (e.g. 2048)
                    That means traling bytes (e.g. 0x00 or 0xff) at each block end are ignored
    --bci           Alias for --count 18, where 18 enough to
                    display the BatchControlInfo, Nrtrde header, RapControlInfo,
                    Notication header etc.
    --search PATH   Skip until tag identified by PATH,
                    e.g. /TransferBatch/AuditControlInfo
                    Omitting first / means: match everywhere
                    A wildcard * matches any tag.
                    Tag numbers instead of names are supported as well.
                    Path can end with a predicate, e.g. /foo/bar[3]
                    returning the 3rd match. Other examples are
                    /foo/bar[1,2,3] or /foo/bar/[1..3,10..]
                    Implies --first
    --aci           alias for --search /TransferBatch/AuditControlInfo
                    or --search /ReturnBatch/RapAuditControlInfo
    --cdr N         alias for --search /TransferBatch/CallEventDetailList/*[N]
                    or --search /ReturnBatch/ReturnDetailList/*[N]
    --first         Stop reading at the end of the first element
                    (i.e. trailing garbage is ignored)
    --count N       Write only first N tags
    --pp            Pretty print content using Lua module
    --pp-file FILE  Lua filename (default: autodetect)

  write-ber:

    -a,--asn FILE   Use ASN.1 grammar to map names in the XML
    --asn-path DIR  see above
    --asn-cfg FILE  see above
    --no-detect     Disable autodetect
    --mmap          Memory-map input file
    --no-fsync      skip fsync/msync call after the last write

  search:

    -a,--asn FILE   ASN.1 for mapping tag names etc.
    --asn-path DIR  see above
    --asn-cfg FILE  see above
    --no-detect     Disable autodetect
    -x,--hex        Also hexadecimal dump the content of each primitive tag
    -e XPATH        Can be used to specify multiple patterns
    --skip BYTES    Skip BYTES of input file
    --first         Stop reading at the end of the first element
                    (i.e. trailing garbage is ignored)
    --count N       Read only the first N tags

  validate:

    --xsd FILE      XSD file for validating (cf. `ged --help`)
                    In case none is given, it is autodetected
                    (using the asn1 search path, cf. --asn-cfg)
    -a,--asn FILE   ASN.1 for mapping tag names etc.
    --asn-path DIR  see above
    --asn-cfg FILE  see above
    --no-detect     Disable autodetect
    --skip BYTES    Skip BYTES of input file
    --first         Stop reading at the end of the first element
                    (i.e. trailing garbage is ignored)

  edit:

    -a,--asn FILE                  ASN.1 for mapping tag names etc.
    --asn-path DIR  see above
    --asn-cfg FILE  see above
    --no-detect     Disable autodetect
    -c,--command NAME XPATH ARGS*  Edit command to apply.
                                   Can be specified multiple times.
        Where command is one of:

          remove XPATH               Remove all elements matching the XPATH
          replace XPATH REGEX SUBST  Replace pattern in all matching elements
                                     The SUBST can include backreferences
          add XPATH SPEC CONTENT     Add nodes under each node of a matching nodeset
                                     Example for SPEC: node1/+node2
                                     meaning that node1 is creating if not present,
                                     where node2 is appended to an existing children
                                     list (if any).
          set-att XPATH NAME VALUE   Add (or replace) a attribute in the nodes
                                     of the XPATH result nodeset.
                                     Useful attributes are:
                                       definite - boolean, interpreted by the BER
                                                  writer
                                       l_size   - set a fixed length for the encoding
                                                  of a length
                                                  (the length does not have to be
                                                   minimally encoded)
                                       uint2int - interpret integer as 32 bit
                                                  unsigned and convert it to
                                                  64 bit signed
          insert XPATH SNIP POS      Insert XML snippet under each node
                                     of the XPATH nodeset, where
                                       SNIP - XML or filename (@FILENAME)
                                       POS  - 1 -> first child, -1 -> last child
                                              2 -> after node,  -2 -> before node
          write-aci                  Compute and rewrite Audit Control Info
                                     (ACI)
    --skip BYTES    Skip BYTES of input file
    --first         Stop reading at the end of the first element
                    (i.e. trailing garbage is ignored)

  compute-aci:

    --indent N      Indentation step size (default: 4)
    -a,--asn FILE   Use ASN.1 grammar for pretty printing (can be specified
                    multiple times)
    --asn-path DIR  Search path when auto-detecting the ASN.1 grammar
                    (can be specified multiple times)
                    (default: $ASN1_PATH, $HOME/.config/xfsx/asn1,
                     /etc/xfsx/asn1, PREFIX/share/xfsx/asn1)
    --asn-cfg FILE  JSON file that configures the auto-detection
                    (default: first detector.json in the asn search path)
    --no-detect     Disable autodetect

  mk-bash-comp:

    -o,--output     Output file (instead of stdout)


BED stands for BER Editor, where the BER acronym means 'Basic Encoding Rules'.
BER is a binary format (think: XML, but binary; Google Protocol Buffers, but
from the eighties). Usually, the structure of a BER file is defined by
an ASN.1 (Abstract Syntax Notation, think: XSD, but kind of independent of
the encoding format) grammar.

2015, Georg Sauthoff <mail@georg.so>

)";

#include <xfsx_config.hh>

#include <iostream>
#include <map>
#include <set>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

#include <xfsx/scratchpad.hh>
#include <xfsx_config.hh>
#include <xfsx/detector.hh>
#include <xfsx/byte.hh>
#include <ixxx/util.hh>

namespace bf = boost::filesystem;
using namespace std;


template <typename R, typename ...T>
struct New_Nth {
  template <typename ...A>
  struct Maker_Base  {
      virtual ~Maker_Base() {}
    virtual std::unique_ptr<R> make(const A&...) const = 0;
  };

  template <typename TT, typename ...A>
  struct Maker : public Maker_Base<A...> {
    std::unique_ptr<R> make(const A&... a) const override
    {
      return std::unique_ptr<R>(new TT(a...));
    }
  };

  template <typename ...A>
    std::unique_ptr<R> make(unsigned i, const A&... a)
  {
    if (i > sizeof...(T))
      throw out_of_range("new_nth: index out of range");
    std::array<std::unique_ptr<Maker_Base<A...> >, sizeof...(T)> r = {
      unique_ptr<Maker_Base<A...> >(new Maker<T, A...>())...  } ;
    return r.at(i)->make(a...);
  }
};


namespace bed {

  namespace command {
    Base::Base(const Arguments &args)
      :
        args_(args)
    {
    }
    Base::~Base() =default;

    namespace edit_op {
        Base::~Base() =default;
    }
  }

  static map<string, Command> command_map = {
    { "write-id"   , Command::WRITE_IDENTITY   },
    { "write-def"  , Command::WRITE_DEFINITE   },
    { "write-indef", Command::WRITE_INDEFINITE },
    { "write-xml"  , Command::WRITE_XML        },
    { "write-ber"  , Command::WRITE_BER        },
    { "search"     , Command::SEARCH_XPATH     },
    { "validate"   , Command::VALIDATE_XSD     },
    { "edit"       , Command::EDIT             },
    { "compute-aci", Command::COMPUTE_ACI      },
    { "write-aci",   Command::WRITE_ACI        },
    { "mk-bash-comp",Command::MK_BASH_COMP     }
  };

  static map<string, Edit_Command> edit_command_map = {
    { "remove"     , Edit_Command::REMOVE    },
    { "replace"    , Edit_Command::REPLACE   },
    { "add"        , Edit_Command::ADD       },
    { "set-att"    , Edit_Command::SET_ATT   },
    { "set_att"    , Edit_Command::SET_ATT   },
    { "insert"     , Edit_Command::INSERT    },
    { "write-aci"  , Edit_Command::WRITE_ACI }
  };

  static map<Edit_Command, unsigned> edit_command_to_argc_map = {
    { Edit_Command::REMOVE,   1u },
    { Edit_Command::REPLACE,  3u },
    { Edit_Command::ADD,      3u },
    { Edit_Command::SET_ATT,  3u },
    { Edit_Command::INSERT,   3u },
    { Edit_Command::WRITE_ACI,0u }
  };

  static map<string, Option> option_map = {
    { "-v"          , Option::VERBOSE      },
    { "--verbose"   , Option::VERBOSE      },
    { "-h"          , Option::HELP         },
    { "--help"      , Option::HELP         },
    { "--version"   , Option::VERSION      },
    { "--indent"    , Option::INDENT       },
    { "-a"          , Option::ASN          },
    { "--asn"       , Option::ASN          },
    { "--asn-path"  , Option::ASN_PATH     },
    { "--asn-cfg"   , Option::ASN_CFG      },
    { "--no-detect" , Option::NO_DETECT    },
    { "-x"          , Option::HEX          },
    { "--hex"       , Option::HEX          },
    { "--tag"       , Option::TAG          },
    { "--class"     , Option::KLASSE       },
    { "--tl"        , Option::TL           },
    { "--t_size"    , Option::T_SIZE       },
    { "--length"    , Option::LENGTH       },
    { "--len"       , Option::LENGTH       },
    { "--off"       , Option::OFFSET       },
    { "--offset"    , Option::OFFSET       },
    { "--skip"      , Option::SKIP         },
    { "--skip0"     , Option::SKIP_ZERO    },
    { "-0"          , Option::SKIP_ZERO    },
    { "--block"     , Option::BLOCK        },
    { "--bci"       , Option::BCI          },
    { "--search"    , Option::SEARCH       },
    { "--aci"       , Option::ACI          },
    { "--cdr"       , Option::CDR          },
    { "--first"     , Option::FIRST        },
    { "--count"     , Option::COUNT        },
    { "-e"          , Option::EXPR         },
    // --expr
    { "--xsd"       , Option::XSD          },
    { "-c"          , Option::COMMAND      },
    { "--command"   , Option::COMMAND      },
    { "-o"          , Option::OUTPUT       },
    { "--output"    , Option::OUTPUT       },
    { "--pp"        , Option::PRETTY_PRINT },
    { "--pp-file"   , Option::PP_FILE      },
    { "--mmap"      , Option::MMAP         },
    { "--mmap-out"  , Option::MMAP_OUT     },
    { "--no-fsync"  , Option::NO_FSYNC     }
  };

  static map<Option, pair<unsigned, unsigned> > option_to_argc_map = {
     { Option::VERBOSE      , { 0, 0 }  },
     { Option::HELP         , { 0, 1 }  },
     { Option::VERSION      , { 0, 0 }  },
     { Option::INDENT       , { 1, 1 }  },
     { Option::ASN          , { 1, 1 }  },
     { Option::ASN_PATH     , { 1, 1 }  },
     { Option::ASN_CFG      , { 1, 1 }  },
     { Option::NO_DETECT    , { 0, 0 }  },
     { Option::HEX          , { 0, 0 }  },
     { Option::TAG          , { 0, 0 }  },
     { Option::KLASSE       , { 0, 0 }  },
     { Option::TL           , { 0, 0 }  },
     { Option::T_SIZE       , { 0, 0 }  },
     { Option::LENGTH       , { 0, 0 }  },
     { Option::OFFSET       , { 0, 0 }  },
     { Option::SKIP         , { 1, 1 }  },
     { Option::SKIP_ZERO    , { 0, 1 }  },
     { Option::BLOCK        , { 1, 1 }  },
     { Option::BCI          , { 0, 0 }  },
     { Option::SEARCH       , { 1, 1 }  },
     { Option::ACI          , { 0, 0 }  },
     { Option::CDR          , { 1, 1 }  },
     { Option::FIRST        , { 0, 0 }  },
     { Option::COUNT        , { 1, 1 }  },
     { Option::EXPR         , { 1, 1 }  },
     { Option::XSD          , { 1, 1 }  },
     { Option::COMMAND      , { 1, 4 }  },
     { Option::OUTPUT       , { 1, 1 }  },
     { Option::PRETTY_PRINT , { 0, 0 }  },
     { Option::PP_FILE      , { 1, 1 }  },
     { Option::MMAP         , { 0, 0 }  },
     { Option::MMAP_OUT     , { 0, 0 }  },
     { Option::NO_FSYNC     , { 0, 0 }  }
  };

  static map<Option, set<Command> > option_comp_map = {
    // Constructing the empty set with { } works with gcc, but not with
    // certain clang/stl combinations
    // (e.g. Apple LLVM 6.0 (based on LLVM 3.5svn))
    // thus we replace it with the equivalent set<Command>()
    // cf. http://cplusplus.github.io/LWG/lwg-defects.html#2193
    // { Option::VERBOSE   ,  { }  },
    { Option::VERBOSE   ,  set<Command>() },
    { Option::HELP      ,  set<Command>() },
    { Option::VERSION   ,  set<Command>() },
    { Option::INDENT    ,  { Command::WRITE_XML, Command::PRETTY_WRITE_XML,
                             Command::COMPUTE_ACI }  },
    { Option::ASN       ,  set<Command>() },
    { Option::ASN_PATH  ,  set<Command>() },
    { Option::ASN_CFG   ,  set<Command>() },
    { Option::NO_DETECT ,  set<Command>() },
    { Option::HEX       ,  { Command::WRITE_XML, Command::PRETTY_WRITE_XML,
                             Command::SEARCH_XPATH }  },
    { Option::TAG       ,  { Command::WRITE_XML, Command::PRETTY_WRITE_XML }  },
    { Option::KLASSE    ,  { Command::WRITE_XML, Command::PRETTY_WRITE_XML }  },
    { Option::TL        ,  { Command::WRITE_XML, Command::PRETTY_WRITE_XML }  },
    { Option::T_SIZE    ,  { Command::WRITE_XML, Command::PRETTY_WRITE_XML }  },
    { Option::LENGTH    ,  { Command::WRITE_XML, Command::PRETTY_WRITE_XML }  },
    { Option::OFFSET    ,  { Command::WRITE_XML, Command::PRETTY_WRITE_XML }  },
    { Option::SKIP      ,  { Command::WRITE_XML, Command::PRETTY_WRITE_XML,
                             Command::SEARCH_XPATH,
                             Command::VALIDATE_XSD, Command::EDIT }  },
    { Option::SKIP_ZERO ,  { Command::WRITE_XML, Command::PRETTY_WRITE_XML }  },
    { Option::BLOCK     ,  { Command::WRITE_XML, Command::PRETTY_WRITE_XML }  },
    { Option::BCI       ,  { Command::WRITE_XML, Command::PRETTY_WRITE_XML }  },
    { Option::SEARCH    ,  { Command::WRITE_XML, Command::PRETTY_WRITE_XML }  },
    { Option::ACI       ,  { Command::WRITE_XML, Command::PRETTY_WRITE_XML }  },
    { Option::CDR       ,  { Command::WRITE_XML, Command::PRETTY_WRITE_XML }  },
    { Option::FIRST     ,  { Command::WRITE_XML, Command::PRETTY_WRITE_XML,
                             Command::SEARCH_XPATH,
                             Command::VALIDATE_XSD, Command::EDIT }  },
    { Option::COUNT     ,  { Command::WRITE_XML, Command::PRETTY_WRITE_XML,
                             Command::SEARCH_XPATH }  },
    { Option::EXPR      ,  { Command::SEARCH_XPATH }  },
    { Option::XSD       ,  { Command::VALIDATE_XSD }  },
    { Option::COMMAND   ,  { Command::EDIT }  },
    { Option::OUTPUT    ,  { Command::MK_BASH_COMP } },
    { Option::PRETTY_PRINT,{ Command::WRITE_XML, Command::PRETTY_WRITE_XML } },
    { Option::PP_FILE   ,  { Command::WRITE_XML, Command::PRETTY_WRITE_XML } },
    { Option::MMAP      ,  { Command::WRITE_IDENTITY, Command::WRITE_INDEFINITE,
                             Command::WRITE_DEFINITE, Command::WRITE_BER,
                             Command::WRITE_XML } },
    { Option::MMAP_OUT  ,  { Command::WRITE_IDENTITY } },
    { Option::NO_FSYNC  ,  { Command::WRITE_IDENTITY, Command::WRITE_INDEFINITE,
                             Command::WRITE_DEFINITE, Command::WRITE_BER,
                             Command::WRITE_XML } }
  };

  static void print_help(const std::string &argv0);
  static void print_version();

  static void apply_verbose(Arguments &a, unsigned, unsigned&,
      unsigned, char **)
  {
    ++a.verbosity;
  }
  static void apply_help(Arguments &, unsigned, unsigned&,
      unsigned, char **argv)
  {
    print_help(*argv);
    exit(0);
  }
  static void apply_version(Arguments &, unsigned, unsigned&,
      unsigned, char ** /*argv*/)
  {
    print_version();
  }
  static void apply_indent(Arguments &a, unsigned i, unsigned&,
      unsigned, char **argv)
  {
    a.indent_size = boost::lexical_cast<unsigned>(argv[i]);
  }
  static void apply_asn(Arguments &a, unsigned i, unsigned&,
      unsigned, char **argv)
  {
    a.asn_filenames.push_back(argv[i]);
    if (a.command == Command::WRITE_XML)
      a.command = Command::PRETTY_WRITE_XML;
  }
  static void apply_asn_path(Arguments &a, unsigned i, unsigned&,
      unsigned, char **argv)
  {
    a.asn_search_path.push_back(argv[i]);
  }
  static void apply_asn_cfg(Arguments &a, unsigned i, unsigned&,
      unsigned, char **argv)
  {
    a.asn_config_filename = argv[i];
  }
  static void apply_no_detect(Arguments &a, unsigned, unsigned&,
      unsigned, char **)
  {
    a.autodetect = false;
  }
  static void apply_hex(Arguments &a, unsigned, unsigned&,
      unsigned, char **)
  {
    a.hex_dump = true;
  }
  static void apply_tag(Arguments &a, unsigned, unsigned&,
      unsigned, char **)
  {
    a.dump_tag = true;
  }
  static void apply_klasse(Arguments &a, unsigned, unsigned&,
      unsigned, char **)
  {
    a.dump_class = true;
  }
  static void apply_tl(Arguments &a, unsigned, unsigned&,
      unsigned, char **)
  {
    a.dump_tl = true;
  }
  static void apply_t_size(Arguments &a, unsigned, unsigned&,
      unsigned, char **)
  {
    a.dump_t = true;
  }
  static void apply_length(Arguments &a, unsigned, unsigned&,
      unsigned, char **)
  {
    a.dump_length = true;
  }
  static void apply_offset(Arguments &a, unsigned, unsigned&,
      unsigned, char **)
  {
    a.dump_offset = true;
  }
  static void apply_skip(Arguments &a, unsigned i, unsigned&,
      unsigned, char **argv)
  {
    a.skip = boost::lexical_cast<size_t>(argv[i]);
  }
  static void apply_skip_zero(Arguments &a, unsigned i, unsigned &j,
      unsigned argc, char **argv)
  {
    if (i < argc && *argv[i] != '-') {
      a.skip_zero = boost::lexical_cast<size_t>(argv[i]);
      ++j;
    } else {
      a.skip_zero = 1;
    }
  }
  static void apply_block(Arguments &a, unsigned i, unsigned&,
      unsigned, char **argv)
  {
    a.block_size = boost::lexical_cast<size_t>(argv[i]);
  }
  static void apply_bci(Arguments &a, unsigned, unsigned&,
      unsigned, char **)
  {
    a.count = 18;
  }
  static void apply_search(Arguments &a, unsigned i, unsigned&,
      unsigned, char **argv)
  {
    a.search_path = argv[i];
    a.stop_after_first = true;
  }
  static void apply_aci(Arguments &a, unsigned, unsigned&,
      unsigned, char **)
  {
    a.skip_to_aci = true;
    a.stop_after_first = true;
  }
  static void apply_cdr(Arguments &a, unsigned i, unsigned&,
      unsigned, char **argv)
  {
    a.kth_cdr = argv[i];
    a.stop_after_first = true;
  }
  static void apply_first(Arguments &a, unsigned, unsigned&,
      unsigned, char **)
  {
    a.stop_after_first = true;
  }
  static void apply_count(Arguments &a, unsigned i, unsigned&,
      unsigned, char **argv)
  {
    a.count = boost::lexical_cast<size_t>(argv[i]);
  }
  static void apply_expr(Arguments &a, unsigned i, unsigned&,
      unsigned, char **argv)
  {
    a.xpaths.push_back(argv[i]);
  }
  static void apply_xsd(Arguments &a, unsigned i, unsigned&,
      unsigned, char **argv)
  {
    a.xsd_filename = argv[i];
  }
  static void apply_command(Arguments &a, unsigned i, unsigned &j,
      unsigned argc, char **argv)
  {
    string command_str(argv[i]);
    try {
      Edit_Command c = edit_command_map.at(command_str);
      unsigned x = edit_command_to_argc_map.at(c);
      auto n = static_cast<unsigned>(c)-1;
      auto op = New_Nth<command::edit_op::Base,
           command::edit_op::Remove,
           command::edit_op::Replace,
           command::edit_op::Add,
           command::edit_op::Set_Att,
           command::edit_op::Insert,
           command::edit_op::Write_ACI>().make(n);
      if (i + int(x) >= argc)
        throw Argument_Error("One or more arguments missing for: "
            + command_str);
      switch (x) {
        case 0: break;
        case 1: op->argv = { argv[i+1] }; break;
        case 2: op->argv = { argv[i+1], argv[i+2] }; break;
        case 3: op->argv = { argv[i+1], argv[i+2], argv[i+3]};
                break;
        default:
                throw logic_error("this argument edit op not implemented yet");
      }
      j = i + x;
      a.edit_ops.push_back(std::move(op));
    } catch (const std::out_of_range &e) {
      throw Argument_Error("Unknown subcommand: " + command_str);
    }
  }
  static void apply_output(Arguments &a, unsigned i, unsigned&,
      unsigned, char **argv)
  {
    a.out_filename = argv[i];
  }

  static void apply_pretty_print(Arguments &a, unsigned, unsigned&,
      unsigned, char ** /*argv*/)
  {
#ifdef XFSX_USE_LUA
    a.pretty_print = true;
#else
    (void)a;
    throw logic_error("not compiled with Lua support");
#endif
  }

  static void apply_pp_file(Arguments &a, unsigned i, unsigned&,
      unsigned, char **argv)
  {
    a.pp_filename = argv[i];
  }
  static void apply_mmap(Arguments &a, unsigned , unsigned&,
      unsigned, char **)
  {
      a.mmap = true;
  }
  static void apply_mmap_out(Arguments &a, unsigned , unsigned&,
      unsigned, char **)
  {
      a.mmap_out = true;
  }
  static void apply_no_fsync(Arguments &a, unsigned , unsigned&,
      unsigned, char **)
  {
      a.fsync = false;
  }

  static map<Option,void (*)(Arguments &a, unsigned i, unsigned &j,
      unsigned argc, char **argv)> option_to_apply_map = {
    { Option::VERBOSE      ,  apply_verbose      },
    { Option::HELP         ,  apply_help         },
    { Option::VERSION      ,  apply_version      },
    { Option::INDENT       ,  apply_indent       },
    { Option::ASN          ,  apply_asn          },
    { Option::ASN_PATH     ,  apply_asn_path     },
    { Option::ASN_CFG      ,  apply_asn_cfg      },
    { Option::NO_DETECT    ,  apply_no_detect    },
    { Option::HEX          ,  apply_hex          },
    { Option::TAG          ,  apply_tag          },
    { Option::KLASSE       ,  apply_klasse       },
    { Option::TL           ,  apply_tl           },
    { Option::T_SIZE       ,  apply_t_size       },
    { Option::LENGTH       ,  apply_length       },
    { Option::OFFSET       ,  apply_offset       },
    { Option::SKIP         ,  apply_skip         },
    { Option::SKIP_ZERO    ,  apply_skip_zero    },
    { Option::BLOCK        ,  apply_block        },
    { Option::BCI          ,  apply_bci          },
    { Option::SEARCH       ,  apply_search       },
    { Option::ACI          ,  apply_aci          },
    { Option::CDR          ,  apply_cdr          },
    { Option::FIRST        ,  apply_first        },
    { Option::COUNT        ,  apply_count        },
    { Option::EXPR         ,  apply_expr         },
    { Option::XSD          ,  apply_xsd          },
    { Option::COMMAND      ,  apply_command      },
    { Option::OUTPUT       ,  apply_output       },
    { Option::PRETTY_PRINT ,  apply_pretty_print },
    { Option::PP_FILE      ,  apply_pp_file      },
    { Option::MMAP         ,  apply_mmap         },
    { Option::MMAP_OUT     ,  apply_mmap_out     },
    { Option::NO_FSYNC     ,  apply_no_fsync     }
  };


  namespace command {

    void Mk_Bash_Comp::execute()
    {
      string name(boost::filesystem::path(args_.argv0).stem().string());

      using namespace xfsx;
      unique_ptr<scratchpad::Writer<char>> y;
      if (args_.out_filename.empty())
          y = unique_ptr<scratchpad::Writer<char>>(
                  new scratchpad::File_Writer<char>(ixxx::util::FD(1)));
      else
          y = unique_ptr<scratchpad::Writer<char>>(
                  new scratchpad::File_Writer<char>(args_.out_filename));
      scratchpad::Simple_Writer<char> x(std::move(y));
      byte::writer::Base w(x);
      w <<
"function _" << name << R"(()
{
  local cmd cur prev sub_cmd
  cmd="$1"
  cur="$2"
  prev="$3"

  if [ $COMP_CWORD -eq 1 ] ; then
    COMPREPLY=($(compgen -W ")";
      for (auto &i : command_map)
        w << i.first << ' ';
      w << R"(" -- "$cur"))
  else
    sub_cmd="${COMP_WORDS[1]}"

    if [[  "$cur" == -* ]]; then
      local opts=""
      case "$sub_cmd" in
)";
      for (auto &i : command_map) {
        w << "        " << i.first << ")\n          opts=\"";
        for (auto &j : option_map) {
          auto &cs = option_comp_map.at(j.second);
          if (cs.empty() || cs.count(i.second))
            w << j.first << ' ';
        }
        w << "\"\n          ;;\n";
      }
      w << R"(        *)
          ;;
      esac
      COMPREPLY=($(compgen -W "$opts" -- "$cur"))
    else
      if [[ "$sub_cmd" == "edit" && "$prev" == "-c" ]] ; then
        COMPREPLY=($(compgen -W ")";
      for (auto &i : edit_command_map)
        w << i.first << ' ';
      w << R"(" -- "$cur"))
      else
        COMPREPLY=($(compgen -f -- "$cur"))
      fi
    fi
  fi
}
complete -F _)" << name << ' ' << name << '\n';
      x.flush();
    }
  }


  static void print_version()
  {
    cout << "bed " << xfsx::config::date() << '\n';
  }

  static void print_help(const std::string &argv0)
  {
    cout << "call: " << argv0 << help_text << '\n';
    print_version();
  }




  Arguments::Arguments(unsigned argc, char **argv)
  {
    parse(argc, argv);
  }

  Arguments::Arguments()
  {
  }
  void Arguments::parse(unsigned argc, char **argv)
  {
    if (!argc)
      throw std::logic_error("argv has to contain at least the program name");
    argv0 = argv[0];
    bool ignore_switches {false};
    for (unsigned i = 1; i < argc; ++i) {
      if (!*argv[i]) {
        throw Argument_Error("Empty argument string");
      } else if (!strcmp(argv[i], "--")) {
        ignore_switches = true;
      } else if (*argv[i] == '-' && argv[i][1] && !ignore_switches) {
        try {
          auto o = option_map.at(argv[i]);
          auto ac = option_to_argc_map.at(o);
          auto &comp = option_comp_map.at(o);
          if (!comp.empty() && !comp.count(command))
            throw Argument_Error("Command " + command_str + " doesn't support"
                " option " + string(argv[i]));
          auto apply = option_to_apply_map.at(o);
          auto j = i + ac.first;
          if (j >= argc)
            throw Argument_Error("missing " + string(argv[i]) + " argument");
          apply(*this, i + 1, j, argc, argv);
          i = j;
        } catch (const std::out_of_range &e) {
          throw Argument_Error("Unknown argument switch: " + string(argv[i]));
        }
      } else {
        positional.emplace_back(argv[i]);
        if (positional.size() == 1) {
          command_str = positional.front();
          try {
            command = command_map.at(command_str);
          } catch (const std::out_of_range &e) {
            throw Argument_Error("Unknown command: " + command_str);
          }
          if (command == Command::SEARCH_XPATH) {
            if (i+1 >= argc)
              throw Argument_Error("missing XPATH argument");
            ++i;
            xpaths.push_back(argv[i]);
          }
        }
      }
    }
    validate();
    canonicalize();
    autodetect_stuff();
    create_cmd();
  }


  void Arguments::validate()
  {
    if (positional.empty())
      throw Argument_Error("Could not find any positional arguments");
    if (positional.size() > 3)
      throw Argument_Error("Too many positional arguments");
    if (positional.size() < 2 && !(command == Command::MK_BASH_COMP))
      throw Argument_Error("No input filename given");
    if (positional.size() > 1)
      in_filename = positional.at(1);
    if (positional.size() > 2)
      out_filename = positional.at(2);

    if ( (    command == Command::WRITE_IDENTITY
           || command == Command::WRITE_DEFINITE
           || command == Command::WRITE_INDEFINITE
           || command == Command::WRITE_BER
           || command == Command::EDIT
           || command == Command::WRITE_ACI
         )
         && out_filename.empty())
      throw Argument_Error("no output file given");
  }

    void Arguments::canonicalize()
    {
        if (mmap && in_filename == "-")
            mmap = false;
        if (mmap_out && out_filename == "-")
            mmap_out = false;
        if (fsync && out_filename == "-")
            fsync = false;
        if ((command == Command::PRETTY_WRITE_XML 
                    || command == Command::WRITE_XML) && out_filename.empty())
            out_filename = "-";
    }


  void Arguments::autodetect_stuff()
  {
      // XXX support auto-detection with stdin
    if (!(autodetect && asn_filenames.empty() && in_filename != "-"))
      return;

    try {
      if (    command == Command::WRITE_XML
           || command == Command::EDIT
           || command == Command::SEARCH_XPATH
           || command == Command::VALIDATE_XSD
           || command == Command::WRITE_ACI
           ) {
        auto r = xfsx::detector::detect_ber(in_filename, asn_config_filename,
            asn_search_path);
        asn_filenames = r.asn_filenames;
        if (command == Command::WRITE_XML && !asn_filenames.empty())
          command = Command::PRETTY_WRITE_XML;
        if (command == Command::VALIDATE_XSD && xsd_filename.empty()
            && !asn_filenames.empty()) {
          xsd_filename = bf::path(asn_filenames.front())
            .replace_extension("xsd").generic_string();
        }
        if (pp_filename.empty())
          pp_filename = r.pp_filename;
      } else if (command == Command::WRITE_BER) {
        auto r = xfsx::detector::detect_xml(in_filename, asn_config_filename,
            asn_search_path);
        asn_filenames = r.asn_filenames;
      }
    } catch (const range_error &e ) {
    }
  }
  unique_ptr<command::Base> Arguments::create_cmd()
  {
    auto n = static_cast<unsigned>(command)-1;
    return New_Nth<command::Base,
                  command::Write_Identity,
                  command::Write_Definite,
                  command::Write_Indefinite,
                  command::Write_XML,
                  command::Pretty_Write_XML,
                  command::Write_BER,
                  command::Search_XPath,
                  command::Validate_XSD,
                  command::Edit,
                  command::Compute_ACI,
                  command::Write_ACI,
                  command::Mk_Bash_Comp
        >().make(n, *this);
  }


}
