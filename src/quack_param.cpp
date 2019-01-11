//
// Created by Zayd Hammoudeh on 11/19/18.
//

#include "quack_class.h"
#include "code_gen_utils.h"

void Quack::Param::Container::generate_code(CodeGen::Settings settings, bool include_param_names,
                                            bool generate_first_comma) {
  bool is_first = true;
  for (auto * param : *this) {
    if (!is_first || generate_first_comma)
      settings.fout_ << ", ";

    is_first = false;
    settings.fout_ << param->type_->generated_object_type_name();

    if (include_param_names)
      settings.fout_ << " " << param->name_;
  }
}
