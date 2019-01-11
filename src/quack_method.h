//
// Created by Zayd Hammoudeh on 10/4/18.
//

#ifndef PROJECT02_QUACK_METHODS_H
#define PROJECT02_QUACK_METHODS_H


#include <map>
#include <string>
#include <iostream>

#include "keywords.h"
#include "ASTNode.h"
#include "quack_param.h"
#include "symbol_table.h"
#include "initialized_list.h"

namespace CodeGen { class Gen; }

namespace Quack {
  // Forward declarations
  class TypeChecker;
  class Class;
  class Program;

  class Method {
    friend class TypeChecker;
    friend class Quack::Class;
    friend class Quack::Program;
    friend class CodeGen::Gen;
   public:
    class Container : public MapContainer<Method> {
     public:
      const void print_original_src(unsigned int indent_depth) override {
        MapContainer<Method>::print_original_src_(indent_depth, "\n");
      }
    };

    Method(std::string name, const std::string &return_type,
           Param::Container* params, AST::Block* block)
      : name_(std::move(name)), params_(params),
        return_type_name_(return_type.empty()? CLASS_NOTHING : return_type), block_(block) { };

    ~Method() {
      delete params_;
      delete block_;
      delete symbol_table_;
      delete init_list_;
    }
    /**
     * Debug method used to print the original source code.
     * @param indent_depth Amount of tabs to indent the block.
     */
    void print_original_src(unsigned int indent_depth = 0) {
      std::string indent_str = std::string(indent_depth, '\t');
      std::cout << indent_str << KEY_DEF << " " << name_ << "(" <<  std::flush;
      params_->print_original_src(0);
      std::cout << ")";

      if (!return_type_name_.empty())
        std::cout << " " << ": " << return_type_name_;

      std::cout << " {\n";
      block_->print_original_src(indent_depth + 1);
      std::cout << (!block_->empty() ? "\n" : "") << indent_str << "}";
    }
    /** Name of the method */
    const std::string name_;
    /** Type of the return object */
    Class* return_type_;

    Param::Container* params_;

    Symbol::Table* symbol_table_ = nullptr;

    InitializedList* init_list_ = nullptr;
    /** Name of the return type of the method (if any) */
    const std::string return_type_name_;
    /** Class of the object associated with the method */
    Class * obj_class_ = nullptr;
   private:
    /** Statements (if any) to perform in method */
    AST::Block* block_ = nullptr;
  };
}

#endif //PROJECT02_QUACK_METHODS_H
