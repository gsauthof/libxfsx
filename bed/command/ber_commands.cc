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
#include "arguments.hh"
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

#if (defined(__MINGW32__) || defined(__MINGW64__))
  #include <windows.h>
#endif

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


    void Write_XML::execute()
    {
      xfsx::xml::Writer_Arguments args;
      apply_arguments(args_, args);
      ixxx::util::Mapped_File in(args_.in_filename);
      if (args_.out_filename.empty()) {
        ixxx::util::FD fd(1);
        fd.set_keep_open(true);
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
        fd.set_keep_open(true);
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
        out_file = ixxx::util::File(args_.out_filename, "wb");
        out = out_file.get();
      }

      xfsx::xml::Pretty_Writer_Arguments args(args_.asn_filenames);
      apply_arguments(args_, args);
      xxxml::doc::Ptr doc = xfsx::xml::l2::generate_tree(in.begin(), in.end(),
          args);
      for (auto &xpath : args_.xpaths) {
        xxxml::xpath::Context_Ptr c = xxxml::xpath::new_context(doc);
        xxxml::xpath::Object_Ptr o = xxxml::xpath::eval(xpath, c);
        switch (o->type) {
          case XPATH_NODESET:
            if (!o->nodesetval)
              throw runtime_error("XPath expression did not match any node");
            for (unsigned i = 0; i < unsigned(o->nodesetval->nodeNr); ++i) {
              xmlNode *node = o->nodesetval->nodeTab[i];
              xxxml::elem_dump(out, doc, node);
            }
            break;
          case XPATH_BOOLEAN:
            fputs((o->boolval ? "true" : "false"), out);
            break;
          case XPATH_NUMBER:
            fprintf(out, "%f", o->floatval);
            break;
          case XPATH_STRING:
            fputs(reinterpret_cast<const char*>(o->stringval), out);
            break;
          default:
            throw runtime_error("Unknown xpath object type");
        }
      }
      fputs("\n", out);
      if (out_file.get()) {
        ixxx::ansi::fflush(out_file);
        int fd = ixxx::posix::fileno(out_file);
#if (defined(__MINGW32__) || defined(__MINGW64__))
        HANDLE h = reinterpret_cast<HANDLE>(_get_osfhandle(fd));
        if (h == INVALID_HANDLE_VALUE)
          throw std::runtime_error("could not get handle for syncing");
        int r = FlushFileBuffers(h);
        if (!r)
          throw std::runtime_error("could not flush buffers"); // GetLastError()
#else
        ixxx::posix::fsync(fd);
#endif
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

      FILE *out = nullptr;
      ixxx::util::File out_file;
      if (args_.out_filename.empty())
        out = stdout;
      else {
        out_file = ixxx::util::File(args_.out_filename, "wb");
        out = out_file.get();
      }
      ixxx::ansi::fputs("validates\n", out);
    }

    void Write_BER::execute()
    {
      xfsx::BER_Writer_Arguments args;
      xfsx::tap::apply_grammar(args_.asn_filenames, args);
      ixxx::util::Mapped_File f(args_.in_filename);
      xfsx::xml::write_ber(f.s_begin(), f.s_end(), args_.out_filename, args);
    }


  }


}
