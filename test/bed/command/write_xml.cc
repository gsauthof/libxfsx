#include <boost/test/unit_test.hpp>
#include <test/test.hh>
#include <boost/filesystem.hpp>


#include <test/bed/helper.hh>

#include <bed/arguments.hh>

#include <xfsx/xfsx.hh>

#include <ixxx/util.hh>
#include <ixxx/ansi.hh>
#include <ixxx/posix.hh>
#include <ixxx/ixxx.hh>


// work-around Boost < 1.59 (e.g. Ubuntu 16)
#ifndef BOOST_TEST_CONTEXT
    #define BOOST_TEST_CONTEXT(A)
#endif

namespace bf = boost::filesystem;
using namespace std;

BOOST_AUTO_TEST_SUITE(bed_)

  BOOST_AUTO_TEST_SUITE(command)

    BOOST_AUTO_TEST_SUITE(write_xml)

      BOOST_AUTO_TEST_CASE(write_xml_skip)
      {
        compare_bed_output("tap_3_12_strip.asn1", "tap_3_12_valid.ber",
            "aci.xml", { "write-xml", "--skip", "740" });
      }

      BOOST_AUTO_TEST_CASE(write_xml_skip_off)
      {
        const char ref[] =
          R"(<SimChargeableSubscriber off='268'>
    <Imsi off='272'>133713371337133</Imsi>
</SimChargeableSubscriber>
)";
        compare_bed_output("tap_3_12_strip.asn1", "tap_3_12_valid.ber",
            "skip_off.xml", { "write-xml",
            "--skip", "268", "--off", "--first" },
            ref, ref + sizeof(ref) - 1);
      }

      BOOST_AUTO_TEST_CASE(write_xml_skip_not_last)
      {
        compare_bed_output("tap_3_12_strip.asn1", "tap_3_12_valid_att.ber",
            "aci_not_last.xml",
            "aci.xml", { "write-xml", "--skip", "741", "--first" });
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
          auto f = ixxx::util::mmap_file(input.generic_string());
          auto g = ixxx::util::mmap_file(input_mod.generic_string(), true, true, n);
          std::copy(f.begin(), f.end(), g.begin());
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
        auto cmd = args.create_cmd();
        cmd->execute();
        {
          BOOST_TEST_CHECKPOINT("Checking output: " << out);
          auto f = ixxx::util::mmap_file(out.generic_string());
          BOOST_REQUIRE(bf::file_size(out));
          BOOST_TEST_CHECKPOINT("Comparing: " << ref << " vs. " << out);
          auto g = ixxx::util::mmap_file(ref.generic_string());
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
        auto cmd = args.create_cmd();
        BOOST_CHECK_THROW(cmd->execute(), xfsx::Unexpected_EOC);
      }


      BOOST_AUTO_TEST_CASE(write_xml_count)
      {
        compare_bed_output("tap_3_12_strip.asn1",
            "tap_3_12_valid.ber", "count.xml",
            { "write-xml", "--count", "18" });
      }

      BOOST_AUTO_TEST_CASE(longest_prefix_match)
      {
        compare_bed_output("tap_3_12_strip.asn1",
            "tap_3_12_valid.ber", "count.xml",
            { "write-xml", "--coun", "18" });
        compare_bed_output("tap_3_12_strip.asn1",
            "tap_3_12_valid.ber", "count.xml",
            { "write-xml", "--cou", "18" });
      }

      BOOST_AUTO_TEST_CASE(longest_prefix_match_not)
      {
        BOOST_CHECK_THROW(
            compare_bed_output("tap_3_12_strip.asn1",
                "tap_3_12_valid.ber", "count.xml",
                { "write-xml", "--co", "18" }),
            bed::Argument_Error
            );
      }

      BOOST_AUTO_TEST_CASE(did_you_mean)
      {
          bool caught = false;
          try {
              compare_bed_output("tap_3_12_strip.asn1",
                    "tap_3_12_valid.ber", "count.xml",
                    { "write-xml", "--co", "18" });
          } catch (const bed::Argument_Error &e) {
              caught = true;
              string s(e.what());
              BOOST_TEST_CONTEXT(e.what()) {
                  BOOST_CHECK(s.find("Did you mean") != s.npos);
                  BOOST_CHECK(s.find("--count") != s.npos);
                  BOOST_CHECK(s.find("--command") != s.npos);
              }
          }
          BOOST_CHECK(caught);
      }

      BOOST_AUTO_TEST_CASE(did_you_mean2)
      {
          for (auto tsize : { "--t-size", "--t-siez" }) {
              bool caught = false;
              try {
                  compare_bed_output("tap_3_12_strip.asn1",
                        "tap_3_12_valid.ber", "count.xml",
                        { "write-xml", "--cou", "18", tsize });
              } catch (const bed::Argument_Error &e) {
                  caught = true;
                  string s(e.what());
                  BOOST_TEST_CONTEXT(e.what()) {
                      BOOST_CHECK(s.find("Did you mean --t_size?") != s.npos);
                  }
              }
              BOOST_CHECK(caught);
          }
      }

      BOOST_AUTO_TEST_CASE(write_xml_bci)
      {
        compare_bed_output("tap_3_12_strip.asn1",
            "tap_3_12_valid.ber", "bci.xml", "count.xml",
            { "write-xml", "--bci" });
      }


      BOOST_AUTO_TEST_CASE(write_xml_search_aci)
      {
        compare_bed_output("tap_3_12_strip.asn1",
            "tap_3_12_valid.ber", "write_xml_search_aci.xml",
            { "write-xml", "--aci"});
      }

      BOOST_AUTO_TEST_CASE(write_xml_search_aci_stop)
      {
        compare_bed_output("tap_3_12_strip.asn1",
            "tap_3_12_trailing_eoc_invalid.ber",
            "write_xml_search_aci_stop.xml",
            "write_xml_search_aci.xml",
            { "write-xml", "--aci"}
            );
      }

      BOOST_AUTO_TEST_CASE(write_xml_search)
      {
        const char ref[] =
          R"(<LocalTimeStamp off='92'>20050405090547</LocalTimeStamp>
)";
        compare_bed_output("tap_3_12_strip.asn1", "tap_3_12_valid.ber",
            "write_xml_search.xml", { "write-xml",
            "--search", "FileAvailableTimeStamp/LocalTimeStamp", "--off"},
            ref, ref + sizeof(ref) - 1);
      }

      BOOST_AUTO_TEST_CASE(write_xml_tag)
      {
        const char ref[] =
          R"(<LocalTimeStamp tag='16' off='92'>20050405090547</LocalTimeStamp>
)";
        compare_bed_output("tap_3_12_strip.asn1", "tap_3_12_valid.ber",
            "write_xml_tag.xml", { "write-xml",
            "--skip", "92", "--off", "--tag", "--first"},
            ref, ref + sizeof(ref) - 1);
      }

      BOOST_AUTO_TEST_CASE(write_xml_class)
      {
        const char ref[] =
          R"(<LocalTimeStamp tag='16' class='APPLICATION' off='92'>20050405090547</LocalTimeStamp>
)";
        compare_bed_output("tap_3_12_strip.asn1", "tap_3_12_valid.ber",
            "write_xml_class.xml", { "write-xml",
            "--skip", "92", "--off", "--tag", "--class", "--first" },
            ref, ref + sizeof(ref) - 1);
      }

      BOOST_AUTO_TEST_CASE(write_xml_cdr)
      {
        compare_bed_output("tap_3_12_strip.asn1", "tap_3_12_valid.ber",
            "write_xml_cdr.xml", { "write-xml", "--cdr", "3" });
      }

      BOOST_AUTO_TEST_CASE(write_xml_cdr_range)
      {
        compare_bed_output("tap_3_12_strip.asn1", "tap_3_12_valid.ber",
            "write_xml_cdr_range.xml", { "write-xml", "--cdr", "3.." });
      }

      BOOST_AUTO_TEST_CASE(write_xml_search_range)
      {
        const char ref[] =
          R"(<RecEntityId>foo</RecEntityId>
<RecEntityId>baz</RecEntityId>
)";
        compare_bed_output("tap_3_12_strip.asn1", "tap_3_12_valid.ber",
            "write_xml_cdr.xml", { "write-xml","--search",
            "RecEntityInfoList/RecEntityInformation/RecEntityId[1,3]" },
            ref, ref + sizeof(ref) - 1);
      }

      BOOST_AUTO_TEST_CASE(write_xml_search_last)
      {
        const char ref[] =
          R"(<CallEventDetailsCount>4</CallEventDetailsCount>
)";
        compare_bed_output("tap_3_12_strip.asn1", "tap_3_12_valid.ber",
            "write_xml_search_last.xml", { "write-xml","--search",
            "CallEventDetailsCount" },
            ref, ref + sizeof(ref) - 1);
      }

      BOOST_AUTO_TEST_CASE(write_xml_search_first)
      {
        compare_bed_output("tap_3_12_strip.asn1", "tap_3_12_valid.ber",
            "write_xml_search_first.xml",
            "../../ber_pretty_xml/tap_3_12_valid.xml", { "write-xml","--search",
            "/TransferBatch" }
           );
      }

      BOOST_AUTO_TEST_CASE(argument_error)
      {
        const char ref[] = "";
        BOOST_CHECK_THROW(
          compare_bed_output("tap_3_12_strip.asn1", "tap_3_12_valid.ber",
              "argument_error.xml", { "write-xml","--foobar" }, ref, ref
             ), bed::Argument_Error);
        BOOST_CHECK_THROW(
          compare_bed_output("tap_3_12_strip.asn1", "tap_3_12_valid.ber",
              "argument_error.xml", { "wite-xml" }, ref, ref
             ), bed::Argument_Error);
      }

      BOOST_AUTO_TEST_CASE(stack_resize_vs_overflow)
      {
        const char ref[] = "";
        BOOST_CHECK_THROW(
          compare_bed_output("tap_3_12_strip.asn1", "deep_invalid.ber",
              "deep_invalid.xml", { "write-xml" }, ref, ref
             ), std::overflow_error);
      }

#if (defined(__MINGW32__) || defined(__MINGW64__))
#else
      BOOST_AUTO_TEST_CASE(open_with_o_trunc)
      {
        bf::path input(test::path::in());
        input /= "tap_3_12_valid.ber";
        run_bed({"bed", "write-xml", input.generic_string(), "/dev/null"});
      }
#endif

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

        compare_bed_output("", "tap_3_12_valid.ber",
            "write_xml_auto.xml", { "write-xml", "--hex", "--off", "--tag", "--class", "--tl", "--t_size", "--length" });

        if (!old_asn1_path.empty())
          ixxx::posix::setenv("ASN1_PATH", old_asn1_path, true);
      }


      BOOST_AUTO_TEST_CASE(autodetect_no_detect_raw)
      {
        string old_asn1_path;
        try { old_asn1_path = ixxx::ansi::getenv("ASN1_PATH"); }
        catch (const ixxx::getenv_error &e) {}
        ixxx::posix::setenv("ASN1_PATH", "", true);

        compare_bed_output("", "asn1c/examples/sample.source.LDAP3/sample-LDAPMessage-1.ber",
            "write_xml_auto_not.xml", { "write-xml" });

        if (!old_asn1_path.empty())
          ixxx::posix::setenv("ASN1_PATH", old_asn1_path, true);
      }

      BOOST_AUTO_TEST_CASE(pp)
      {
        string old_lua_path;
        try { old_lua_path = ixxx::ansi::getenv("ASN1_PATH"); }
        catch (const ixxx::getenv_error &e) {}
        string a {test::path::in() + "/../../libgrammar/test/in/asn1"};
        string b {test::path::in() + "/../../config"};
        string c {test::path::in() + "/../../libgrammar/grammar/xml"};
        string d {test::path::in() + "/../../telephone-code"};
        ixxx::posix::setenv("ASN1_PATH", a + ":" + b + ":" + c + ":" + d, true);
        compare_bed_output("", "tap_3_12_valid.ber",
            "write_xml_pp.xml",
            { "write-xml", "--pp" }
           );
        if (!old_lua_path.empty())
          ixxx::posix::setenv("ASN1_PATH", old_lua_path, true);
      }

      BOOST_AUTO_TEST_CASE(stop_after_first_indef)
      {
        const char ref[] =
          R"(<i tag='0' class='CONTEXT_SPECIFIC'>
    <i tag='1' class='CONTEXT_SPECIFIC'>
        <p tag='0' class='CONTEXT_SPECIFIC'>skip</p>
        <p tag='1' class='CONTEXT_SPECIFIC'>this</p>
        <p tag='2' class='CONTEXT_SPECIFIC'>.</p>
        <p tag='3' class='CONTEXT_SPECIFIC'>23</p>
    </i>
</i>
)";
        compare_bed_output("", "skip_test.ber",
            "stop_after_first_indef.xml", { "write-xml","--first" },
            ref, ref + sizeof(ref) - 1);
      }

      BOOST_AUTO_TEST_CASE(stop_after_double_def)
      {
        const char ref[] =
          R"(<c tag='0' class='CONTEXT_SPECIFIC'>
    <c tag='3' class='CONTEXT_SPECIFIC'>
        <p tag='0' class='PRIVATE'>Hello</p>
        <p tag='1' class='CONTEXT_SPECIFIC'>World</p>
    </c>
</c>
)";
        compare_bed_output("", "skip_test.ber",
            "stop_after_double_def.xml", { "write-xml","--first", "--skip", "27" },
            ref, ref + sizeof(ref) - 1);
      }

      BOOST_AUTO_TEST_CASE(stop_after_def)
      {
        const char ref[] =
          R"(<c tag='2' class='PRIVATE'>
    <p tag='2' class='PRIVATE'>dead</p>
    <p tag='3' class='PRIVATE'>beef</p>
    <p tag='4' class='PRIVATE'>1</p>
</c>
)";
        compare_bed_output("", "skip_test.ber",
            "stop_after_def.xml", { "write-xml","--first", "--skip", "45" },
            ref, ref + sizeof(ref) - 1);
      }

      BOOST_AUTO_TEST_CASE(skip_zero)
      {
        compare_bed_output("", "zero_test0.ber",
            "skip_zero.xml", { "write-xml","--skip0", "--off" });
      }

      BOOST_AUTO_TEST_CASE(zero_throw)
      {
        BOOST_CHECK_THROW(
          compare_bed_output("", "zero_test0.ber",
              "skip_zero.xml", { "write-xml", "--off" })
          , xfsx::Unexpected_EOC);
      }

      BOOST_AUTO_TEST_CASE(skip_block)
      {
        compare_bed_output("", "block_test0.ber",
              "block_zero.xml", { "write-xml", "-0", "2048", "--off" });
      }

      BOOST_AUTO_TEST_CASE(read_block)
      {
        compare_bed_output("", "block_test0.ber",
              "block_zero.xml", { "write-xml", "--block", "2048", "--off" });
      }


    BOOST_AUTO_TEST_SUITE_END() // write_xml

  BOOST_AUTO_TEST_SUITE_END() // command

BOOST_AUTO_TEST_SUITE_END() // bed_
