#include <boost/test/unit_test.hpp>
#include <test/test.hh>
#include <boost/filesystem.hpp>

#include <test/bed/helper.hh>

#include <bed/command.hh>
#include <bed/command/write_aci.hh>

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
        catch (const ixxx::runtime_error &e) {}
        string a {test::path::in() + "/../../libgrammar/test/in/asn1"};
        string b {test::path::in() + "/../../config"};
        string c {test::path::in() + "/../../libgrammar/grammar/xml"};
        ixxx::posix::setenv("ASN1_PATH", a + ":" + b + ":" + c, true);
        compare_bed_output("",
            "tap_3_12_valid.ber", "write_aci_auto.ber",
            "edit_aci.ber",
            { "write-aci" }
            );
        if (!old_asn1_path.empty())
          ixxx::posix::setenv("ASN1_PATH", old_asn1_path, true);
      }

    BOOST_AUTO_TEST_SUITE_END() // write_aci

  BOOST_AUTO_TEST_SUITE_END() // command

BOOST_AUTO_TEST_SUITE_END() // bed_
