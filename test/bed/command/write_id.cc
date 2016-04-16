#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>


#include <test/bed/helper.hh>

#include <bed/command.hh>

BOOST_AUTO_TEST_SUITE(bed_)

  BOOST_AUTO_TEST_SUITE(command)

    BOOST_AUTO_TEST_SUITE(write_id)

      BOOST_AUTO_TEST_CASE(basic)
      {
        compare_bed_output("tap_3_12_strip.asn1", "tap_3_12_valid.ber",
            "write_identity.ber", "../../../in/tap_3_12_valid.ber",
            { "write-id" }
           );
      }

    BOOST_AUTO_TEST_SUITE_END() // write_id

  BOOST_AUTO_TEST_SUITE_END() // command

BOOST_AUTO_TEST_SUITE_END() // bed_
