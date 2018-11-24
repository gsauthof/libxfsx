#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>


#include <test/bed/helper.hh>


BOOST_AUTO_TEST_SUITE(bed_)

  BOOST_AUTO_TEST_SUITE(command)

    BOOST_AUTO_TEST_SUITE(write_ber)

      BOOST_AUTO_TEST_CASE(basic)
      {
        compare_bed_output("tap_3_12_strip.asn1",
            "../ref/ber_pretty_xml/tap_3_12_valid.xml",
            "write_ber.ber",
            "../../../in/tap_3_12_valid.ber",
            { "write-ber" }
           );
      }

    BOOST_AUTO_TEST_SUITE_END() // write_ber

  BOOST_AUTO_TEST_SUITE_END() // command

BOOST_AUTO_TEST_SUITE_END() // bed_
