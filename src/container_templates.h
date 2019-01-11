//
// Created by Zayd Hammoudeh on 10/4/18.
//

#ifndef PROJECT02_CONTAINER_TEMPLATES_H
#define PROJECT02_CONTAINER_TEMPLATES_H

#include <string>
#include <map>
#include <set>
#include <iostream>
#include <stdexcept>

#include "keywords.h"
#include "exceptions.h"

template<typename _T>
class ObjectContainer {
 private:
  virtual bool exists(const std::string &obj_name) = 0;
  virtual void add(_T *new_obj) = 0;
  virtual _T* get(const std::string &str) = 0;
  virtual unsigned long count() = 0;
  virtual void clear() = 0;
 public:
  virtual ~ObjectContainer() = default;
  bool empty() { return count() == 0; };

  virtual const void print_original_src(unsigned int indent_depth) = 0;
};


template <typename _T>
class MapContainer : public ObjectContainer<_T> {
 public:
  MapContainer() = default;
  /** Clean up all classes. */
  ~MapContainer() {
    for (const auto &pair : objs_)
      delete pair.second;
  }
  /**
   * Iterator accessor for object pairs in the map container
   *
   * @return Iterator to the beginning of the map.
   */
  typename std::map<std::string, _T*>::iterator begin() { return objs_.begin(); }
  /**
   * Iterator accessor for object pairs in the map container
   *
   * @return Iterator to the end of the map.
   */
  typename std::map<std::string, _T*>::iterator end() { return objs_.end(); }
  /**
   * Check if the object exists in the map.
   *
   * @param obj_name Name of the object.
   * @return True if the object exists in the map container.
   */
  bool exists(const std::string &obj_name) override {
    return objs_.find(obj_name) != objs_.end();
  }
  /**
   * Add the object to the container
   *
   * @param new_obj New object to add.
   */
  void add(_T *new_obj) override {
    if (exists(new_obj->name_))
      throw ParserException("Duplicate object: " + new_obj->name_);
    objs_[new_obj->name_] = new_obj;
  }
  /**
   * Extract the specified object by its object name. If the specified object name does not
   * exist, then return nullptr.
   *
   * @param str Name of the object to be extracted.
   * @return A pointer to the object with the specified name if it exists and
   * nullptr otherwise.
   */
  _T* get(const std::string &str) override {
    auto itr = objs_.find(str);
    if (itr == objs_.end())
      return OBJECT_NOT_FOUND;
    return itr->second;
  }
  /**
   * Access for the number of objects in the container.
   *
   * @return Numebr of objects in the container
   */
  unsigned long count() override { return objs_.size(); }
  /**
   * Delete all stored objects in the maps.
   */
  void clear() override {
    objs_.clear();
  }
  /**
   * Print the object container for debug.
   *
   * @param indent_depth
   */
  const void print_original_src_(unsigned int indent_depth, const std::string &print_sep) {
    bool is_first = true;

    for (auto &pair : objs_) {
      if (!is_first)
        std::cout << print_sep;
      is_first = false;
      pair.second->print_original_src(indent_depth);
    }
  }

 protected:
  std::map<std::string,_T*> objs_;
};


template<typename _T>
class VectorContainer : public ObjectContainer<_T> {
 public:
  VectorContainer() = default;

  ~VectorContainer() {
    for (auto const &ptr : objs_)
      delete ptr;
  }
  /**
   * Accessor for an iterator to the beginning of the objects container.
   *
   * @return Pointer to the beginning of the objects container.
   */
  typename std::vector<_T*>::const_iterator begin() const { return objs_.begin(); }
  typename std::vector<_T*>::iterator begin() { return objs_.begin(); }
  /**
   * Accessor for an iterator to the end of the objects container.
   *
   * @return Pointer to the end of the objects container.
   */
  typename std::vector<_T*>::const_iterator end() const { return objs_.end(); }
  typename std::vector<_T*>::iterator end() { return objs_.end(); }
  /**
   * Check if the specified parameter name exists.
   *
   * @param name Name of the parameter
   * @return True if the parameter exists.
   */
  bool exists(const std::string &name) override { return names_.find(name) != names_.end(); }
  /**
   * Adds the passed object to the container.
   * @param new_obj Object to add.
   */
  void add(_T* new_obj) override {
    if (exists(new_obj->name_))
      throw DuplicateParamException(new_obj->name_);
    objs_.emplace_back(new_obj);
    names_.emplace(new_obj->name_);
  };
  _T* get(const std::string &name) override {
    if (!exists(name))
      return OBJECT_NOT_FOUND;

    for (auto &obj: objs_)
      if (obj->name_ == name)
        return obj;
    throw std::runtime_error("Unable to get object with key: " + name);
  }
  /**
   * Delete all stored objects in the map.
   */
  void clear() override {
    objs_.clear();
    names_.clear();
  }
  /**
   * Accessor for the container element count.
   * @return Number of elements in the container.
   */
  unsigned long count() override { return objs_.size(); }
  /**
   * Print the object container for debug.
   *
   * @param indent_depth
   */
  const void print_original_src_(unsigned int indent_depth, const std::string &print_sep) {
    bool is_first = true;

    for (auto &obj : objs_) {
      if (!is_first)
        std::cout << print_sep;
      is_first = false;
      obj->print_original_src(indent_depth);
    }
  }
  /**
   * Accessor for the index of the container.
   *
   * @param i Index to
   * @return
   */
  _T* operator [](int i) {
    return objs_[i];
  }

 private:
  /**
   * Builds a constant version of this object.  Used in code avoiding duplication of const methods.
   * @return Const version of this.
   */
  const VectorContainer<_T>* get_const() { return static_cast<const VectorContainer<_T>*>(this); }

  std::vector<_T*> objs_;
  std::set<std::string> names_;
};

#endif //PROJECT02_CONTAINER_TEMPLATES_H
