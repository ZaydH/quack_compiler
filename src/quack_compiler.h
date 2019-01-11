#ifndef PROJECT01_QUACKCOMPILER_H
#define PROJECT01_QUACKCOMPILER_H

#include <string>
#include <fstream>
#include <iostream>

#include "lex.yy.h"
#include "quack_program.h"
#include "quack_class.h"
#include "code_generator.h"
#include "type_checker.h"
#include "keywords.h"
#include "compiler_utils.h"
#include "messages.h"


namespace Quack {
  class Compiler {
   public:
    Compiler() {
      initialize();
    }

    ~Compiler() {
      for (const auto &prog : progs_)
        delete prog;
    }

    /**
     * Parse the input command line arguments and configure the compiler.
     *
     * @param argc Number of input arguments
     * @param argv List of input arguments
     */
    void parse_args(unsigned int argc, char *argv[]) {
      if (argc == 1) {
        std::cerr << "Insufficient input arguments.  At least one parameter required."
                  << std::flush;
        exit(EXIT_FAILURE);
      }

      int c;
      while ((c = getopt(argc, argv, "t")) != -1) {
        if (c == 't') {
          std::cerr << "Warning: Running in debugging mode" << std::endl;
          debug_ = true;
        }
      }
      // Verify that there is at least one file to parse
      unsigned int num_files = argc - optind;
      if (num_files == 0) {
        std::cerr << "No source files specified for parsing. At least one is required."
                  << std::flush;
        exit(EXIT_FAILURE);
      }

      input_files_.reserve(num_files);
      progs_.reserve(num_files);
      for (unsigned int i = 0; i < num_files; i++)
        input_files_.emplace_back(argv[i + optind]);
    }

    void run() {
      num_errs_ = 0;
      for (const std::string &file_path : input_files_) {
        Quack::Class::Container::reset();

        std::ifstream f_in(file_path);

        // If specified file does not exist, report an error then continue
        if (!f_in) {
          std::cerr << "Unable to locate input file: " << file_path << std::endl;
          num_errs_++;
          continue;
        }

        report::reset_error_count();

        Quack::Program *prog = nullptr;
        try {
          prog = parse(f_in, file_path);
        } catch (ScannerException &e) {
          Quack::Utils::print_exception_info_and_exit(e, EXIT_SCANNER);
        } catch (ParserException &e) {
          Quack::Utils::print_exception_info_and_exit(e, EXIT_PARSER);
        }
        f_in.close();

        if (report::ok()) {
          progs_.emplace_back(prog);
        }

        auto type_checker = Quack::TypeChecker();
        type_checker.run(prog);

        CodeGen::Gen gen(prog, file_path);
        gen.run();
      }
    }

    void initialize() {
      Class::Container *classes = Class::Container::singleton();

      if (classes->empty()) {
        classes->add(new ObjectClass());
        classes->add(new IntClass());
        classes->add(new StringClass());
        classes->add(new BooleanClass());
      }
    }

   private:

    Quack::Program* parse(std::istream &f_in, const std::string &file_path) {
      yy::Lexer lexer(f_in);
      Quack::Program *prog;
      auto * parser = new yy::parser(lexer, &prog);

      if (parser->parse() != 0 || !report::ok()) {
        report::bail();
      } else {
        std::cout << "Parse successful for file: " << file_path << std::endl;
        if (debug_)
          prog->print_original_src();
      }
      delete parser;

      return prog;
    }

    /**
     * Select to run the compiler in debug mode.
     */
    bool debug_ = false;
    /**
     * Input file to be compiled.
     */
    std::vector<std::string> input_files_;
    std::vector<Quack::Program *> progs_;

    unsigned int num_errs_ = 0;
  };
}

#endif //PROJECT01_QUACKCOMPILER_H
