// Copyright 2015, Georg Sauthoff <mail@georg.so>

/* {{{ LGPLv3

    This file is part of libxfsx.

    libxfsx is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    libxfsx is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with libxfsx.  If not, see <http://www.gnu.org/licenses/>.

}}} */
#ifndef BED_ARGUMENTS_HH
#define BED_ARGUMENTS_HH

#include <string>
#include <deque>
#include <vector>
#include <stdexcept>
#include <memory>
#include <iostream>

namespace bed {

  enum class Command {
    NONE,
    WRITE_IDENTITY,
    WRITE_DEFINITE,
    WRITE_INDEFINITE,
    WRITE_XML,
    PRETTY_WRITE_XML,
    WRITE_BER,
    SEARCH_XPATH,
    VALIDATE_XSD,
    EDIT,
    COMPUTE_ACI,
    WRITE_ACI,
    MK_BASH_COMP
  };

  class Arguments;

  namespace command {

    struct Base {
        Base(const Arguments &args);
        virtual ~Base();
        virtual void execute() = 0;
      protected:
        const Arguments &args_;
    };

    struct Write_Identity : Base { using Base::Base; void execute() override; };
    struct Write_Definite : Base { using Base::Base; void execute() override; };
    struct Write_Indefinite:Base { using Base::Base; void execute() override; };
    struct Write_XML : Base { using Base::Base; void execute() override; };
    struct Pretty_Write_XML:Base { using Base::Base; void execute() override; };
    struct Write_BER : Base { using Base::Base; void execute() override; };
    struct Search_XPath : Base { using Base::Base; void execute() override; };
    struct Validate_XSD : Base { using Base::Base; void execute() override; };
    struct Edit : Base { using Base::Base; void execute() override; };
    struct Compute_ACI : Base { using Base::Base; void execute() override; };
    struct Write_ACI : Base { using Base::Base; void execute() override; };
    struct Mk_Bash_Comp : Base { using Base::Base; void execute() override; };

    namespace edit_op {

      struct Detail;

      struct Base {
        virtual ~Base();
        std::vector<std::string> argv;
        virtual void execute(Detail &d) = 0;
      };
      struct Remove    : public Base { void execute(Detail &d) override; };
      struct Replace   : public Base { void execute(Detail &d) override; };
      struct Add       : public Base { void execute(Detail &d) override; };
      struct Set_Att   : public Base { void execute(Detail &d) override; };
      struct Insert    : public Base { void execute(Detail &d) override; };
      struct Write_ACI : public Base { void execute(Detail &d) override; };
    }

  }


  class Argument_Error : public std::runtime_error {
    private:
    public:
      using std::runtime_error::runtime_error;
  };

  class Arguments {
    private:
      void parse(unsigned argc, char **argv);
      void validate();
      void autodetect_stuff();
      void create_cmd();
    public:
      Arguments();
      Arguments(unsigned argc, char **argv);

      std::string argv0;
      std::deque<std::string> positional;

      std::string  in_filename;
      std::string out_filename;
      std::deque<std::string> asn_filenames;
      std::deque<std::string> asn_search_path;
      std::string asn_config_filename;
      bool autodetect  {true};
      std::string xsd_filename;
      unsigned indent_size {4};
      bool hex_dump    {false};
      bool dump_tag    {false};
      bool dump_class  {false};
      bool dump_tl     {false};
      bool dump_t      {false};
      bool dump_length {false};
      bool dump_offset {false};
      size_t skip  {0};
      uint32_t skip_zero {0};
      uint32_t block_size {0};
      bool stop_after_first {false};
      size_t count {0};
      bool pretty_print {false};
      std::string pp_filename;

      std::string search_path;
      bool skip_to_aci {false};
      std::string kth_cdr;

      std::deque<std::string> xpaths;
      std::deque<std::unique_ptr<command::edit_op::Base> > edit_ops;

      Command  command   {Command::NONE};
      std::string command_str;
      std::unique_ptr<command::Base> cmd;
      unsigned verbosity {0};
  };
}


#endif
