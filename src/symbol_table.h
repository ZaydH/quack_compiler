#ifndef PROJECT02_SYMBOL_TABLE_H
#define PROJECT02_SYMBOL_TABLE_H

#include <assert.h>

#include <string>
#include <map>

#include "compiler_utils.h" // Uses hash for map
//#include "quack_class.h"
#include "exceptions.h"

typedef std::pair<std::string, bool> SymbolKey;

// Forward declarations
namespace Quack{ class Class; }
namespace CodeGen { class Gen; }

class Symbol {
  friend class CodeGen::Gen;
  friend class Quack::Class;
  friend class Table;
 public:
  class Table {
   public:
    /**
     * Deletes all symbols in the table.
     */
    ~Table() {
      for (const auto &symbol_info : objs_)
        delete symbol_info.second;
    }
//    /**
//     * Adds a new symbol the table.  The symbol is set to the base class of all classes.
//     * @param symbol_name Name of the symbol
//     * @param is_field True if the new symbol is a field
//     */
//    void add_new(const std::string &symbol_name, bool is_field) {
//      add_new(symbol_name, is_field, BASE_CLASS);
//    }
    /**
     * Updates the class of the specified
     * @param symbol
     * @param new_class
     */
    void add_new(const std::string &symbol_name, bool is_field, Quack::Class *new_class) {
      SymbolKey key(symbol_name, is_field);
      objs_[key] = new Symbol(symbol_name, is_field, new_class);

      is_dirty_ = true;
    }
    /**
     * Updates the class of the specified symbol.  If the object class has changed, the symbol
     * table is marked as dirty.
     *
     * @param symbol_name Name of the symbol to update.
     * @param new_class True if the corresponding symbol is a class field.
     */
    void update(const std::string &symbol_name, bool is_field, Quack::Class *new_class) {
      SymbolKey key(symbol_name, is_field);

      assert(exists(key));
      is_dirty_ = is_dirty_ || (objs_[key]->get_type() != new_class);
      objs_[key]->set_type(new_class);
    }
    /**
     * Updates the class of the specified symbol.  If the object class has changed, the symbol
     * table is marked as dirty.
     *
     * @param symbol_name Name of the symbol to update.
     * @param new_class True if the corresponding symbol is a class field.
     */
    void update(const Symbol * symbol, Quack::Class *new_class) {
      update(symbol->name_, symbol->is_field_, new_class);
    }
    /**
     * Accessor for whether the symbol table is dirty, i.e., whether it has changed since the
     * last time the dirty was clear.
     *
     * @return True if a change has occurred.
     */
    inline const bool is_dirty() const { return is_dirty_; }
    /**
     * Resets the dirty flag for the symbol table.
     */
    inline void clear_dirty() { is_dirty_ = false; }
    /**
     * Accessor for a symbol table item.
     *
     * @param symbol_name Name of the symbol
     * @param is_field True if the symbol is a field.
     *
     * @return Corresponding symbol object.
     */
    Symbol* get(const std::string &symbol_name, bool is_field) const {
      SymbolKey key(symbol_name, is_field);
      assert(exists(key));

      auto itr = objs_.find(key);
      if (itr == objs_.end())
        throw UnknownSymbolException(symbol_name);
      return itr->second;
    }
    /**
     * Returns an iterator to the beginning of the objects in the symbol table
     *
     * @return Beginning of the symbol table
     */
    typename std::map<SymbolKey, Symbol*>::iterator begin() { return objs_.begin(); }
    /**
     * Returns an iterator the end of the object in the symbol table
     *
     * @return End of the symbol table
     */
    typename std::map<SymbolKey, Symbol*>::iterator end() { return objs_.end(); }
   private:
    /**
     * Checks whether the specified key exists in the symbol table.
     *
     * @param key Symbol key
     * @return True if the key exists.
     */
    bool exists(const SymbolKey &key) const {
      return objs_.find(key) != objs_.end();
    }

    std::map<SymbolKey,Symbol*> objs_;

    bool is_dirty_ = false;
  };
  /**
   * Initialize a new symbol.  The class is set to
   *
   * @param name Name of the symbol
   * @param is_field True if the symbol is a class field.
   */
  Symbol(const std::string &name, bool is_field) : Symbol(name, is_field, nullptr) {}

  Symbol(std::string name, bool is_field, Quack::Class* q_class)
      : name_(std::move(name)), is_field_(is_field), class_(q_class) {}
  /**
   * Updates the class of the symbol.
   *
   * @param q_class New class for the symbol.
   */
  Quack::Class* get_type() const { return class_; }

 private:
  /**
   * Updates the class of the symbol.
   *
   * @param q_class New class for the symbol.
   */
  void set_type(Quack::Class *q_class) { class_ = q_class; }

  std::string name_;
  bool is_field_;
  Quack::Class * class_;
};

#endif //PROJECT02_SYMBOL_TABLE_H
