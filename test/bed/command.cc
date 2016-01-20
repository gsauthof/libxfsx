#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include <unistd.h>
#include <fcntl.h>

#include <ixxx/util.hh>
#include <bed/command.hh>
#include <bed/command/ber_commands.hh>
#include <bed/arguments.hh>

#include <test/test.hh>

using namespace std;
namespace bf = boost::filesystem;

BOOST_AUTO_TEST_SUITE(bed_)


  BOOST_AUTO_TEST_SUITE(command)

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
      asn /= asn1_filename;
      bf::path input(in_path);
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
      argvv.push_back( "--asn");
      argvv.push_back(asn.generic_string());
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

    BOOST_AUTO_TEST_CASE(search_xpath)
    {
      bf::path in_path(test::path::in());
      bf::path asn(in_path);
      asn /= "../../libgrammar/test/in/asn1/tap_3_12_strip.asn1";
      bf::path input(in_path);
      input /= "asn1c/examples/sample.source.TAP3/sample-DataInterChange-1.ber";
      bf::path out_path(test::path::out());
      bf::path out(out_path);
      out /= "bed/command";
      out /= "search_xpath.xml";
      BOOST_TEST_CHECKPOINT("Removing: " << out);
      bf::remove(out);
      BOOST_TEST_CHECKPOINT("Create directories: " << out);
      bf::create_directories(out.parent_path());

      bed::Arguments args;
      args.in_filename = input.generic_string();
      args.asn_filenames.push_back(asn.generic_string());
      args.out_filename = out.generic_string();
      args.xpaths.push_back("//Sender");
      bed::command::Search_XPath c(args);
      c.execute();
      ixxx::util::Mapped_File f(out.generic_string());
      BOOST_TEST_CHECKPOINT("Checking output: " << out);
      BOOST_REQUIRE(bf::file_size(out));
      BOOST_CHECK_EQUAL(string(f.begin(), f.end()),
          "<Sender>WERFD</Sender>\n");
    }

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

    BOOST_AUTO_TEST_CASE(lxml_ber_writer)
    {
      bf::path in_path(test::path::in());
      bf::path asn(in_path);
      asn /= "../../libgrammar/test/in/asn1/tap_3_12_strip.asn1";
      bf::path input(in_path);
      input /= "tap_3_12_valid.ber";
      bf::path out_path(test::path::out());
      bf::path out(out_path);
      out /= "bed/command";
      out /= "edit_identity.ber";
      BOOST_TEST_CHECKPOINT("Removing: " << out);
      bf::remove(out);
      BOOST_TEST_CHECKPOINT("Create directories: " << out);
      bf::create_directories(out.parent_path());

      bed::Arguments args;
      args.in_filename = input.generic_string();
      args.asn_filenames.push_back(asn.generic_string());
      args.out_filename = out.generic_string();
      args.edit_ops.emplace_back(bed::Edit_Command::REMOVE, "/foobar");
      bed::command::Edit c(args);
      c.execute();
      {
        ixxx::util::Mapped_File f(out.generic_string());
        BOOST_TEST_CHECKPOINT("Checking output: " << out);
        BOOST_REQUIRE(bf::file_size(out));
        ixxx::util::Mapped_File g(input.generic_string());
        BOOST_CHECK(std::equal(f.begin(), f.end(), g.begin(), g.end()));
      }
    }

    BOOST_AUTO_TEST_CASE(edit_remove)
    {
      bf::path in_path(test::path::in());
      bf::path asn(in_path);
      asn /= "../../libgrammar/test/in/asn1/tap_3_12_strip.asn1";
      bf::path input(in_path);
      input /= "tap_3_12_valid.ber";
      bf::path ref(in_path);
      ref /= "tap_3_12_valid_removed.ber";
      bf::path out_path(test::path::out());
      bf::path out(out_path);
      out /= "bed/command";
      out /= "edit_remove.ber";
      BOOST_TEST_CHECKPOINT("Removing: " << out);
      bf::remove(out);
      BOOST_TEST_CHECKPOINT("Create directories: " << out);
      bf::create_directories(out.parent_path());

      bed::Arguments args;
      args.in_filename = input.generic_string();
      args.asn_filenames.push_back(asn.generic_string());
      args.out_filename = out.generic_string();
      args.edit_ops.emplace_back(bed::Edit_Command::REMOVE, "//MobileOriginatedCall");
      bed::command::Edit c(args);
      c.execute();
      {
        ixxx::util::Mapped_File f(out.generic_string());
        BOOST_TEST_CHECKPOINT("Checking output: " << out);
        BOOST_REQUIRE(bf::file_size(out));
        BOOST_TEST_CHECKPOINT("Comparing: " << ref << " vs. " << out);
        ixxx::util::Mapped_File g(ref.generic_string());
        BOOST_CHECK(std::equal(f.begin(), f.end(), g.begin(), g.end()));
      }
    }

    BOOST_AUTO_TEST_CASE(edit_replace)
    {
      bf::path in_path(test::path::in());
      bf::path asn(in_path);
      asn /= "../../libgrammar/test/in/asn1/tap_3_12_strip.asn1";
      bf::path input(in_path);
      input /= "tap_3_12_valid.ber";
      bf::path ref(in_path);
      ref /= "tap_3_12_valid_replaced.ber";
      bf::path out_path(test::path::out());
      bf::path out(out_path);
      out /= "bed/command";
      out /= "tap_3_12_valid_replaced.ber";
      BOOST_TEST_CHECKPOINT("Removing: " << out);
      bf::remove(out);
      BOOST_TEST_CHECKPOINT("Create directories: " << out);
      bf::create_directories(out.parent_path());

      bed::Arguments args;
      args.in_filename = input.generic_string();
      args.asn_filenames.push_back(asn.generic_string());
      args.out_filename = out.generic_string();
      args.edit_ops.emplace_back(bed::Edit_Command::REPLACE, "//LocalTimeStamp",
          "^[0-9]{4}(.*)$", "2016\\1");
      bed::command::Edit c(args);
      c.execute();
      {
        ixxx::util::Mapped_File f(out.generic_string());
        BOOST_TEST_CHECKPOINT("Checking output: " << out);
        BOOST_REQUIRE(bf::file_size(out));
        BOOST_TEST_CHECKPOINT("Comparing: " << ref << " vs. " << out);
        ixxx::util::Mapped_File g(ref.generic_string());
        BOOST_CHECK(std::equal(f.begin(), f.end(), g.begin(), g.end()));
      }
    }

    BOOST_AUTO_TEST_CASE(edit_add)
    {
      bf::path in_path(test::path::in());
      bf::path asn(in_path);
      asn /= "../../libgrammar/test/in/asn1/tap_3_12_strip.asn1";
      bf::path input(in_path);
      input /= "tap_3_12_valid.ber";
      bf::path ref(in_path);
      ref /= "tap_3_12_valid_add_osi.ber";
      bf::path out_path(test::path::out());
      bf::path out(out_path);
      out /= "bed/command";
      out /= "tap_3_12_valid_add_osi.ber";
      BOOST_TEST_CHECKPOINT("Removing: " << out);
      bf::remove(out);
      BOOST_TEST_CHECKPOINT("Create directories: " << out);
      bf::create_directories(out.parent_path());

      bed::Arguments args;
      args.in_filename = input.generic_string();
      args.asn_filenames.push_back(asn.generic_string());
      args.out_filename = out.generic_string();
      args.edit_ops.emplace_back(bed::Edit_Command::ADD, "//AuditControlInfo",
          "OperatorSpecInfoList/OperatorSpecInformation", "Patched for xyz");
      args.edit_ops.emplace_back(bed::Edit_Command::ADD, "//AuditControlInfo",
          "OperatorSpecInfoList/+OperatorSpecInformation", "Patchdate: 2015-05-01");
      bed::command::Edit c(args);
      c.execute();
      {
        ixxx::util::Mapped_File f(out.generic_string());
        BOOST_TEST_CHECKPOINT("Checking output: " << out);
        BOOST_REQUIRE(bf::file_size(out));
        BOOST_TEST_CHECKPOINT("Comparing: " << ref << " vs. " << out);
        ixxx::util::Mapped_File g(ref.generic_string());
        BOOST_CHECK(std::equal(f.begin(), f.end(), g.begin(), g.end()));
      }
    }

    BOOST_AUTO_TEST_CASE(edit_set_att)
    {
      bf::path in_path(test::path::in());
      bf::path asn(in_path);
      asn /= "../../libgrammar/test/in/asn1/tap_3_12_strip.asn1";
      bf::path input(in_path);
      input /= "tap_3_12_valid.ber";
      bf::path ref(in_path);
      ref /= "tap_3_12_valid_att.ber";
      bf::path out_path(test::path::out());
      bf::path out(out_path);
      out /= "bed/command";
      out /= "tap_3_12_valid_att.ber";
      BOOST_TEST_CHECKPOINT("Removing: " << out);
      bf::remove(out);
      BOOST_TEST_CHECKPOINT("Create directories: " << out);
      bf::create_directories(out.parent_path());

      bed::Arguments args;
      args.in_filename = input.generic_string();
      args.asn_filenames.push_back(asn.generic_string());
      args.out_filename = out.generic_string();
      args.edit_ops.emplace_back(bed::Edit_Command::SET_ATT, "//TransferBatch",
          "definite", "false");
      args.edit_ops.emplace_back(bed::Edit_Command::SET_ATT, "//CallEventDetailList",
          "definite", "false");
      args.edit_ops.emplace_back(bed::Edit_Command::SET_ATT, "//GprsCall",
          "l_size", "4");
      bed::command::Edit c(args);
      c.execute();
      {
        ixxx::util::Mapped_File f(out.generic_string());
        BOOST_TEST_CHECKPOINT("Checking output: " << out);
        BOOST_REQUIRE(bf::file_size(out));
        BOOST_TEST_CHECKPOINT("Comparing: " << ref << " vs. " << out);
        ixxx::util::Mapped_File g(ref.generic_string());
        BOOST_CHECK(std::equal(f.begin(), f.end(), g.begin(), g.end()));
      }
    }

    BOOST_AUTO_TEST_CASE(edit_set_uint)
    {
      bf::path in_path(test::path::in());
      bf::path asn(in_path);
      asn /= "../../libgrammar/test/in/asn1/tap_3_12_strip.asn1";
      bf::path input(in_path);
      input /= "tap_3_12_negative_volume.ber";
      bf::path ref(in_path);
      ref /= "tap_3_12_negative_volume_fixed.ber";
      bf::path out_path(test::path::out());
      bf::path out(out_path);
      out /= "bed/command";
      out /= "tap_3_12_negative_volume_fixed.ber";
      BOOST_TEST_CHECKPOINT("Removing: " << out);
      bf::remove(out);
      BOOST_TEST_CHECKPOINT("Create directories: " << out);
      bf::create_directories(out.parent_path());

      bed::Arguments args;
      args.in_filename = input.generic_string();
      args.asn_filenames.push_back(asn.generic_string());
      args.out_filename = out.generic_string();
      args.edit_ops.emplace_back(bed::Edit_Command::SET_ATT,
          "//DataVolumeIncoming",
          "uint2int", "true");
      bed::command::Edit c(args);
      c.execute();
      {
        ixxx::util::Mapped_File f(out.generic_string());
        BOOST_TEST_CHECKPOINT("Checking output: " << out);
        BOOST_REQUIRE(bf::file_size(out));
        BOOST_TEST_CHECKPOINT("Comparing: " << ref << " vs. " << out);
        ixxx::util::Mapped_File g(ref.generic_string());
        BOOST_CHECK(std::equal(f.begin(), f.end(), g.begin(), g.end()));
      }
    }

    BOOST_AUTO_TEST_CASE(edit_insert)
    {
      bf::path in_path(test::path::in());
      bf::path asn(in_path);
      asn /= "../../libgrammar/test/in/asn1/tap_3_12_strip.asn1";
      bf::path input(in_path);
      input /= "tap_3_12_valid.ber";
      bf::path ref(in_path);
      ref /= "tap_3_12_valid_add_osi.ber";
      bf::path out_path(test::path::out());
      bf::path out(out_path);
      out /= "bed/command";
      out /= "tap_3_12_valid_insert_osi.ber";
      BOOST_TEST_CHECKPOINT("Removing: " << out);
      bf::remove(out);
      BOOST_TEST_CHECKPOINT("Create directories: " << out);
      bf::create_directories(out.parent_path());

      bed::Arguments args;
      args.in_filename = input.generic_string();
      args.asn_filenames.push_back(asn.generic_string());
      args.out_filename = out.generic_string();
      args.edit_ops.emplace_back(bed::Edit_Command::INSERT,
          "//AuditControlInfo",
          "<OperatorSpecInfoList>\n"
          "  <OperatorSpecInformation>Patched for xyz</OperatorSpecInformation>\n"
          "  <OperatorSpecInformation>Patchdate: 2015-05-01</OperatorSpecInformation>\n"
          "</OperatorSpecInfoList>", "-1");
      bed::command::Edit c(args);
      c.execute();
      {
        ixxx::util::Mapped_File f(out.generic_string());
        BOOST_TEST_CHECKPOINT("Checking output: " << out);
        BOOST_REQUIRE(bf::file_size(out));
        BOOST_TEST_CHECKPOINT("Comparing: " << ref << " vs. " << out);
        ixxx::util::Mapped_File g(ref.generic_string());
        BOOST_CHECK(std::equal(f.begin(), f.end(), g.begin(), g.end()));
      }
    }

    BOOST_AUTO_TEST_CASE(edit_insert_file)
    {
      bf::path in_path(test::path::in());
      bf::path asn(in_path);
      asn /= "../../libgrammar/test/in/asn1/tap_3_12_strip.asn1";
      bf::path input(in_path);
      input /= "tap_3_12_valid.ber";
      bf::path ref(in_path);
      ref /= "tap_3_12_valid_add_osi.ber";
      bf::path out_path(test::path::out());
      bf::path out(out_path);
      out /= "bed/command";
      out /= "tap_3_12_valid_insert_osi.ber";
      BOOST_TEST_CHECKPOINT("Removing: " << out);
      bf::remove(out);
      bf::path xml(out_path);
      xml /= "bed/command";
      xml /= "osi.xml";
      BOOST_TEST_CHECKPOINT("Removing: " << xml);
      bf::remove(xml);
      {
        const char inp[] =
          "<OperatorSpecInfoList>\n"
          "  <OperatorSpecInformation>Patched for xyz</OperatorSpecInformation>\n"
          "  <OperatorSpecInformation>Patchdate: 2015-05-01</OperatorSpecInformation>\n"
          "</OperatorSpecInfoList>\n";
        ixxx::util::FD fd(xml.generic_string(), O_CREAT | O_WRONLY | O_TRUNC, 0666);
        write(fd, inp, sizeof(inp)-1);
        fd.close();
      }
      BOOST_TEST_CHECKPOINT("Create directories: " << out);
      bf::create_directories(out.parent_path());

      bed::Arguments args;
      args.in_filename = input.generic_string();
      args.asn_filenames.push_back(asn.generic_string());
      args.out_filename = out.generic_string();
      args.edit_ops.emplace_back(bed::Edit_Command::INSERT,
          "//AuditControlInfo",
          "@" + xml.generic_string(), "-1");
      bed::command::Edit c(args);
      c.execute();
      {
        ixxx::util::Mapped_File f(out.generic_string());
        BOOST_TEST_CHECKPOINT("Checking output: " << out);
        BOOST_REQUIRE(bf::file_size(out));
        BOOST_TEST_CHECKPOINT("Comparing: " << ref << " vs. " << out);
        ixxx::util::Mapped_File g(ref.generic_string());
        BOOST_CHECK(std::equal(f.begin(), f.end(), g.begin(), g.end()));
      }
    }

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

    BOOST_AUTO_TEST_CASE(write_xml_count)
    {
      compare_bed_output("tap_3_12_strip.asn1",
          "tap_3_12_valid.ber", "count.xml",
          { "write-xml", "--count", "18" });
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

  BOOST_AUTO_TEST_SUITE_END() // command


BOOST_AUTO_TEST_SUITE_END() // bed_
