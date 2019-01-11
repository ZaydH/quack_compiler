//
// Created by Zayd Hammoudeh on 11/15/18.
//

#ifndef TYPE_CHECKER_CODE_GENERATOR_H
#define TYPE_CHECKER_CODE_GENERATOR_H

#include <string>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <stack>

#include "quack_program.h"
#include "quack_class.h"
#include "quack_param.h"
#include "compiler_utils.h"
#include "ASTNode.h"

namespace CodeGen {
  class Gen {
   public:

    Gen(Quack::Program * prog, const std::string &quack_filename) : prog_(prog) {
      #ifdef _WIN32
        char file_sep = '\\';
      #else
        char file_sep = '/';
      #endif
      std::size_t per_loc = quack_filename.rfind('.');
      std::size_t slash_loc = quack_filename.rfind(file_sep);

      // Preserve path and filename for the generated code
      if (per_loc==std::string::npos || (slash_loc != std::string::npos && per_loc < slash_loc)) {
        output_file_path_ = quack_filename;
      } else if (per_loc == 0 || (slash_loc != std::string::npos && per_loc == slash_loc + 1)) {
        throw std::runtime_error("It appears you have only file extension and no file name");
      } else {
        output_file_path_ = quack_filename.substr(0, per_loc);
      }
      output_file_path_ += ".c";

      fout_.open(output_file_path_);
    }

    ~Gen() {
      fout_.close();
    }
    /**
     * Generates the output file associated with the specified program.
     */
    void run() {
      export_includes();

      std::vector<Quack::Class*> user_classes = topologically_sort_classes();

      CodeGen::Settings settings(fout_);
      for (auto q_class : user_classes)
        q_class->generate_code(settings);

      export_main(settings);
      std::cout << "Code generation completed successfully." << std::endl;
    }

   private:
    /**
     * Classes are topologically sorted.  This is needed to ensure that inherited classes
     * have the functions of their super classes already defined in the generated code.
     * This approach relies on a stack to ensure super classes are found before sub classes
     *
     * @return Tpologically sorted classes
     */
    static std::vector<Quack::Class*> topologically_sort_classes() {
      std::vector<Quack::Class*> user_classes;
      for (auto & class_pair : *Quack::Class::Container::singleton()) {
        std::stack<Quack::Class*> stack;
        Quack::Class* q_class = class_pair.second;
        if (!q_class->is_user_class())
          continue;
        do {
          bool found = false;
          for (auto * temp_class : user_classes) {
            if (temp_class->name_ == q_class->name_) {
              found = true;
              break;
            }
          }

          if (found)
            break;

          stack.push(q_class);
          q_class = q_class->super_;
        } while(q_class && q_class->is_user_class());

        // Add all new classes with super classes first
        while(!stack.empty()) {
          user_classes.emplace_back(stack.top());
          stack.pop();
        }
      }
      return user_classes;
    }
    /** Write any includes to the beginning of the generated file. */
    void export_includes() {
      std::pair<std::string, bool> libs[] = {{"stdlib", false},
                                             {"stdio", false},
                                             {"stdbool", false},
                                             {"builtins", true}};
      for (auto &lib_pair : libs) {
        fout_ << "#include " << (lib_pair.second ? "\"" : "<")
              << lib_pair.first << ".h" << (lib_pair.second ? "\"" : ">") << "\n";
      }
      fout_ << std::endl;
    }
    /**
     * Helper function to generate the C code associated with the main function call.
     *
     * @param main_subfunc_name Name of the function to be called inside main.
     */
    void generate_main(CodeGen::Settings settings, const std::string &main_subfunc_name) {
      Quack::Class * nothing_class = Quack::Class::Container::Nothing();

      fout_ << "\n"
            << nothing_class->generated_object_type_name() << " " << main_subfunc_name << "() {\n";

      settings.return_type_ = Quack::Class::Container::Nothing();
      settings.st_ = prog_->main_->symbol_table_;

      Quack::Class::generate_symbol_table(settings, 1, prog_->main_);
      AST::ASTNode::generate_one_line_comment(settings, 1, "main Method Body");
      prog_->main_->block_->generate_code(settings, 0);

      fout_ << AST::ASTNode::indent_str(1) << "return none;\n"
            << "}" << std::endl;

      settings.return_type_ = nullptr;
      settings.st_ = nullptr;
    }
    /** Writes the main() function to the output file. */
    void export_main(CodeGen::Settings settings) {
      generate_main(settings, METHOD_MAIN);

      fout_ << "\n" << "int main() {"
            << "\n" << AST::ASTNode::indent_str(1) << METHOD_MAIN << "();\n"
            << "}" << std::endl;
    }
    /** Location to which the generated code is written */
    std::string output_file_path_;
    /** Filestream where the generated code is written */
    std::ofstream fout_;

    const Quack::Program * prog_;
  };
}

#endif //TYPE_CHECKER_CODE_GENERATOR_H
