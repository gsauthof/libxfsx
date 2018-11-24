#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>


#include <test/bed/helper.hh>


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
            { "search", "//Sender", "--skip", "741", "--first" },
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

  BOOST_AUTO_TEST_SUITE_END() // command

BOOST_AUTO_TEST_SUITE_END() // bed_
