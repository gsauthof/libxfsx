#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <algorithm>

#include <ixxx/util.hh>
#include <bed/command/ber_commands.hh>
#include <bed/arguments.hh>

#include <test/test.hh>
#include <test/bed/helper.hh>

using namespace std;
namespace bf = boost::filesystem;

    void run_bed(const vector<string> &argvv)
    {
      vector<char *> argv;
      for (auto &s : argvv)
        argv.push_back(const_cast<char*>(s.c_str()));
      argv.push_back(nullptr);
      bed::Arguments parsed_args(argvv.size(), argv.data());
      auto cmd = parsed_args.create_cmd();
      cmd->execute();
    }

    void compare_bed_output(
        const string &asn1_filename,
        const string &input_filename,
        const string &output_filename,
        const vector<string> &args,
        const char *ref_begin,
        const char *ref_end)
    {
      bf::path in_path(test::path::in());
      bf::path asn(in_path);
      asn /= "../../libgrammar/test/in/asn1/";
      if (!asn1_filename.empty())
        asn /= asn1_filename;
      bf::path input(in_path);
      if (!input_filename.find(test::path::out()))
        input = input_filename;
      else
        input /= input_filename;
      bf::path out_path(test::path::out());
      bf::path out(out_path);
      out /= "bed/command";
      out /= output_filename;
      BOOST_TEST_CHECKPOINT("Removing: " << out);
      bf::remove(out);
      BOOST_TEST_CHECKPOINT("Create directories: " << out);
      bf::create_directories(out.parent_path());

      BOOST_TEST_CHECKPOINT("Reading: " << input);
      vector<string> argvv = { "./bed" };
      argvv.insert(argvv.end(), args.begin(), args.end());
      if (!asn1_filename.empty()) {
        argvv.push_back( "--asn");
        argvv.push_back(asn.generic_string());
      }
      argvv.push_back(input.generic_string());
      argvv.push_back(out.generic_string());
      run_bed(argvv);
      BOOST_TEST_CHECKPOINT("Checking output: " << out);
      auto f = ixxx::util::mmap_file(out.generic_string());
      BOOST_REQUIRE(bf::file_size(out));

      BOOST_TEST_CHECKPOINT("Comparing: " << out);
      if (boost::algorithm::ends_with(output_filename, ".ber"))
        BOOST_CHECK(std::equal(f.s_begin(), f.s_end(), ref_begin, ref_end));
      else
        BOOST_CHECK_EQUAL(string(f.s_begin(), f.s_end()),
            string(ref_begin, ref_end));
    }

    void compare_bed_output(
        const string &asn1_filename,
        const string &input_filename,
        const string &output_filename,
        const string &ref_filename,
        const vector<string> &args)
    {
      bf::path ref(test::path::ref());
      ref /= "bed/command";
      ref /= ref_filename;
      BOOST_TEST_CHECKPOINT("Opening reference: " << ref);
      auto g = ixxx::util::mmap_file(ref.generic_string());
      compare_bed_output(asn1_filename, input_filename, output_filename,
          args, g.s_begin(), g.s_end());
    }
    void compare_bed_output(
        const string &asn1_filename,
        const string &input_filename,
        const string &output_filename,
        const vector<string> &args)
    {
      compare_bed_output(asn1_filename, input_filename, output_filename,
          output_filename, args);
    }

