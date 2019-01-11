//
// Created by Zayd Hammoudeh on 10/15/18.
//

#include "quack_class.h"

Quack::Class::Container* Quack::Class::Container::singleton() {
  static Container all_classes_;
  if (all_classes_.empty()) {
    all_classes_.add(new ObjectClass());
    all_classes_.add(new BooleanClass());
    all_classes_.add(new IntClass());
    all_classes_.add(new StringClass());
    all_classes_.add(new NothingClass());
  }
  return &all_classes_;
}
