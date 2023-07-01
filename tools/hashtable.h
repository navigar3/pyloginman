#ifndef _HASHTABLE_H

#define _HASHTABLE_H


#include <stdint.h>

#define strlf(arg) 1+strlen(arg)

void * hmalloc(size_t s);
void * hcalloc(size_t n, size_t s);


typedef enum {False, True} bool;

typedef char * string_t;

#define HT_VERSION 0x00000001
#define FILE_MAGICK "htd"

/* Keep track of dynamically allocated memory where needed.     */
/*  This allow to track pointer that need to be manually freed. */
typedef enum {P_dyn, P_stat, P_unspec} pointer_type;
typedef struct
{
  pointer_type pt;
  void * p;
} ptr_t;

/* Store in memory data before write it to file. 
 *  Keep track of allocation size and extend if needed */
typedef struct 
{
  int (*init)(void *, void *);
} genobj;

typedef struct buffer_o
{
  int (*init)(void *, void *);
  
  char * addr;
  size_t msize;
  size_t used;
  size_t newwinsz;
  
  int (*write)(struct buffer_o *, size_t, char *);
  int (*destroy)(struct buffer_o *);
  
  /* Private */
  
  int (*_realloc)(struct buffer_o *, size_t);
} buffer;

#define new(class, ...) newobj(_ ## class ## _init, sizeof(class), ##__VA_ARGS__)
#define CALL(obj, met, ...) obj->met(obj, ##__VA_ARGS__)
/* */

/* Top level file sections descriptor. */
typedef struct section
{
  uint32_t sectype;
  uint64_t seclen;
  
  void * sec_content;
  struct section * next_section;
} section_t;

typedef struct
{
  uint32_t ht_version_num;
  uint32_t num_of_sections;
  uint32_t sections_global_flags;
  
  section_t * sections_list;
} data_t;
/* Top level file sections des ends. */

/* Properties section:
 *  store data structure properties used by program to decode 
 *  and encode data in memory. */
#define  HT_SECTION_PROPS  0x00000001

#define CAT(x, y) PRIMITIVE_CAT(x, y)
#define PRIMITIVE_CAT(x, y) x ## y

#define DF_FILL(X, Y) X Y;
#define DF_FUN(X) DF_FILL X
#define DF_HELP_1(X) DF_FUN(X) DF_HELP_2
#define DF_HELP_2(X) DF_FUN(X) DF_HELP_1
#define DF_COMP(X) CAT(DF_HELP_1 ((uint64_t,  seclen)) X, _END)
#define DF_HELP_1_END
#define DF_HELP_2_END

#define DF_WR(X, Y) CAT(DF_CPY_, X) (Y)
#define DF_WFUN(X) DF_WR X
#define DF_HELP_3(X) DF_WFUN(X) DF_HELP_4
#define DF_HELP_4(X) DF_WFUN(X) DF_HELP_3
#define DF_HELP_3_END
#define DF_HELP_4_END
#define DF_WRITE(pre, dest) DF_WRITE_H(pre, dest, DF_HT_PROPS)
#define DF_WRITE_H(pre, dest, X) \
  { \
    ht_props_t * _p = pre; \
    void * _dest = dest; \
    uint32_t _wb = 0; \
    CAT(DF_HELP_3 X, _END) \
  }


#define DF_CPY_void(X)
#define DF_CPY_string_t(X) \
  { \
    uint32_t _len = strlf(_p->X); \
    memcpy(_dest+_wb, _p->X, _len); \
    _wb += _len; \
  }
#define DF_CPY_uint64_t(X) \
  { \
    uint32_t _len = sizeof(_p->X); \
    memcpy(_dest+_wb, &(_p->X), _len); \
    _wb += _len; \
  }
#define DF_CPY_uint32_t(X) \
  { \
    uint32_t _len = sizeof(_p->X); \
    memcpy(_dest+_wb, &(_p->X), _len); \
    _wb += _len; \
  }
#define DF_CPY_bool(X) \
  { \
    uint8_t _val = (uint8_t)(_p->X); \
    memcpy(_dest+_wb, &_val, 1); \
    _wb++; \
  }

#define DF_LD(X, Y) CAT(DF_LD_, X) (Y)
#define DF_LFUN(X) DF_LD X
#define DF_HELP_5(X) DF_LFUN(X) DF_HELP_6
#define DF_HELP_6(X) DF_LFUN(X) DF_HELP_5
#define DF_HELP_5_END
#define DF_HELP_6_END
#define DF_LOAD(pre, src) DF_LOAD_H(pre, src, DF_HT_PROPS)
#define DF_LOAD_H(pre, src, X) \
  { \
    ht_props_t * _p = pre; \
    char * _src = src; \
    uint32_t _lb = 0; \
    CAT(DF_HELP_5 X, _END) \
  }
  
#define DF_LD_void(X)
#define DF_LD_string_t(X) \
  { \
    uint32_t _len = strlf(_src+_lb); \
    _p->X = _src + _lb; \
    _lb += _len; \
  }
#define DF_LD_uint64_t(X) \
  { \
    _p->X = (uint64_t)(*(_src+_lb)); \
    _lb += sizeof(uint64_t); \
  }
#define DF_LD_uint32_t(X) \
  { \
    _p->X = (uint32_t)(*(_src+_lb)); \
    _lb += sizeof(uint32_t); \
  }
#define DF_LD_bool(X) \
  { \
    _p->X = (bool)(*(_src+_lb)); \
    _lb += 1; \
  }

#if HT_VERSION==0x00000001
#define DF_HT_PROPS \
  ((string_t,  hash_algo))\
  ((string_t,  hash_dl_helper))\
  ((void,      * (*hash_fun_init)(void * start)))\
  ((void,      * (*hash_function)(void * data)))\
  ((uint32_t,  hash_modulus))\
  ((bool,      has_key))\
  ((bool,      has_fixed_key_size))\
  ((uint32_t,  key_size))\
  ((bool,      has_full_hash_array))\
  ((bool,      has_fixed_size))\
  ((uint32_t,  fixed_size_len))

#endif

typedef struct ht_props_s
{
  DF_COMP(DF_HT_PROPS)
} ht_props_t;

/* Properties section ends.*/


/* Metadata section:
 *  define metadata informations for the file. 
 *  These informations are not used by the program itself but 
 *  are related to stored data. */
#define  HT_SECTION_META  0x00000002

struct smeta_field
{
  uint16_t propname_len;
  uint16_t propval_len;
  ptr_t propname;
  ptr_t propval;
};

struct smeta_block
{
  struct smeta_field * field_content;
  struct smeta_block * next_field;
};

typedef struct
{
  uint64_t seclen;
  
  struct smeta_block * list;
} smeta_t;

/* Add metadata field in metadata section */
int add_meta_field(
  smeta_t * sm,
  uint16_t name_len,
  uint16_t value_len,
  char * name,
  char * value
);

/* Save metadata section into file. */
int write_data_meta(int fd, smeta_t * metadata);

/* Load metadata section from file. */
int load_meta_section(char * secaddr, uint64_t seclen);

/* Metadata defs ends. */


/* Hash table object */
#define structgen_hashentry_part_ff
#define structgen_hashentry_part_fn \
  uint32_t contentsize;
#define structgen_hashentry_part_nf \
  uint32_t keysize;
#define structgen_hashentry_part_nn \
  uint32_t keysize; uint32_t contentsize;
#define structgen_hashentry(PF) \
typedef struct hashentry_ ## PF ## _s \
{ \
  struct hashentry_ ## PF ## _s * next_entry; \
  \
  ptr_t keyp; \
  ptr_t contentp; \
  \
  structgen_hashentry_part_ ## PF \
} hashentry_ ## PF ## _t;

structgen_hashentry(ff)
structgen_hashentry(fn)
structgen_hashentry(nn)
structgen_hashentry(nf)

#undef structgen_hashentry_part_ff
#undef structgen_hashentry_part_fn
#undef structgen_hashentry_part_nn
#undef structgen_hashentry_part_nf
#undef structgen_hashentry

typedef hashentry_ff_t hashentry_t;

typedef struct hasharray_s
{
  uint64_t num_of_entries;
  
  void * first_entry; /* Can be hashentry_[nf][nf]_t */
} hasharray_t;

typedef struct hashlist_s
{
  char * hashkey;
  uint64_t num_of_entries;
  void * first_entry;
  
  struct hashlist_s * next;
} hashlist_t;

#define pht(a) ((a->_ht == harr) ? \
  ((hasharray_t *)(a->_ht)) : ((hashlist_t *)(a->_ht)));

#define pha(p, n) ((hasharray_t *)(p + n*sizeof(hasharray_t))) 

struct lookup_res
{
  void * key;
  void * content;
  
  uint32_t keysize;
  uint32_t contentsize;
};

struct hashtable_init_params
{
  char * file_name;
  ht_props_t * htp;
};

typedef struct hashtable_o
{
  int (*init)(void *, void *);
  
  uint32_t (*compute_hash)(struct hashtable_o *, 
                           uint32_t keysize, char * key);
  int (*push)(struct hashtable_o *, 
              uint32_t keysize, 
              pointer_type kt, char * key, 
              uint32_t datasize,
              pointer_type dt,
              void * data);
  int * (*lookup)(struct hashtable_o *, char * key, 
                  void * resptr, struct lookup_res *);
  int (*write_hashtable)(struct hashtable_o *, char * file_name);
  int (*destroy)(struct hashtable_o *);
  
  //Debug
  int (*fill_sample_data)(struct hashtable_o *);
  int (*print_sample_data)(struct hashtable_o *);
  
  
  /* Private part begin */
  
  data_t _gdata;
  ht_props_t * _htp;
  
  uint64_t _num_of_classes;
  uint64_t _num_of_entries;
  
  void * _ht; /* * _ht may be hasharray_t pointer to pointer or
                 hashlist_t pointer */
  
  int (*_append_section)(struct hashtable_o *, 
                         uint32_t stype, void * s);
  int (*_load_data_from_file)(struct hashtable_o *, char *file_name);
  int (*_load_meta_section)(struct hashtable_o *, 
                            char * secaddr, uint64_t seclen);
  int (*_load_props_section)(struct hashtable_o *, 
                             char * secaddr, uint64_t seclen);
  int (*_write_data_meta)(struct hashtable_o *, 
                          int fd, smeta_t * metadata);
  
  void * (*_newentry)(struct hashtable_o *,
                      uint32_t keysize,
                      pointer_type kt, char * key,
                      uint32_t datasize, 
                      pointer_type dt, void * data);
  int (*_lookup_into_class)(struct hashtable_o *, 
                            void * modclass,
                            uint32_t keysize, char * key,
                            void * resptr, struct lookup_res * ls);
  
} hashtable;
/* Hash table object ends */

#endif
