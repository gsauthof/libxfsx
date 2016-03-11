#ifndef TEST_BED_HELPER_HH
#define TEST_BED_HELPER_HH

#include <string>
#include <vector>

    void compare_bed_output(
        const std::string &asn1_filename,
        const std::string &input_filename,
        const std::string &output_filename,
        const std::vector<std::string> &args,
        const char *ref_begin,
        const char *ref_end);

    void compare_bed_output(
        const std::string &asn1_filename,
        const std::string &input_filename,
        const std::string &output_filename,
        const std::string &ref_filename,
        const std::vector<std::string> &args);

    void compare_bed_output(
        const std::string &asn1_filename,
        const std::string &input_filename,
        const std::string &output_filename,
        const std::vector<std::string> &args);

#endif
