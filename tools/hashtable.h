#ifndef _HASHTABLE_H

#define _HASHTABLE_H


#include <stdint.h>
#include <stdbool.h>

//typedef enum {False, True} bool;

typedef bool __bool;

typedef uint16_t keysize_t;

#ifdef _HASHTABLE_H_PRIVATE

#define FULL_HASH_MAX_SZ 2^16

/* Keep track of dynamically allocated memory where needed.     */
/*  This allow to track pointer that need to be manually freed. */
typedef enum {P_dyn, P_stat, P_unspec} pointer_type;
typedef struct
{
  pointer_type pt;
  void * p;
} ptr_t;
/* */

#define strlf(arg) 1+strlen(arg)

typedef char * string_t;

#define HT_VERSION 0x00000001
#define FILE_MAGICK "htd"

/* Store in memory data before write it to file. 
 *  Keep track of allocation size and extend if needed */
typedef struct 
{
  int (*init)(void *, void *);
  
  int (*write)(void *, size_t, void *);
  size_t (*get_mapsz)(void *);
  char * (*get_map)(void *);
  int (*destroy)(void *);
  
  /* Private */
  
  char * _addr;
  size_t _msize;
  size_t _used;
  size_t _newwinsz;
  
  int (*_realloc)(void *, size_t);
} buffer;

newclass_h(buffer)
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
#define DF_CPY___bool(X) \
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
    _p->X = *((uint64_t *)(_src+_lb)); \
    _lb += sizeof(uint64_t); \
  }
#define DF_LD_uint32_t(X) \
  { \
    _p->X = *((uint32_t *)(_src+_lb)); \
    _lb += sizeof(uint32_t); \
  }
#define DF_LD___bool(X) \
  { \
    _p->X = (bool)(*(_src+_lb)); \
    _lb += 1; \
  }

#if HT_VERSION==0x00000001
#define DF_HT_PROPS \
  ((uint32_t,  ver_major))\
  ((uint32_t,  ver_minor))\
  ((string_t,  hash_algo))\
  ((string_t,  hash_dl_helper))\
  ((void,      * (*hash_fun_init)(void * start)))\
  ((void,      * (*hash_function)(void * data)))\
  ((uint32_t,  hash_modulus))\
  ((__bool,      has_only_key))\
  ((__bool,      has_fixed_key_size))\
  ((uint32_t,  key_size))\
  ((__bool,      has_full_hash_array))\
  ((__bool,      has_fixed_size))\
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
#define  HT_SECTION_META  HT_SECTION_PROPS+1

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
/* Metadata defs ends. */

/* Hash table object */
#define structgen_hashentry_part_ff \
  ptr_t contentp; 
#define structgen_hashentry_part_fn \
  uint32_t contentsize; ptr_t contentp;
#define structgen_hashentry_part_nf \
  keysize_t keysize; ptr_t contentp;
#define structgen_hashentry_part_nn \
  keysize_t keysize; uint32_t contentsize; ptr_t contentp;
#define structgen_hashentry_part_kf
#define structgen_hashentry_part_kn \
  keysize_t keysize;
#define structgen_hashentry(PF) \
typedef struct hashentry_ ## PF ## _s \
{ \
  struct hashentry_ ## PF ## _s * next_entry; \
  \
  ptr_t keyp; \
  \
  structgen_hashentry_part_ ## PF \
} hashentry_ ## PF ## _t;

structgen_hashentry(ff)
structgen_hashentry(fn)
structgen_hashentry(nn)
structgen_hashentry(nf)
structgen_hashentry(kf)
structgen_hashentry(kn)

#undef structgen_hashentry_part_ff
#undef structgen_hashentry_part_fn
#undef structgen_hashentry_part_nn
#undef structgen_hashentry_part_nf
#undef structgen_hashentry_part_kf
#undef structgen_hashentry_part_kn
#undef structgen_hashentry

typedef hashentry_kf_t hashentry_t;

typedef struct hasharray_s
{
  uint16_t num_of_entries;
  
  void * first_entry; /* Can be hashentry_[knf][nf]_t */
} hasharray_t;

typedef struct hashlist_s
{
  char * hashkey;
  uint32_t num_of_entries;
  void * first_entry;
  
  struct hashlist_s * next;
} hashlist_t;

#define pht(a) ((a->_ht == harr) ? \
  ((hasharray_t *)(a->_ht)) : ((hashlist_t *)(a->_ht)));

#define pha(p, n) ((hasharray_t *)(p + n*sizeof(hasharray_t))) 


#define  HT_SECTION_DATA  HT_SECTION_META+1

typedef struct
{
  uint64_t seclen;
  
  char * raw_data_map;
} sdata_t;

#endif

struct lookup_res
{
  void * key;
  void * content;
  
  keysize_t keysize;
  uint32_t contentsize;
};


struct hashtable_init_params
{
  char *    map;  /* Load from memory mapped @map */
  
  char *    hash_algo;
  char *    hash_dl_helper;
  uint32_t  hash_modulus;
  bool      has_only_key;
  bool      has_fixed_key_size;
  uint32_t  key_size;
  bool      has_fixed_size;
  uint32_t  fixed_size_len;
};

typedef struct 
{
  int (*init)(void *, void *);
  
  char * (*get_table_name)(void *);
  uint32_t (*compute_hash)(void *, 
                           uint32_t keysize, char * key);
  int (*add_meta_field)(void *,
                        uint16_t name_len, uint16_t value_len,
                        char * name, char * value);
  int (*push)(void *, 
              keysize_t keysize, char * key, 
              uint32_t datasize, void * data);
  
  /* Search for key into table.
   *  
   *  if key is found: a) store pointer to key entry struct into 
   *                      *resptr if resptr is not null;
   *                   b) fill *ls stucture if ls is not null;
   *                   c) return 0.
   * 
   *  if key is not found:
   *                   a) store pointer to first key_ entry struct 
   *                      where key_ < key into *resptr if 
   *                      resptr is not null;
   *                   b) return -1. */
  int (*lookup)(void *, keysize_t ks, char * key, 
                  void * resptr, struct lookup_res *);
                  
  int (*write_hashtable)(void *, int fd);
  int (*destroy)(void *);
  
  //Debug
  int (*fill_sample_data)(void *);
  int (*print_sample_data)(void *);
  
  #ifdef _HASHTABLE_H_PRIVATE
  /* Private part begin */
  
  char * _fmap_table_entry;
  size_t _fmap_table_offs;
  char * _table_name;
  
  data_t _gdata;
  smeta_t * _metadata;
  ht_props_t * _htp;
  
  uint64_t _num_of_classes;
  uint64_t _num_of_entries;
  
  void * _ht; /* * _ht may be hasharray_t pointer to pointer or
                 hashlist_t pointer */
  
  int (*_initialize_data_sec)(void *);
  int (*_append_section)(void *, 
                         uint32_t stype, void * s);
  int (*_push)(void *, 
              keysize_t keysize, 
              pointer_type kt, char * key, 
              uint32_t datasize,
              pointer_type dt,
              void * data, void * precomp_hash);
  void * (*_newentry)(void *,
                      keysize_t keysize,
                      pointer_type kt, char * key,
                      uint32_t datasize, 
                      pointer_type dt, void * data);
  int (*_lookup_into_class)(void *, 
                            void * modclass,
                            keysize_t keysize, char * key,
                            void * resptr, struct lookup_res * ls);
  size_t (*_load_data_from_map)(void *, char * map);
  int (*_load_meta_section)(void *, 
                            char * secaddr, uint64_t seclen);
  int (*_load_props_section)(void *, 
                             char * secaddr, uint64_t seclen);
  int (*_load_data_section)(void *, 
                            char * secaddr, uint64_t seclen);
  int (*_load_entry)(void *, char * map);
  int (*_write_data_meta)(void *, 
                          int fd, smeta_t * metadata);
  buffer * (*_fill)(void *);
  int (*_fill_buff)( void *, buffer * b, void * entry);
  
  #endif
  
} hashtable;

newclass_h(hashtable)
/* Hash table object ends */

/* Multitables dictionaries */
#ifdef _HASHTABLE_H_PRIVATE
struct htableslist
{
  struct htableslist * next;
  
  ptr_t table_name;
  hashtable * table;
};
#endif

typedef struct 
{
  int (*init)(void *, void *);
  
  hashtable * (*newtable)(void *, char * tablename, 
                  struct hashtable_init_params *);
  hashtable * (*lookup)(void *, char * tablename);
  uint32_t (*get_num_of_tables)(void *);
  void * (*get_first_table_ref)(void *);
  void * (*get_next_table_ref)(void *, void * current_ref);
  hashtable * (*get_table_by_ref)(void *, void * ref);
  int (*loadtables)(void *, char * file_name);
  int (*savetables)(void *, char * file_name);
  int (*destroy)(void *);
  
  #ifdef _HASHTABLE_H_PRIVATE
  uint16_t _num_of_tables;
  struct htableslist * _hts;
  
  size_t (*_loadtable)(void *, char * tablename, 
                            char * map);
  #endif
  
} htables;

newclass_h(htables)
/* */

#endif
