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

#include <xfsx/tlc_reader.hh>
#include <xfsx/tlc_writer.hh>
#include <xfsx/xfsx.hh>
#include <xfsx/scratchpad.hh>
#include <xfsx/byte.hh>
#include <xfsx/detector.hh>

#include <ixxx/util.hh>
#include <ixxx/ixxx.hh>
#include <xxxml/util.hh>
#include <grammar/grammar.hh>
#include <grammar/asn1/grammar.hh>

#if (defined(__MINGW32__) || defined(__MINGW64__))
  #include <windows.h>
#else
    #include <sys/mman.h>
#endif

using namespace std;

using u8 = xfsx::u8;

namespace bed {

  namespace command {


      template <typename Char>
      xfsx::scratchpad::Simple_Reader<Char> mk_simple_reader(const bed::Arguments &args)
      {
          using namespace xfsx;
          if (args.mmap) {
              // bed::Arguments::canonicalize() protects us from this
              assert(args.in_filename != "-");
              return scratchpad::mk_simple_reader_mapped<Char>(args.in_filename);
          } else {
              if (args.in_filename == "-")
                  return scratchpad::mk_simple_reader<Char>(ixxx::util::FD(0));
              else
                  return scratchpad::mk_simple_reader<Char>(args.in_filename);
          }
      }

      template<typename Char>
      xfsx::scratchpad::Simple_Writer<Char> mk_simple_writer(const bed::Arguments &args)
      {
          using namespace xfsx;
          scratchpad::Simple_Writer<Char> w;
          if (args.mmap_out) {
              assert(args.out_filename != "-");
              assert(args.fsync == false);
              size_t n = 0;
              if (!n) {
                  struct stat st;
                  ixxx::posix::stat(args.in_filename, &st);
                  n = st.st_size;
              }
              w = scratchpad::mk_simple_writer_mapped<Char>(args.out_filename, n);
          } else {
              if (args.out_filename == "-")
                  w = scratchpad::mk_simple_writer<Char>(ixxx::util::FD(1));
              else
                  w = scratchpad::mk_simple_writer<Char>(args.out_filename);
          }
          if (args.fsync)
              w.set_sync(true);
          return w;
      }


    void Write_Identity::execute()
    {
        auto r = mk_simple_reader<xfsx::u8>(args_);
        auto w = mk_simple_writer<xfsx::u8>(args_);
        xfsx::ber::write_identity(r, w);
        w.flush();
        w.sync();
    }

    void Write_Definite::execute()
    {
        // XXX support mmap output
        auto r = mk_simple_reader<u8>(args_);
        auto w = mk_simple_writer<u8>(args_);
        xfsx::ber::write_definite(r, w);
        // already flushed
        w.sync();
    }

    void Write_Indefinite::execute()
    {
        // XXX support mmap output
        auto r = mk_simple_reader<xfsx::u8>(args_);
        auto w = mk_simple_writer<xfsx::u8>(args_);
        xfsx::ber::write_indefinite(r, w);
        w.flush();
        w.sync();
    }


    // XXX eliminate in favour of just Pretty_Write?
    void Write_XML::execute()
    {
      xfsx::xml::Pretty_Writer_Arguments args;
      apply_arguments(args_, args);

      auto r = mk_simple_reader<xfsx::u8>(args_);
      auto w = mk_simple_writer<char>(args_);
      xfsx::xml::pretty_write(r, w, args);
      w.flush();
    }

    void Pretty_Write_XML::execute()
    {
        auto r = mk_simple_reader<xfsx::u8>(args_);
        auto as = args_;

        if (as.asn_filenames.size() == 1 && as.asn_filenames[0] == "-") {
            r.next(256);
            string filename("(stdin)");
            auto dt = xfsx::detector::detect_ber(r.window().first,
                    r.window().second, filename, as.asn_config_filename,
                    as.asn_search_path);
            as.asn_filenames = dt.asn_filenames;
            if (as.pp_filename.empty())
                as.pp_filename = dt.pp_filename;
        }

      xfsx::xml::Pretty_Writer_Arguments args(as.asn_filenames);
      apply_arguments(as, args);

      auto w = mk_simple_writer<char>(as);
      xfsx::xml::pretty_write(r, w, args);
      w.flush();
    }

    void Search_XPath::execute()
    {
      auto in = ixxx::util::mmap_file(args_.in_filename);

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
            auto rank = xxxml::get_prop(*i, "rank");
            o << (*i)->name << '(' << rank.get() << ')';
            ++i;
          }
          for (; i != path.end(); ++i) {
            auto rank = xxxml::get_prop(*i, "rank");
            o << '/' << (*i)->name << '(' << rank.get() << ')';
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
      auto in = ixxx::util::mmap_file(args_.in_filename);

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
      auto r = mk_simple_reader<char>(args_);
      xfsx::BER_Writer_Arguments args;
      deque<string> asn_filenames(args_.asn_filenames);
      if (asn_filenames.size() == 1 && asn_filenames[0] == "-") {
            r.next(2048);
            string filename("(stdin)");
            auto dt = xfsx::detector::detect_xml(r.window().first,
                    r.window().second, filename, args_.asn_config_filename,
                    args_.asn_search_path);
            asn_filenames = dt.asn_filenames;
      }
      xfsx::tap::apply_grammar(asn_filenames, args);

      // XXX support mmap output -> the output file needs to be truncated then
      // note that for windows, the output must be unmapped before the final
      // truncate
      auto w = mk_simple_writer<xfsx::u8>(args_);
      xfsx::xml::write_ber(r, w, args);
    }


  }


}
