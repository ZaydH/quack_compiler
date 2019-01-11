//
// Created by Zayd Hammoudeh on 10/4/18.
//

#ifndef PROJECT02_QUACK_PROGRAM_H
#define PROJECT02_QUACK_PROGRAM_H

#include <vector>
#include <string>
#include <iostream>

#include "ASTNode.h"
#include "quack_class.h"
#include "quack_method.h"
#include "keywords.h"

namespace CodeGen { class Gen; }

namespace Quack {
  // Forward Declarations
  class TypeChecker;

  class Program {
    friend class TypeChecker;
    friend class CodeGen::Gen;

   public:
    explicit Program(Class::Container *classes, AST::Block *block) : classes_(classes) {
      main_ = new Method(METHOD_MAIN, CLASS_NOTHING, new Param::Container(), block);
    }

    ~Program() {
      //delete classes_; // Cleared when the singleton is cleared
      delete main_;
    }

    void print_original_src() {
      if (classes_)
        classes_->print_original_src(0);
      std::cout << "\n";
      if (main_ && main_->block_)
        main_->block_->print_original_src(0);
      std::cout << std::flush;
    }

   private:
    Class::Container *classes_;
    Quack::Method* main_;
  };
}

#endif //PROJECT02_QUACK_PROGRAM_H
