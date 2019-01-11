#include <utility>

//
// Created by Michal Young on 9/12/18.
//

#ifndef ASTNODE_H
#define ASTNODE_H

#include <assert.h>

#include <string>
#include <sstream>
#include <vector>
#include <iostream>
#include <fstream>
#include <iomanip>

#include "keywords.h"
#include "initialized_list.h"
#include "exceptions.h"
#include "symbol_table.h"
#include "compiler_utils.h"
#include "code_gen_utils.h"

#define NO_RETURN_VAR ""
#define PRINT_INDENT(a) (settings.fout_ << AST::ASTNode::indent_str(a))
#define PADDING_WIDTH 4

// Forward declaration
namespace Quack { class Class; }

namespace AST {
  // Abstract syntax tree.  ASTNode is abstract base class for all other nodes.
  // This is not the final AST for Quack ... it's a quick-and-dirty version
  // that I copied over from the calculator example, and it isn't even the
  // final version from the calculator example.

  struct ASTNode {
    virtual ~ASTNode() = default;

    virtual void print_original_src(unsigned int indent_depth = 0) = 0;
    virtual bool check_initialize_before_use(InitializedList &inits, InitializedList *all_inits,
                                             bool is_method) = 0;
    /**
     * Updates the initialized list in the initialized per use check.
     *
     * @param inits Set of initialized variables.
     * @param is_constructor True if the function being checked is a constructor.
     */
    virtual void update_initialized_list(InitializedList &inits, bool is_constructor) {}
    /**
     * Implements type inference for a single note in the AST.
     *
     * @param st Symbol table for the method
     * @param return_type Return type from the block
     * @param parent_type Type of the parent node.  If parent node has no type, then BASE_CLASS.
     *
     * @return True if type inference was successful.
     */
    virtual bool perform_type_inference(TypeCheck::Settings &settings,
                                        Quack::Class * parent_type) = 0;
    /**
     * Updates the symbol table and the nodes using an inferred type
     *
     * @param st Symbol table for the method
     * @param inferred_type Type inferred for updating
     * @param this_class Class type of the object
     *
     * @return True if the update is successful
     */
    virtual bool update_inferred_type(TypeCheck::Settings &settings, Quack::Class *inferred_type,
                                      bool is_field) {
      std::string msg = "Unpredicted node type encountered in inferred type update";
      throw TypeCheckerException("Unexpected", msg);
    }
    /**
     * Updates the type for the node.
     *
     * @param type New type for the node
     */
    inline void set_node_type(Quack::Class * new_type) { type_ = new_type; }
    /**
     * Accessor for the type of the node.
     * @return Node type
     */
    inline Quack::Class* get_node_type() const { return type_; }

    virtual std::string generate_code(CodeGen::Settings &settings, unsigned indent_lvl,
                                      bool is_lhs) const = 0;

    void generate_eval_branch(CodeGen::Settings settings, const unsigned indent_lvl,
                              const std::string &true_label, const std::string &false_label);

    static std::string indent_str(unsigned indent_level) {
      return std::string(indent_level, '\t');
    }
    /**
     * Helper function used to create a label using the label header and a unique integer
     * to ensure that there are no duplicate labels.
     *
     * @param label_header Header used for the label
     * @return Unique label
     */
    static const std::string define_new_label(const std::string &label_header) {
      std::ostringstream ss;
      ss << label_header << std::setfill('0') << std::setw(PADDING_WIDTH) << label_cnt_++;
      return ss.str();
    }
    /**
     * Standardized helper function to generte a label in the output.
     *
     * @param settings Code generation settings
     * @param indent_lvl Level of indentation
     * @param label Label to generate
     */
    static void generate_label(CodeGen::Settings &settings, unsigned indent_lvl,
                               const std::string &label, bool add_new_line=false) {
      PRINT_INDENT(indent_lvl);
      settings.fout_ << label << ": ; /* Null statement */";
      if (add_new_line)
        settings.fout_ << "\n";
    }
    /**
     * Standard helper function to jump to the passed label.
     *
     * @param settings
     * @param indent_lvl
     * @param label Label to go to.
     */
    static void generate_goto(CodeGen::Settings &settings, unsigned indent_lvl,
                              const std::string &label, bool add_new_line=false) {
      PRINT_INDENT(indent_lvl);
      settings.fout_ << "goto " << label << ";";
      if (add_new_line)
        settings.fout_ << "\n";
    }
    /**
     * Helper function used to generate temporary variable names
     *
     * @return Temporary variable name
     */
    static const std::string define_new_temp_var() {
      std::ostringstream ss;
      ss << TEMP_VAR_HEADER << std::setfill('0') << std::setw(PADDING_WIDTH) << var_cnt_++;
      return ss.str();
    }
    /**
     * Helper function that standardizes the generation of new temporary variables.
     *
     * @param var_to_store Variable to store in a temporary
     * @param settings Code generation settings
     * @param indent_lvl Level of indentation
     * @return Pointer to the memory location
     */
    std::string generate_temp_var(const std::string &var_to_store, CodeGen::Settings settings,
                                  unsigned indent_lvl, bool is_lhs) const;
    /**
     * Standardizes creating a one line comment.
     *
     * @param settings Code generator settings
     * @param indent_lvl Indentation level
     * @param msg Comment message
     */
    static void generate_one_line_comment(CodeGen::Settings settings, const unsigned indent_lvl,
                                          const std::string &msg) {
      PRINT_INDENT(indent_lvl);
      settings.fout_ << "/* " << msg << " */\n";
    }
    /**
     * Checks whether the statement has a return on all paths.
     *
     * @return True the tree node has a return on all possible subpaths
     */
    virtual bool contains_return_all_paths() { return false; }

   protected:
    /** Type for the node */
    Quack::Class * type_ = nullptr;
    /**
     * Counter for label generator for GoTo's
     */
    static unsigned long label_cnt_;
    /**
     * Counter for temporary variables created in the code
     */
    static unsigned long var_cnt_;
  };

  /* A block is a sequence of statements or expressions.
   * For simplicity we'll just make it a sequence of ASTNode,
   * and leave it to the parser to build valid structures.
   */
  class Block {
   public:
    Block() : stmts_{std::vector<ASTNode *>()} {}

    ~Block() {
      for (const auto &ptr : stmts_)
        delete ptr;
    }

    void append(ASTNode *stmt) { stmts_.push_back(stmt); }

    void print_original_src(unsigned int indent_depth) {
      bool is_first = true;
      std::string indent_str = std::string(indent_depth, '\t');
      for (const auto &stmt : stmts_) {
       if (!is_first)
         std::cout << "\n";
       is_first = false;

       std::cout << indent_str;
       stmt->print_original_src(indent_depth);
       std::cout << ";";
      }
    }

    bool check_initialize_before_use(InitializedList &inits, InitializedList *all_inits,
                                     bool is_method) {
      bool success = true;
      for (auto &stmt : stmts_)
        success = success && stmt->check_initialize_before_use(inits, all_inits, is_method);
      return success;
    }

    bool perform_type_inference(TypeCheck::Settings &settings) {
      for (auto * stmt : stmts_)
        stmt->perform_type_inference(settings, nullptr);
      return true;
    }
    /**
     * Generates the code for a block of statements
     *
     * This function also increases the indent level for all statements in the block.
     *
     * @param settings
     * @param indent_lvl Incoming number of indents
     */
    void generate_code(CodeGen::Settings &settings, unsigned indent_lvl = 0) {
      std::string indent_str = AST::ASTNode::indent_str(indent_lvl);

      for (auto * stmt : stmts_)
        stmt->generate_code(settings, indent_lvl + 1, false);
    }
    /**
     * Checks whether the block has a return on all paths through the block.
     *
     * @return True if it contains a return on all paths.
     */
    bool contains_return_all_paths() {
      for (auto * stmt : stmts_)
        if (stmt->contains_return_all_paths())
          return true;
      return false;
    }

    bool empty() { return stmts_.empty(); }
   private:
    std::vector<ASTNode *> stmts_;
  };

  class If : public ASTNode {
   public:
    explicit If(ASTNode *cond, Block* truepart, Block* falsepart) :
        cond_{cond}, truepart_{truepart}, falsepart_{falsepart} {};

    ~If() {
      delete cond_;
      delete truepart_;
      delete falsepart_;
    }

    void print_original_src(unsigned int indent_depth = 0) override {
      std::string indent_str = std::string(indent_depth, '\t');
      std::cout << "if ";
      cond_->print_original_src();
      std::cout << " {\n";
      truepart_->print_original_src(indent_depth + 1);
      std::cout << (!truepart_->empty() ? "\n" : "") << indent_str << "}";

      if (!falsepart_->empty()) {
        std::cout << " else {\n";
        falsepart_->print_original_src(indent_depth + 1);
        std::cout << "\n" << indent_str << "}";
      }
    }
    /**
     * Specialized implementation of the initialize before use type check.  The initialized list
     * is modified by the function and represents the set of variables in the intersection of
     * the variables from the true and false blocks.
     *
     * @param inits Initialized list modified in place
     * @return True if the check passed.
     */
    bool check_initialize_before_use(InitializedList &inits, InitializedList *all_inits,
                                     bool is_method) override {
      InitializedList false_lits = inits;

      bool success = cond_->check_initialize_before_use(inits, all_inits, is_method)
                     && truepart_->check_initialize_before_use(inits, all_inits, is_method)
                     && falsepart_->check_initialize_before_use(false_lits, all_inits, is_method);
      // This is used in the constructor to make sure fields initialized on all paths
      if (all_inits != nullptr) {
        *all_inits = inits;
        all_inits->var_union(false_lits);
      }

      inits.var_intersect(false_lits);

      return success;
    }
    /**
     * Generates the code for an If block.
     *
     * @param settings Code generator settings
     * @param indent_lvl Indentation level for the generated code
     * @return No variable name returned.  This is because no temporary variables are associated
     *         with an If.
     */
    std::string generate_code(CodeGen::Settings &settings, unsigned indent_lvl,
                              bool is_lhs) const override {
      if (is_lhs)
        throw std::runtime_error("LHS is not possible in IF block");

      std::string if_label = define_new_label("if");
      std::string else_label = define_new_label("else");
      std::string end_if_label = define_new_label("end_if");

      cond_->generate_eval_branch(settings, indent_lvl, if_label, else_label);

      generate_one_line_comment(settings, indent_lvl, "True Part If");
      generate_label(settings, indent_lvl, if_label, true);

      truepart_->generate_code(settings, indent_lvl + 1);

      generate_goto(settings, indent_lvl, end_if_label, true);

      generate_one_line_comment(settings, indent_lvl, "False Part If");
      generate_label(settings, indent_lvl, else_label, true);

      if (falsepart_)
        falsepart_->generate_code(settings, indent_lvl + 1);

      generate_one_line_comment(settings, indent_lvl, "End If");
      generate_label(settings, indent_lvl, end_if_label, true);

      return NO_RETURN_VAR;
    }
    /**
     * Checks whether the if block contains a return in both the true and false parts
     *
     * @return True both the true and false parts always raise a return
     */
    bool contains_return_all_paths() override {
      return truepart_->contains_return_all_paths() && falsepart_->contains_return_all_paths();
    }

    bool perform_type_inference(TypeCheck::Settings &settings, Quack::Class * parent_type) override;
   private:
    ASTNode *cond_; // The boolean expression to be evaluated
    Block *truepart_; // Execute this block if the condition is true
    Block *falsepart_; // Execute this block if the condition is false
  };

  /* Identifiers like x and literals like 42 are the
   * leaves of the AST.  A literal can only be evaluated
   * for value_ (the 'eval' method), but an identifier
   * can also be evaluated for location (when we want to
   * store something in it).
   */
  struct Ident : public ASTNode {
    explicit Ident(const char* txt) : text_{txt} {}

    void print_original_src(unsigned int indent_depth = 0) override { std::cout << text_; }

    bool check_initialize_before_use(InitializedList &inits, InitializedList *all_inits,
                                     bool is_method) override {
      return check_ident_initialized(inits, all_inits, false);
    }
    /**
     * Generalized function for checking identifiers including constructor identifiers.
     *
     * @param inits Initialized variable set
     * @param all_inits All initialized variables so far
     * @param is_field True if the identifier corresponds to a field
     * @return True if the identifier is initialized
     */
    bool check_ident_initialized(InitializedList &inits, InitializedList *all_inits,
                                 bool is_field=false) {
      if (text_ == OBJECT_SELF)
        return true;

      if (!inits.exists(text_, is_field)) {
        throw UnitializedVarException(typeid(this).name(), text_, is_field);
//        return false;
      }
      return true;
    }
    /**
     * Adds the implicit identifier to the initialized variable list.  If this function is called
     * directly, the initialized variable is marked as not a field of the class.
     *
     * @param inits Set of initialized variables.
     * @param is_constructor True if the function is a constructor.  It has no effect in this
     *                       function.
     */
    void update_initialized_list(InitializedList &inits, bool is_constructor) override {
      add_identifier_to_initialized(inits, false);
    }
    /**
     * Add the identifier to the initialized variable list.
     *
     * @param inits Set of initialized variables
     * @param is_field True if the identifier corresponds to a field.
     */
    void add_identifier_to_initialized(InitializedList &inits, bool is_field) {
      inits.add(text_, is_field);
    }
    /**
     * Updates the symbol table and the node of the symbol in the symbol table and the AST>
     *
     * @param st Symbol table
     * @param inferred_type Inferred type of the symbol
     *
     * @return True if the inferred type was successfully updated
     */
    bool update_inferred_type(TypeCheck::Settings &settings, Quack::Class *inferred_type,
                              bool is_field) override;

    bool perform_type_inference(TypeCheck::Settings &settings, Quack::Class * parent_type) override;
    /**
     * Simply prints the identifier name.
     *
     * @param settings Code generator settings.
     * @param indent_lvl Level of indentation.
     */
    std::string generate_code(CodeGen::Settings &settings, unsigned indent_lvl ,
                              bool is_lhs) const override {
      return text_;
    }
    /** Identifier name */
    const std::string text_;
  };

  template <typename _T>
  struct Literal : public ASTNode {
    explicit Literal(const _T &v) : value_{v} {}
    /**
     * No initialization before use check for a literal.  This function simply returns true.
     *
     * @param inits Set of initialized variables.  Not changed in the funciton.
     * @return True always.
     */
    bool check_initialize_before_use(InitializedList &inits, InitializedList *all_inits,
                                     bool is_method) override {
      return true;
    }

    bool perform_type_inference(TypeCheck::Settings &settings, Quack::Class * parent_type) override{
      throw AmbiguousInferenceException(typeid(this).name(), "Not able to infer type for literal");
    }
    /** Value of the literal */
    const _T value_;
   protected:
    /**
     * Helper function that standardizes code generation for all literals.
     *
     * @param settings Code generator settings object
     * @param indent_lvl Indention level
     * @param gen_func_name_ Function used to generate objects of a given type
     */
    std::string generate_lit_code(CodeGen::Settings &settings, unsigned indent_lvl,
                                  const std::string &gen_func_name_) const;
  };

  struct IntLit : public Literal<int>{
    explicit IntLit(int v) : Literal(v) {};

    void print_original_src(unsigned int indent_depth = 0) override {
      std::cout << std::to_string(value_);
    }

    std::string generate_code(CodeGen::Settings &settings, unsigned indent_lvl,
                              bool is_lhs) const override {
      return generate_lit_code(settings, indent_lvl, GENERATE_LIT_INT_FUNC);
    }

    bool perform_type_inference(TypeCheck::Settings &settings, Quack::Class * parent_type) override;
  };

  struct BoolLit : public Literal<bool>{
    explicit BoolLit(bool v) : Literal(v) {};

    void print_original_src(unsigned int indent_depth = 0) override {
      std::cout << (value_ ? "true" : "false");
    }

    std::string generate_code(CodeGen::Settings &settings, unsigned indent_lvl,
                              bool is_lhs) const override {
      if (value_)
        return GENERATED_LIT_TRUE;
      return GENERATED_LIT_FALSE;
    }

    bool perform_type_inference(TypeCheck::Settings &settings, Quack::Class * parent_type) override;
  };

 struct NothingLit : public Literal<std::string>{
    NothingLit() : Literal<std::string>("")  {};

    void print_original_src(unsigned int indent_depth = 0) override {
      std::cout << GENERATED_LIT_NONE;
    }

    std::string generate_code(CodeGen::Settings &settings, unsigned indent_lvl,
                              bool is_lhs) const override {
      return GENERATED_LIT_NONE;
    }

    bool perform_type_inference(TypeCheck::Settings &settings, Quack::Class * parent_type) override;
  };

  struct StrLit : public Literal<std::string> {
    explicit StrLit(const char* v) : Literal<std::string>(std::string(v)) {}

    void print_original_src(unsigned int indent_depth = 0) override {
      std::cout  << "\"" << value_ << "\"";
    }
    /**
     * Generates the code to create a string literal.  It relies on the standardized literal
     * generation method.
     *
     * @param settings Code generator setting
     * @param indent_lvl Level of indention
     */
    std::string generate_code(CodeGen::Settings &settings, unsigned indent_lvl,
                              bool is_lhs) const override {
      std::ostringstream ss;
      ss << GENERATE_LIT_STRING_FUNC << "(\"" << value_ << "\")";
      return generate_temp_var(ss.str(), settings, indent_lvl, false);
    }

    bool perform_type_inference(TypeCheck::Settings &settings, Quack::Class * parent_type) override;
  };

  struct Return : public ASTNode {
    ASTNode* right_;

    explicit Return(ASTNode* right) : right_(right) {}

    ~Return() {
      delete right_;
    }

    void print_original_src(unsigned int indent_depth = 0) override {
      std::cout << "return ";
      right_->print_original_src();
    }
    /**
     * Checks if the initialize before use test passes on the right subexpression.
     *
     * @param inits Set of initialized variables.
     * @return True if the initialized before use test passes for the right subexpression.
     */
    bool check_initialize_before_use(InitializedList &inits, InitializedList *all_inits,
                                     bool is_method) override {
      return right_->check_initialize_before_use(inits, all_inits, is_method);
    }
    /**
     * Generates the C code associated with a return statement.  The implementation is quite simple
     *
     * @param settings Code generator settings
     * @param indent_lvl Level of indentation
     */
    std::string generate_code(CodeGen::Settings &settings, unsigned indent_lvl,
                              bool is_lhs) const override;
    /**
     * Always returns true since this is a return statement.
     *
     * @return Always true
     */
    bool contains_return_all_paths() override { return true; }

    bool perform_type_inference(TypeCheck::Settings &settings, Quack::Class * parent_type) override;
  };

  struct While : public ASTNode {
    ASTNode* cond_;
    Block* body_;

    explicit While(ASTNode* cond, Block* body) : cond_(cond), body_(body) {};

    ~While() {
      delete cond_;
      delete body_;
    }

    void print_original_src(unsigned int indent_depth = 0) override {
      std::string indent_str = std::string(indent_depth, '\t');
      std::cout << "While ";
      cond_->print_original_src();
      std::cout << " {\n";

      body_->print_original_src(indent_depth + 1);

      if (!body_->empty())
        std::cout << "\n";
      std::cout << indent_str << "}";
    }
    /**
     * Performs an initialize before use test on the while loop.  It checks the conditional
     * statement as well as the body of the while loop.
     *
     * @param inits Set of initialized variables.
     * @return True if the initialized before use test passes for the conditional statement
     *         as well as the body for the while loop
     */
    bool check_initialize_before_use(InitializedList &inits, InitializedList *all_inits,
                                     bool is_method) override {
      bool success = cond_->check_initialize_before_use(inits, all_inits, is_method);

      InitializedList body_temp_list = inits;
      success = success && body_->check_initialize_before_use(body_temp_list, all_inits, is_method);

      if (all_inits != nullptr)
        all_inits->var_union(body_temp_list);

      return success;
    }

    bool perform_type_inference(TypeCheck::Settings &settings, Quack::Class * parent_type) override;

    std::string generate_code(CodeGen::Settings &settings, unsigned indent_lvl,
                              bool is_lhs) const override{
      if (is_lhs)
        throw std::runtime_error("While loop cannot be on LHS");

      std::string test_cond_label = define_new_label("test_cond");
      std::string loop_again_label = define_new_label("loop_again");
      std::string end_while_label = define_new_label("end_while");

      generate_one_line_comment(settings, indent_lvl, "WHILE Loop Start");
      generate_goto(settings, indent_lvl, test_cond_label, true);
      generate_label(settings, indent_lvl, loop_again_label, true);

      // Body of the loop is a simple block
      body_->generate_code(settings, indent_lvl + 1);

      generate_label(settings, indent_lvl, test_cond_label, true);

      // Checks while condition
      cond_->generate_eval_branch(settings, indent_lvl, loop_again_label, end_while_label);
      generate_label(settings, indent_lvl, end_while_label, true);

      // Comment for clarity. Delete if cluttering
      generate_one_line_comment(settings, indent_lvl, "END WHILE Loop");

      return NO_RETURN_VAR;
    }
  };

  struct RhsArgs : public ASTNode {
    std::vector<ASTNode*> args_;

    RhsArgs() {}

    ~RhsArgs() {
      for (const auto &arg: args_)
        delete arg;
    }
    /**
     * Accessor for number of arguments in the node.
     * @return Number of arguments
     */
    unsigned long count() { return args_.size(); }
//    /**
//     * Checks whether there are any input arguments
//     * @return True if there are no arguments
//     */
//    bool empty() { return args_.empty(); }

    void add(ASTNode* new_node) { args_.emplace_back(new_node); }

    void print_original_src(unsigned int indent_depth) override {
      bool is_first = true;
      for (const auto &arg : args_) {
        if (!is_first)
          std::cout << ", ";
        is_first = false;
        arg->print_original_src(indent_depth);
      }
    }
    /**
     * Not supported.  Argument code must be generated specially.
     *
     * @param settings Code generator settings
     * @param indent_lvl Level of indentation
     */
    std::string generate_code(CodeGen::Settings &settings, unsigned indent_lvl,
                              bool is_lhs) const override {
      throw std::runtime_error("Cannot generate RHS args similar to normal args");
    }
    /**
     * Generates the source code for all arguments in the argument set.
     *
     * @param settings Code generator settings
     * @param indent_lvl Level of indentation in the generated code
     * @return Vector containing the variables (usually temporary) for each argument in the array
     */
    std::vector<std::string>* generate_args(CodeGen::Settings &settings,
                                            unsigned indent_lvl) const {
      std::vector<std::string> * gen_args = new std::vector<std::string>();

      for (auto * arg: args_) {
        if (auto arg_cast = dynamic_cast<Ident *>(arg)) {
          gen_args->emplace_back(arg_cast->text_);
        } else {
          // Cannot have ARGS on LHS even if incoming is LHS
          std::string temp_var = arg->generate_code(settings, indent_lvl, false);
          gen_args->emplace_back(temp_var);
        }
      }

      return gen_args;
    }

    bool perform_type_inference(TypeCheck::Settings &settings, Quack::Class * parent_type) override{
      std::string msg = "Type inference not valid for right hand side args";
      throw TypeInferenceException("UnexpectedStateReached", msg);
    }

    /**
     * Checks if the initialize before use test passes on the right subexpression.
     *
     * @param inits Set of initialized variables.
     * @return True if the initialized before use test passes for the right subexpression.
     */
    bool check_initialize_before_use(InitializedList &inits, InitializedList *all_inits,
                                     bool is_method) override {
      bool success = true;
      for (auto &arg : args_)
        success = success && arg->check_initialize_before_use(inits, all_inits, is_method);
      return success;
    }
  };

  struct FunctionCall : public ASTNode {
    const std::string ident_;
    RhsArgs* args_;

    FunctionCall(const char* ident, RhsArgs* args) : ident_(ident), args_(args) {}

    ~FunctionCall() {
      delete args_;
    }
    /**
     * Checks if the initialize before use test passes on the right subexpression.
     *
     * @param inits Set of initialized variables.
     * @return True if the initialized before use test passes for the right subexpression.
     */
    bool check_initialize_before_use(InitializedList &inits, InitializedList *all_inits,
                                     bool is_method) override {
      return args_->check_initialize_before_use(inits, all_inits, is_method);
    }

    void print_original_src(unsigned int indent_depth) override {
      std::cout << ident_ << "(";
      args_->print_original_src(indent_depth);
      std::cout << ")";
    }
    /**
     * This generate_code method only gets called directly for constructors.  All other calls
     * will be through an ObjectCall object and will have a special implementation (see method
     * generate_object_call);
     *
     * @param settings Code generation settings
     * @param indent_lvl Level of indentation
     * @return Variable where the output of the constructor is stored.
     */
    std::string generate_code(CodeGen::Settings &settings, unsigned indent_lvl,
                              bool is_lhs) const override;

    bool perform_type_inference(TypeCheck::Settings &settings, Quack::Class * parent_type) override;
    /**
     * Function call for an object name.
     *
     * @param object_name Name of the object whose method is being called.
     * @param settings Code generator settings
     * @param indent_lvl Level of indentation
     * @return Variable where the function call will be stored
     */
    std::string generate_object_call(Quack::Class * obj_type, std::string object_name,
                                     CodeGen::Settings &settings, unsigned indent_lvl,
                                     bool is_lhs) const;
  };


  struct ObjectCall : public ASTNode {
    ASTNode* object_;
    ASTNode* next_;

    ObjectCall(ASTNode* object, ASTNode* next) : object_(object), next_(next) {}

    ~ObjectCall() {
      delete object_;
      delete next_;
    }

    void print_original_src(unsigned int indent_depth) override {
      object_->print_original_src(indent_depth);
      std::cout << ".";
      next_->print_original_src(indent_depth);
    }
    /**
     * Checks if the initialize before use test passes on the right subexpression.
     *
     * @param inits Set of initialized variables.
     * @return True if the initialized before use test passes for the right subexpression.
     */
    bool check_initialize_before_use(InitializedList &inits, InitializedList *all_inits,
                                     bool is_method) override {
      if (auto obj = dynamic_cast<Ident*>(object_)) {
        if (all_inits != nullptr && obj->text_ == OBJECT_SELF) {
          if (auto next = dynamic_cast<Ident *>(next_))
            return next->check_ident_initialized(inits, all_inits, true);
        } else if(is_method && obj->text_ == OBJECT_SELF) {
          // No need to check fields for this class outside the constructor
          return true;
        }
      }

      bool success = object_->check_initialize_before_use(inits, all_inits, is_method);
      if (dynamic_cast<Ident*>(next_) != nullptr)
        return success;

      return success && next_->check_initialize_before_use(inits, all_inits, is_method);
    }

    void update_initialized_list(InitializedList &inits, bool is_field) override {
      if (auto obj = dynamic_cast<Ident*>(object_))
        if (is_field && obj->text_ == OBJECT_SELF)
          if (auto next = dynamic_cast<Ident*>(next_))
            next->add_identifier_to_initialized(inits, true);
    }
    /**
     * Processes object calls in the quack program.  Object calls take two forms namely:
     * obj.<field> and obj.<method>(...).  Dynamic casting is used to determine which of the
     * two applies for this specific case of code generator.
     *
     * @param settings Code generator settings
     * @param indent_lvl Level of indentation.
     * @param is_lhs True if the node corresponds to a left hand side.
     * @return Any object call string.
     */
    std::string generate_code(CodeGen::Settings &settings, unsigned indent_lvl,
                              bool is_lhs) const override {
      // Handle the bottom out of the recursion
      if (auto obj = dynamic_cast<Ident*>(object_))
        return process_object_call(obj->text_, settings, indent_lvl, is_lhs);

      std::string left_obj = object_->generate_code(settings, indent_lvl, is_lhs);
      return process_object_call(left_obj, settings, indent_lvl, is_lhs);
    }
    /**
     * After the left object is processed, process the right object.
     *
     * @param left_obj Left object of the object call
     * @param settings Code generator settings
     * @param indent_lvl Level of indentation
     *
     * @return Variable containing the combined results of the object call
     */
    const std::string process_object_call(const std::string &left_obj, CodeGen::Settings &settings,
                                          unsigned indent_lvl, bool is_lhs) const;

    bool perform_type_inference(TypeCheck::Settings &settings, Quack::Class * parent_type) override;

    bool update_inferred_type(TypeCheck::Settings &settings, Quack::Class *inferred_type,
                              bool is_field) override;
  };

  struct BinOp : public ASTNode {
    std::string opsym;
    ASTNode *left_;
    ASTNode *right_;

    BinOp(const std::string &sym, ASTNode *l, ASTNode *r) : opsym{sym}, left_{l}, right_{r} {};

    ~BinOp() {
      delete left_;
      delete right_;
    }

    void print_original_src(unsigned int indent_depth = 0) override {
      std::cout << "(";
      left_->print_original_src();
      std::cout << " " << opsym << " ";
      right_->print_original_src();
      std::cout << ")";
    }
    /**
     * Checks if the initialize before use test on the two subexpressions passes.
     *
     * @param inits Set of initialized variables.
     * @return True if the initialized before use test passes for both subexpressions.
     */
    bool check_initialize_before_use(InitializedList &inits, InitializedList *all_inits,
                                     bool is_method) override {
      return left_->check_initialize_before_use(inits, all_inits, is_method)
             && (opsym == UNARY_OP_NOT
                 || (right_ && right_->check_initialize_before_use(inits, all_inits, is_method)));
    }
    /**
     * Helper function to get the method name that desugars the binary operator.
     *
     * @param op Binary operator value
     *
     * @return Desugared method name.
     */
    static const std::string op_lookup(const std::string &op) {
      if (op == "+")
        return METHOD_ADD;
      else if (op == "-")
        return METHOD_SUBTRACT;
      else if (op == "*")
        return METHOD_MULTIPLY;
      else if (op == "/")
        return METHOD_DIVIDE;
      else if (op == ">=")
        return METHOD_GEQ;
      else if (op == ">")
        return METHOD_GT;
      else if (op == "<=")
        return METHOD_LEQ;
      else if (op == "<")
        return METHOD_LT;
      else if (op == "==")
        return METHOD_EQUALITY;
      throw UnknownBinOpException(op);
    }
    /**
     * Binary operators are syntactic sugar for function cals.  Therefore, turn a binary operator
     * node in the tree into an object call.
     *
     * @param settings Code generator settings
     * @param indent_lvl Level of indentation
     */
    virtual std::string generate_code(CodeGen::Settings &settings, unsigned indent_lvl,
                                      bool is_lhs) const override {
      if (is_lhs)
        throw std::runtime_error("Boolean operator cannot be on LHS");

      // Create the ObjectCall stand-in AST node
      RhsArgs args;
      args.add(right_);
      args.args_[0]->set_node_type(right_->get_node_type());

      FunctionCall func_call(op_lookup(opsym).c_str(), &args);
      func_call.set_node_type(this->type_);

      ObjectCall obj_call(left_, &func_call);
      obj_call.set_node_type(this->type_);
      std::string obj_out = obj_call.generate_code(settings, indent_lvl, is_lhs);

      // Clean up the memory to prevent deleting memory accidentally
      args.args_[0] = nullptr;
      obj_call.object_ = nullptr;
      obj_call.next_ = nullptr;
      func_call.args_ = nullptr;

      // No deletion needed.  Relies on the destructor of ObjectCall which is on the stack
      return obj_out;
    }

    virtual bool perform_type_inference(TypeCheck::Settings &settings,
                                        Quack::Class * parent_type) override;
  };

  struct BoolOp : public BinOp {
    /** Boolean operator constructor */
    BoolOp(const std::string &sym, ASTNode *l, ASTNode *r) : BinOp(sym, l, r) {};

    std::string generate_code(CodeGen::Settings &settings, unsigned indent_lvl,
                              bool is_lhs) const override {
      if (is_lhs)
        throw std::runtime_error("BoolOp cannot be a left hand side");

      if (opsym == UNARY_OP_NOT) {
        generate_one_line_comment(settings, indent_lvl, "NOT Start");
        std::string op_var = left_->generate_code(settings, indent_lvl, is_lhs);
        std::string gen_var = "(" + op_var + " == " + GENERATED_LIT_FALSE + ")";
        return generate_temp_var(gen_var, settings, indent_lvl, false);
      }
      // Variable that will store the evaluated result
      std::string eval_bool = generate_temp_var(GENERATED_LIT_FALSE, settings, indent_lvl, false);

      // Labels for jumping
      std::string bool_halfway = define_new_label(opsym + "_HALFWAY");
      std::string bool_true = define_new_label(opsym + "_TRUE");
      std::string bool_end = define_new_label(opsym + "_END");

      // Left Side of Boolean
      generate_one_line_comment(settings, indent_lvl, opsym + " Left Condition");
      if (opsym == METHOD_AND) {
        left_->generate_eval_branch(settings, indent_lvl + 1, bool_halfway, bool_end);
      } else if (opsym == METHOD_OR) {
        left_->generate_eval_branch(settings, indent_lvl + 1, bool_true, bool_halfway);
      } else {
        throw std::runtime_error("Unknown Boolean operator \"" + opsym + "\"");
      }

      generate_label(settings, indent_lvl, bool_halfway);

      generate_one_line_comment(settings, indent_lvl, opsym + " Right Condition");
      right_->generate_eval_branch(settings, indent_lvl + 1, bool_true, bool_end);

      // Short Circuit True
      generate_one_line_comment(settings, indent_lvl, "Boolean Get True");
      generate_label(settings, indent_lvl, bool_true, true);
      PRINT_INDENT(indent_lvl);
      settings.fout_ << eval_bool << " = " << GENERATED_LIT_TRUE << ";\n";


      // End Boolean
      generate_label(settings, indent_lvl, bool_end, true);
      generate_one_line_comment(settings, indent_lvl, opsym + " End");
      return eval_bool;
    }

    /**
     * Special handling of the short circuit Boolean operators
     *
     * @param settings Code generator settings
     * @param indent_lvl Level of indentaiton
     * @param true_label Label to jump to if the Boolean operator evaluates to true
     * @param false_label Label to jump to if the Boolean operator evaluates to false
     */
    void generate_eval_bool_op(CodeGen::Settings settings, const unsigned indent_lvl,
                               const std::string &true_label, const std::string &false_label) {
      if (opsym == UNARY_OP_NOT) {
        return left_->generate_eval_branch(settings, indent_lvl, false_label, true_label);
      }

      std::string halfway_label = define_new_label("halfway");
      if (opsym == METHOD_AND) {
        generate_one_line_comment(settings, indent_lvl, "Generate AND");
        left_->generate_eval_branch(settings, indent_lvl + 1, halfway_label, false_label);
        generate_label(settings, indent_lvl, halfway_label, true);
      } else if (opsym == METHOD_OR) {
        generate_one_line_comment(settings, indent_lvl, "Generate OR");
        left_->generate_eval_branch(settings, indent_lvl + 1, true_label, halfway_label);
        generate_label(settings, indent_lvl, halfway_label, true);
      } else {
        throw std::runtime_error("Unknown Boolean operator " + opsym);
      }
      right_->generate_eval_branch(settings, indent_lvl + 1, true_label, false_label);
    }

    bool perform_type_inference(TypeCheck::Settings &settings, Quack::Class *parent_type) override;
  };

  struct UniOp : public ASTNode {
    std::string opsym;
    ASTNode *right_;

    UniOp(const std::string &sym, ASTNode *r) : opsym{std::move(sym)}, right_{r} {};

    ~UniOp() { delete right_; }

    void print_original_src(unsigned int indent_depth = 0) override {
      std::cout << "(" << opsym << " ";
      right_->print_original_src();
      std::cout << ")";
    }
    /**
     * Checks if the initialize before use test passes on the right subexpression.
     *
     * @param inits Set of initialized variables.
     * @return True if the initialized before use test passes for the right subexpression.
     */
    bool check_initialize_before_use(InitializedList &inits, InitializedList *all_inits,
                                     bool is_method) override {
      return right_->check_initialize_before_use(inits, all_inits, is_method);
    }

    std::string generate_code(CodeGen::Settings &settings, unsigned indent_lvl,
                              bool is_lhs) const override;

    bool perform_type_inference(TypeCheck::Settings &settings, Quack::Class * parent_type) override;
  };

  struct Typing : public ASTNode {
    Typing(ASTNode* expr, const std::string &type_name) : expr_(expr), type_name_(type_name) {}

    ~Typing() {
      delete expr_;
    }

    ASTNode* expr_;
    std::string type_name_;

    void print_original_src(unsigned int indent_depth) override {
      expr_->print_original_src(indent_depth);
      if (!type_name_.empty())
        std::cout << " : " << type_name_;
    }

    std::string generate_code(CodeGen::Settings &settings, unsigned indent_lvl,
                              bool is_lhs) const override;
//    /**
//     * Helper function used to check if the specified type name actually exists.
//     *
//     * @param type_name Name of the type used
//     * @return True if the type_name_ is a valid class.
//     */
//    bool check_type_name_exists(const std::string &type_name) const;

    bool check_initialize_before_use(InitializedList &inits, InitializedList *all_inits,
                                     bool is_method) override {
//      if (!check_type_name_exists(type_name_))
//        return false;

      return expr_->check_initialize_before_use(inits, all_inits, is_method);
    }
    /**
     * Used in an assignment statement to update the set of initialized variables.
     *
     * @param inits Set of initialized variables.
     * @param is_constructor True if the function is a constructor.
     */
    void update_initialized_list(InitializedList &inits, bool is_constructor) override {
      expr_->update_initialized_list(inits, is_constructor);
    }

    bool update_inferred_type(TypeCheck::Settings &settings, Quack::Class *inferred_type,
                              bool is_field) override;

    bool perform_type_inference(TypeCheck::Settings &settings, Quack::Class * parent_type) override{
      configure_initial_typing(BASE_CLASS);

      bool success = expr_->perform_type_inference(settings, parent_type);

      return success && verify_typing();
    }

   private:
    /**
     * Configures the initial typing of the node predicated on the specified type name.
     *
     * @param other_type Other type n
     * @return
     */
    bool configure_initial_typing(Quack::Class* other_type);
    /**
     * Checks that the typing of the expression is compatible with the typing of the node (if any)
     *
     * @return True if typing successfully verified
     */
    bool verify_typing();
  };

  class TypeAlternative {
   public:
    TypeAlternative(const std::string &t1, const std::string &t2, Block* block)
              : type_names_{t1,t2}, block_(block) {}

    ~TypeAlternative() { delete block_; }

    void print_original_src(unsigned int indent_depth) {
      std::cout << type_names_[0] << " : " << type_names_[1] << " {\n";
      block_->print_original_src(indent_depth + 1);

      std::cout << "\n" << std::string(indent_depth, '\t') << "}";
    }

    std::string type_names_[2];
    Block* block_;
  };


  struct Assn : public ASTNode {
    Typing* lhs_;
    ASTNode* rhs_;

    Assn(Typing* lhs, ASTNode* rhs) : lhs_(lhs), rhs_(rhs) {};

    ~Assn() {
      delete lhs_;
      delete rhs_;
    }

    void print_original_src(unsigned int indent_depth = 0) override {
      lhs_->print_original_src(indent_depth);
      std::cout << " = ";
      rhs_->print_original_src(indent_depth);
    }
    /**
     * Verifies that both the left and right hand side of statement pass the initialize before
     * use test.  It next adds the assigned variable to the initialized list.
     * @param inits Set of initialized variables
     * @return True if all initialized before use test passes.
     */
    bool check_initialize_before_use(InitializedList &inits, InitializedList *all_inits,
                                     bool is_method) override {
      bool success = rhs_->check_initialize_before_use(inits, all_inits, is_method);
      if (!success)
        return false;

      bool is_constructor = (all_inits != nullptr);
      lhs_->update_initialized_list(inits, is_constructor);
      if (is_constructor)
        all_inits->var_union(inits);

      return lhs_->check_initialize_before_use(inits, all_inits, is_method);
    }

    bool perform_type_inference(TypeCheck::Settings &settings, Quack::Class * parent_type) override{
      bool success = rhs_->perform_type_inference(settings, nullptr);

      success = success && lhs_->update_inferred_type(settings, rhs_->get_node_type(), false);

      success = success && lhs_->perform_type_inference(settings, nullptr);
      return success;
    }
    /**
     * Generates the code for an assignment.
     *
     * @param settings Code generator settings
     * @param indent_lvl Level of indentation
     * @return No return information since not applicable to an assignment
     */
    std::string generate_code(CodeGen::Settings &settings, unsigned indent_lvl,
                              bool is_lhs) const override;
  };

  struct Typecase : public ASTNode {
    Typecase(ASTNode* expr, std::vector<TypeAlternative*>* alts) : expr_(expr), alts_(alts) {}

    ~Typecase() {
      delete expr_;
      for (const auto &alt : *alts_)
        delete alt;
      delete alts_;
    }

    void print_original_src(unsigned int indent_depth) override {
      std::string indent_str = std::string(indent_depth, '\t');
      std::cout << KEY_TYPECASE << " ";
      expr_->print_original_src(0);
      std::cout << " {\n";
      bool is_first = true;

      // Print the alternate blocks
      for (const auto &alt : *alts_) {
        if (!is_first)
          std::cout << "\n";
        is_first = false;
        std::cout << indent_str << "\t";
        alt->print_original_src(indent_depth + 1);
      }

      if (!alts_->empty())
        std::cout << "\n";
      std::cout << indent_str << "}";
    }

    bool perform_type_inference(TypeCheck::Settings &settings, Quack::Class * parent_type) override;

    bool check_initialize_before_use(InitializedList &inits, InitializedList *all_inits,
                                     bool is_method) override {
      bool success = expr_->check_initialize_before_use(inits, all_inits, false);
      all_inits->var_union(inits);

      for (unsigned i = 0; i < alts_->size(); i++) {
        TypeAlternative * alt = (*alts_)[i];
        InitializedList type_init(inits);

        // Add the typed object
        type_init.add(alt->type_names_[0], false);
        success = success && alt->block_->check_initialize_before_use(type_init, all_inits, false);

        // Update the initialized variable lists.
        if (i == 0)
          inits = type_init;
        // Removed as no effect. Anyway not correct as may not match any typecase
//        else
//          inits.var_intersect(type_init);
        all_inits->var_union(type_init);
      }

      return success;
    }
    /**
     * Typecase always returns false since no guarantee all objects are not guaranteed to
     * match a statement.
     *
     * @return Always false
     */
    bool contains_return_all_paths() override { return false; }

    std::string generate_code(CodeGen::Settings &settings, unsigned indent_lvl,
                              bool is_lhs) const override;

   private:
    ASTNode* expr_;
    std::vector<TypeAlternative*>* alts_;
  };
}
#endif //ASTNODE_H
