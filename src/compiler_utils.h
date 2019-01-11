//
// Created by Zayd Hammoudeh on 11/7/18.
//

#ifndef PROJECT02_COMPILER_UTILS_H
#define PROJECT02_COMPILER_UTILS_H

#include <string>

#include "keywords.h"
#include "exceptions.h"
#include "symbol_table.h"

namespace std {
  template<>
  struct hash<std::pair<std::string, bool>> {
    inline size_t operator()(const std::pair<std::string, bool> &v) const {
      std::hash<bool> bool_hasher;
      std::hash<std::string> string_hasher;
      return string_hasher(v.first) ^ bool_hasher(v.second);
    }
  };
}

// Forward Declaration
namespace Quack { class Class; class Method; }

namespace TypeCheck {
  struct Settings {

    Settings() : st_(nullptr), this_class_(nullptr), return_type_(nullptr), is_constructor_(false){}

    Symbol::Table * st_;
    Quack::Class * this_class_;
    Quack::Class * return_type_;
    bool is_constructor_;
  };
}

namespace Quack {
  struct Utils {
    /**
     * Standardizes printing the type checker error and then exits the program
     * @param e Exception info
     * @param exit_code Integer code with which to exit the program
     */
    static void print_exception_info_and_exit(const std::exception &e, const int exit_code) {
      std::string name = typeid(e).name();

      // C++ May prepend the class name with a number. Remove it.
      unsigned i = 0;
      while (i < name.size() && name[i] >= '0' && name[i] <= '9')
        i++;
      i = i == name.size() ? 0 : i;

      std::cerr << name.substr(i) << " | " << e.what() << std::endl;
      exit(exit_code);
    }
  };
}

#endif //PROJECT02_COMPILER_UTILS_H
