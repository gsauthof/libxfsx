#include <boost/test/unit_test.hpp>
#include <test/test.hh>
#include <boost/filesystem.hpp>

#include <test/bed/helper.hh>

#include <bed/command.hh>
#include <bed/command/write_aci.hh>
#include <bed/arguments.hh>

#include <ixxx/ixxx.hh>
#include <ixxx/ansi.hh>
#include <ixxx/posix.hh>

#include <string>

using namespace std;

BOOST_AUTO_TEST_SUITE(bed_)


  BOOST_AUTO_TEST_SUITE(command)


    BOOST_AUTO_TEST_SUITE(write_aci)

      BOOST_AUTO_TEST_CASE(basic)
      {
        compare_bed_output("tap_3_12_strip.asn1",
            "tap_3_12_valid.ber", "write_aci.ber",
            "edit_aci.ber",
            { "write-aci" }
            );
      }

      BOOST_AUTO_TEST_CASE(autodetect)
      {
        string old_asn1_path;
        try { old_asn1_path = ixxx::ansi::getenv("ASN1_PATH"); }
        catch (const ixxx::getenv_error &e) {}
        string a {test::path::in() + "/../../libgrammar/test/in/asn1"};
        string b {test::path::in() + "/../../config"};
        string c {test::path::in() + "/../../libgrammar/grammar/xml"};
        string d {test::path::in() + "/../../telephone-code"};
        ixxx::posix::setenv("ASN1_PATH", a + ":" + b + ":" + c + ":" + d, true);
        compare_bed_output("",
            "tap_3_12_valid.ber", "write_aci_auto.ber",
            "edit_aci.ber",
            { "write-aci" }
            );
        if (!old_asn1_path.empty())
          ixxx::posix::setenv("ASN1_PATH", old_asn1_path, true);
      }

      BOOST_AUTO_TEST_CASE(no_prior_aci)
      {
        string out(test::path::out() + "/bed/command/aci_removed.ber");
        boost::filesystem::remove(out);
        {
          vector<string> argvv = { "./bed", "edit", "-c", "remove",
              "/*/AuditControlInfo", "--asn"  };
          argvv.push_back(test::path::in()
              + "/../../libgrammar/test/in/asn1/tap_3_12_strip.asn1");
          argvv.push_back(test::path::in() + "/tap_3_12_valid.ber");
          argvv.push_back(out);
          vector<char *> argv;
          for (auto &s : argvv)
            argv.push_back(&*s.begin());
          argv.push_back(nullptr);
          bed::Arguments parsed_args(argvv.size(), argv.data());
          bed::command::execute(parsed_args);
        }
        compare_bed_output("tap_3_12_strip.asn1",
            out, "aci_removed_fixed.ber",
            "edit_aci.ber",
            { "write-aci" }
            );
      }

    BOOST_AUTO_TEST_SUITE_END() // write_aci

  BOOST_AUTO_TEST_SUITE_END() // command

BOOST_AUTO_TEST_SUITE_END() // bed_
