#ifndef PROJECT02_QUACK_TYPE_CHECKER_H
#define PROJECT02_QUACK_TYPE_CHECKER_H

#include <iostream>

#include "quack_class.h"
#include "quack_program.h"
#include "initialized_list.h"
#include "symbol_table.h"

namespace Quack {
  class TypeChecker {
   public:
    TypeChecker() = default;

    void run(Program* prog) {
      try {
        perform_initial_checks();
        Class::check_well_formed_hierarchy();
        perform_return_all_paths_check();
      } catch (TypeCheckerException &e) {
        Quack::Utils::print_exception_info_and_exit(e, EXIT_CLASS_HIERARCHY);
      }

      try {
        perform_initialized_before_use_check(prog);
      } catch (TypeCheckerException &e) {
        Quack::Utils::print_exception_info_and_exit(e, EXIT_INITIALIZE_BEFORE_USE);
      }

      try {
        type_inference(prog);

        check_super_type_field_types();
      } catch (TypeCheckerException &e) {
        Quack::Utils::print_exception_info_and_exit(e, EXIT_TYPE_INFERENCE);
      }
      std::cout << "Type checker completed successfully." << std::endl;
    }
   private:
    /**
     * Updates the most basic type structure.  This includes that all type names
     * exist and that the super class exists.
     */
    void perform_initial_checks() {
      for (auto &class_pair : *Class::Container::singleton()) {
        Class * q_class = class_pair.second;
        q_class->initial_type_check();
      }
    }
    void perform_return_all_paths_check() {
      for (const auto &class_pair : *Quack::Class::Container::singleton()) {
        Quack::Class * q_class = class_pair.second;
        if (!q_class->is_user_class())
          continue;

        for (const auto &method_pair : *q_class->methods_) {
          Method * method = method_pair.second;

          bool return_on_all_paths = method->block_->contains_return_all_paths();
          if (return_on_all_paths)
            continue;

          Quack::Class * nothing = Quack::Class::Container::Nothing();
          if (!nothing->is_subtype(method->return_type_))
            throw ReturnAllPathsException(q_class->name_, method->name_);

          // Append return Nothing to the path
          method->block_->append(new AST::Return(new AST::NothingLit()));
        }
      }
    }
    /**
     * Performs all initialize before use tests for all Quack classes.  The function also checks
     * that all super fields are tested as well.
     *
     * @param prog Quack program being type checked.
     */
    void perform_initialized_before_use_check(Program* prog) {
      // Type check constructor first since the fields are need for the method checks
      // and for the super class field checks.
      for (auto &class_pair : *Class::Container::singleton()) {
        Class *q_class = class_pair.second;
        perform_constructor_initialize_before_use(q_class);
      }

      verify_all_super_fields_initialized();

      // Checks all methods other than the constructor
      for (auto &class_pair : *Class::Container::singleton()) {
        Class *q_class = class_pair.second;

        // For all methods, the class fields are initialized
        InitializedList fields_list;
        for (auto &field_info : *q_class->fields_) {
          Field * field = field_info.second;
          fields_list.add(field->name_, true);
          if (q_class->has_method(field->name_))
            throw DuplicateMemberException(q_class->name_, field->name_);
          if (field->name_ == q_class->name_)
            throw FieldClassMatchException(field->name_);
        }

        for (auto &method_pair : *q_class->methods_) {
          Method * method = method_pair.second;

          InitializedList init_list(fields_list);
          add_params_to_initialized_list(init_list, method->params_);

          auto * all_inits = new InitializedList(init_list);
          method->block_->check_initialize_before_use(init_list, all_inits, true);

          all_inits->var_union(init_list);
          method->init_list_ = all_inits;
        }
      }

      // Verifies the main block (i.e., any statments not in a class method)
      InitializedList main_inits;
      auto * all_inits = new InitializedList();
      prog->main_->block_->check_initialize_before_use(main_inits, all_inits, false);

      all_inits->var_union(main_inits);
      prog->main_->init_list_ = all_inits;
    }

    /**
     * Performs an initialize before use check on a Quack class's constructor.  No other methods
     * (if any) from the Quack class are checked in this method.  The method does populate the
     * Field::Container() for the class.
     *
     * @param q_class Class whose constructor is being checked
     */
    void perform_constructor_initialize_before_use(Quack::Class * q_class) {
      std::string name = q_class->name_;

      InitializedList init_list;
      add_params_to_initialized_list(init_list, q_class->constructor_->params_);

      auto all_inits = new InitializedList(init_list);
      q_class->constructor_->block_->check_initialize_before_use(init_list, all_inits, true);

      for (const auto &init_var : all_inits->all_items()) {
        // Only consider the class fields when caring about initialized before use.
        if (!init_var.second)
          continue;
        if (!init_list.exists(init_var.first, init_var.second)) {
          std::string msg = "Constructor for class " + q_class->name_
                            + " does not initialize on all paths";
          throw TypeCheckerException("Constructor", msg);
        }

      }
      q_class->constructor_->init_list_ = all_inits;

      for (const auto &var_info : init_list.vars_) {
        if (!var_info.second)
          continue;
        q_class->fields_->add_by_name(var_info.first);
      }
    }
    /**
     * Adds the parameters to the initialized this.  This is generally used before a function
     * call
     * @param params
     */
    static void add_params_to_initialized_list(InitializedList &init_list,
                                               const Param::Container * params,
                                               bool is_field = false) {
      for (const auto &param : *params)
        init_list.add(param->name_, is_field);
    }
    /**
     * This verifies that all fields of a superclass are initialized in the subclass.  The overall
     * algorithm only checks each class with its parent.  If all parent pairwise tests pass,
     * then the field structure is valid.
     *
     * @return True if all fields of the super class are initialized in the subclass.
     */
    bool verify_all_super_fields_initialized() {
      Class::Container* all_classes = Class::Container::singleton();

      for (auto &class_pair : *all_classes) {
        Class *q_class = class_pair.second;
        if (!q_class->is_user_class())
          continue;

        if (q_class->super_ == all_classes->get(CLASS_OBJ))
          continue;

        if (!q_class->fields_->is_super_set(q_class->super_->fields_))
          throw MissingSuperFieldsException(q_class->name_);
      }

      return true;
    }
    /**
     * Performs flow insensitive type inference of all class methods, the class constructors,
     * and the program's main method.
     *
     * @param prog Quack program to analyze.
     *
     * @return True if type inference passed.
     */
    bool type_inference(Program* prog) {
      for (auto &class_info : *Quack::Class::Container::singleton()) {
        Quack::Class * q_class = class_info.second;

        // Test constructor first
        function_type_inference(q_class, q_class->constructor_);

        update_field_classes(q_class);

        // Perform type inference on each method
        for (auto &method_info : *q_class->methods_) {
          auto * method = method_info.second;
          function_type_inference(q_class, method);
        }
      }

      // Performs inference on the main function
      function_type_inference(nullptr, prog->main_);
      return true;
    }
    /**
     * Updates the Class' field container and sets the type of each field.  It relies on the
     * constructor's symbol table.
     *
     * @param q_class Class whose fields are being updated
     */
    void update_field_classes(Quack::Class * q_class) {
      for (auto &field_info : *q_class->fields_) {
        Field* field = field_info.second;
        const Symbol* sym = q_class->constructor_->symbol_table_->get(field->name_, true);

        field->type_ = sym->get_type();
      }
    }
    /**
     * Performs type inference for the method. This may be a class method or main.
     *
     * @param q_class Class associated with the method
     * @param method
     * @return True if type inference is successful.
     */
    bool function_type_inference(Quack::Class * q_class, Quack::Method* method) {
      auto* st = new Symbol::Table();

      // Use init list to build list of variables
      for (auto &init_var : *method->init_list_) {
        std::string var_name = init_var.first;
        bool is_field = init_var.second;

        Class * field_type = is_field ? q_class->fields_->get(var_name)->type_ : nullptr;
        st->add_new(var_name, is_field, field_type);
      }

      // Populate the type of function parameters
      for (auto * param : *method->params_)
        st->update(param->name_, false, param->type_);

      TypeCheck::Settings settings;
      settings.st_ = st;
      settings.is_constructor_ = (q_class != nullptr && method->name_ == q_class->name_);
      settings.return_type_ = settings.is_constructor_ ? nullptr : method->return_type_;
      settings.this_class_ = q_class;

      do {
        st->clear_dirty();
        method->block_->perform_type_inference(settings);
      } while (st->is_dirty());

      // Store the symbol
      method->symbol_table_ = st;

      return true;
    }
    /**
     * Function checks that the type of subclass fields is complaint with the type of super class
     * fields.  Compliance is determined by the subclass fields being the same or subclasses of
     * the super type fields.
     *
     * @return True if the super class field type check passes.
     */
    bool check_super_type_field_types() {
      for (const auto &class_info : *Quack::Class::Container::singleton()) {
        Class * q_class = class_info.second;
        if (!q_class->is_user_class())
          continue;

        Class * super = q_class->super_;
        for (const auto &field_info : *q_class->fields_) {
          Field * field = field_info.second;
          // If super class does not have this object, move on
          if (!super->fields_->exists(field->name_))
            continue;

          Class * super_field_type = super->fields_->get(field->name_)->type_;
          if (!field->type_->is_subtype(super_field_type))
            throw SubTypeFieldTypeException(q_class->name_, field->name_);
        }
      }
      return true;
    }
  };
}

#endif //PROJECT02_QUACK_TYPE_CHECKER_H
