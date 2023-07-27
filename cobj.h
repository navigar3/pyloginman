#ifndef  _COBJ_H

#define _COBJ_H

#include <stdio.h>
#include <malloc.h>

typedef struct 
{
  int (*init)(void *, void *);
} genobj;

#define newclass_h(class) \
  void * _ ## class ## _create(int (*init)(void *, void *), void * init_par);\
  int _ ## class ## _init(void * , void * );

#define newclass_creator(class) \
  void * _ ## class ## _create(int (*init)(void *, void *), \
                               void * init_par) \
  { \
    genobj * newo; \
    if(!(newo = calloc(1, sizeof(class))) ) \
    {\
      fprintf(stderr, \
              "%s:%d: Failed to allocate memory for class \"%s\"\n", \
              __FILE__, __LINE__, #class); \
              return NULL; \
    }\
    \
    newo->init = init; \
    \
    if(newo->init(newo, init_par)) \
      {\
      fprintf(stderr, \
              "%s:%d: Failed to initialize object of class \"%s\"\n", \
              __FILE__, __LINE__, #class); \
              return NULL; \
      }\
    \
    return newo; \
    \
  }


#define _this_obj_ptr(x) ( ( x * ) _this )
#define _thisclassname(x) x
#define _newclsn(x) newclass_creator(x)
#define _Parent(x) ((_Parent_nm_cat(_PARENT_CLASS_, x) *)(this->__Parent))
#define _Parent_nm_cat(x, y) x ## y
#define _class_nm_cat(x, y) _class_nm_cat_primitive(x, y)
#define _class_nm_cat_primitive(x, y) _ ## x ## _ ## y
#define _class_nm_args(x, ...) ( void * _this, ##__VA_ARGS__ )

#define this _this_obj_ptr(_CLASS_NAME)

#define thisclassname _thisclassname(_CLASS_NAME)

#define thisclass_creator _newclsn(_CLASS_NAME)

#define Parent _Parent(_CLASS_NAME)

#define clsm(met, ...) _class_nm_cat(_CLASS_NAME, met) _class_nm_args(_CLASS_NAME, ##__VA_ARGS__)

#define clsmlnk(met) this->met = _class_nm_cat(_CLASS_NAME, met) 

#define new(class, ...) _ ## class ## _create (_ ## class ## _init, ##__VA_ARGS__)

#define CALL(obj, met, ...) obj->met(obj, ##__VA_ARGS__)

#endif
