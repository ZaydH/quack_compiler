#include <utility>

#ifndef PROJECT02_QUACK_FIELD_H
#define PROJECT02_QUACK_FIELD_H

#include <utility>
#include <cstring>
#include <assert.h>

#include "container_templates.h"

namespace Quack {
  // Forward declaration
  class Class;

  /**
   * Encapsulates Quack a single class field.
   */
  struct Field {
    struct Container : public MapContainer<Field> {
     /**
       * Prints the user defined classes only.  It will raise a runtime error if called.  Never
       * need to print the fields.
       *
       * @param indent_depth Depth to tab the contents.
       */
      const void print_original_src(unsigned int indent_depth) override {
        // Do not implement.  Makes no sense since not needed.
        assert(false);
      }
      /**
       * Modified version of the field that automates boxing the field objects.
       *
       * @param name Name of the field to add.
       */
      void add_by_name(const std::string &name) {
        add(new Field(name));
      }
      /**
       * Checks whether the implicit Field::Container() object is a superset of \p other's fields.
       *
       * @param other Set of fields to be checked against the implicit container.
       * @return True if the implicit container is a superset.
       */
      bool is_super_set(Container *other) {
        for (const auto &field_info : *other) {
          if (!exists(field_info.first))
            return false;
        }
        return true;
      }
    };
    /**
     * Initialize a field stored as a member of a class.
     * @param name Name of the field
     */
    explicit Field(std::string name) : name_(std::move(name)) { }

    std::string name_;
    /** Type of the field object **/
    Class* type_ = nullptr;
  };
}

#endif //PROJECT02_QUACK_FIELD_H
