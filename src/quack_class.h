//
// Created by Zayd Hammoudeh on 10/4/18.
//

#ifndef PROJECT02_QUACK_CLASSES_H
#define PROJECT02_QUACK_CLASSES_H

#include <map>
#include <cstring>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <fstream>

#include "container_templates.h"
#include "quack_method.h"
#include "quack_field.h"
#include "keywords.h"

// Forward declaration
namespace CodeGen{ class Gen; }

namespace Quack {

  // Forward declaration
  class TypeChecker;

  class Class {
    friend class TypeChecker;
    friend class CodeGen::Gen;
   public:

    class Container : public MapContainer<Class> {
     public:
      /**
       * Singleton object containing all of the class information.
       *
       * @return Pointer to a set of classes.
       */
      static Container* singleton();
      /**
       * Empty all stored classes in the singleton.
       */
      static void reset() {
        Container* classes = singleton();
        for (auto &pair : *classes)
          delete pair.second;
        classes->clear();
      }
      /**
       * Prints the user defined classes only.
       *
       * @param indent_depth Depth to tab the contents.
       */
      const void print_original_src(unsigned int indent_depth) override {
        auto * print_class = new Container();

        for (const auto &pair : objs_) {
          if (pair.first == CLASS_OBJ || pair.first == CLASS_INT
              || pair.first == CLASS_STR || pair.first == CLASS_BOOL)
            continue;
          print_class->add(pair.second);
        }
        print_class->MapContainer<Class>::print_original_src_(indent_depth, "\n\n");
      }
      /**
       * Static accessor to get the Integer class.
       *
       * @return Integer class reference.
       */
      static Class* Int() {
        return singleton()->get(CLASS_INT);
      }
      /**
       * Static accessor to get the Boolean class.
       *
       * @return Boolean class reference.
       */
      static Class* Bool() {
        return singleton()->get(CLASS_BOOL);
      }
      /**
       * Static accessor to get the Boolean class.
       *
       * @return Boolean class reference.
       */
      static Class* Nothing() {
        return singleton()->get(CLASS_NOTHING);
      }
      /**
       * Static accessor to get the String class.
       *
       * @return String class reference.
       */
      static Class* Str() {
        return singleton()->get(CLASS_STR);
      }
      /**
       * Static accessor to get the Object class.
       *
       * @return Object class reference
       */
      static Class* Obj() {
        return singleton()->get(CLASS_OBJ);
      }

      Container(Container const&) = delete;       // Don't Implement
      Container& operator=(Container const&) = delete;  // Don't implement
     private:
      /**
       * Private constructor since it is a singleton.
       */
      Container() : MapContainer<Class>() {};          // Don't implement
    };

    Class(char* name, char * super_type, Param::Container* params,
          AST::Block* constructor, Method::Container* methods)
      : name_(name), super_type_name_(strcmp(super_type, "") == 0 ? CLASS_OBJ : super_type),
        super_(OBJECT_NOT_FOUND), methods_(methods), gen_methods_(nullptr), gen_fields_(nullptr) {

      fields_ = new Field::Container();

      // Constructor has same function name as the class
      constructor_ = new Method(name, this->name_, params, constructor);
      constructor_->return_type_ = this;

      for (const auto &method_info : *methods_) {
        Quack::Method * method = method_info.second;
        method->obj_class_ = this;
//        if (name_ == method->name_)
//          throw ParserException("Method name cannot match class name for class " + name_);
      }

//      Container* classes = Container::singleton();
//      if (classes->exists(name))
//        throw("Duplicate class named \"" + name_ + "\"");

//      if (name_ == CLASS_NOTHING)
//        throw ParserException("Invalid class name \"" + name_ + "\"");
    }
    /**
     * Clear all dynamic memory in the object.
     */
    virtual ~Class() {
      delete constructor_;
      delete methods_;
      delete fields_;

      delete gen_methods_;
      delete gen_fields_;
    }
    /**
     * Checks whether this class (or any of its super classes) has a field with the specified name.
     *
     * @param name Field name
     * @return True if the field exists
     */
    bool has_field(const std::string &name) {
      if (fields_->exists(name) || (super_ && super_->has_field(name)))
        return true;
      return false;
    }
    /**
     * Checks if this class (or any of its super classes) has the specified method.
     *
     * @param name Name of the method to check
     * @return True if the class has a method with the specified name
     */
    bool has_method(const std::string &name) {
      if (methods_->exists(name) || (super_ && super_->has_method(name)))
        return true;
      return false;
    }
    /**
     * Accessor for a method by the method's name.
     *
     * @param name Method's name
     * @return Method pointer.
     */
    Method* get_method(const std::string &name) {
      if (methods_->exists(name))
        return methods_->get(name);
      if (super_ == BASE_CLASS)
        return OBJECT_NOT_FOUND;
      return super_->get_method(name);
    }
    /**
     * Checks all classes for any cyclical inheritance.
     */
    static void check_well_formed_hierarchy() {
      for (auto & class_pair : *Container::singleton()) {
        Class* quack_class = class_pair.second;

        std::vector<Class*> super_list; // Compiler needs this because cant pass a temp by reference
        Class* base_parent = quack_class->has_no_cyclic_inheritance(super_list);

        if (base_parent != BASE_CLASS) {
          std::string str = "Class " + quack_class->name_ + " has a cyclic inheritance";
          throw CyclicInheritenceException("CyclicInheritance", str);
        }
      }

      // Check that the return type of inherited methods is subtype of super method
      for (auto & class_pair : *Container::singleton()) {
        Quack::Class * q_class = class_pair.second;
        if (q_class->super_ == BASE_CLASS)
          continue;

        for (auto &method_info : *q_class->methods_) {
          Method * method = method_info.second;
          if (!q_class->super_->has_method(method->name_))
            continue;

          Method * super_method = q_class->super_->get_method(method->name_);

          // Args count must match
          if (method->params_->count() != super_method->params_->count())
            throw InheritedMethodParamCountException(q_class->name_, method->name_);

          Quack::Class * method_rtype = method->return_type_;
          Quack::Class * super_rtype = super_method->return_type_;

          if (!method_rtype->is_subtype(super_rtype))
            throw InheritedMethodReturnTypeException(q_class->name_, method->name_);

          for (unsigned i = 0; i < method->params_->count(); i++) {
            Quack::Class * meth_param_type = (*method->params_)[i]->type_;
            Quack::Class * super_param_type = (*super_method->params_)[i]->type_;
            if (!super_param_type->is_subtype(meth_param_type)) {
              throw InheritedMethodParamTypeException(q_class->name_, method->name_,
                                                      (*method->params_)[i]->name_);
            }
          }
        }
      }
    }
    /**
     * Used to check for cyclical class inheritance.  When classes are correctly configured,
     * each class eventually points back (through the Object class) back to a single common class.
     * If there is a cyclic dependency, the current class dependency will eventually appear in the
     * \p all_super list built via recursive calls.
     *
     * @param all_super All super classes observed so far.
     * @return true if the class has no cyclic dependencies.
     */
    Class* has_no_cyclic_inheritance(std::vector<Class *> &all_super) {
      if (super_ == BASE_CLASS)
        return BASE_CLASS;

      // If the class appears in a super, then there is a cycle
      if (std::find(all_super.begin(), all_super.end(), this) != all_super.end())
        return this;

      all_super.emplace_back(this);
      return super_->has_no_cyclic_inheritance(all_super);
    }
    /**
     * Perform least common ancestor determination on the implicit class and the \p other class.
     *
     * @param other Another
     * @return
     */
    Class* least_common_ancestor(Class * other) {
      return Class::least_common_ancestor(this, other);
    }
    /**
     * Helper function used to find the least common ancestor of two classes.  If there is no
     * common ancestor, the function
     * @param c1 First class to compare
     * @param c2 Second class to compare
     * @return Class shared by the two classes in the hierarchy
     */
    static Class* least_common_ancestor(Class* c1, Class* c2) {
      assert(c1 != nullptr && c2 != nullptr);
      if (c1 == c2)
        return c1;

      std::vector<std::vector<Class*>> class_paths(2);
      Class* classes[] = {c1, c2};
      // Build the list of paths
      for (int i = 0; i < 2; i++) {
        Class * q_class = classes[i];
        do {
          class_paths[i].emplace_back(q_class);
          q_class = q_class->super_;
        } while (q_class != BASE_CLASS);
      }

      // iterate through the lists in reverse to find last class both share
      unsigned long size0 = class_paths[0].size();
      unsigned long size1 = class_paths[1].size();
      auto min_len = std::min<unsigned long>(size0, size1);
      Class * last_shared = BASE_CLASS;
      for (unsigned long i = 1; i <= min_len; i++) {
        if (class_paths[0][size0 - i] != class_paths[1][size1 - i]) {
          assert(last_shared != BASE_CLASS);
          return last_shared;
        }

        last_shared = class_paths[0][size0 - i];
      }
      // One is a proper subset of the other
      return last_shared;
    }
    /**
     * Check if the class is of the specified type.
     *
     * @param name Name of the type
     * @return True if the class is of the specified type.
     */
    bool is_type(const std::string &name) {
      if (name_ == name)
        return true;
      if (super_)
        return super_->is_type(name);
      return false;
    }
    /**
     * Determines if the implicit class is a subtype of the specified type.
     *
     * @param other_type Type to check whether this class is a subtype
     * @return True if this class is a subtype of \p other_type.
     */
    bool is_subtype(Class * other_type) {
      if (other_type == BASE_CLASS)
        return true;

      Class * super = this;
      while (super != BASE_CLASS) {
        if (super == other_type)
          return true;
        super = super->super_;
      }
      return false;
    }
    /**
     * Performs an initial type check and configuration for the class's super class as well
     * as for the method parameters, return types, and constructor parameters.
     */
    void initial_type_check() {
      configure_super_class();

      configure_method_params(*constructor_->params_);
      for (auto &method_pair : *methods_) {
        Method * method = method_pair.second;
        configure_method_params(*method->params_);

        if (Quack::Class::Container::singleton()->get(method->name_) != OBJECT_NOT_FOUND)
          throw MethodClassNameCollisionException(method->name_);

        // Check and configure the method return type.
//        if (method->return_type_name_ == CLASS_NOTHING) {
//          method->return_type_ = BASE_CLASS;
//        } else {
        method->return_type_ = Container::singleton()->get(method->return_type_name_);
        if (method->return_type_ == OBJECT_NOT_FOUND) {
          throw UnknownTypeException("Class: " + this->name_ + ", method " + method->name_
                                   + ", unknown return type \"" + method->return_type_name_ +
                                   "\"");
        }
//        }
      }
    }
    /** Name of the class */
    const std::string name_;
    /**
     * Builds a method name for a class
     *
     * @param method Method object
     * @return Method name for the class
     */
    static std::string generated_method_name(Class* q_class, Method * method) {
      return q_class->name_ + "_method_" + method->name_;
    }
//    /**
//     * Accessor for all the methods in the class.
//     * @return Methods in the class.
//     */
//    inline const Method::Container* methods() const { return methods_; }
    /**
     * Debug function used to print a representation of the original quack source code
     * used to visualize the AST.
     *
     * @param indent_depth Depth to indent the generated code. Used for improved readability.
     */
    void print_original_src(unsigned int indent_depth) {
      std::string indent_str = std::string(indent_depth, '\t');
      std::cout << indent_str << KEY_CLASS << " " << name_ << "(";

      constructor_->params_->print_original_src(0);
      std::cout << ")";
      if (!super_type_name_.empty())
        std::cout << " " << KEY_EXTENDS << " " << super_type_name_;
      std::cout << " {\n";

      constructor_->print_original_src(indent_depth + 1);
      if (!constructor_->block_->empty() && !methods_->empty()) {
        std::cout << "\n\n";
      }
      methods_->print_original_src(indent_depth + 1);

      std::cout << "\n" << indent_str << "}";
    }
    /**
     * Base classes are built into the Quack language and include Boolean, Integer, String,
     * and Object.  User classes are NOT built into the Quack language by default and are
     * defined by the user..
     *
     * @return True if the class is abase class.
     */
    virtual bool is_user_class() const { return true; }
    /**
     * Accessor for the class constructor.
     *
     * @return Constructor method
     */
    Method* get_constructor() { return constructor_; }
    /**
     * Type name for generated objects of this type
     * @return
     */
    const std::string generated_object_type_name() const {
      return "obj_" + name_;
    }
    /**
     * Type used to for the clazz field of objects of this type.
     *
     * @return Class name for object
     */
    const std::string generated_clazz_type_name() const {
      return "class_" + name_;
    }
    /**
     * Used to define the struct that stores the class information.
     *
     * @return Class struct information
     */
    const std::string generated_struct_clazz_name() const {
      return generated_clazz_type_name() + "_struct";
    }
    /**
     * Gets the name used for constructors of this class.
     *
     * @return Constructor function name
     */
    const std::string generated_constructor_name() const {
      return "new_" + name_;
    }
    /**
     * Gets the name of the struct object used to store the clazz information include super class.
     * This function is used in typecase statements and in the generated class definitions.
     *
     * @return Name of the struct clazz struct const object
     */
    const std::string generated_clazz_obj_struct_name() const {
      return this->generated_clazz_obj_name() + "_struct";
    }
    /**
     * Generates all code associated with a specific class
     *
     * @param settings Code generator settings
     */
    void generate_code(CodeGen::Settings settings) {
      assert(this->is_user_class());

      settings.fout_ << "/*======================= " << name_ << " =======================*/\n"
                     << "/* Typedefs Required for Separation of class and object structs */\n"
                     << "struct " << generated_struct_clazz_name() << ";\n"
                     << "typedef struct " << generated_struct_clazz_name()
                     << "* " << generated_clazz_type_name() << ";\n"
                     << std::endl;

      generate_object_struct(settings);
      settings.fout_ << "\n";
      generate_clazz_struct(settings);

      generate_all_prototypes(settings);

      generate_clazz_object(settings);

      generate_constructor(settings);
      generate_methods(settings);

      settings.fout_ << std::endl;
    }
   private:
    /**
     * Generates the struct that contains all methods for the class including the constructor.
     *
     * @param settings Code generator settings
     */
    void generate_clazz_struct(CodeGen::Settings settings) {
      settings.fout_ << "struct " << generated_struct_clazz_name() << " {";

      // Put constructor function pointer
      std::string indent = AST::ASTNode::indent_str(1);
      settings.fout_ << "\n" << indent << Container::Obj()->generated_clazz_type_name() << " "
                     << GENERATED_SUPER_FIELD << ";";

      settings.fout_ << "\n" << indent << generated_object_type_name()
                     << " (*" << METHOD_CONSTRUCTOR << ")(";
      constructor_->params_->generate_code(settings, false, false);
      settings.fout_ << ");";

      // Function pointers for all other methods
      build_generated_methods(this);
      for (auto method_info : *gen_methods_) {
        settings.fout_ << "\n" << indent
                       << method_info.second->return_type_->generated_object_type_name()
                       << " (*" << method_info.second->name_ << ")(";

        // Parameters - Use the implicit object then the parameter list
        settings.fout_ << method_info.first->generated_object_type_name();
        method_info.second->params_->generate_code(settings, false);

        settings.fout_ << ");";
      }
      settings.fout_ << "\n};\n";

    }
    /**
     * Generates the object struct for the class.
     *
     * @param settings Code generator settings
     */
    void generate_object_struct(CodeGen::Settings settings) {
      settings.fout_ << "typedef struct " << generated_malloc_obj_name() << " {";
      // ToDo Add super
      // Method object field
      settings.fout_ << "\n" << AST::ASTNode::indent_str(1)
                     << generated_clazz_type_name() << " " << GENERATED_CLASS_FIELD << ";";

      build_generated_fields(this);
      for (auto field_info : *gen_fields_) {
        settings.fout_ << "\n" << AST::ASTNode::indent_str(1)
                       << field_info.second->type_->generated_object_type_name() << " "
                       << field_info.second->name_ << ";";
      }
      settings.fout_ << "\n} * " << generated_object_type_name() << ";\n";
    }
    const std::string generated_clazz_obj_name() const {
      return "the_class_" + name_;
    }
    /**
     * Struct that stores the allocated memory size of an object of this type.
     *
     * @return Struct name used in malloc of object memory
     */
    const std::string generated_malloc_obj_name() const {
      return "obj_" + name_ + "_struct";
    }
    /**
     * Helper function used to generate the code for a method prototype.  It should not be used
     * for a constructor. Likewise, it includes no preceding indents nor does not include
     * any curly brackets.
     *
     * @param settings Code generator constructor
     * @param method Method whose prototype will be generated.
     */
    void generate_method_prototype(CodeGen::Settings settings, Method* method,
                                   bool is_constructor=false) {
      settings.fout_ << method->return_type_->generated_object_type_name() << " ";

      if (is_constructor)
        settings.fout_ << generated_constructor_name();
      else
        settings.fout_ << generated_method_name(this, method);

      settings.fout_ << "(";

      if (!is_constructor) {
        settings.fout_ << generated_object_type_name() << " " << OBJECT_SELF;
      }

      method->params_->generate_code(settings, true, !is_constructor);
      settings.fout_ << ")";
    }
    /**
     * Generates all prototypes for all methods and the constructor.
     *
     * @param settings Code generator settings
     */
    void generate_all_prototypes(CodeGen::Settings settings) {
      settings.fout_ << "\n";

      generate_method_prototype(settings, constructor_, true);
      settings.fout_ << ";\n";

      for (const auto &method : *methods_) {
        generate_method_prototype(settings, method.second);
        settings.fout_ << ";\n";
      }
    }
    /**
     * The "Clazz" object contains a lookup of all the methods in the class itself. This is
     * attached as a field in the constructor.
     *
     * @param settings Code generator settings
     */
    void generate_clazz_object(CodeGen::Settings settings) {
      std::string class_obj_struct = generated_clazz_obj_struct_name();

      settings.fout_ << "\nstruct " << generated_struct_clazz_name() << " "
                     << class_obj_struct << " = {";

      std::string indent_str = AST::ASTNode::indent_str(1);

      std::string super_obj_struct = super_->generated_clazz_obj_struct_name();
      // Include in the super reference a cast to prevent compiler warnings
      settings.fout_ << "\n" << indent_str
                     << "(" << Quack::Class::Container::Obj()->generated_clazz_type_name() << ")"
                     << "&" << super_obj_struct;

      settings.fout_ << ",\n" << indent_str << generated_constructor_name();

      build_generated_methods(this);
      for (auto method_info : *gen_methods_) {
        settings.fout_ << ",\n" << indent_str
                       << generated_method_name(method_info.first, method_info.second);
      }

      settings.fout_ << "\n};\n\n"
                     << generated_clazz_type_name() << " " << generated_clazz_obj_name()
                     << " = &" << class_obj_struct << ";";
    }
    /**
     * Generates code defining all non-fields and non-implicit parameters in a method.
     *
     * @param settings Code generator settings
     * @param indent_lvl Indentation level.
     * @param st Symbol table containing the symbols in a method
     */
    static void generate_symbol_table(CodeGen::Settings settings, unsigned indent_lvl,
                                      Method * method) {
      std::string indent_str = AST::ASTNode::indent_str(indent_lvl);

      for (const auto &symbol_info : *method->symbol_table_) {
        Symbol * sym = symbol_info.second;
        if (sym->is_field_ || method->params_->get(sym->name_) || sym->name_ == OBJECT_SELF)
          continue;

        settings.fout_ << indent_str << sym->get_type()->generated_object_type_name()
                       << " " << sym->name_ << ";\n";
      }
    }
    /**
     * Generates code for the class constructor.
     *
     * @param settings Code generator settings
     */
    void generate_constructor(CodeGen::Settings settings) {
      settings.return_type_ = this;
      settings.st_ = constructor_->symbol_table_;

      settings.fout_ << "\n";
      generate_method_prototype(settings, constructor_, true);
      settings.fout_ << " {";

      // Allocate the memory for the object itself
      std::string indent_str = AST::ASTNode::indent_str(1);
      settings.fout_ << "\n" << indent_str << generated_object_type_name() << " " << OBJECT_SELF
                     << " = (" << generated_object_type_name() << ")malloc(sizeof(struct "
                     << generated_malloc_obj_name() << "));\n";

      // Define the object that will store the class methods
      settings.fout_ << indent_str << OBJECT_SELF << "->" << GENERATED_CLASS_FIELD
                     << " = " << generated_clazz_obj_name() << ";";

      generate_symbol_table(settings, 1, constructor_);
      settings.fout_ << "\n" << AST::ASTNode::indent_str(1) << "/* Method statements */\n";
      constructor_->block_->generate_code(settings, 0);

      settings.fout_ << "\n" << indent_str << "return " << OBJECT_SELF << ";";
      settings.fout_ << "\n}\n";

      settings.return_type_ = nullptr;
      settings.st_ = nullptr;
    }
    /**
     * Generates the C code associated with all methods in the class.
     *
     * @param settings Code generator settings
     */
    void generate_methods(CodeGen::Settings settings) {
      for (const auto &method_info : *methods_) {
        Method * method = method_info.second;

        settings.return_type_ = method->return_type_;
        settings.st_ = method->symbol_table_;

        // Define function header
        settings.fout_ << "\n";
        generate_method_prototype(settings, method);
        settings.fout_ << " {\n";

        generate_symbol_table(settings, 1, method);

        method->block_->generate_code(settings, 0);

        settings.fout_ << "}\n";
      }
      settings.return_type_ = nullptr;
      settings.st_ = nullptr;
    }
    /** Container used to store generated objects in the class */
    template <typename _S>
    class GenObjContainer : public std::vector<std::pair<Class *, _S*>> {};
    /**
     * Configures the super class pointer for the object.
     */
    void configure_super_class() {
      // Object is the top of the object hierarchy so handle specially.
      if (this->name_ == CLASS_OBJ) {
        super_ = BASE_CLASS;
        return;
      }

      std::string super_name = (super_type_name_.empty()) ? CLASS_OBJ : super_type_name_;

      Container* classes = Container::singleton();
      if (!classes->exists(super_type_name_)) {
        std::string msg = "For class, \"" + name_ + "\", unknown super class: " + super_type_name_;
        throw ClassHierarchyException("UnknownSuper", msg);
      }
      super_ = classes->get(super_name);
    }
    /**
     * Basic configuration of method parameters including verify no parameter has type nothing
     * and then links the parameters to their associated types.
     *
     * @param params Set of parameters.
     */
    void configure_method_params(Param::Container &params) {
      for (auto &param : params) {
        if (param->type_name_ == CLASS_NOTHING) {
          std::string msg = "Parameter " + param->name_ + " cannot have type \"" CLASS_NOTHING "\"";
          throw ClassHierarchyException("NothingParam", msg);
        }

        Class* type_class = Container::singleton()->get(param->type_name_);
        if (type_class == OBJECT_NOT_FOUND)
          throw UnknownTypeException(param->type_name_);
        param->type_ = type_class;
      }
    }
    /** Name of the super class of this type */
    const std::string super_type_name_;
    /** Pointer to the super class of this class. */
    Class *super_;
    /** Statements in the constructor */
    Method* constructor_;
    /**
     * Build the generated methods list that will be used in the code generation step.  The order
     * of methods must match the order of ALL inherited classes to allow for pointer casting.
     *
     * @return Vector of generated methods in order matching super classes.
     */
    static GenObjContainer<Method>* build_generated_methods(Class * q_class) {
      q_class->gen_methods_ =  build_generated_list<Method>(q_class, q_class->gen_methods_,
                                                            q_class->methods_,
                                                            Class::build_generated_methods);
      return q_class->gen_methods_;
    }
    /**
     * Build the generated field list that will be used in code generation.  Order of the fields
     * matches the order of all inherited classes.
     *
     * @param q_class Class of interest generating the fields
     * @return Vector of the generated fields along with the associated class that generates it
     */
    static GenObjContainer<Field>* build_generated_fields(Class * q_class) {
      q_class->gen_fields_ = build_generated_list<Field>(q_class, q_class->gen_fields_,
                                                         q_class->fields_,
                                                         Class::build_generated_fields);
      return q_class->gen_fields_;
    }
    /**
     * Helper function used to build the generated objects (i.e., fields or methods) for code
     * generation.
     *
     * @tparam _T Type of the class object
     * @param gen_vec
     * @param container
     * @return Updated list of fields and methods
     */
    template <typename _T>
    static GenObjContainer<_T>* build_generated_list(Class * q_class, GenObjContainer<_T>* gen_vec,
                                                     MapContainer<_T>* container,
                                                     GenObjContainer<_T>* (*gen_func)(Class*)) {
      // Repeat building the object container multiple times for the same object
      if (gen_vec)
        return gen_vec;

      // Get all objects from the super class
      if (q_class->super_) {
        auto * temp_gen_fields = gen_func(q_class->super_);
        gen_vec = new GenObjContainer<_T>(*temp_gen_fields);
      } else {
        gen_vec = new GenObjContainer<_T>();
      }

      // Add remaining objects in this class
      unsigned long start_size = gen_vec->size();
      for (const auto &obj_pair : *container) {
        _T * obj = obj_pair.second;
        bool found = false;
        // "Override" the existing object in the vector
        for (unsigned i = 0; i < gen_vec->size(); i++) {
          if ((*gen_vec)[i].second->name_ == obj->name_) {
            (*gen_vec)[i] = std::pair<Class*, _T*>(q_class, obj);
            found = true;
            break;
          }
        }
        // If not found, then add this item to the list
        if (found)
          continue;
        gen_vec->emplace_back(std::pair<Class *, _T*>(q_class, obj));
      }

      // Sort only the stuff that is added
      std::sort(gen_vec->begin() + start_size, gen_vec->end(), template_pair_less_than<_T>);
      return gen_vec;
    }
    /**
     * Sort function used to compare two pair obbjects
     *
     * @tparam _T Template type stored in the pair
     * @param a First object
     * @param b Second object
     * @return True if the name_ field of the \p a is less than the name_ field of \p b.
     */
    template <typename _T>
    static bool template_pair_less_than(std::pair<Class*, _T*> a, std::pair<Class*, _T*> b) {
      return a.second->name_ < b.second->name_;
    }
   protected:
    /** All methods supported by the class */
    Method::Container* methods_;
    /** Name of all fields in the class */
    Field::Container* fields_;
    /** Generated methods for the class in order*/
    GenObjContainer<Method>* gen_methods_;
    /** Generated fields for the class in order */
    GenObjContainer<Field>* gen_fields_;
    /**
     * Used to add binary operation methods to the a class.  Only used for base classes
     * like Obj, Boolean, Integer, etc.
     *
     * @param method_name Name of the binary operation method
     * @param return_type Return type of binary operation
     * @param param_type Type of the other parameter's type
     */
    void add_binop_method(const std::string &method_name, const std::string &return_type,
                          const std::string &param_type) {

      auto * params = new Param::Container();
      params->add(new Param(FIELD_OTHER_LIT_NAME, param_type));

      Method * method = new Method(method_name, return_type, params, new AST::Block());
      method->init_list_ = new InitializedList();
      method->obj_class_ = this;

      methods_->add(method);
    }

    void add_unary_op_method(const std::string &method_name, const std::string &return_type) {
      auto * params = new Param::Container();
      Method * method = new Method(method_name, return_type, params, new AST::Block());
      method->obj_class_ = this;
      methods_->add(method);
    }
  };

  struct ObjectClass : public Class {
    ObjectClass()
        : Class(strdup(CLASS_OBJ), strdup(""), new Param::Container(),
                new AST::Block(), new Method::Container()) {
      add_base_methods();
    }
    /**
     * Create the print and equality methods for a base Object.
     */
    void add_base_methods() {
      add_binop_method(METHOD_EQUALITY, CLASS_BOOL, CLASS_OBJ);
      // Unary op ok to use since function takes no args
      add_unary_op_method(METHOD_PRINT, CLASS_OBJ);
      add_unary_op_method(METHOD_STR, CLASS_STR);
    }
    /**
     * Object class is a base class in Quack so this function always returns true since if it is
     * a base class, it cannot be a user class.
     *
     * @return False always.
     */
    bool is_user_class() const override { return false; }
  };

  struct NothingClass : public Class {
    explicit NothingClass()
        : Class(strdup(CLASS_NOTHING), strdup(CLASS_OBJ), new Param::Container(),
                new AST::Block(), new Method::Container()) { }
    /**
    * Primitives are all base (i.e., not user) classes in Quack so this function always returns
    * true.
    *
    * @return False always.
    */
    bool is_user_class() const override { return false; }
  };

  /**
   * Represents all literal objects (e.g., integer, boolean, string).  Encapsulated in this
   * subclass to standardize some value information.
   */
  struct PrimitiveClass : public Class {
    explicit PrimitiveClass(char* name)
            : Class(strdup(name), strdup(CLASS_OBJ), new Param::Container(),
                    new AST::Block(), new Method::Container()) { }
    /**
    * Primitives are all base (i.e., not user) classes in Quack so this function always returns
    * true.
    *
    * @return False always.
    */
    bool is_user_class() const override { return false; }
  };

  struct IntClass : public PrimitiveClass {
    IntClass() : PrimitiveClass(strdup(CLASS_INT)) {
      add_unary_op_method(METHOD_STR, CLASS_STR);

      add_binop_method(METHOD_ADD, CLASS_INT, CLASS_INT);
      add_binop_method(METHOD_SUBTRACT, CLASS_INT, CLASS_INT);
      add_binop_method(METHOD_MULTIPLY, CLASS_INT, CLASS_INT);
      add_binop_method(METHOD_DIVIDE, CLASS_INT, CLASS_INT);

      add_binop_method(METHOD_EQUALITY, CLASS_BOOL, CLASS_OBJ);
      add_binop_method(METHOD_LEQ, CLASS_BOOL, CLASS_INT);
      add_binop_method(METHOD_LT, CLASS_BOOL, CLASS_INT);
      add_binop_method(METHOD_GEQ, CLASS_BOOL, CLASS_INT);
      add_binop_method(METHOD_GT, CLASS_BOOL, CLASS_INT);
    }
  };

  struct StringClass : public PrimitiveClass {
    StringClass() : PrimitiveClass(strdup(CLASS_STR)) {
      add_unary_op_method(METHOD_STR, CLASS_STR);

      add_binop_method(METHOD_ADD, CLASS_STR, CLASS_STR);

      add_binop_method(METHOD_EQUALITY, CLASS_BOOL, CLASS_OBJ);
      add_binop_method(METHOD_LEQ, CLASS_BOOL, CLASS_STR);
      add_binop_method(METHOD_LT, CLASS_BOOL, CLASS_STR);
      add_binop_method(METHOD_GEQ, CLASS_BOOL, CLASS_STR);
      add_binop_method(METHOD_GT, CLASS_BOOL, CLASS_STR);
    }
  };

  struct BooleanClass : public PrimitiveClass {
    BooleanClass() : PrimitiveClass(strdup(CLASS_BOOL)) {
      add_unary_op_method(METHOD_STR, CLASS_STR);

      add_binop_method(METHOD_EQUALITY, CLASS_BOOL, CLASS_OBJ);

//      add_binop_method(METHOD_OR, CLASS_BOOL, CLASS_BOOL);
//      add_binop_method(METHOD_AND, CLASS_BOOL, CLASS_BOOL);
    }
  };
}

#endif //PROJECT02_QUACK_CLASSES_H
