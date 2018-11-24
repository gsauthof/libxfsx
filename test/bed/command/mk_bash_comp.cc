#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include <test/test.hh>
#include <test/bed/helper.hh>

#include <ixxx/util.hh>

#include <algorithm>

namespace bf = boost::filesystem;
using namespace std;

BOOST_AUTO_TEST_SUITE(bed_)

  BOOST_AUTO_TEST_SUITE(command)

    BOOST_AUTO_TEST_SUITE(mk_bash_comp)

      BOOST_AUTO_TEST_CASE(basic)
      {
        bf::path out(test::path::out());
        out /= "bed/command";
        out /= "mk_bash_comp.sh";
        bf::remove(out);
        bf::create_directories(out.parent_path());
        run_bed({"bed", "mk-bash-comp", "-o", out.generic_string() });

        BOOST_TEST_CHECKPOINT("Checking output: " << out);
        BOOST_REQUIRE(bf::file_size(out));
        auto f = ixxx::util::mmap_file(out.generic_string());
        const char q[] = "complete -F _bed bed";
        BOOST_CHECK(search(f.s_begin(), f.s_end(), q, q+sizeof(q)-1) != f.s_end());
      }

    BOOST_AUTO_TEST_SUITE_END() // mk_bash_comp

  BOOST_AUTO_TEST_SUITE_END() // command

BOOST_AUTO_TEST_SUITE_END() // bed_

