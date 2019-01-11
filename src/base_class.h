//
// Created by Zayd Hammoudeh on 11/15/18.
//

#include <stdlib.h>

typedef struct {
  void* (*constructor)(Obj* self);
  void* super_;
  char* (*PRINT)(Obj* self);
  char* (*STR)(Obj self);
} Obj;

Q_String Obj_STR(Obj* self) {
  char *buf = malloc(20);
  scanf_s(buf, "<object %08x>", self)
  return buf;
}

void Obj_PRINT(Obj * self) {
  return self->STR();
}

void* Obj_Constructor(Obj * self) {
  self = (Obj*) malloc(sizeof(Obj));

  self->super_ = NULL;
  self->PRINT = Obj_PRINT;
  self->STR = Obj_STR;

  return (void*) self;
}
