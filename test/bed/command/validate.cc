#include <boost/test/unit_test.hpp>
#include <test/test.hh>

#include <boost/filesystem.hpp>


#include <test/bed/helper.hh>

#include <bed/command.hh>
#include <bed/arguments.hh>
#include <bed/command/ber_commands.hh>

namespace bf = boost::filesystem;

BOOST_AUTO_TEST_SUITE(bed_)

  BOOST_AUTO_TEST_SUITE(command)

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

      BOOST_AUTO_TEST_CASE(xsd_result_valid_cmd)
      {
        const char ref[] = "validates\n";

        bf::path in_path(test::path::in());
        bf::path xsd(in_path);
        xsd /= "../../libgrammar/example/tap_3_12.xsd";

        compare_bed_output("tap_3_12_strip.asn1", "tap_3_12_valid.ber",
            "validate_ok.log",
            { "validate", "--xsd", xsd.generic_string() },
            ref, ref + sizeof(ref) - 1
           );
      }

    BOOST_AUTO_TEST_SUITE_END() // validate

  BOOST_AUTO_TEST_SUITE_END() // command

BOOST_AUTO_TEST_SUITE_END() // bed_
