//
// Created by Zayd Hammoudeh on 11/7/18.
//

#ifndef PROJECT02_EXCEPTIONS_H
#define PROJECT02_EXCEPTIONS_H

#include <exception>
#include <string>
#include <sstream>


struct BaseCompilerException : public std::exception {
  BaseCompilerException(const char *type, const char *error) {
    std::stringstream ss;
    ss << "(" << type << "): " << error;
    msg_ = ss.str();
  }

  BaseCompilerException(const char *type, const std::string &error)
          : BaseCompilerException(type, error.c_str()) {}

  virtual const char * what() const throw () {
    return msg_.c_str();
  };

 protected:
  std::string msg_;
};

//=====================================================================================//
//                    Exceptions for the Different Compiler Stages                     //
//=====================================================================================//

struct ScannerException : public BaseCompilerException {
  ScannerException(const char *error)
      : BaseCompilerException("Scanner", error) {}

  ScannerException(const std::string &error)
      : ScannerException(error.c_str()) {}
};


struct ParserException : public BaseCompilerException {
  ParserException(const char *error)
      : BaseCompilerException("Parser", error) {}

  ParserException(const std::string &error)
      : ParserException(error.c_str()) {}
};


struct TypeCheckerException : public BaseCompilerException {
  TypeCheckerException(const char *type, const char *error)
        : BaseCompilerException(type, error) {}

  TypeCheckerException(const char *type, const std::string &error)
      : TypeCheckerException(type, error.c_str()) {}
};

//=====================================================================================//
//               Three Primary Classes of Type Checker Exceptions                      //
//=====================================================================================//

struct ClassHierarchyException : public TypeCheckerException {
  ClassHierarchyException(const char *type, const char *error)
      : TypeCheckerException(type, error) {}

  ClassHierarchyException(const char *type, const std::string &error)
      : TypeCheckerException(type, error.c_str()) {}
};


struct InitializeBeforeUseException : public TypeCheckerException {
  InitializeBeforeUseException(const char *type, const char *error)
      : TypeCheckerException(type, error) {}

  InitializeBeforeUseException(const char *type, const std::string &error)
      : TypeCheckerException(type, error) {}
};

struct TypeInferenceException : TypeCheckerException {
  explicit TypeInferenceException(const char* error_name, const char* msg)
      : TypeCheckerException(error_name, msg) {}

  explicit TypeInferenceException(const char* error_name, const std::string &msg)
      : TypeCheckerException(error_name, msg.c_str()) {}
};

//=====================================================================================//
//                         Well Formed Hierarchy Exceptions                            //
//=====================================================================================//

struct CyclicInheritenceException : public ClassHierarchyException {
  CyclicInheritenceException(const char *type, const char *error)
      : ClassHierarchyException(type, error) {}

  CyclicInheritenceException(const char *type, const std::string &error)
      : CyclicInheritenceException(type, error.c_str()) {}
};

struct InheritedMethodReturnTypeException : public ClassHierarchyException {
  InheritedMethodReturnTypeException(const std::string &class_name, const std::string &method_name)
      : ClassHierarchyException("InheritedMethodType", build_error_msg(class_name, method_name)) {}

 private:
  static std::string build_error_msg(const std::string &class_name, const std::string &method_name){
    return "Class \"" + class_name + "\" has method \"" + method_name + "\" whose return type "
           + "is not a subtype of its super class.";
  }
};

struct InheritedMethodParamCountException : public ClassHierarchyException {
  InheritedMethodParamCountException(const std::string &class_name, const std::string &method_name)
      : ClassHierarchyException("InheritedMethodParam", build_error_msg(class_name, method_name)) {}

 private:
  static std::string build_error_msg(const std::string &class_name, const std::string &method_name){
    return "Class \"" + class_name + "\" has method \"" + method_name + "\" whose parameter count "
           + "does not match its super class.";
  }
};

struct InheritedMethodParamTypeException : public ClassHierarchyException {
  InheritedMethodParamTypeException(const std::string &class_name, const std::string &method_name,
                                    const std::string &param_name)
      : ClassHierarchyException("InheritedMethodParam",
                                build_error_msg(class_name, method_name, param_name)) {}

 private:
  static std::string build_error_msg(const std::string &class_name, const std::string &method_name,
                                     const std::string &param_name){
    return "Class \"" + class_name + "\" has method \"" + method_name + "\" whose parameter \""
           + param_name + "\" does not match its super class parameter type.";
  }
};

struct UnknownTypeException : public ClassHierarchyException {
  explicit UnknownTypeException(const std::string &class_name)
      : ClassHierarchyException("TypeError", build_error_msg(class_name)) {}

  explicit UnknownTypeException(const char *type, const std::string &msg)
      : ClassHierarchyException("TypeError", msg) {}

 private:
  static std::string build_error_msg(const std::string &class_name) {
    return "Unknown class \"" + class_name + "\"";
  }
};


class ReturnAllPathsException : public TypeCheckerException {
 public:
  explicit ReturnAllPathsException(const std::string &type_name, const std::string &method_name)
      : TypeCheckerException("MissingReturnMismatch", build_error_msg(type_name, method_name)) {}
 private:
  static std::string build_error_msg(const std::string &type_name, const std::string &method_name) {
    return "Method " + method_name + " for class " + type_name + " does not have a return on all "
           + " paths.  Cannot append \"return none\" without causing a type errorr.";
  }
};

class UnknownSymbolException : public TypeCheckerException {
 public:
  explicit UnknownSymbolException(const std::string &symbol_name)
      : TypeCheckerException("SymbolError", build_error_msg(symbol_name)) {}
 private:
  static std::string build_error_msg(const std::string &symbol_name) {
    return "Unknown symbol \"" + symbol_name + "\"";
  }
};


struct AmbiguousInferenceException : TypeCheckerException {
  AmbiguousInferenceException(const char *type, const char *error)
    : TypeCheckerException(type, error) {}
};


struct UnitializedVarException : public InitializeBeforeUseException {
  UnitializedVarException(const char *type, const std::string &var_name, bool is_field)
                   : InitializeBeforeUseException(type, build_error_msg(var_name, is_field)) { }
 private:
  static std::string build_error_msg(const std::string &var_name, bool is_field) {
    std::stringstream ss;
    ss << (is_field ? "Field v" : "V") << "ariable \"" << var_name
       << "\" is used before initialization.";
    return ss.str();
  }
};

struct DuplicateMemberException : InitializeBeforeUseException {
  explicit DuplicateMemberException(const std::string &type_name, const std::string &field_name)
      : InitializeBeforeUseException("ClassError",
                                     "Class \"" + type_name + "\" has duplicate field and method "
                                     + field_name) {}
};

struct FieldClassMatchException : InitializeBeforeUseException {
  explicit FieldClassMatchException(const std::string &field_name)
      : InitializeBeforeUseException("ClassError",
                                     "Class \"" + field_name + "\" has a field of the same name") {}
};

struct MissingSuperFieldsException : public TypeCheckerException {
  explicit MissingSuperFieldsException(const std::string &class_name)
                            : TypeCheckerException("MissingField", build_error_msg(class_name)) { }
 private:
  static const std::string build_error_msg(const std::string &class_name) {
    return "Class \"" + class_name + "\" missing fields from its super class.";
  }
};

struct MethodClassNameCollisionException : public TypeCheckerException {
  explicit MethodClassNameCollisionException(const std::string &name)
      : TypeCheckerException("NameCollision", build_error_msg(name)) { }
 private:
  static const std::string build_error_msg(const std::string &name) {
    return "\"" + name + "\" is both a class and method name.";
  }
};

struct SubTypeFieldTypeException : public TypeCheckerException {
  explicit SubTypeFieldTypeException(const std::string &class_name, const std::string &field_name)
      : TypeCheckerException("SubtypeFieldType", build_error_msg(class_name, field_name)) { }

 private:
  static std::string build_error_msg(const std::string &class_name, const std::string &field_name) {
    return "Class " + class_name + " field \"" + field_name + "\" type not subtype of super class.";
  }
};


struct DuplicateParamException : public TypeCheckerException {
  explicit DuplicateParamException(const std::string &param_name)
                  : TypeCheckerException("DuplicateParam",
                                         "Duplicate parameter \"" + param_name + "\"") {}
};


//=====================================================================================//
//                            Type Inference Exceptions                                //
//=====================================================================================//


struct UnknownConstructorException : TypeInferenceException {
  explicit UnknownConstructorException(const std::string &type_name)
      : TypeInferenceException("ClassError",
                               "Unknown class for constructor \"" + type_name + "\"") {}
};

struct UnknownBinOpException : public TypeInferenceException {
  explicit UnknownBinOpException(const std::string &op)
      : TypeInferenceException("UnknownOp",
                               "(UnknownOp): Unknown binary operator \"" + op + "\"") {}
};

#endif //PROJECT02_EXCEPTIONS_H
