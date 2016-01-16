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
#include "ber_commands.hh"

#include <stdexcept>
#include <iostream>
#include <stdio.h>
#include <string.h>

#include <boost/lexical_cast.hpp>

#include <bed/arguments.hh>
#include <xfsx/ber2ber.hh>
#include <xfsx/ber2xml.hh>
#include <xfsx/ber2lxml.hh>
#include <xfsx/lxml2ber.hh>
#include <xfsx/xml2ber.hh>
#include <xfsx/tap.hh>
#include <xfsx/path.hh>
#include <ixxx/util.hh>
#include <ixxx/ixxx.h>
#include <xxxml/util.hh>
#include <grammar/grammar.hh>
#include <grammar/asn1/grammar.hh>

using namespace std;

namespace bed {

  namespace command {


    void Write_Identity::execute()
    {
      ixxx::util::Mapped_File in(args_.in_filename);
      ixxx::util::Mapped_File out(args_.out_filename, false, true, in.size());

      xfsx::ber::write_identity(in.begin(), in.end(), out.begin(), out.end());

      out.close();
    }

    void Write_Definite::execute()
    {
      ixxx::util::Mapped_File in(args_.in_filename);

      xfsx::ber::write_definite(in.begin(), in.end(), args_.out_filename);
    }

    void Write_Indefinite::execute()
    {
      ixxx::util::Mapped_File in(args_.in_filename);

      xfsx::ber::write_indefinite(in.begin(), in.end(), args_.out_filename);
    }

    static void apply_arguments(const Arguments &a,
        xfsx::xml::Writer_Arguments &b)
    {
      b.indent_size      = a.indent_size;
      b.hex_dump         = a.hex_dump;
      b.dump_tl          = a.dump_tl;
      b.dump_t           = a.dump_t;
      b.dump_length      = a.dump_length;
      b.dump_offset      = a.dump_offset;
      b.skip             = a.skip;
      b.stop_after_first = a.stop_after_first;
      b.count            = a.count;

      if (b.search_path.empty()) {
        if (a.skip_to_aci) {
          b.search_path = xfsx::tap::aci_path();
        } else {
          auto x = xfsx::path::parse(a.search_path);
          b.search_path = std::move(x.first);
          b.search_everywhere = x.second;
        }
      }
    }

    static void apply_arguments(const Arguments &a,
        xfsx::xml::Pretty_Writer_Arguments &b)
    {
      if (a.skip_to_aci) {
        b.search_path = xfsx::tap::aci_path(b.translator);
      } else {
        auto x = xfsx::path::parse(a.search_path, b.name_translator);
        b.search_path = std::move(x.first);
        b.search_everywhere = x.second;
      }

      apply_arguments(a, *static_cast<xfsx::xml::Writer_Arguments*>(&b));
    }

    void Write_XML::execute()
    {
      xfsx::xml::Writer_Arguments args;
      apply_arguments(args_, args);
      ixxx::util::Mapped_File in(args_.in_filename);
      if (args_.out_filename.empty()) {
        ixxx::util::FD fd(1);
        xfsx::byte::writer::File w(fd, 4096);
        xfsx::xml::write(in.begin(), in.end(), w, args);
        w.flush();
      } else {
        xfsx::xml::write(in.begin(), in.end(), args_.out_filename, args);
      }
    }

    void Pretty_Write_XML::execute()
    {
      ixxx::util::Mapped_File in(args_.in_filename);

      xfsx::xml::Pretty_Writer_Arguments args(args_.asn_filenames);
      apply_arguments(args_, args);

      if (args_.out_filename.empty()) {
        ixxx::util::FD fd(1);
        xfsx::byte::writer::File w(fd, 4096);
        xfsx::xml::pretty_write(in.begin(), in.end(), w, args);
        w.flush();
      } else {
        xfsx::xml::pretty_write(in.begin(), in.end(), args_.out_filename, args);
      }
    }

    void Search_XPath::execute()
    {
      ixxx::util::Mapped_File in(args_.in_filename);

      FILE *out = nullptr;
      ixxx::util::File out_file;
      if (args_.out_filename.empty())
        out = stdout;
      else {
        out_file = ixxx::util::File(args_.out_filename, "w");
        out = out_file.get();
      }

      xfsx::xml::Pretty_Writer_Arguments args(args_.asn_filenames);
      apply_arguments(args_, args);
      xxxml::doc::Ptr doc = xfsx::xml::l2::generate_tree(in.begin(), in.end(),
          args);
      for (auto &xpath : args_.xpaths) {
        xxxml::xpath::Context_Ptr c = xxxml::xpath::new_context(doc);
        xxxml::xpath::Object_Ptr o = xxxml::xpath::eval(xpath, c);
        switch (o.get()->type) {
          case XPATH_NODESET:
            if (!o.get()->nodesetval)
              throw runtime_error("XPath expression did not match any node");
            for (unsigned i = 0; i < unsigned(o.get()->nodesetval->nodeNr); ++i) {
              xmlNode *node = o.get()->nodesetval->nodeTab[i];
              xxxml::elem_dump(out, doc, node);
            }
            break;
          case XPATH_BOOLEAN:
            fputs((o.get()->boolval ? "true" : "false"), out);
            break;
          case XPATH_NUMBER:
            fprintf(out, "%f", o.get()->floatval);
            break;
          case XPATH_STRING:
            fputs(reinterpret_cast<const char*>(o.get()->stringval), out);
            break;
          default:
            throw runtime_error("Unknown xpath object type");
        }
      }
      fputs("\n", out);
      if (out_file.get()) {
        ixxx::ansi::fflush(out_file);
        int fd = ixxx::posix::fileno(out_file);
        fsync(fd);
        out_file.close();
      }
    }


    class XSD_Validation_Error_Handler {
      private:
        std::ostream &o;
      public:
        XSD_Validation_Error_Handler()
          :
            o(cout)
        {
        }
        XSD_Validation_Error_Handler(std::ostream &o)
          :
            o(o)
        {
        }
        void handle(const xmlError *e)
        {
          deque<const xmlNode*> path;
          const xmlNode *n = static_cast<const xmlNode *>(e->node);
          while (n) {
            if (n->type != XML_ELEMENT_NODE)
              break;
            path.push_front(n);
            n = n->parent;
          }
          auto i = path.begin();
          if (i != path.end()) {
            const char *rank = xxxml::get_prop(*i, "rank");
            o << (*i)->name << '(' << rank << ')';
            ++i;
          }
          for (; i != path.end(); ++i) {
            const char *rank = xxxml::get_prop(*i, "rank");
            o << '/' << (*i)->name << '(' << rank << ')';
          }
          o << " : " << e->message;
          size_t message_size = strlen(e->message);
          if (!message_size || e->message[message_size-1] != '\n')
            o << '\n';
        }
    };

    static void handle_xsd_validation_error(void * userData, xmlErrorPtr error)
    {
      XSD_Validation_Error_Handler *h =
        static_cast<XSD_Validation_Error_Handler*>(
          userData);
      h->handle(error);
    }


    void Validate_XSD::execute()
    {
      ixxx::util::Mapped_File in(args_.in_filename);

      if (args_.xsd_filename.empty())
        throw runtime_error("No XSD filename given");
      xxxml::schema::Parser_Ctxt_Ptr pc = xxxml::schema::new_parser_ctxt(
          args_.xsd_filename);
      xxxml::schema::Ptr schema = xxxml::schema::parse(pc);
      xxxml::schema::Valid_Ctxt_Ptr v = xxxml::schema::new_valid_ctxt(schema);
      XSD_Validation_Error_Handler h;
      xxxml::schema::set_valid_structured_errors(v,
          handle_xsd_validation_error, &h);

      xfsx::xml::Pretty_Writer_Arguments args(args_.asn_filenames);
      args.dump_rank = true;
      apply_arguments(args_, args);
      xxxml::doc::Ptr doc = xfsx::xml::l2::generate_tree(in.begin(), in.end(),
          args);

      xxxml::schema::validate_doc(v, doc);
      cout << "validates\n";
    }

    static void apply_grammar(const std::deque<std::string> &asn_filenames,
        xfsx::BER_Writer_Arguments &args)
    {
      if (asn_filenames.empty())
        return;
      grammar::Grammar g = xfsx::tap::read_asn_grammar(asn_filenames);
      args.translator = xfsx::Name_Translator(
          grammar::map_name_to_shape_klasse_tag(g));
      xfsx::tap::init_dereferencer(g, args.dereferencer);
      xfsx::tap::init_typifier(args.typifier);
    }

    void Write_BER::execute()
    {
      xfsx::BER_Writer_Arguments args;
      apply_grammar(args_.asn_filenames, args);
      ixxx::util::Mapped_File f(args_.in_filename);
      xfsx::xml::write_ber(f.s_begin(), f.s_end(), args_.out_filename, args);
    }

    void Edit::execute()
    {
      ixxx::util::Mapped_File in(args_.in_filename);

      xfsx::xml::Pretty_Writer_Arguments pretty_args(args_.asn_filenames);
      apply_arguments(args_, pretty_args);

      xxxml::doc::Ptr doc = xfsx::xml::l2::generate_tree(in.begin(), in.end(),
          pretty_args);

      for (auto &edit_op : args_.edit_ops) {
        switch (edit_op.command) {
          case Edit_Command::REMOVE:
            xxxml::util::remove(doc, edit_op.argv.at(0));
            break;
          case Edit_Command::REPLACE:
            xxxml::util::replace(doc, edit_op.argv.at(0),
                edit_op.argv.at(1), edit_op.argv.at(2));
            break;
          case Edit_Command::ADD:
            xxxml::util::add(doc, edit_op.argv.at(0),
                edit_op.argv.at(1), edit_op.argv.at(2));
            break;
          case Edit_Command::SET_ATT:
            xxxml::util::set_attribute(doc, edit_op.argv.at(0),
                edit_op.argv.at(1), edit_op.argv.at(2));
            break;
          case Edit_Command::INSERT:
            {
              if (!edit_op.argv.at(1).empty() && edit_op.argv.at(1)[0] == '@') {
                ixxx::util::Mapped_File m(edit_op.argv.at(1).substr(1));
                xxxml::util::insert(doc, edit_op.argv.at(0),
                    m.s_begin(), m.s_end(),
                    boost::lexical_cast<int>(edit_op.argv.at(2)));
              } else {
                xxxml::util::insert(doc, edit_op.argv.at(0),
                    edit_op.argv.at(1).data(),
                    edit_op.argv.at(1).data() + edit_op.argv.at(1).size(),
                    boost::lexical_cast<int>(edit_op.argv.at(2)));
              }
            }
            break;
          default:
            throw logic_error("Edit command not implemented yet");
        }
      }

      xfsx::BER_Writer_Arguments args;
      apply_grammar(args_.asn_filenames, args);

      xfsx::xml::l2::write_ber(doc, args_.out_filename, args);
    }

  }


}
