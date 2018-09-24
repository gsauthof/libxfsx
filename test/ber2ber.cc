#include "ber2ber.hh"

#include <boost/test/unit_test.hpp>
#include <boost/test/parameterized_test.hpp>
#include <boost/filesystem.hpp>

#include <ixxx/ixxx.hh>
#include <ixxx/util.hh>

#include <array>
#include <algorithm>
#include <iostream>


#include <xfsx/ber2ber.hh>
#include <xfsx/xfsx.hh>

#include "test.hh"

using namespace std;

namespace bf = boost::filesystem;

using u8 = xfsx::u8;

static void compare_identity(const char *rel_filename_str)
{
  bf::path rel_filename(rel_filename_str);
  bf::path rel_filename_parent(rel_filename.parent_path());
  bf::path filename(rel_filename.filename());
  bf::path out_filename(filename.stem());
  out_filename += ".ber";

  bf::path in_path(test::path::in());
  in_path /= rel_filename_parent;
  bf::path in(in_path);
  in /= filename;

  bf::path suffix("identity");
  bf::path out_path(test::path::out());
  out_path /= suffix;
  out_path /= rel_filename_parent;
  bf::path out(out_path);
  out /= out_filename;

  BOOST_TEST_MESSAGE("in path: " << in_path
      <<  ", out path: " << out_path);
  BOOST_TEST_MESSAGE("in: " << in << ", out: " << out);

  BOOST_TEST_CHECKPOINT("remove");
  bf::remove(out);
  BOOST_TEST_CHECKPOINT("create directories");
  bf::create_directories(out_path);

    BOOST_TEST_CHECKPOINT("map file" << in.generic_string());
    auto f = ixxx::util::mmap_file(in.generic_string());
    BOOST_TEST_CHECKPOINT("create write file");
    {
      ixxx::util::FD fd(out.generic_string(), O_CREAT | O_WRONLY, 0666);
      ixxx::posix::ftruncate(fd, f.size());
    }
    BOOST_TEST_CHECKPOINT("open write file");
    auto o = ixxx::util::mmap_file(out.generic_string(), true, true);
    BOOST_REQUIRE_EQUAL(f.size(), o.size());
    BOOST_TEST_CHECKPOINT("write file");
    xfsx::ber::write_identity(f.begin(), f.end(), o.begin(), o.end());
    BOOST_TEST_CHECKPOINT("comparing file");
    bool are_equal = std::equal(f.begin(), f.end(), o.begin(), o.end());
    if (!are_equal)
      cerr << "Files are not equal: " << in << " vs. " << out << '\n';
    BOOST_CHECK(are_equal);

}

static unsigned read_for_throw(const u8 *begin, const u8 *end)
{
  using namespace xfsx;
  Vertical_Reader r(begin, end);
  unsigned i = 0;
  for (auto &x : r) {
    i += x.length;
  }
  return i;
}


static void check_vert_read_throw(const char *rel_filename_str)
{
  bf::path rel_filename(rel_filename_str);
  bf::path in(test::path::in());
  in /= rel_filename;

  BOOST_TEST_CHECKPOINT("map file" << in.generic_string());
  auto f = ixxx::util::mmap_file(in.generic_string());

  unsigned i = 0;
  BOOST_CHECK_THROW(i = read_for_throw(f.begin(), f.end()),
      std::exception);
  BOOST_CHECK_EQUAL(i, 0u);
}

static void check_flat_no_throw(const char *rel_filename_str)
{
  bf::path rel_filename(rel_filename_str);
  bf::path in(test::path::in());
  in /= rel_filename;

  BOOST_TEST_CHECKPOINT("map file" << in.generic_string());
  auto f = ixxx::util::mmap_file(in.generic_string());

  using namespace xfsx;
  Reader r(f.begin(), f.end());
  unsigned i = 0;
  for (auto &x : r) {
    i += x.tl_size;
  }
  BOOST_CHECK(i > 0);
}

static void check_reference(
    const char *suffix,
    std::function<void(const u8 *, const u8 *, const string &)> fn,
    const char *rel_filename_str)
{
  bf::path rel_filename(rel_filename_str);
  bf::path in(test::path::in());
  in /= rel_filename;

  bf::path out(test::path::out());
  out /= suffix;
  out /= rel_filename;
  out.replace_extension("out");

  bf::path ref(test::path::ref());
  ref /= suffix;
  ref /= rel_filename;
  ref.replace_extension("out");

  BOOST_TEST_MESSAGE("in: " << in);
  BOOST_TEST_MESSAGE("out: " << out);

  BOOST_TEST_CHECKPOINT("remove: " << out);
  bf::remove(out);
  BOOST_TEST_CHECKPOINT("create directory: " << out.parent_path());
  bf::create_directories(out.parent_path());

  BOOST_TEST_CHECKPOINT("map file: " << in.generic_string());
  auto f = ixxx::util::mmap_file(in.generic_string());

  BOOST_TEST_CHECKPOINT("write to: " << in.generic_string());
  fn(f.begin(), f.end(), out.generic_string());

  BOOST_TEST_CHECKPOINT("comparing files: " << ref << " vs. " << out);
  if (bf::file_size(ref)) {
    // mmap of zero-length file fails as specified by POSIX ...
    auto r = ixxx::util::mmap_file(ref.generic_string());
    auto o = ixxx::util::mmap_file(out.generic_string());
    bool are_equal = std::equal(r.begin(), r.end(), o.begin(), o.end());
    if (!are_equal) {
      cerr << "Files are not equal: " << ref << " vs. " << out << "\n";
    }
    BOOST_CHECK(are_equal);
  } else {
    BOOST_CHECK(!bf::file_size(out));
  }
}

static void write_indef(const u8 *begin, const u8 *end,
    const string &filename)
{
  xfsx::ber::write_indefinite(begin, end, filename);

  // size_t written = 0;
  // {
  //   ixxx::util::FD fd(filename, O_CREAT | O_WRONLY, 0666);
  //   ixxx::posix::ftruncate(fd, (end-begin)*2);
  // }
  // ixxx::util::RW_Mapped_File g(filename);
  // auto r = xfsx::ber::write_indefinite(begin, end, g.begin(), g.end());
  // written = r - g.begin();
  // boost::filesystem::resize_file(filename, written);
}

static void compare_indefinite(const char *rel_filename_str)
{
  check_reference("indefinite", write_indef, rel_filename_str);
}

static void write_def(const u8 *begin, const u8 *end,
    const string &filename)
{
  xfsx::ber::write_definite(begin, end, filename);
}

static void compare_definite(const char *rel_filename_str)
{
  check_reference("definite", write_def, rel_filename_str);
}

boost::unit_test::test_suite *create_ber2ber_suite()
{
  const array<const char *, 15> filenames = {
      "asn1c/asn1c/tests/data-62/data-62-01.ber",
      "asn1c/asn1c/tests/data-62/data-62-10.ber",
      "asn1c/asn1c/tests/data-62/data-62-11.ber",
      "asn1c/asn1c/tests/data-62/data-62-14.ber",
      "asn1c/asn1c/tests/data-62/data-62-16.ber",
      "asn1c/asn1c/tests/data-62/data-62-20.ber",
      "asn1c/asn1c/tests/data-62/data-62-22.ber",
      "asn1c/asn1c/tests/data-62/data-62-25.ber",
      "asn1c/asn1c/tests/data-62/data-62-27.ber",
      "asn1c/asn1c/tests/data-62/data-62-32.ber",
      "asn1c/examples/sample.source.LDAP3/sample-LDAPMessage-1.ber",
      "asn1c/examples/sample.source.PKIX1/sample-Certificate-1.der",
      "asn1c/examples/sample.source.TAP3/sample-DataInterChange-1.ber",
      // bad, but still readable on a lexical level
      "asn1c/asn1c/tests/data-62/data-62-13-B.ber",
      "asn1c/asn1c/tests/data-62/data-62-15-B.ber"

  };
  const array<const char*, 13> bad_filenames = {
    "asn1c/asn1c/tests/data-62/data-62-02-B.ber",
    "asn1c/asn1c/tests/data-62/data-62-03-B.ber",
    "asn1c/asn1c/tests/data-62/data-62-04-B.ber",
    "asn1c/asn1c/tests/data-62/data-62-05-B.ber",
    "asn1c/asn1c/tests/data-62/data-62-06-B.ber",
    "asn1c/asn1c/tests/data-62/data-62-07-B.ber",
    "asn1c/asn1c/tests/data-62/data-62-12-B.ber",
    "asn1c/asn1c/tests/data-62/data-62-17-B.ber",
    "asn1c/asn1c/tests/data-62/data-62-18-B.ber",
    "asn1c/asn1c/tests/data-62/data-62-19-B.ber",
    "asn1c/asn1c/tests/data-62/data-62-21-B.ber",
    "asn1c/asn1c/tests/data-62/data-62-23-B.ber",
    "asn1c/asn1c/tests/data-62/data-62-26-B.ber"
  };
  const array<const char*, 9> bad_but_flat_ok_filenames = {
    "asn1c/asn1c/tests/data-62/data-62-02-B.ber",
    "asn1c/asn1c/tests/data-62/data-62-03-B.ber",
    "asn1c/asn1c/tests/data-62/data-62-04-B.ber",
    "asn1c/asn1c/tests/data-62/data-62-05-B.ber",
    "asn1c/asn1c/tests/data-62/data-62-06-B.ber",
    "asn1c/asn1c/tests/data-62/data-62-17-B.ber",
    "asn1c/asn1c/tests/data-62/data-62-21-B.ber",
    "asn1c/asn1c/tests/data-62/data-62-23-B.ber",
    "asn1c/asn1c/tests/data-62/data-62-26-B.ber"
  };

  auto xfsx_       = BOOST_TEST_SUITE("xfsx_b2b");
  auto ber2ber     = BOOST_TEST_SUITE("ber2ber");
  auto ber2ber_flat= BOOST_TEST_SUITE("ber2ber_flat");
  auto ber_fail    = BOOST_TEST_SUITE("ber_fail");
  auto ber_no_fail = BOOST_TEST_SUITE("ber_no_fail");
  auto indef       = BOOST_TEST_SUITE("indef");
  auto def         = BOOST_TEST_SUITE("def");

  ber2ber->add(BOOST_PARAM_TEST_CASE(&compare_identity,
        filenames.begin(), filenames.end()));

  ber2ber_flat->add(BOOST_PARAM_TEST_CASE(&compare_identity,
        bad_but_flat_ok_filenames.begin(), bad_but_flat_ok_filenames.end()));

  ber_fail->add(BOOST_PARAM_TEST_CASE(&check_vert_read_throw,
        bad_filenames.begin(), bad_filenames.end()));
  ber_no_fail->add(BOOST_PARAM_TEST_CASE(&check_flat_no_throw,
        bad_but_flat_ok_filenames.begin(), bad_but_flat_ok_filenames.end()));

  indef->add(BOOST_PARAM_TEST_CASE(&compare_indefinite,
        filenames.begin(), filenames.end()));
  def->add(BOOST_PARAM_TEST_CASE(&compare_definite,
        filenames.begin(), filenames.end()));
  xfsx_->add(ber2ber);
  xfsx_->add(ber2ber_flat);
  xfsx_->add(ber_fail);
  xfsx_->add(ber_no_fail);
  xfsx_->add(indef);
  xfsx_->add(def);
  return xfsx_;
}

