#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <algorithm>

#include <ixxx/util.hh>
#include <bed/command.hh>
#include <bed/command/ber_commands.hh>
#include <bed/arguments.hh>

#include <test/test.hh>

using namespace std;
namespace bf = boost::filesystem;

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
      vector<char *> argv;
      for (auto &s : argvv)
        argv.push_back(&*s.begin());
      argv.push_back(nullptr);
      bed::Arguments parsed_args(argvv.size(), argv.data());
      bed::command::execute(parsed_args);
      BOOST_TEST_CHECKPOINT("Checking output: " << out);
      ixxx::util::Mapped_File f(out.generic_string());
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
      ixxx::util::Mapped_File g(ref.generic_string());
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

BOOST_AUTO_TEST_SUITE(bed_)

  BOOST_AUTO_TEST_SUITE(command)


    BOOST_AUTO_TEST_SUITE(search)

      BOOST_AUTO_TEST_CASE(search_xpath)
      {
        const char ref[] = "<Sender>WERFD</Sender>\n";
        compare_bed_output("tap_3_12_strip.asn1",
            "asn1c/examples/sample.source.TAP3/sample-DataInterChange-1.ber", "search_xpath.xml",
            { "search", "//Sender" },
            ref, ref+sizeof(ref)-1);
      }

      BOOST_AUTO_TEST_CASE(search_skip)
      {
        const char ref[] = "\n";
        compare_bed_output("tap_3_12_strip.asn1",
            "tap_3_12_valid_att.ber", "search_skip.xml",
            { "search", "//Sender", "--skip", "741" },
            ref, ref+sizeof(ref)-1);
      }

      BOOST_AUTO_TEST_CASE(search_first)
      {
        const char ref[] = "<Sender>WERFD</Sender>\n";
        compare_bed_output("tap_3_12_strip.asn1",
            "tap_3_12_trailing_eoc_invalid.ber", "search_first.xml",
            { "search", "//Sender", "--first" },
            ref, ref+sizeof(ref)-1);
      }

      BOOST_AUTO_TEST_CASE(search_count)
      {
        const char ref[] = "<Sender>WERFD</Sender>\n";
        compare_bed_output("tap_3_12_strip.asn1",
            "tap_3_12_valid.ber", "search_count.xml",
            { "search", "//Sender", "--count", "18" },
            ref, ref+sizeof(ref)-1);
      }

      BOOST_AUTO_TEST_CASE(search_count_not_found)
      {
        const char ref[] = "\n";
        compare_bed_output("tap_3_12_strip.asn1",
            "tap_3_12_valid.ber", "count_not_found.xml",
            { "search", "//CallEventDetailsCount", "--count", "18" },
            ref, ref+sizeof(ref)-1);
      }

      BOOST_AUTO_TEST_CASE(search_print_indefinite)
      {
        const char ref[] =
          R"(<CurrencyConversion definite="false">
  <ExchangeRateCode>1</ExchangeRateCode>
  <NumberOfDecimalPlaces>5</NumberOfDecimalPlaces>
  <ExchangeRate>116203</ExchangeRate>
</CurrencyConversion>
)";
        compare_bed_output("tap_3_12_strip.asn1", "tap_3_12_valid_most_indef.ber",
            "search_print_indefinite.xml", { "search", "//CurrencyConversion" },
            ref, ref + sizeof(ref) - 1);
      }

    BOOST_AUTO_TEST_SUITE_END() // search

    BOOST_AUTO_TEST_SUITE(validate)

      BOOST_AUTO_TEST_CASE(xsd_result_invalid)
      {
        bf::path in_path(test::path::in());
        bf::path asn(in_path);
        asn /= "../../libgrammar/test/in/asn1/tap_3_12_strip.asn1";
        bf::path xsd(in_path);
        xsd /= "../../libgrammar/example/tap_3_12.xsd";
        bf::path input(in_path);
        input /= "asn1c/examples/sample.source.TAP3/sample-DataInterChange-1.ber";
        bf::path out_path(test::path::out());
        bf::path out(out_path);
        out /= "bed/command";
        out /= "xsd_invalid.xml";
        BOOST_TEST_CHECKPOINT("Removing: " << out);
        bf::remove(out);
        BOOST_TEST_CHECKPOINT("Create directories: " << out);
        bf::create_directories(out.parent_path());

        bed::Arguments args;
        args.in_filename = input.generic_string();
        args.asn_filenames.push_back(asn.generic_string());
        args.xsd_filename = xsd.generic_string();
        args.out_filename = out.generic_string();
        bed::command::Validate_XSD c(args);
        BOOST_CHECK_THROW(c.execute(), std::runtime_error);
      }

      BOOST_AUTO_TEST_CASE(xsd_result_valid)
      {
        bf::path in_path(test::path::in());
        bf::path asn(in_path);
        asn /= "../../libgrammar/test/in/asn1/tap_3_12_strip.asn1";
        bf::path xsd(in_path);
        xsd /= "../../libgrammar/example/tap_3_12.xsd";
        bf::path input(in_path);
        input /= "tap_3_12_valid.ber";
        bf::path out_path(test::path::out());
        bf::path out(out_path);
        out /= "bed/command";
        out /= "xsd_invalid.xml";
        BOOST_TEST_CHECKPOINT("Removing: " << out);
        bf::remove(out);
        BOOST_TEST_CHECKPOINT("Create directories: " << out);
        bf::create_directories(out.parent_path());

        bed::Arguments args;
        args.in_filename = input.generic_string();
        args.asn_filenames.push_back(asn.generic_string());
        args.xsd_filename = xsd.generic_string();
        args.out_filename = out.generic_string();
        bed::command::Validate_XSD c(args);
        BOOST_CHECK_NO_THROW(c.execute());
      }

    BOOST_AUTO_TEST_SUITE_END() // validate


    BOOST_AUTO_TEST_SUITE(write_xml)

      BOOST_AUTO_TEST_CASE(write_xml_skip)
      {
        compare_bed_output("tap_3_12_strip.asn1", "tap_3_12_valid.ber",
            "aci.xml", { "write-xml", "--skip", "740" });
      }

      BOOST_AUTO_TEST_CASE(write_xml_skip_off)
      {
        const char ref[] =
          R"(<SimChargeableSubscriber off='268'>
    <Imsi off='272'>133713371337133</Imsi>
</SimChargeableSubscriber>
)";
        compare_bed_output("tap_3_12_strip.asn1", "tap_3_12_valid.ber",
            "skip_off.xml", { "write-xml",
            "--skip", "268", "--off" },
            ref, ref + sizeof(ref) - 1);
      }

      BOOST_AUTO_TEST_CASE(write_xml_skip_not_last)
      {
        compare_bed_output("tap_3_12_strip.asn1", "tap_3_12_valid_att.ber",
            "aci_not_last.xml",
            "aci.xml", { "write-xml", "--skip", "741" });
      }

      BOOST_AUTO_TEST_CASE(write_xml_stop_first)
      {
        bf::path in_path(test::path::in());
        bf::path asn(in_path);
        asn /= "../../libgrammar/test/in/asn1/tap_3_12_strip.asn1";
        bf::path input(in_path);
        input /= "tap_3_12_valid.ber";
        bf::path input_mod(test::path::out());
        input_mod /= "bed/command/first.ber";
        BOOST_TEST_CHECKPOINT("Removing: " << input_mod);
        bf::remove(input_mod);
        BOOST_TEST_CHECKPOINT("Creating: " << input_mod);
        {
          size_t n = boost::filesystem::file_size(input);
          n += 2;
          ixxx::util::Mapped_File f(input.generic_string());
          ixxx::util::Mapped_File g(input_mod.generic_string(), true, true, n);
          std::copy(f.begin(), f.end(), g.begin());
          g.close();
        }
        bf::path ref(test::path::ref());
        ref /= "ber_pretty_xml";
        ref /= "tap_3_12_valid.xml";
        bf::path out_path(test::path::out());
        bf::path out(out_path);
        out /= "bed/command";
        out /= "first.xml";
        BOOST_TEST_CHECKPOINT("Removing: " << out);
        bf::remove(out);
        BOOST_TEST_CHECKPOINT("Create directories: " << out);
        bf::create_directories(out.parent_path());

        BOOST_TEST_CHECKPOINT("Reading: " << input);
        vector<string> argvv = { "./bed", "write-xml", "--asn",
          asn.generic_string(), input_mod.generic_string(), out.generic_string(),
          "--first" };
        vector<char *> argv;
        for (auto &s : argvv)
          argv.push_back(&*s.begin());
        argv.push_back(nullptr);
        bed::Arguments args(argvv.size(), argv.data());
        bed::command::execute(args);
        {
          BOOST_TEST_CHECKPOINT("Checking output: " << out);
          ixxx::util::Mapped_File f(out.generic_string());
          BOOST_REQUIRE(bf::file_size(out));
          BOOST_TEST_CHECKPOINT("Comparing: " << ref << " vs. " << out);
          ixxx::util::Mapped_File g(ref.generic_string());
          BOOST_CHECK(std::equal(f.begin(), f.end(), g.begin(), g.end()));
        }
      }

      BOOST_AUTO_TEST_CASE(write_xml_throw)
      {
        bf::path in_path(test::path::in());
        bf::path asn(in_path);
        asn /= "../../libgrammar/test/in/asn1/tap_3_12_strip.asn1";
        bf::path input(in_path);
        input /= "tap_3_12_trailing_eoc_invalid.ber";
        bf::path ref(test::path::ref());
        ref /= "ber_pretty_xml";
        ref /= "tap_3_12_valid.xml";
        bf::path out_path(test::path::out());
        bf::path out(out_path);
        out /= "bed/command";
        out /= "throw_first.xml";
        BOOST_TEST_CHECKPOINT("Removing: " << out);
        bf::remove(out);
        BOOST_TEST_CHECKPOINT("Create directories: " << out);
        bf::create_directories(out.parent_path());

        BOOST_TEST_CHECKPOINT("Reading: " << input);
        vector<string> argvv = { "./bed", "write-xml", "--asn",
          asn.generic_string(), input.generic_string(), out.generic_string()
          };
        vector<char *> argv;
        for (auto &s : argvv)
          argv.push_back(&*s.begin());
        argv.push_back(nullptr);
        bed::Arguments args(argvv.size(), argv.data());
        BOOST_CHECK_THROW(bed::command::execute(args), std::runtime_error);
      }


      BOOST_AUTO_TEST_CASE(write_xml_count)
      {
        compare_bed_output("tap_3_12_strip.asn1",
            "tap_3_12_valid.ber", "count.xml",
            { "write-xml", "--count", "18" });
      }

      BOOST_AUTO_TEST_CASE(write_xml_bci)
      {
        compare_bed_output("tap_3_12_strip.asn1",
            "tap_3_12_valid.ber", "bci.xml", "count.xml",
            { "write-xml", "--bci" });
      }


      BOOST_AUTO_TEST_CASE(write_xml_search_aci)
      {
        compare_bed_output("tap_3_12_strip.asn1",
            "tap_3_12_valid.ber", "write_xml_search_aci.xml",
            { "write-xml", "--aci"});
      }

      BOOST_AUTO_TEST_CASE(write_xml_search_aci_stop)
      {
        compare_bed_output("tap_3_12_strip.asn1",
            "tap_3_12_trailing_eoc_invalid.ber",
            "write_xml_search_aci_stop.xml",
            "write_xml_search_aci.xml",
            { "write-xml", "--aci"}
            );
      }

      BOOST_AUTO_TEST_CASE(write_xml_search)
      {
        const char ref[] =
          R"(<LocalTimeStamp off='92'>20050405090547</LocalTimeStamp>
)";
        compare_bed_output("tap_3_12_strip.asn1", "tap_3_12_valid.ber",
            "write_xml_search.xml", { "write-xml",
            "--search", "FileAvailableTimeStamp/LocalTimeStamp", "--off"},
            ref, ref + sizeof(ref) - 1);
      }

      BOOST_AUTO_TEST_CASE(write_xml_tag)
      {
        const char ref[] =
          R"(<LocalTimeStamp tag='16' off='92'>20050405090547</LocalTimeStamp>
)";
        compare_bed_output("tap_3_12_strip.asn1", "tap_3_12_valid.ber",
            "write_xml_tag.xml", { "write-xml",
            "--skip", "92", "--off", "--tag"},
            ref, ref + sizeof(ref) - 1);
      }

      BOOST_AUTO_TEST_CASE(write_xml_class)
      {
        const char ref[] =
          R"(<LocalTimeStamp class='APPLICATION' tag='16' off='92'>20050405090547</LocalTimeStamp>
)";
        compare_bed_output("tap_3_12_strip.asn1", "tap_3_12_valid.ber",
            "write_xml_class.xml", { "write-xml",
            "--skip", "92", "--off", "--tag", "--class" },
            ref, ref + sizeof(ref) - 1);
      }

      BOOST_AUTO_TEST_CASE(write_xml_cdr)
      {
        compare_bed_output("tap_3_12_strip.asn1", "tap_3_12_valid.ber",
            "write_xml_cdr.xml", { "write-xml", "--cdr", "3" });
      }

      BOOST_AUTO_TEST_CASE(write_xml_cdr_range)
      {
        compare_bed_output("tap_3_12_strip.asn1", "tap_3_12_valid.ber",
            "write_xml_cdr_range.xml", { "write-xml", "--cdr", "3.." });
      }

      BOOST_AUTO_TEST_CASE(write_xml_search_range)
      {
        const char ref[] =
          R"(<RecEntityId>foo</RecEntityId>
<RecEntityId>baz</RecEntityId>
)";
        compare_bed_output("tap_3_12_strip.asn1", "tap_3_12_valid.ber",
            "write_xml_cdr.xml", { "write-xml","--search",
            "RecEntityInfoList/RecEntityInformation/RecEntityId[1,3]" },
            ref, ref + sizeof(ref) - 1);
      }

      BOOST_AUTO_TEST_CASE(write_xml_search_last)
      {
        const char ref[] =
          R"(<CallEventDetailsCount>4</CallEventDetailsCount>
)";
        compare_bed_output("tap_3_12_strip.asn1", "tap_3_12_valid.ber",
            "write_xml_search_last.xml", { "write-xml","--search",
            "CallEventDetailsCount" },
            ref, ref + sizeof(ref) - 1);
      }

      BOOST_AUTO_TEST_CASE(write_xml_search_first)
      {
        compare_bed_output("tap_3_12_strip.asn1", "tap_3_12_valid.ber",
            "write_xml_search_first.xml",
            "../../ber_pretty_xml/tap_3_12_valid.xml", { "write-xml","--search",
            "/TransferBatch" }
           );
      }

      BOOST_AUTO_TEST_CASE(argument_error)
      {
        const char ref[] = "";
        BOOST_CHECK_THROW(
          compare_bed_output("tap_3_12_strip.asn1", "tap_3_12_valid.ber",
              "argument_error.xml", { "write-xml","--foobar" }, ref, ref
             ), bed::Argument_Error);
        BOOST_CHECK_THROW(
          compare_bed_output("tap_3_12_strip.asn1", "tap_3_12_valid.ber",
              "argument_error.xml", { "wite-xml" }, ref, ref
             ), bed::Argument_Error);
      }

    BOOST_AUTO_TEST_SUITE_END() // write_xml

  BOOST_AUTO_TEST_SUITE_END() // command

BOOST_AUTO_TEST_SUITE_END() // bed_
