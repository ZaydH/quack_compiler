//
// Created by Michal Young on 9/12/18.
//

#include <string>

#include "ASTNode.h"
#include "quack_class.h"
#include "quack_param.h"
#include "quack_method.h"
#include "exceptions.h"
#include "keywords.h"
#include "compiler_utils.h"


namespace AST {

  unsigned long ASTNode::label_cnt_ = 0;
  unsigned long ASTNode::var_cnt_ = 0;

  std::string ASTNode::generate_temp_var(const std::string &var_to_store,
                                         CodeGen::Settings settings,
                                         unsigned indent_lvl, bool is_lhs) const {
    std::string var_name = define_new_temp_var();
    PRINT_INDENT(indent_lvl);
    settings.fout_ << type_->generated_object_type_name() << " " << (is_lhs?"* ":"") << var_name
                   << " = " << (is_lhs ? "&(":"") <<  var_to_store << (is_lhs?")":"") << ";\n";
    if (!is_lhs)
      return var_name;
    return "(*" + var_name + ")";
  }

  void ASTNode::generate_eval_branch(CodeGen::Settings settings, const unsigned indent_lvl,
                                     const std::string &true_label, const std::string &false_label){
    if (auto bool_lit = dynamic_cast<BoolLit*>(this)) {
      if (bool_lit->value_)
        generate_goto(settings, indent_lvl, true_label);
      else
        generate_goto(settings, indent_lvl, false_label);
      return;
    }
    if (auto bool_op = dynamic_cast<BoolOp*>(this))
      return bool_op->generate_eval_bool_op(settings, indent_lvl, true_label, false_label);

    std::string gen_var = this->generate_code(settings, indent_lvl, false);
    PRINT_INDENT(indent_lvl);
    settings.fout_ << "if(" GENERATED_LIT_TRUE " == " << gen_var << ") { goto "
                   << true_label << "; }\n";

    if (false_label != GENERATED_NO_JUMP)
      generate_goto(settings, indent_lvl, false_label, true);
  }

//  bool Typing::check_type_name_exists(const std::string &type_name) const {
//    if (!type_name.empty() && !Quack::Class::Container::singleton()->exists(type_name)) {
//      throw UnknownTypeException(type_name);
//    }
//    return true;
//  }

  bool Typing::update_inferred_type(TypeCheck::Settings &settings, Quack::Class *inferred_type,
                                    bool is_field) {
    // Handle the initial typing
    bool success = configure_initial_typing(inferred_type);

    type_ = type_->least_common_ancestor(inferred_type);

    success = success && expr_->update_inferred_type(settings, type_, is_field);

    type_ = Quack::Class::least_common_ancestor(type_, expr_->get_node_type());
    if (type_ == BASE_CLASS) {
      std::string msg = "Unable to reconcile types " + type_name_ + " and " + inferred_type->name_;
      throw TypeInferenceException("TypingError", msg);
    }

    success = success && verify_typing();
    return success;
  }

  bool Typing::configure_initial_typing(Quack::Class *other_type) {
    // Handle the initial typing
    if (type_ == BASE_CLASS) {
      if (!type_name_.empty()) {
        type_ = Quack::Class::Container::singleton()->get(type_name_);
        if (type_ == OBJECT_NOT_FOUND)
          throw TypeInferenceException("TypingError", "Unknown type name \"" + type_name_ + "\"");
      } else {
        type_ = other_type;
      }
    }
//
//    if (type_ == BASE_CLASS)
//      throw TypeInferenceException("TypingError", "Unable to resolve initial type");
    return true;
  }

  bool Typing::verify_typing() {
    if (!expr_->get_node_type()->is_subtype(type_)) {
      std::string msg = "Unable to cast type " + expr_->get_node_type()->name_
                        + " to " + type_->name_;
      throw TypeInferenceException("TypingError", msg);
    }
    if (type_ == BASE_CLASS)
      type_ = expr_->get_node_type();
    return true;
  }

  bool If::perform_type_inference(TypeCheck::Settings &settings, Quack::Class *) {
//    cond_->set_node_type(Quack::Class::Container::Bool());
    bool success = cond_->perform_type_inference(settings, nullptr);
    if (cond_->get_node_type() != Quack::Class::Container::Bool())
      throw TypeInferenceException("IfCondType", "If conditional not of type Bool");

    success = success && truepart_->perform_type_inference(settings);
    success = success && falsepart_->perform_type_inference(settings);

    return success;
  }

  bool IntLit::perform_type_inference(TypeCheck::Settings &settings, Quack::Class *) {
    set_node_type(Quack::Class::Container::Int());
    return true;
  }

  bool BoolLit::perform_type_inference(TypeCheck::Settings &settings, Quack::Class *) {
    set_node_type(Quack::Class::Container::Bool());
    return true;
  }

  bool NothingLit::perform_type_inference(TypeCheck::Settings &settings, Quack::Class *) {
    set_node_type(Quack::Class::Container::Nothing());
    return true;
  }

  bool StrLit::perform_type_inference(TypeCheck::Settings &settings, Quack::Class *) {
    set_node_type(Quack::Class::Container::Str());
    return true;
  }

  bool While::perform_type_inference(TypeCheck::Settings &settings, Quack::Class *) {
    bool success = cond_->perform_type_inference(settings, nullptr);
    if (cond_->get_node_type() != Quack::Class::Container::Bool())
      throw TypeInferenceException("WhileCondType", "While conditional not of type Bool");

    success = success && body_->perform_type_inference(settings);

    return success;
  }

  bool FunctionCall::perform_type_inference(TypeCheck::Settings &settings,
                                            Quack::Class * parent_type) {
    // Only need to check node type once
    Quack::Method * method;
    if (parent_type == nullptr) {
      Quack::Class * obj_class = Quack::Class::Container::singleton()->get(ident_);
      if (obj_class == OBJECT_NOT_FOUND)
        throw UnknownConstructorException(ident_);

      method = obj_class->get_constructor();
    } else {
      method = parent_type->get_method(ident_);
      if (method == OBJECT_NOT_FOUND) {
        std::string msg = "Unknown method \"" + ident_ + "\" in cls \"" + parent_type->name_ + "\"";
        throw TypeInferenceException("MethodError", msg);
      }
    }

    // Verify that argument count is correct
    Quack::Param::Container* params = method->params_;
    if (params->count() != args_->count())
      throw TypeInferenceException("FunctionCall", "Wrong arg count for method " + ident_);

    // Iterate through the arguments and set the
    for (unsigned i = 0; i < args_->count(); i++) {
      ASTNode* arg = args_->args_[i];
      Quack::Param * param = (*params)[i];
      if (arg->get_node_type() == BASE_CLASS)
        arg->set_node_type(param->type_);

      if (!arg->get_node_type()->is_subtype(param->type_))
        throw TypeInferenceException("FunctionCall", "Param " + param->name_ + " type error");

      arg->perform_type_inference(settings, nullptr);
    }

    Quack::Class * rtrn_type = method->return_type_;

    if (rtrn_type != nullptr) {
      if (get_node_type() == BASE_CLASS) {
        set_node_type(rtrn_type);
      } else {
        set_node_type(rtrn_type->least_common_ancestor(get_node_type()));
        if (get_node_type() == BASE_CLASS)
          throw TypeInferenceException("FunctionCall", "Unable to reconcile FunctionCall return");
      }
    }
    return true;
  }

  bool Return::perform_type_inference(TypeCheck::Settings &settings, Quack::Class * parent_type) {
    type_ = settings.return_type_;
    if (settings.is_constructor_) {
      bool success = right_->perform_type_inference(settings, parent_type);
      if (right_->get_node_type() != settings.this_class_)
        throw TypeInferenceException("InvalidReturn", "Constructor return must match class");
      return success;
    }

    if ((type_ == nullptr || right_ == nullptr) && (type_ != nullptr || right_ != nullptr))
      throw TypeInferenceException("ReturnNothing", "Mismatch of return " CLASS_NOTHING);

    right_->perform_type_inference(settings, nullptr);

    Quack::Class * right_type = right_->get_node_type();
    if (right_type == nullptr)
      throw TypeCheckerException("ReturnType", "Return has no type");
    if (!right_type->is_subtype(type_))
      throw TypeCheckerException("ReturnType", "Invalid return type \"" + right_type->name_ + "\"");

    return true;
  }

  std::string Return::generate_code(CodeGen::Settings &settings, unsigned indent_lvl,
                                    bool is_lhs) const {
    if (is_lhs)
      throw std::runtime_error("Return cannot be on left hand side");

    std::string temp_var_name = right_->generate_code(settings, indent_lvl, is_lhs);

    PRINT_INDENT(indent_lvl);
    settings.fout_ << "return "
                   << "(" << settings.return_type_->generated_object_type_name() << ")"
                   << "(" << temp_var_name << ");\n";

    return NO_RETURN_VAR;
  }

  bool UniOp::perform_type_inference(TypeCheck::Settings &settings, Quack::Class *) {
    right_->perform_type_inference(settings, nullptr);
    type_ = right_->get_node_type();

    if (type_ == BASE_CLASS)
      throw TypeInferenceException("UniOp", "Unary operator called on statement of no type");

    if (opsym == UNARY_OP_NEG && type_ != Quack::Class::Container::Int()) {
      std::string msg = "Operator \"" + opsym + "\" does not match type " + type_->name_;
      throw TypeInferenceException("UniOp", msg);
    }
    return true;
  }

  std::string UniOp::generate_code(CodeGen::Settings &settings, unsigned indent_lvl, bool) const {
    if (opsym != UNARY_OP_NEG)
      throw std::runtime_error("Only unary operation supported is \"" UNARY_OP_NEG "\"");

    Quack::Class * int_class = Quack::Class::Container::Int();
    auto * left = new IntLit(0);
    left->set_node_type(int_class);

    BinOp bin_op(opsym, left, right_);
    bin_op.set_node_type(int_class);
    std::string var_out = bin_op.generate_code(settings, indent_lvl, false);

    // Release memory to prevent getting deleted
    bin_op.right_ = nullptr;
    return var_out;
  }

  bool BinOp::perform_type_inference(TypeCheck::Settings &settings, Quack::Class * parent_type) {

    bool success = left_->perform_type_inference(settings, nullptr);

    // Check the method information
    std::string msg, method_name = op_lookup(opsym);
    Quack::Class * l_type = left_->get_node_type();
    Quack::Method* method = l_type->get_method(method_name);
    if (method == OBJECT_NOT_FOUND) {
      msg = "Operator \"" + opsym + "\" does not exist for class " + l_type->name_;
      throw TypeInferenceException("BinOp", msg);
    }
    if (method->params_->count() != 1) {
      msg = "Binary operator \"" + opsym + "\" for class \"" + l_type->name_
            + "\" should take exactly one argument";
      throw TypeInferenceException("BinOp", msg);
    }

    // Verify the right type is valid
    Quack::Param* param = (*method->params_)[0];
    success = success && right_->perform_type_inference(settings, parent_type);
    Quack::Class * r_type = right_->get_node_type();

    if (!r_type->is_subtype(param->type_)) {
      msg = "Invalid right type \"" + r_type->name_ + "\" for operator \"" + opsym + "\"";
      throw TypeInferenceException("BinOp", msg);
    }

    if (type_ == nullptr)
      type_ = method->return_type_;
    else
      type_ = type_->least_common_ancestor(method->return_type_);

    // Reconcile binary operator return type
    if (type_ == BASE_CLASS) {
      msg = "Invalid return type for binary operator \"" + opsym + "\"";
      throw TypeInferenceException("BinOp", msg);
    }
    return success;
  }

  bool Ident::update_inferred_type(TypeCheck::Settings &settings, Quack::Class *inferred_type,
                                   bool is_field) {
    Symbol * sym = settings.st_->get(text_, is_field);
    assert(sym != OBJECT_NOT_FOUND);

    if (((is_field && settings.is_constructor_) || !is_field) && sym->get_type() == BASE_CLASS)
      settings.st_->update(sym, inferred_type);
    else
      settings.st_->update(sym, inferred_type->least_common_ancestor(sym->get_type()));

    type_ = (type_ == BASE_CLASS) ? sym->get_type() : sym->get_type()->least_common_ancestor(type_);
    if (type_ == BASE_CLASS)
      throw TypeInferenceException("TypeUpdateFail",
                                   "Type inference failed for variable " + text_);
    return true;
  }

  bool Ident::perform_type_inference(TypeCheck::Settings &settings, Quack::Class *parent_type) {
    // If the identifier is this, then mark as the type of this
    if (settings.this_class_ != nullptr && text_ == OBJECT_SELF) {
      Quack::Class * tc = settings.this_class_;
      type_ = (type_ == nullptr) ? tc : type_->least_common_ancestor(tc);

      if (type_ == nullptr)
        throw TypeInferenceException("ThisError", "Invalid type for \"" OBJECT_SELF "\" class");
      return true;
    }

    if (parent_type != nullptr && !parent_type->has_field(text_)) {
      std::string msg = "Unknown field \"" + text_ + "\" for type \"" + parent_type->name_ + "\"";
      throw TypeInferenceException("FieldError", msg);
    }

    Symbol * sym = settings.st_->get(text_, parent_type != nullptr);
    type_ = (type_ == nullptr) ? sym->get_type() : type_->least_common_ancestor(sym->get_type());
    return true;
  }

  bool ObjectCall::update_inferred_type(TypeCheck::Settings &settings, Quack::Class *inferred_type,
                                        bool) {
    if (auto obj = dynamic_cast<Ident *>(object_)) {
      if (obj->text_ == OBJECT_SELF) {
        if (auto next = dynamic_cast<Ident *>(next_)) {
          if (settings.is_constructor_) {
            // Only in a constructor can the field type be change
            next->update_inferred_type(settings, inferred_type, true);
          } else {
            // Outside the constructor, use the field type
            Symbol *sym = settings.st_->get(next->text_, true);
            if (!inferred_type->is_subtype(sym->get_type()))
              throw TypeInferenceException("InferenceError", "Type error for field " + next->text_);
            next->update_inferred_type(settings, sym->get_type(), true);
          }

          type_ = next->get_node_type();
          return true;
        }
      }
    }

    type_ = inferred_type;
    return true;
  }

  bool ObjectCall::perform_type_inference(TypeCheck::Settings &settings, Quack::Class *){
    bool success = true;
    Quack::Class * obj_class;

    // Process the object. Could be either a "this", symbol, or expression
    if (auto obj = dynamic_cast<Ident *>(object_)) {
      if (obj->text_ == OBJECT_SELF)
        obj_class = settings.this_class_;
      else
        obj_class = settings.st_->get(obj->text_, false)->get_type();
    } else {
      // Type infer the object then pass that information to
      success = object_->perform_type_inference(settings, nullptr);
      obj_class = object_->get_node_type();
    }
    object_->set_node_type(obj_class);

    // Infer the call then update the node type
    success = success && next_->perform_type_inference(settings, obj_class);
    if (type_ == nullptr)
      type_ = next_->get_node_type();
    else
      type_ = type_->least_common_ancestor(next_->get_node_type());

    // Some methods (e.g., print) return Nothing
//    if (type_ == BASE_CLASS)
//      throw TypeInferenceException("ObjectCall", "Unable to resolve object call");
    return success;
  }

  const std::string ObjectCall::process_object_call(const std::string &left_obj,
                                                    CodeGen::Settings &settings,
                                                    unsigned indent_lvl, bool is_lhs) const {
    // Use a dynamic cast to handle a method call, e.g. obj.<FuncName>(..)
    if (auto func_call = dynamic_cast<FunctionCall*>(next_)) {
      return func_call->generate_object_call(object_->get_node_type(), left_obj,
                                             settings, indent_lvl, is_lhs);
    }

    // Use a dynamic cast to handle a field reference, e.g., obj.<FieldName>
    // Store the field value in a temporary variable
    if (auto ident = dynamic_cast<Ident*>(next_))
      return generate_temp_var(left_obj + "->" + ident->text_, settings, indent_lvl, is_lhs);

    // THe code should never get here.  This indicates a logic error in the compiler
    throw std::runtime_error("Unexpected bottoming out of ObjectCall code generation");
  }

  bool Typecase::perform_type_inference(TypeCheck::Settings &settings, Quack::Class *) {
    // type case does not have a type
    type_ = Quack::Class::Container::Nothing();
    Quack::Class::Container * all_classes = Quack::Class::Container::singleton();

    bool success = expr_->perform_type_inference(settings, nullptr);

    for (auto * alt : *alts_) {
      std::string new_sym_name = alt->type_names_[0];
      std::string new_sym_type_name = alt->type_names_[1];

      Quack::Class * new_sym_type = all_classes->get(new_sym_type_name);
      if (new_sym_type == OBJECT_NOT_FOUND)
        throw UnknownTypeException(new_sym_type_name);

      // Check and configure the type for the typecase symbol
      Symbol * sym = settings.st_->get(new_sym_name, false);
      if (sym->get_type() != nullptr && !new_sym_type->is_subtype(sym->get_type())) {
        std::string msg = "Cannot reconcile type for variable " + new_sym_name;
        throw TypeInferenceException("TypecaseError", msg);
      }
      settings.st_->update(new_sym_name, false, new_sym_type);

      success = success && alt->block_->perform_type_inference(settings);

      // Verify the symbol name did not change after inference
      if (sym->get_type() != new_sym_type) {
        std::string msg = "Typecase var \"" + new_sym_name + "\" inferred type mismatch.";
        throw TypeInferenceException("TypecaseMismatch", msg);
      }
    }

    return success;
  }

  //====================================================================//
  //                   Code Generation Related Method                   //
  //====================================================================//

  template<typename _T>
  std::string Literal<_T>::generate_lit_code(CodeGen::Settings &settings, unsigned indent_lvl,
                                             const std::string &gen_func_name_) const {
    std::ostringstream ss;
    ss << gen_func_name_ << "(" << value_ << ")";
    return generate_temp_var(ss.str(), settings, indent_lvl, false);
  }

  std::string Typing::generate_code(CodeGen::Settings &settings, unsigned indent_lvl,
                                    bool is_lhs) const {

    std::string gen_var = expr_->generate_code(settings, indent_lvl, is_lhs);
    if (type_name_.empty())
      return gen_var;

//    std::string cast_var = "(" + type_->generated_object_type_name() + ")" + gen_var;
//    return generate_temp_var(cast_var, settings, indent_lvl, is_lhs);
    // Generate a cast temp variable
    return generate_temp_var(gen_var, settings, indent_lvl, is_lhs);
  }


  std::string FunctionCall::generate_code(CodeGen::Settings &settings, unsigned indent_lvl,
                                          bool is_lhs) const {
    Quack::Class * q_class = Quack::Class::Container::singleton()->get(ident_);
    Quack::Param::Container * params = q_class->get_constructor()->params_;
    assert(q_class);

    std::vector<std::string> * arg_vars = args_->generate_args(settings, indent_lvl);

    std::ostringstream ss;
    if (q_class != get_node_type())
      ss << "(" << get_node_type()->generated_object_type_name() << ")";
    ss << q_class->generated_constructor_name() << "(";
    assert(arg_vars->size() == params->count());
    for (unsigned i = 0; i < arg_vars->size(); i++) {
      if (i != 0)
        ss << ", ";
      ss << "(" << (*params)[i]->type_->generated_object_type_name() << ")" << (*arg_vars)[i];
    }
    ss << ")";
    delete arg_vars;

    return generate_temp_var(ss.str(), settings, indent_lvl, is_lhs);
  }

  std::string FunctionCall::generate_object_call(Quack::Class * obj_type, std::string object_name,
                                                 CodeGen::Settings &settings, unsigned indent_lvl,
                                                 bool is_lhs) const {
    std::vector<std::string>* func_tmp_args = args_->generate_args(settings, indent_lvl);

    Quack::Method * method = obj_type->get_method(ident_);

    std::ostringstream ss;
    ss << object_name << "->" << GENERATED_CLASS_FIELD << "->" << ident_ << "("
        << "(" << method->obj_class_->generated_object_type_name() << ")" << object_name;

    Quack::Param::Container * params = method->params_;
    assert(func_tmp_args->size() == params->count());
    for (unsigned i = 0; i < params->count(); i ++) {
      Quack::Class * param_type = (*params)[i]->type_;
      ss << ", " << "(" << param_type->generated_object_type_name() << ")" << (*func_tmp_args)[i];
    }
    ss << ")";
    delete func_tmp_args;

    return generate_temp_var(ss.str(), settings, indent_lvl, is_lhs);
  }

  std::string Assn::generate_code(CodeGen::Settings &settings, unsigned indent_lvl,
                                  bool is_lhs) const {
    if (is_lhs)
      throw std::runtime_error("Cannot have assignment on LHS");

    std::string rhs_var = rhs_->generate_code(settings, indent_lvl, false);
    std::string lhs_var = lhs_->generate_code(settings, indent_lvl, true);

    PRINT_INDENT(indent_lvl);
    settings.fout_ << lhs_var << " = "
                   << "(" << lhs_->get_node_type()->generated_object_type_name() << ")"
                   << "(" << rhs_var << ");\n";
    return NO_RETURN_VAR;
  }

  bool BoolOp::perform_type_inference(TypeCheck::Settings &settings, Quack::Class *) {

    bool success = left_->perform_type_inference(settings, nullptr);

    Quack::Class * bool_class = Quack::Class::Container::Bool();

    std::string msg;
    if (left_->get_node_type() != bool_class) {
      msg = "Invalid left type \"" + left_->get_node_type()->name_ + "\" for op " + opsym + "";
      throw TypeInferenceException("BinOp", msg);
    }

    // Check the method information
    if (opsym == METHOD_AND || opsym == METHOD_OR) {
      if (right_ == nullptr) {
        msg = "Right child missing for binary op " + opsym + "";
        throw TypeInferenceException("BinOp", msg);
      }

      success = success && right_->perform_type_inference(settings, nullptr);
      if (right_->get_node_type() != bool_class) {
        msg = "Invalid left type \"" + right_->get_node_type()->name_ + "\" for op " + opsym + "";
        throw TypeInferenceException("BinOp", msg);
      }
    } else if (opsym == METHOD_NOT) {
      // Nothing to do since single OP
    } else {
      throw TypeInferenceException("BoolOp", "Boolean operator \"" + opsym + "\" does not exist");
    }

    if (type_ == nullptr)
      type_ = bool_class;
    else
      type_ = type_->least_common_ancestor(bool_class);

    // Reconcile binary operator return type
    if (type_ != bool_class) {
      msg = "Invalid return type for Boolean operator \"" + opsym + "\"";
      throw TypeInferenceException("BoolOp", msg);
    }
    return success;
  }

  std::string Typecase::generate_code(CodeGen::Settings &settings, unsigned indent_lvl,
                                      bool is_lhs) const {
    if (is_lhs)
      throw std::runtime_error("Cannot have typecase on LHS");

    // End of the TypeCase
    std::string end_typecase = define_new_label("end_typecase");

    // Build the label set
    std::vector<std::string> labels;
    labels.reserve(alts_->size() + 1);
    for (auto * alt : *alts_)
      labels.emplace_back(define_new_label("typecase_" + alt->type_names_[1] + "_"));
    labels.emplace_back(end_typecase);

    generate_one_line_comment(settings, indent_lvl, "Typecase START");

    std::string typecase_var = expr_->generate_code(settings, indent_lvl, false);

    for (unsigned i = 0; i < alts_->size(); i++) {
      TypeAlternative * alt = (*alts_)[i];

      std::string tc_name = alt->type_names_[1];

      generate_one_line_comment(settings, indent_lvl, "Typecase Type - " + tc_name);

      // Start the typecase check for this type
      generate_label(settings, indent_lvl, labels[i], true);

      Quack::Class * typecase_class = Quack::Class::Container::singleton()->get(tc_name);

      // Go To Next typecase check
      PRINT_INDENT(indent_lvl);
      settings.fout_ << "if(!" << GENERATED_IS_SUBTYPE_FUNC << "("
                     << "(" << Quack::Class::Container::Obj()->generated_clazz_type_name() << ")"
                     << typecase_var << "->"
                     << GENERATED_CLASS_FIELD << ", "
                     << "(" << Quack::Class::Container::Obj()->generated_clazz_type_name() << ")"
                     << "(&" << typecase_class->generated_clazz_obj_struct_name() << ")"
                     << ")) { goto " << labels[i+1] <<  "; }\n";

      // Set assign the expression
      auto * var = new Ident(alt->type_names_[0].c_str());
      Quack::Class * var_class = settings.st_->get(var->text_, false)->get_type();
      var->set_node_type(var_class);
      auto * typing = new Typing(var, "");
      typing->set_node_type(var_class);

      auto * other_var = new Ident(typecase_var.c_str());
      Assn assn(typing, other_var);
      assn.generate_code(settings, indent_lvl+1, false);
      // Prevent an issue where the rhs is deleted
      assn.rhs_ = nullptr;

      alt->block_->generate_code(settings, indent_lvl);

      // End fhe type case
      generate_goto(settings, indent_lvl, end_typecase, true);
    }

    generate_label(settings, indent_lvl, end_typecase, true);
    generate_one_line_comment(settings, indent_lvl, "Typecase END");

    return NO_RETURN_VAR;
  }

}
