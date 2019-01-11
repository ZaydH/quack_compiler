//
// Created by Zayd Hammoudeh on 11/16/18.
//

#ifndef TYPE_CHECKER_CODE_GEN_UTILS_H
#define TYPE_CHECKER_CODE_GEN_UTILS_H

#include <fstream>
#include "symbol_table.h"

// Forward Declaration
namespace Quack { class Class; }

namespace CodeGen {
  struct Settings {
    std::ofstream & fout_;
    Quack::Class * return_type_;
    Symbol::Table * st_;

    explicit Settings(std::ofstream& fout) : fout_(fout), return_type_(nullptr), st_(nullptr) {}
  };
}

#endif //TYPE_CHECKER_CODE_GEN_UTILS_H
