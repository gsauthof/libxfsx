#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>


#include <test/bed/helper.hh>

#include <bed/command.hh>
#include <bed/command/compute_aci.hh>

BOOST_AUTO_TEST_SUITE(bed_)


  BOOST_AUTO_TEST_SUITE(command)


    BOOST_AUTO_TEST_SUITE(compute_aci)

      BOOST_AUTO_TEST_CASE(basic)
      {
        const char ref[] =R"(<AuditControlInfo>
    <EarliestCallTimeStamp>
        <LocalTimeStamp>20140301140342</LocalTimeStamp>
        <UtcTimeOffset>+0200</UtcTimeOffset>
    </EarliestCallTimeStamp>
    <LatestCallTimeStamp>
        <LocalTimeStamp>20140302151252</LocalTimeStamp>
        <UtcTimeOffset>-0500</UtcTimeOffset>
    </LatestCallTimeStamp>
    <TotalCharge>71200</TotalCharge>
    <TotalTaxValue>0</TotalTaxValue>
    <TotalDiscountValue>0</TotalDiscountValue>
    <CallEventDetailsCount>5</CallEventDetailsCount>
</AuditControlInfo>
)";
        compare_bed_output("tap_3_12_strip.asn1",
            "tap_3_12_timestamps.ber", "compute_aci.xml",
            { "compute-aci" },
            ref, ref+sizeof(ref)-1);
      }

    BOOST_AUTO_TEST_SUITE_END() // compute_aci

  BOOST_AUTO_TEST_SUITE_END() // command

BOOST_AUTO_TEST_SUITE_END() // bed_
