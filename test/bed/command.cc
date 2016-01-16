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
      bf::path in_path(test::path::in());
      bf::path asn(in_path);
      asn /= "../../libgrammar/test/in/asn1/tap_3_12_strip.asn1";
      bf::path input(in_path);
      input /= "tap_3_12_valid.ber";
      bf::path ref(test::path::ref());
      ref /= "bed/command";
      ref /= "aci.xml";
      bf::path out_path(test::path::out());
      bf::path out(out_path);
      out /= "bed/command";
      out /= "aci.xml";
      BOOST_TEST_CHECKPOINT("Removing: " << out);
      bf::remove(out);
      BOOST_TEST_CHECKPOINT("Create directories: " << out);
      bf::create_directories(out.parent_path());

      BOOST_TEST_CHECKPOINT("Reading: " << input);
      vector<string> argvv = { "./bed", "write-xml", "--asn",
        asn.generic_string(), input.generic_string(), out.generic_string(),
        "--skip", "740" };
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

    BOOST_AUTO_TEST_CASE(write_xml_skip_off)
    {
      bf::path in_path(test::path::in());
      bf::path asn(in_path);
      asn /= "../../libgrammar/test/in/asn1/tap_3_12_strip.asn1";
      bf::path input(in_path);
      input /= "tap_3_12_valid.ber";
      bf::path out_path(test::path::out());
      bf::path out(out_path);
      out /= "bed/command";
      out /= "skip_off.xml";
      BOOST_TEST_CHECKPOINT("Removing: " << out);
      bf::remove(out);
      BOOST_TEST_CHECKPOINT("Create directories: " << out);
      bf::create_directories(out.parent_path());

      BOOST_TEST_CHECKPOINT("Reading: " << input);
      vector<string> argvv = { "./bed", "write-xml", "--asn",
        asn.generic_string(), input.generic_string(), out.generic_string(),
        "--skip", "268", "--off" };
      vector<char *> argv;
      for (auto &s : argvv)
        argv.push_back(&*s.begin());
      argv.push_back(nullptr);
      bed::Arguments args(argvv.size(), argv.data());
      bed::command::execute(args);

      BOOST_TEST_CHECKPOINT("Checking output: " << out);
      ixxx::util::Mapped_File f(out.generic_string());
      BOOST_REQUIRE(bf::file_size(out));
      BOOST_CHECK_EQUAL(string(f.s_begin(), f.s_end()),
          "<SimChargeableSubscriber off='268'>\n"
          "    <Imsi off='272'>133713371337133</Imsi>\n"
          "</SimChargeableSubscriber>\n");
    }

    BOOST_AUTO_TEST_CASE(write_xml_skip_not_last)
    {
      bf::path in_path(test::path::in());
      bf::path asn(in_path);
      asn /= "../../libgrammar/test/in/asn1/tap_3_12_strip.asn1";
      bf::path input(in_path);
      input /= "tap_3_12_valid_att.ber";
      bf::path ref(test::path::ref());
      ref /= "bed/command";
      ref /= "aci.xml";
      bf::path out_path(test::path::out());
      bf::path out(out_path);
      out /= "bed/command";
      out /= "aci.xml";
      BOOST_TEST_CHECKPOINT("Removing: " << out);
      bf::remove(out);
      BOOST_TEST_CHECKPOINT("Create directories: " << out);
      bf::create_directories(out.parent_path());

      BOOST_TEST_CHECKPOINT("Reading: " << input);
      vector<string> argvv = { "./bed", "write-xml", "--asn",
        asn.generic_string(), input.generic_string(), out.generic_string(),
        "--skip", "741" };
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
      bf::path in_path(test::path::in());
      bf::path asn(in_path);
      asn /= "../../libgrammar/test/in/asn1/tap_3_12_strip.asn1";
      bf::path input(in_path);
      input /= "tap_3_12_valid_att.ber";
      bf::path out_path(test::path::out());
      bf::path out(out_path);
      out /= "bed/command";
      out /= "search_skip.xml";
      BOOST_TEST_CHECKPOINT("Removing: " << out);
      bf::remove(out);
      BOOST_TEST_CHECKPOINT("Create directories: " << out);
      bf::create_directories(out.parent_path());

      BOOST_TEST_CHECKPOINT("Reading: " << input);
      vector<string> argvv = { "./bed", "search", "//Sender", "--asn",
        asn.generic_string(), input.generic_string(), out.generic_string(),
        "--skip", "741"  };
      vector<char *> argv;
      for (auto &s : argvv)
        argv.push_back(&*s.begin());
      argv.push_back(nullptr);
      bed::Arguments args(argvv.size(), argv.data());
      bed::command::execute(args);
      ixxx::util::Mapped_File f(out.generic_string());
      BOOST_TEST_CHECKPOINT("Checking output: " << out);
      BOOST_REQUIRE(bf::file_size(out));
      BOOST_CHECK_EQUAL(string(f.begin(), f.end()),
          "\n");
    }

    BOOST_AUTO_TEST_CASE(search_first)
    {
      bf::path in_path(test::path::in());
      bf::path asn(in_path);
      asn /= "../../libgrammar/test/in/asn1/tap_3_12_strip.asn1";
      bf::path input(in_path);
      input /= "tap_3_12_trailing_eoc_invalid.ber";
      bf::path out_path(test::path::out());
      bf::path out(out_path);
      out /= "bed/command";
      out /= "search_first.xml";
      BOOST_TEST_CHECKPOINT("Removing: " << out);
      bf::remove(out);
      BOOST_TEST_CHECKPOINT("Create directories: " << out);
      bf::create_directories(out.parent_path());

      BOOST_TEST_CHECKPOINT("Reading: " << input);
      vector<string> argvv = { "./bed", "search", "//Sender", "--asn",
        asn.generic_string(), input.generic_string(), out.generic_string(),
        "--first" };
      vector<char *> argv;
      for (auto &s : argvv)
        argv.push_back(&*s.begin());
      argv.push_back(nullptr);
      bed::Arguments args(argvv.size(), argv.data());
      bed::command::execute(args);
      ixxx::util::Mapped_File f(out.generic_string());
      BOOST_TEST_CHECKPOINT("Checking output: " << out);
      BOOST_REQUIRE(bf::file_size(out));
      BOOST_CHECK_EQUAL(string(f.begin(), f.end()),
          "<Sender>WERFD</Sender>\n");
    }

    BOOST_AUTO_TEST_CASE(write_xml_count)
    {
      bf::path in_path(test::path::in());
      bf::path asn(in_path);
      asn /= "../../libgrammar/test/in/asn1/tap_3_12_strip.asn1";
      bf::path input(in_path);
      input /= "tap_3_12_valid.ber";
      bf::path ref(test::path::ref());
      ref /= "bed/command";
      ref /= "count.xml";
      bf::path out_path(test::path::out());
      bf::path out(out_path);
      out /= "bed/command";
      out /= "count.xml";
      BOOST_TEST_CHECKPOINT("Removing: " << out);
      bf::remove(out);
      BOOST_TEST_CHECKPOINT("Create directories: " << out);
      bf::create_directories(out.parent_path());

      BOOST_TEST_CHECKPOINT("Reading: " << input);
      vector<string> argvv = { "./bed", "write-xml", "--asn",
        asn.generic_string(), input.generic_string(), out.generic_string(),
        "--count", "18" };
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

    BOOST_AUTO_TEST_CASE(search_count)
    {
      bf::path in_path(test::path::in());
      bf::path asn(in_path);
      asn /= "../../libgrammar/test/in/asn1/tap_3_12_strip.asn1";
      bf::path input(in_path);
      input /= "tap_3_12_valid.ber";
      bf::path out_path(test::path::out());
      bf::path out(out_path);
      out /= "bed/command";
      out /= "search_count.xml";
      BOOST_TEST_CHECKPOINT("Removing: " << out);
      bf::remove(out);
      BOOST_TEST_CHECKPOINT("Create directories: " << out);
      bf::create_directories(out.parent_path());

      BOOST_TEST_CHECKPOINT("Reading: " << input);
      vector<string> argvv = { "./bed", "search", "//Sender", "--asn",
        asn.generic_string(), input.generic_string(), out.generic_string(),
        "--count", "18" };
      vector<char *> argv;
      for (auto &s : argvv)
        argv.push_back(&*s.begin());
      argv.push_back(nullptr);
      bed::Arguments args(argvv.size(), argv.data());
      bed::command::execute(args);
      ixxx::util::Mapped_File f(out.generic_string());
      BOOST_TEST_CHECKPOINT("Checking output: " << out);
      BOOST_REQUIRE(bf::file_size(out));
      BOOST_CHECK_EQUAL(string(f.begin(), f.end()),
          "<Sender>WERFD</Sender>\n");
    }

    BOOST_AUTO_TEST_CASE(search_count_not_found)
    {
      bf::path in_path(test::path::in());
      bf::path asn(in_path);
      asn /= "../../libgrammar/test/in/asn1/tap_3_12_strip.asn1";
      bf::path input(in_path);
      input /= "tap_3_12_valid.ber";
      bf::path out_path(test::path::out());
      bf::path out(out_path);
      out /= "bed/command";
      out /= "count_not_found.xml";
      BOOST_TEST_CHECKPOINT("Removing: " << out);
      bf::remove(out);
      BOOST_TEST_CHECKPOINT("Create directories: " << out);
      bf::create_directories(out.parent_path());

      BOOST_TEST_CHECKPOINT("Reading: " << input);
      vector<string> argvv = { "./bed", "search", "//CallEventDetailsCount",
        "--asn",
        asn.generic_string(), input.generic_string(), out.generic_string(),
        "--count", "18" };
      vector<char *> argv;
      for (auto &s : argvv)
        argv.push_back(&*s.begin());
      argv.push_back(nullptr);
      bed::Arguments args(argvv.size(), argv.data());
      bed::command::execute(args);
      ixxx::util::Mapped_File f(out.generic_string());
      BOOST_TEST_CHECKPOINT("Checking output: " << out);
      BOOST_REQUIRE(bf::file_size(out));
      BOOST_CHECK_EQUAL(string(f.begin(), f.end()),
          "\n");
    }

    BOOST_AUTO_TEST_CASE(write_xml_search_aci)
    {
      bf::path in_path(test::path::in());
      bf::path asn(in_path);
      asn /= "../../libgrammar/test/in/asn1/tap_3_12_strip.asn1";
      bf::path input(in_path);
      input /= "tap_3_12_valid.ber";
      bf::path out_path(test::path::out());
      bf::path out(out_path);
      out /= "bed/command";
      out /= "write_xml_search_aci.xml";
      BOOST_TEST_CHECKPOINT("Removing: " << out);
      bf::remove(out);
      BOOST_TEST_CHECKPOINT("Create directories: " << out);
      bf::create_directories(out.parent_path());

      BOOST_TEST_CHECKPOINT("Reading: " << input);
      vector<string> argvv = { "./bed", "write-xml",
        "--aci",
        "--asn",
        asn.generic_string(), input.generic_string(), out.generic_string()
        };
      vector<char *> argv;
      for (auto &s : argvv)
        argv.push_back(&*s.begin());
      argv.push_back(nullptr);
      bed::Arguments args(argvv.size(), argv.data());
      bed::command::execute(args);
      ixxx::util::Mapped_File f(out.generic_string());
      BOOST_TEST_CHECKPOINT("Checking output: " << out);
      BOOST_REQUIRE(bf::file_size(out));

      bf::path ref(test::path::ref());
      ref /= "bed/command";
      ref /= "write_xml_search_aci.xml";
      BOOST_TEST_CHECKPOINT("Comparing: " << ref << " vs. " << out);
      ixxx::util::Mapped_File g(ref.generic_string());

      BOOST_CHECK_EQUAL(string(f.s_begin(), f.s_end()),
          string(g.s_begin(), g.s_end()));
    }

    BOOST_AUTO_TEST_CASE(write_xml_search_aci_stop)
    {
      bf::path in_path(test::path::in());
      bf::path asn(in_path);
      asn /= "../../libgrammar/test/in/asn1/tap_3_12_strip.asn1";
      bf::path input(in_path);
      input /= "tap_3_12_trailing_eoc_invalid.ber";
      bf::path out_path(test::path::out());
      bf::path out(out_path);
      out /= "bed/command";
      out /= "write_xml_search_aci_stop.xml";
      BOOST_TEST_CHECKPOINT("Removing: " << out);
      bf::remove(out);
      BOOST_TEST_CHECKPOINT("Create directories: " << out);
      bf::create_directories(out.parent_path());

      BOOST_TEST_CHECKPOINT("Reading: " << input);
      vector<string> argvv = { "./bed", "write-xml",
        "--aci",
        "--asn",
        asn.generic_string(), input.generic_string(), out.generic_string()
        };
      vector<char *> argv;
      for (auto &s : argvv)
        argv.push_back(&*s.begin());
      argv.push_back(nullptr);
      bed::Arguments args(argvv.size(), argv.data());
      bed::command::execute(args);
      ixxx::util::Mapped_File f(out.generic_string());
      BOOST_TEST_CHECKPOINT("Checking output: " << out);
      BOOST_REQUIRE(bf::file_size(out));

      bf::path ref(test::path::ref());
      ref /= "bed/command";
      ref /= "write_xml_search_aci.xml";
      BOOST_TEST_CHECKPOINT("Comparing: " << ref << " vs. " << out);
      ixxx::util::Mapped_File g(ref.generic_string());

      BOOST_CHECK_EQUAL(string(f.s_begin(), f.s_end()),
          string(g.s_begin(), g.s_end()));
    }

    BOOST_AUTO_TEST_CASE(write_xml_search)
    {
      bf::path in_path(test::path::in());
      bf::path asn(in_path);
      asn /= "../../libgrammar/test/in/asn1/tap_3_12_strip.asn1";
      bf::path input(in_path);
      input /= "tap_3_12_valid.ber";
      bf::path out_path(test::path::out());
      bf::path out(out_path);
      out /= "bed/command";
      out /= "write_xml_search.xml";
      BOOST_TEST_CHECKPOINT("Removing: " << out);
      bf::remove(out);
      BOOST_TEST_CHECKPOINT("Create directories: " << out);
      bf::create_directories(out.parent_path());

      BOOST_TEST_CHECKPOINT("Reading: " << input);
      vector<string> argvv = { "./bed", "write-xml",
        "--search", "FileAvailableTimeStamp/LocalTimeStamp", "--off",
        "--asn",
        asn.generic_string(), input.generic_string(), out.generic_string()
        };
      vector<char *> argv;
      for (auto &s : argvv)
        argv.push_back(&*s.begin());
      argv.push_back(nullptr);
      bed::Arguments args(argvv.size(), argv.data());
      bed::command::execute(args);
      ixxx::util::Mapped_File f(out.generic_string());
      BOOST_TEST_CHECKPOINT("Checking output: " << out);
      BOOST_REQUIRE(bf::file_size(out));

      BOOST_CHECK_EQUAL(string(f.s_begin(), f.s_end()),
          string("<LocalTimeStamp off='92'>20050405090547</LocalTimeStamp>\n"));
    }


  BOOST_AUTO_TEST_SUITE_END() // command


BOOST_AUTO_TEST_SUITE_END() // bed_
