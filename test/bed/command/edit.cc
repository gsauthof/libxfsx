#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include <ixxx/util.hh>
#include <fcntl.h>
#include <unistd.h>

#include <test/test.hh>
#include <test/bed/helper.hh>

#include <bed/command.hh>
#include <bed/command/edit.hh>
#include <bed/arguments.hh>

namespace bf = boost::filesystem;
using namespace std;


BOOST_AUTO_TEST_SUITE(bed_)

  BOOST_AUTO_TEST_SUITE(command)

    BOOST_AUTO_TEST_SUITE(edit)

      BOOST_AUTO_TEST_CASE(lxml_ber_writer)
      {
        compare_bed_output("tap_3_12_strip.asn1", "tap_3_12_valid.ber",
            "edit_identity.ber", "../../../in/tap_3_12_valid.ber",
            { "edit", "-c", "remove", "/foobar" }
           );
      }

      BOOST_AUTO_TEST_CASE(edit_remove)
      {
        compare_bed_output("tap_3_12_strip.asn1", "tap_3_12_valid.ber",
            "edit_remove.ber", "../../../in/tap_3_12_valid_removed.ber",
            { "edit", "-c", "remove", "//MobileOriginatedCall" }
           );
      }

      BOOST_AUTO_TEST_CASE(edit_remove_indefinite)
      {
        compare_bed_output("tap_3_12_strip.asn1", "tap_3_12_valid_most_indef.ber",
            "edit_remove_indefinite.ber",
            { "edit", "-c", "remove", "(/*/CallEventDetailList/*)[3]" }
           );
      }

      BOOST_AUTO_TEST_CASE(edit_replace)
      {
        compare_bed_output("tap_3_12_strip.asn1", "tap_3_12_valid.ber",
            "tap_3_12_valid_replaced.ber", "../../../in/tap_3_12_valid_replaced.ber",
            { "edit", "-c", "replace", "//LocalTimeStamp",
                                       "^[0-9]{4}(.*)$", "2016\\1" }
           );
      }

      BOOST_AUTO_TEST_CASE(edit_add)
      {
        compare_bed_output("tap_3_12_strip.asn1", "tap_3_12_valid.ber",
            "tap_3_12_valid_add_osi.ber", "../../../in/tap_3_12_valid_add_osi.ber",
            { "edit",
              "-c", "add", "//AuditControlInfo",
        "OperatorSpecInfoList/OperatorSpecInformation", "Patched for xyz",
              "-c", "add", "//AuditControlInfo",
        "OperatorSpecInfoList/+OperatorSpecInformation", "Patchdate: 2015-05-01"
            }
           );
      }

      BOOST_AUTO_TEST_CASE(edit_set_att)
      {
        compare_bed_output("tap_3_12_strip.asn1", "tap_3_12_valid.ber",
            "tap_3_12_valid_att.ber", "../../../in/tap_3_12_valid_att.ber",
            { "edit",
              "-c", "set-att", "//TransferBatch", "definite", "false",
              "-c", "set-att", "//CallEventDetailList", "definite", "false",
              "-c", "set-att", "//GprsCall", "l_size", "4"
            }
           );
      }

      BOOST_AUTO_TEST_CASE(edit_set_uint)
      {
        compare_bed_output("tap_3_12_strip.asn1", "tap_3_12_negative_volume.ber",
            "tap_3_12_negative_volume_fixed.ber",
            "../../../in/tap_3_12_negative_volume_fixed.ber",
            { "edit",
              "-c", "set-att", "//DataVolumeIncoming", "uint2int", "true"
            }
           );
      }

      BOOST_AUTO_TEST_CASE(edit_insert)
      {
        compare_bed_output("tap_3_12_strip.asn1", "tap_3_12_valid.ber",
            "tap_3_12_valid_insert_osi.ber",
            "../../../in/tap_3_12_valid_add_osi.ber",
            { "edit",
              "-c", "insert", "//AuditControlInfo",
            "<OperatorSpecInfoList>\n"
            "  <OperatorSpecInformation>Patched for xyz</OperatorSpecInformation>\n"
            "  <OperatorSpecInformation>Patchdate: 2015-05-01</OperatorSpecInformation>\n"
            "</OperatorSpecInfoList>", "-1"
            }
           );
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
        args.edit_ops.push_back(unique_ptr<bed::command::edit_op::Base>(
              new bed::command::edit_op::Insert));
        args.edit_ops.back()->argv = {
            "//AuditControlInfo", "@" + xml.generic_string(), "-1" };
        bed::command::Edit c(args);
        c.execute();
        {
          auto f = ixxx::util::mmap_file(out.generic_string());
          BOOST_TEST_CHECKPOINT("Checking output: " << out);
          BOOST_REQUIRE(bf::file_size(out));
          BOOST_TEST_CHECKPOINT("Comparing: " << ref << " vs. " << out);
          auto g = ixxx::util::mmap_file(ref.generic_string());
          BOOST_CHECK(std::equal(f.begin(), f.end(), g.begin(), g.end()));
        }
      }

      BOOST_AUTO_TEST_CASE(compute_aci)
      {
        compare_bed_output("tap_3_12_strip.asn1", "tap_3_12_valid.ber",
            "edit_aci.ber",
            { "edit", "-c", "write-aci" }
           );
      }

    BOOST_AUTO_TEST_SUITE_END() // edit

  BOOST_AUTO_TEST_SUITE_END() // command

BOOST_AUTO_TEST_SUITE_END() // bed_
