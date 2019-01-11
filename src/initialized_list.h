//
// Created by Zayd Hammoudeh on 11/3/18.
//

#ifndef PROJECT02_INITIALIZED_LIST_H
#define PROJECT02_INITIALIZED_LIST_H

#include <unordered_set>
#include <string>

#include "compiler_utils.h" // Used for hash


typedef std::pair<std::string, bool> InitVar;

// Forward Declaration
namespace Quack {
  class TypeChecker;
}

class InitializedList {
  friend class Quack::TypeChecker;
 public:
  InitializedList() = default;
  /**
   * Copy constructor.  Only copys the initialized variables set.
   * @param other Object to be copied.
   */
  InitializedList(const InitializedList& other) { this->vars_ = other.vars_; }
  /**
   * Assignment operator overload.  Needed because of conditionals in the program.
   * @param other Object used to set the other object.
   * @return
   */
  InitializedList& operator=(const InitializedList &other){
    this->vars_ = other.vars_;
    return *this;
  }
  /**
   * Adds the specified variable name to the initialized variable list.
   * @param var_name Name of the variable to add
   */
  inline void add(const std::string &var_name, bool is_field) {
    vars_.insert(InitVar(var_name, is_field));
  }
  /**
   * Checks whether the specified variable name exists in the initialized variable list.
   * @param var_name Name of the variable to check.
   */
  inline bool exists(const std::string &var_name, bool is_field) {
    return vars_.find(InitVar(var_name, is_field)) != vars_.end();
  }
  /**
   * Accessor for an iterator to the beginning of the initialized variables.
   *
   * @return Pointer to the beginning of the initialized variables.
   */
  typename std::unordered_set<InitVar>::const_iterator begin() const { return vars_.begin(); }
  typename std::unordered_set<InitVar>::iterator begin() { return vars_.begin(); }
  /**
   * Accessor for an iterator to the end of the initialized variables.
   *
   * @return Pointer to the end of the initialized variables.
   */
  typename std::unordered_set<InitVar>::const_iterator end() const { return vars_.end(); }
  typename std::unordered_set<InitVar>::iterator end() { return vars_.end(); }
  /**
   * Takes the intersection of the initialized variable list.
   *
   * @param other Other initialized list to take in the intersection
   */
  void var_intersect(const InitializedList &other) {
    std::unordered_set<InitVar> intersect, large, small;

    // Iterate over the smaller list for better p
    small = (vars_.size() > other.vars_.size()) ? other.vars_ : vars_;
    large = (small == vars_) ? other.vars_ : vars_;
    for (const auto &var_name : small) {
      auto loc = large.find(var_name);
      if (loc != large.end())
        intersect.insert(var_name);
    }

    vars_ = intersect;
  }
  /**
   * Accessor for all initialized variables.
   *
   * @return Initialized variable set.
   */
  const std::unordered_set<InitVar> all_items() const { return vars_; }
  /**
   * Takes the intersection of the initialized variable list.
   *
   * @param other Other initialized list to take in the intersection
   */
  void var_union(const InitializedList& other) {
    for (const auto &var_info : other.vars_)
      add(var_info.first, var_info.second);
  }
  /**
   * Accessor for the number of variables in the initialized list.
   *
   * @return Initialized list count.
   */
  inline unsigned count() { return vars_.size(); }


 protected:
  std::unordered_set<InitVar> vars_;
};

#endif //PROJECT02_INITIALIZED_LIST_H
