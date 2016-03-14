#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>


#include <test/bed/helper.hh>

#include <bed/command.hh>
#include <bed/command/write_aci.hh>

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

    BOOST_AUTO_TEST_SUITE_END() // compute_aci

  BOOST_AUTO_TEST_SUITE_END() // command

BOOST_AUTO_TEST_SUITE_END() // bed_
