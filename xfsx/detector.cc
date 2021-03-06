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
#include "detector.hh"

#include <stdexcept>

#include <boost/property_tree/json_parser.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include <ixxx/ixxx.hh>
#include <ixxx/util.hh>
#include <xxxml/util.hh>

#include <xfsx/xml_writer_arguments.hh>
#include <xfsx/ber2lxml.hh>
#include <xfsx/xml2lxml.hh>
#include <xfsx_config.hh>
#include <xfsx/character.hh>

#include <xfsx/scratchpad.hh>

using namespace std;

namespace xfsx {

  namespace detector {

    namespace KEY {
      const char DEFINITIONS[]        = "definitions";
      const char INITIAL_GRAMMARS[]   = "initial_grammars";
      const char RESULTING_GRAMMARS[] = "resulting_grammars";
      const char RESULTING_CONSTRAINTS[] = "resulting_constraints";
      const char RESULTING_PP[]       = "resulting_pp";
      const char NAME[]               = "name";
      const char PATH[]               = "path";
      const char LONG_NAME[]          = "long_name";
      const char VARIABLES[]          = "variables";
      const char MAJOR[]              = "major";
      const char MINOR[]              = "minor";
    }

    xxxml::doc::Ptr read_ber_header(
            const u8 *begin, const u8 *end,
        size_t count,
        const std::deque<std::string> &asn_filenames)
    {
      xfsx::xml::Pretty_Writer_Arguments args(asn_filenames);
      args.count = count;
      xxxml::doc::Ptr doc = xfsx::xml::l2::generate_tree(begin, end, args);
      return doc;
    }

    xxxml::doc::Ptr read_ber_header(
        const std::string &filename,
        size_t count,
        const std::deque<std::string> &asn_filenames)
    {
      auto in = ixxx::util::mmap_file(filename);
      return read_ber_header(in.begin(), in.end(), count, asn_filenames);
    }


    xxxml::doc::Ptr read_xml_header(
            const char *begin, const char *end,
        size_t count,
        const std::deque<std::string> &asn_filenames)
    {
        (void)asn_filenames;
      xxxml::doc::Ptr doc = xfsx::xml::l2::generate_tree(begin, end, count);
      return doc;
    }

    xxxml::doc::Ptr read_xml_header(
        const std::string &filename,
        size_t count,
        const std::deque<std::string> &asn_filenames)
    {
      auto in = ixxx::util::mmap_file(filename);
      return read_xml_header(in.s_begin(), in.s_end(), count, asn_filenames);
    }

    static std::deque<std::string> get_list(
        const boost::property_tree::ptree &list)
    {
      deque<string> r;
      for (auto &e : list)
        r.push_back(e.second.get_value<string>());
      return r;
    }

    class Detector {
      private:
        const std::string &filename_;
        const Read_Function &read_fn_;
        const std::deque<std::string> &asn_search_path_;
        boost::property_tree::ptree root_;
        std::map<std::string, std::string> var_map_;
        Result result_;

        void test_definition(const boost::property_tree::ptree &definition);
        void read_variables(
            const xxxml::doc::Ptr &doc,
            const boost::property_tree::ptree &variables);
        void assign_result(const boost::property_tree::ptree &definition);
        std::deque<std::string> map_which(const std::deque<std::string> &
            filenames);
        std::string replace_variables(const std::string &x);
        std::deque<std::string> replace_variables(
            const std::deque<std::string> &l);
      public:
        Detector(
          const std::string &filename,
          const std::string &config_filename,
          const Read_Function &read_fn,
          const std::deque<std::string> &asn_search_path);
        Result test();
    };
    Detector::Detector(
      const std::string &filename,
      const std::string &config_filename,
      const Read_Function &read_fn,
      const std::deque<std::string> &asn_search_path)
      :
        filename_(filename),
        read_fn_(read_fn),
        asn_search_path_(asn_search_path)
    {
        boost::property_tree::json_parser::read_json(config_filename, root_);
    }
    Result Detector::test()
    {
      result_ = Result();
      for (auto &definition : root_.get_child(KEY::DEFINITIONS)) {
        test_definition(definition.second);
        if (!result_.name.empty())
          return result_;
      }
      throw range_error("Could not autodect type of file: " + filename_);
      return result_;
    }
    std::deque<std::string> Detector::map_which(const std::deque<std::string> &
        filenames)
    {
      deque<string> r;
      for (auto &f : filenames)
        r.push_back(ixxx::util::which(asn_search_path_, f));
      return r;
    }
    void Detector::test_definition(
        const boost::property_tree::ptree &definition)
    {
      auto initial_grammars = get_list(definition.get_child(
            KEY::INITIAL_GRAMMARS));
      initial_grammars = map_which(initial_grammars);
      try {
        xxxml::doc::Ptr doc = read_fn_(18, initial_grammars);
        read_variables(doc, definition.get_child(KEY::VARIABLES));
      } catch (const runtime_error &e) {
        return;
      }
      assign_result(definition);
    }
    void Detector::read_variables(
        const xxxml::doc::Ptr &doc,
        const boost::property_tree::ptree &variables)
    {
      map<string, string> var_map;
      try {
        for (auto &variable : variables) {
          string v(
            xxxml::util::xpath::get_string(
              doc, variable.second.get<string>(KEY::PATH)));
          if (v.empty())
            return;
          character::verify_filename_part(v);
          var_map[variable.second.get<string>(KEY::NAME)] = std::move(v);
        }
      } catch (const runtime_error &e) {
        return;
      }
      var_map_ = std::move(var_map);
    }
    void Detector::assign_result(
        const boost::property_tree::ptree &definition)
    {
      if (var_map_.empty())
        return;
      auto resulting_grammars = get_list(
          definition.get_child(KEY::RESULTING_GRAMMARS));
      resulting_grammars = replace_variables(resulting_grammars);
      resulting_grammars = map_which(resulting_grammars);
      result_.asn_filenames = resulting_grammars;

      if (definition.count(KEY::RESULTING_CONSTRAINTS)) {
        auto resulting_constraints = get_list(
            definition.get_child(KEY::RESULTING_CONSTRAINTS));
        resulting_constraints = replace_variables(resulting_constraints);
        resulting_constraints = map_which(resulting_constraints);
        result_.constraint_filenames = resulting_constraints;
      }
      if (definition.count(KEY::RESULTING_PP)) {
        string f(replace_variables(definition.get<string>(KEY::RESULTING_PP)));
        result_.pp_filename = ixxx::util::which(asn_search_path_, f);
      }

      result_.name = definition.get<string>(KEY::NAME);
      result_.long_name = definition.get<string>(KEY::LONG_NAME);
      result_.major = boost::lexical_cast<size_t>(var_map_.at(KEY::MAJOR));
      result_.minor = boost::lexical_cast<size_t>(var_map_.at(KEY::MINOR));
    }

    std::string Detector::replace_variables(const std::string &x)
    {
      string s(x);
      for (auto &v : var_map_) {
        string p;
        p += '{';
        p += v.first;
        p += '}';
        boost::algorithm::replace_all(s, p, v.second);
      }
      return s;
    }

    std::deque<std::string> Detector::replace_variables(
        const std::deque<std::string> &l)
    {
      deque<string> r;
      for (auto &x : l) {
        r.push_back(replace_variables(x));
      }
      return r;
    }

    std::deque<std::string> default_asn_search_path()
    {
        deque<string> r;
        try {
            string v(ixxx::ansi::getenv("ASN1_PATH"));
            boost::algorithm::split(r, v, boost::algorithm::is_any_of(":"));
        } catch (const ixxx::getenv_error &e) {
        }
        try {
            string v(ixxx::ansi::getenv("XDG_CONFIG_HOME"));
            r.push_back(v + "/xfsx/asn1");
        } catch (const ixxx::getenv_error &e) {
            try {
                string v(ixxx::ansi::getenv("HOME"));
                r.push_back(v + "/xfsx/asn1");
            } catch (const ixxx::getenv_error &e) {
            }
        }
#if (defined(__MINGW32__) || defined(__MINGW64__))
#else
        r.push_back("/etc/xfsx/asn1");
#endif
        r.push_back(string(config::prefix()) + "/share/xfsx/asn1");
        return r;
    }

    std::string default_config_filename(const std::deque<std::string> &asn_search_path)
    {
      string r(ixxx::util::which(asn_search_path, "detector.json"));
      return r;
    }

    Result detect(
        const std::string &filename,
        const std::string &config_filename,
        const Read_Function &read_fn,
        const std::deque<std::string> &asn_search_path
        )
    {
        string def_config_filename;
        std::deque<std::string> def_asn_search_path;
        if (def_asn_search_path.empty())
            def_asn_search_path = default_asn_search_path();
        const std::deque<std::string> &sp = asn_search_path.empty() ? def_asn_search_path : asn_search_path;
        if (config_filename.empty())
            def_config_filename = default_config_filename(sp);
        const std::string &cfn = config_filename.empty() ? def_config_filename : config_filename;

      Detector d(filename, cfn, read_fn, sp);
      return d.test();
    }

    Result detect_ber(
            const u8 *begin, const u8 *end,
        const std::string &filename,
        const std::string &config_filename,
        const std::deque<std::string> &asn_search_path
        )
    {
        return detect(filename, config_filename, [begin, end](
                    size_t count,
                    const std::deque<std::string> &asn_filenames) {
                return read_ber_header(begin, end, count, asn_filenames); },
                asn_search_path);
    }

    Result detect_ber(
        const std::string &filename,
        const std::string &config_filename,
        const std::deque<std::string> &asn_search_path
        )
    {
        auto r = scratchpad::mk_simple_reader<u8>(filename);
        r.next(256);
        const u8 *begin = r.window().first;
        const u8 *end = r.window().second;
        return detect_ber(begin, end, filename, config_filename, asn_search_path);
    }

    Result detect_xml(
            const char *begin, const char *end,
        const std::string &filename,
        const std::string &config_filename,
        const std::deque<std::string> &asn_search_path
        )
    {
      return detect(filename, config_filename, [begin, end](
                    size_t count,
                    const std::deque<std::string> &asn_filenames) {
                return read_xml_header(begin, end, count, asn_filenames); },
          asn_search_path);
    }

    Result detect_xml(
        const std::string &filename,
        const std::string &config_filename,
        const std::deque<std::string> &asn_search_path
        )
    {
        auto r = scratchpad::mk_simple_reader<char>(filename);
        r.next(256);
        const char *begin = r.window().first;
        const char *end = r.window().second;
        return detect_xml(begin, end, filename, config_filename, asn_search_path);
    }

  }


}
