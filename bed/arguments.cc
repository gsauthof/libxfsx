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
static const char help_text[] =
R"( COMMAND [OPTIONS] INPUT [OUTPUT]

Commands:

  write-id      Decode a BER file and encode it without changes.
                Mainly for testing purposes, i.e. to detect errors in the input
                file or in the decoding/encoding machinery.
                This operation has constant memory usage.

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


Files:

  The OUTPUT argument is mandatory for most commands. For the xml command
  it is optional and when it is omitted, the XML is written to stdout.

Arguments:

  General:

    -v              increase the verbosity
    -h,--help       display this text
    --version       version string

  write-xml:

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
    --tl            Include size of the encoded tag-length-length (TL)
    --t_size        Include size of the encoded tag
    --length        Include length of the content
    --off           Include byte offset of the tag
    --skip BYTES    Skip BYTES of input file
    --first         Stop reading at the end of the first element
                    (i.e. trailing garbage is ignored)
    --count N       Write only first N tags

  write-ber:

    -a,--asn FILE   Use ASN.1 grammar to map names in the XML
    --asn-path DIR  see above
    --asn-cfg FILE  see above
    --no-detect     Disable autodetect

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
    --skip BYTES    Skip BYTES of input file
    --first         Stop reading at the end of the first element
                    (i.e. trailing garbage is ignored)


BED stands for BER Editor, where the BER acronym means 'Basic Encoding Rules'.
BER is a binary format (think: XML, but binary; Google Protocol Buffers, but
from the eighties). Usually, the structure of a BER file is defined by
an ASN.1 (Abstract Syntax Notation, think: XSD, but kind of independent of
the encoding format) grammar.

2015, Georg Sauthoff <mail@georg.so>
          
)";

#include <iostream>
#include <map>
#include <stdlib.h>
#include <string.h>

#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

#include <xfsx/config.hh>
#include <xfsx/detector.hh>

namespace bf = boost::filesystem;
using namespace std;

static map<string, bed::Command> command_map = {
  { "write-id"   , bed::Command::WRITE_IDENTITY   },
  { "write-def"  , bed::Command::WRITE_DEFINITE   },
  { "write-indef", bed::Command::WRITE_INDEFINITE },
  { "write-xml"  , bed::Command::WRITE_XML        },
  { "write-ber"  , bed::Command::WRITE_BER        },
  { "search"     , bed::Command::SEARCH_XPATH     },
  { "validate"   , bed::Command::VALIDATE_XSD     },
  { "edit"       , bed::Command::EDIT             } 
};

static map<string, bed::Edit_Command> edit_command_map = {
  { "remove"     , bed::Edit_Command::REMOVE  },
  { "replace"    , bed::Edit_Command::REPLACE },
  { "add"        , bed::Edit_Command::ADD     },
  { "set-att"    , bed::Edit_Command::SET_ATT },
  { "set_att"    , bed::Edit_Command::SET_ATT },
  { "insert"     , bed::Edit_Command::INSERT  } 
};

static map<bed::Edit_Command, unsigned> edit_command_to_argc_map = {
  { bed::Edit_Command::REMOVE,  1u },
  { bed::Edit_Command::REPLACE, 3u },
  { bed::Edit_Command::ADD,     3u },
  { bed::Edit_Command::SET_ATT, 3u },
  { bed::Edit_Command::INSERT,  3u }
};

static void print_version()
{
  cout << "bed " << xfsx::config::date() << '\n';
}

static void print_help(const std::string &argv0)
{
  cout << "call: " << argv0 << help_text << '\n';
  print_version();
}


namespace bed {


  Arguments::Arguments(int argc, char **argv)
  {
    parse(argc, argv);
  }

  Arguments::Arguments()
  {
  }
  void Arguments::parse(int argc, char **argv)
  {
    if (!argc)
      throw std::logic_error("argv has to contain at least the program name");
    argv0 = argv[0];
    bool ignore_switches {false};
    for (int i = 1; i < argc; ++i) {
      if (ignore_switches) {
        positional.emplace_back(argv[i]);
      } else {
        if (!*argv[i]) {
          throw Argument_Error("Empty argument string");
        } else if (!strcmp(argv[i], "--")) {
          ignore_switches = true;
        } else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
          print_help(argv0);
          exit(0);
        } else if (!strcmp(argv[i], "-v")) {
          ++verbosity;
        } else if (!strcmp(argv[i], "--version")) {
          print_version();
          exit(0);
        } else if (!strcmp(argv[i], "--indent")) {
          ++i;
          if (i >= argc)
            throw Argument_Error("indent size argument missing");
          indent_size = boost::lexical_cast<unsigned>(argv[i]);
        } else if (!strcmp(argv[i], "-a") || !strcmp(argv[i], "--asn")) {
          ++i;
          if (i >= argc)
            throw Argument_Error("ASN.1 filename argument missing");
          asn_filenames.push_back(argv[i]);
          if (command == Command::WRITE_XML)
            command = Command::PRETTY_WRITE_XML;
        } else if (!strcmp(argv[i], "--asn-path")) {
          ++i;
          if (i >= argc)
            throw Argument_Error("ASN.1 search path argument missing");
          asn_search_path.push_back(argv[i]);
        } else if (!strcmp(argv[i], "--asn-cfg")) {
          ++i;
          if (i >= argc)
            throw Argument_Error("ASN.1 config argument (JSON file) missing");
          asn_config_filename = argv[i];
        } else if (!strcmp(argv[i], "--no-detect")) {
          autodetect = false;
        } else if (!strcmp(argv[i], "-x") || !strcmp(argv[i], "--hex")) {
          hex_dump = true;
        } else if (!strcmp(argv[i], "--tl")) {
          dump_tl = true;
        } else if (!strcmp(argv[i], "--t_size")) {
          dump_t = true;
        } else if (!strcmp(argv[i], "--length") || !strcmp(argv[i], "--len")) {
          dump_length = true;
        } else if (!strcmp(argv[i], "--off") || !strcmp(argv[i], "--offset")) {
          dump_offset = true;
        } else if (!strcmp(argv[i], "--skip")) {
          ++i;
          if (i >= argc)
            throw Argument_Error("skip argument missing");
          skip = boost::lexical_cast<size_t>(argv[i]);
        } else if (!strcmp(argv[i], "--first")) {
          stop_after_first = true;
        } else if (!strcmp(argv[i], "--count")) {
          ++i;
          if (i >= argc)
            throw Argument_Error("count argument missing");
          count = boost::lexical_cast<size_t>(argv[i]);
        } else if (!strcmp(argv[i], "--xsd")) {
          ++i;
          if (i >= argc)
            throw Argument_Error("XSD filename argument missing");
          xsd_filename = argv[i];
        } else if (!strcmp(argv[i], "-e")) {
          ++i;
          if (i >= argc)
            throw Argument_Error("XPATH argument is missing");
          xpaths.push_back(argv[i]);
        } else if (!strcmp(argv[i], "-c") || !strcmp(argv[i], "--command")) {
          ++i;
          if (i >= argc)
            throw Argument_Error("edit command argument is missing");
          string command_str(argv[i]);
          try {
            Edit_Command c = edit_command_map.at(command_str);
            unsigned x = edit_command_to_argc_map.at(c);
            if (i + int(x) >= argc)
              throw Argument_Error("One or more arguments missing for: "
                  + command_str);
            switch (x) {
              case 0: edit_ops.emplace_back(c); break;
              case 1: edit_ops.emplace_back(c, argv[i+1]); break;
              case 2: edit_ops.emplace_back(c, argv[i+1], argv[i+2]); break;
              case 3: edit_ops.emplace_back(c, argv[i+1], argv[i+2], argv[i+3]);
                      break;
              default:
                throw logic_error("this argument edit op not implemented yet");
            }
            i += x;
          } catch (const std::out_of_range &e) {
              throw Argument_Error("Unknown command: " + command_str);
          }
        } else if (*argv[i] == '-') {
          throw Argument_Error("Unknown argument switch: " + string(argv[i]));
        } else {
          positional.emplace_back(argv[i]);
          if (positional.size() == 1) {
            string command_str(positional.front());
            try {
              command = command_map.at(command_str);
            } catch (const std::out_of_range &e) {
              throw Argument_Error("Unknown command: " + command_str);
            }
            if (command == Command::SEARCH_XPATH) {
              if (i+1 >= argc)
                throw Argument_Error("missing XPATH argument");
              if (argv[i+1][0] != '-') {
                ++i;
                xpaths.push_back(argv[i]);
              }
            }
          }
        }
      }
    }
    if (positional.empty())
      throw Argument_Error("Could not find any positional arguments");
    if (positional.size() > 3)
      throw Argument_Error("Too many positional arguments");
    if (positional.size() < 2)
      throw Argument_Error("No input filename given");
    in_filename = positional.at(1);
    if (positional.size() > 2)
      out_filename = positional.at(2);

    if ( (    command == Command::WRITE_IDENTITY
           || command == Command::WRITE_DEFINITE
           || command == Command::WRITE_INDEFINITE
           || command == Command::WRITE_BER
           || command == Command::EDIT
         )
         && out_filename.empty())
      throw Argument_Error("no output file given");

    if (autodetect && asn_filenames.empty()) {
      if (    command == Command::WRITE_XML
           || command == Command::EDIT
           || command == Command::SEARCH_XPATH
           || command == Command::VALIDATE_XSD) {
        try {
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
        } catch (const range_error &e ) {
        }
      } else if (command == Command::WRITE_BER) {
        try {
          auto r = xfsx::detector::detect_xml(in_filename, asn_config_filename,
              asn_search_path);
          asn_filenames = r.asn_filenames;
        } catch (const range_error &e ) {
        }
      }
    }
  }

  Edit_Op::Edit_Op(Edit_Command c)
    :
      command(c)
  {
  }
  Edit_Op::Edit_Op(Edit_Command c,
        const std::string &a1)
    :
      command(c),
      argv(1)
  {
    argv[0] = a1;
  }
  Edit_Op::Edit_Op(Edit_Command c,
        const std::string &a1, const std::string &a2)
    :
      command(c),
      argv(2)
  {
    argv[0] = a1;
    argv[1] = a2;
  }
  Edit_Op::Edit_Op(Edit_Command c,
        const std::string &a1, const std::string &a2, const std::string &a3)
    :
      command(c),
      argv(3)
  {
    argv[0] = a1;
    argv[1] = a2;
    argv[2] = a3;
  }

}
