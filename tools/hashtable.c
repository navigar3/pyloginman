#define _GNU_SOURCE 1

#include <stdio.h>
#include <malloc.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "cobj.h"

#define _HASHTABLE_H_PRIVATE
#include "hashtable.h"


#define perrandexit(msg) \
  { perror(msg); exit(EXIT_FAILURE); }

void * hcalloc(size_t n, size_t s)
{
  void * ptr = calloc(n, s);
  
  if (!ptr)
    perrandexit("calloc()");
  
  return ptr;
}

void * hmalloc(size_t s)
{
  void * ptr = malloc(s);
  
  if (!ptr)
    perrandexit("malloc()");
  
  return ptr;
}

/***********************************/
/* Implement buffer class methods */
#ifndef _CLASS_NAME
#define _CLASS_NAME buffer

thisclass_creator

int clsm(_realloc, size_t newsz)
{
  size_t newsize = (newsz) ? newsz : this->_msize + this->_newwinsz;
  
  if (!(this->_addr = 
          mremap(this->_addr, this->_msize, newsize,  MREMAP_MAYMOVE)))
    perrandexit("mremap()");
  
  this->_msize = newsize;
  
  return 0;
}

int clsm(write, size_t bsz, void * p)
{
  if (this->_used + bsz > this->_msize)
    CALL(this, _realloc, 0);
  
  char * sp = this->_addr + this->_used;
  
  for (size_t i = 0; i<bsz; i++, sp++, p++)
    *sp = *((char *)p);
  
  this->_used += bsz;
  
  return 0;
}

size_t clsm(get_mapsz)
{
  return this->_used;
}

char * clsm(get_map)
{
  return this->_addr;
}

int clsm(destroy)
{
  if (munmap(this->_addr, this->_msize))
    perrandexit("munmap()");
  
  free(this);
  
  return 0;
}

int clsm(init, void * init_params)
{
  clsmlnk(_realloc);
  clsmlnk(write);
  clsmlnk(destroy);
  clsmlnk(get_map);
  clsmlnk(get_mapsz);
  
  size_t sz = (init_params) ? *((int *)init_params) : 4096;
  
  if (!(this->_addr = 
          mmap(NULL, sz, 
               PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 
               -1, 0)
       )
     )
    return 1;
    
  this->_msize = sz;
  this->_used = 0;
  this->_newwinsz = 4096;
  
  return 0;
}

#endif
#undef _CLASS_NAME
/* buffer object methods ends. */
/*******************************/

/**************************************/
/* Implement hashtable object methods */
#ifndef _CLASS_NAME
#define _CLASS_NAME hashtable

thisclass_creator

uint32_t clsm(compute_hash, uint32_t keysize, char * key)
{
  uint32_t modulo = 0;
    
  /* Split into 4 bytes sequences */
  uint32_t keywords = (keysize / 4) * 4;
  int i = 0;
  for ( ; i<keywords; i+=4)
    modulo = 
      (*((uint32_t *)(key + i)) + 
        modulo) % this->_htp->hash_modulus;
  
  if (i-keysize)
  {
    uint32_t lastword = 0;
    uint8_t * plastword = ((uint8_t *)&lastword);
    
    for (int j=i; j<keysize; j++)
      *(plastword+j) = *(key+j);
    
    modulo = (lastword + modulo) % this->_htp->hash_modulus;
  }
  
  //fprintf(stderr, "Computed modulo=%d\n", modulo);
  
  return modulo;
}

/* Push element into hashtable list */
int clsm(push_into_list, uint32_t keysize, char * key, void * data)
{
  
  return 0;
}
/* */

/* Private _newentry method macro generator: 
 * add new entry into hash array/list */
#define fungen_hashtable__newentry_part_ff \
  newentry->contentp.p = data; \
  newentry->contentp.pt = dt;
#define fungen_hashtable__newentry_part_fn \
  newentry->contentsize = datasize; \
  newentry->contentp.p = data; \
  newentry->contentp.pt = dt;
#define fungen_hashtable__newentry_part_nf \
  newentry->keysize = keysize; \
  newentry->contentp.p = data; \
  newentry->contentp.pt = dt;
#define fungen_hashtable__newentry_part_nn \
  newentry->keysize = keysize; \
  newentry->contentsize = datasize; \
  newentry->contentp.p = data; \
  newentry->contentp.pt = dt;
#define fungen_hashtable__newentry_part_kf
#define fungen_hashtable__newentry_part_kn \
  newentry->keysize = keysize;
#define fungen_hashtable__newentry(PF) \
void * _hashtable__newentry_ ## PF \
( \
  void * _this, \
  keysize_t keysize, pointer_type kt, char * key, \
  uint32_t datasize, pointer_type dt, void * data \
) \
{ \
  /* Allocate memory for keypkt and set it */ \
  hashentry_ ## PF ## _t * newentry = \
    hcalloc(1, sizeof(hashentry_ ## PF ## _t)); \
  \
  fungen_hashtable__newentry_part_ ## PF \
  \
  newentry->keyp.p = key; \
  newentry->keyp.pt = kt; \
  \
  return newentry; \
}

fungen_hashtable__newentry(ff)
fungen_hashtable__newentry(fn)
fungen_hashtable__newentry(nn)
fungen_hashtable__newentry(nf)
fungen_hashtable__newentry(kf)
fungen_hashtable__newentry(kn)

#undef fungen_hashtable__newentry_part_ff
#undef fungen_hashtable__newentry_part_fn
#undef fungen_hashtable__newentry_part_nn
#undef fungen_hashtable__newentry_part_nf
#undef fungen_hashtable__newentry_part_kf
#undef fungen_hashtable__newentry_part_kn
#undef fungen_hashtable__newentry
/* */

/* Search for key into hasharray_t modclass (modulo class)
 *  with NON Fixed keysizes.
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
 *                   c) return -1. */
int _hashtable__lookup_into_class_array_n(
  void * _this, 
  void * modclass,
  keysize_t keysize, char * key,
  void * resptr, struct lookup_res * ls)
{
  hashentry_nn_t ** entry = (hashentry_nn_t **)
    (&(((hasharray_t *)modclass)->first_entry));
  
  
  for ( ; *entry; entry = &((*entry)->next_entry))
  {
    if (keysize < (*entry)->keysize)
    {
      if (resptr)
        *((hashentry_nn_t ***)resptr) = entry;
      return -1;
    }
    
    else if (keysize == (*entry)->keysize)
    {
      int i = keysize - 1;
      
      for ( ; i>=0; i--)
      {
        if ( key[i] < *(((char *)((*entry)->keyp.p)) + i) )
        {
          if (resptr)
            *((hashentry_nn_t ***)resptr) = entry;
          return -1;
        }
        
        else if ( key[i] > *(((char *)((*entry)->keyp.p)) + i) )
          break;
      }
        
      if (i < 0)
      {
        if (resptr)
            *((hashentry_nn_t ***)resptr) = entry;
        
        if (ls)
        {
          ls->key = (*entry)->keyp.p;
          ls->keysize = (*entry)->keysize;
          if (! this->_htp->has_only_key)
          {
            ls->content = (*entry)->contentp.p;
            if (this->_htp->has_fixed_size)
              ls->contentsize = (*entry)->contentsize;
          }
        }
        
        return 0;
      }
      
    }
  }
  
  if (resptr)
    *((hashentry_nn_t ***)resptr) = entry;
  
  return -1;
}
/* */

/* Search for key into hasharray_t modclass (modulo class)
 *  with Fixed keysizes.
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
 *                   c) return -1. */
int _hashtable__lookup_into_class_array_f(
  void * _this, 
  void * modclass,
  keysize_t keysize, char * key,
  void * resptr, struct lookup_res * ls)
{
  hashentry_nn_t ** entry = (hashentry_nn_t **)
    (&(((hasharray_t *)modclass)->first_entry));
  
  int i = keysize - 1;
  
  for ( ; i>=0; i--)
  {
    if ( key[i] < *(((char *)((*entry)->keyp.p)) + i) )
    {
      if (resptr)
        *((hashentry_nn_t ***)resptr) = entry;
      return -1;
    }
    
    else if ( key[i] > *(((char *)((*entry)->keyp.p)) + i) )
      break;
  }
    
  if (i < 0)
  {
    if (resptr)
        *((hashentry_nn_t ***)resptr) = entry;
    
    if (ls)
    {
      ls->key = (*entry)->keyp.p;
      ls->keysize = (*entry)->keysize;
      if (! this->_htp->has_only_key)
      {
        ls->content = (*entry)->contentp.p;
        if (this->_htp->has_fixed_size)
          ls->contentsize = (*entry)->contentsize;
      }
    }
    
    return 0;
  }
      
  
  if (resptr)
    *((hashentry_nn_t ***)resptr) = entry;
  
  return -1;
}
/* */

/* Public method lookup */
int clsm(lookup, keysize_t keysize, char * key, 
         void * resptr, struct lookup_res * lr)
{
  uint32_t keyhash = CALL(this, compute_hash, 
                          (this->_htp->has_fixed_key_size) ? 
                          this->_htp->key_size : keysize, key);
  
  return CALL(this, _lookup_into_class, pha(this->_ht, keyhash),
                    keysize, key, resptr, lr);
}
/* */

/* Push element into hashtable having full hash array */
int clsm(push_into_array, 
  keysize_t keysize, pointer_type kt, char * key, 
  uint32_t datasize, pointer_type dt, void * data, void * precomp_hash)
{
  uint32_t keyhash;
  
  if (precomp_hash)
    keyhash = *((uint32_t *)precomp_hash);
  else
    /* Compute key hash */ 
    keyhash = CALL(this, compute_hash, 
                   (this->_htp->has_fixed_key_size) ? 
                   this->_htp->key_size : keysize, key);
  
  hashentry_t ** he, * ne;
  
  /* Allocate new item and set it */
  ne = CALL(this, _newentry, 
            keysize, kt, key, 
            datasize, kt, data);
  
  /* find new item position in list */
  int lkres = CALL(this, _lookup_into_class, pha(this->_ht, keyhash),
                   keysize, key, &he, NULL);
  
  if (lkres == 0)
    fprintf(stderr, "Found two matching keys!\n");
  
  /* Add new item to list */
  ne->next_entry = *he;
  *he = ne;
  
  /* Increment num_of_entries */
  this->_num_of_entries += 1;
  pha(this->_ht, keyhash)->num_of_entries += 1;
  return 0;
}
/* */

/* Push element into hashtable */
int clsm(push, 
     keysize_t keysize, char * key, uint32_t datasize, void * data)
{
  /* Allocate memory for key and data and copy them */
  char * keycopy = hmalloc(sizeof(char)*keysize);
  char * datacopy = hmalloc(sizeof(char)*datasize);
  
  memcpy(keycopy, key, keysize);
  memcpy(datacopy, data, datasize);
  
  /* push element in dictionary */
  CALL(this, _push, 
       keysize, P_dyn, keycopy, datasize, P_dyn, datacopy, NULL);
  
  return 0;
}
/* */

/* Fill sample data (debug purpose) */
int clsm(fill_sample_data)
{
  if (this->_htp)
    perrandexit("fill_sample_data(): htp struct is not NULL!\n");
  
  this->_htp = hcalloc(1, sizeof(ht_props_t));
  
  ht_props_t * htp = this->_htp;
  
  htp->seclen = 0;
  
  htp->ver_major = 0;
  htp->seclen += sizeof(htp->ver_major);
  
  htp->ver_minor = 1;
  htp->seclen += sizeof(htp->ver_minor);
  
  htp->hash_algo = hmalloc(strlf("mod"));
  memcpy(htp->hash_algo, "mod", strlf("mod"));
  htp->seclen += strlf("mod");
  
  htp->hash_dl_helper = hmalloc(1);
  *(htp->hash_dl_helper) = '\0';
  htp->seclen += 1;
  
  htp->hash_fun_init = NULL;
  htp->hash_function = NULL;
  
  htp->hash_modulus = 10;
  htp->seclen += sizeof(htp->hash_modulus);
  
  htp->has_only_key = False;
  htp->seclen += 1;
  
  htp->has_fixed_key_size = False;
  htp->seclen += 1;
  
  htp->key_size = 0;
  htp->seclen += sizeof(htp->key_size);
  
  htp->has_fixed_size = False;
  htp->seclen += 1;
  
  htp->has_full_hash_array = True;
  htp->seclen += 1;
  
  htp->fixed_size_len = 0;
  htp->seclen += sizeof(htp->fixed_size_len);
  
  CALL(this, _append_section, HT_SECTION_PROPS, htp);
  
  
  smeta_t * sm = hcalloc(1, sizeof(smeta_t));
  
  this->_metadata = sm;
  
  const char * meta_fields_test[][2] = {
      {"nome1", "valore1"}, 
      {"pappa", "ciccia"},
      {"cane", "gatto"}, 
      {"ciao", "come stai?"}, 
      {"ancora tu", "ma non dovevamo vederci piu'"},
      {"ma va, ma va", "sapessi quanta gente che c'e' sta'"},
      NULL
    };
  
  for (int i=0; *meta_fields_test[i]; i++)
    CALL(this, add_meta_field,
         strlen(meta_fields_test[i][0])+1,
         strlen(meta_fields_test[i][1])+1,
         (char *)meta_fields_test[i][0],
         (char *)meta_fields_test[i][1]);
  
  CALL(this, _append_section, HT_SECTION_META, sm);
  
  return 0;
}
/* */

/* Print sample data (debug purpose) */
int clsm(print_sample_data)
{
  char *_vtype[] = {"Dyn", "Stat", "Unspec"};
  
  printf("Exploring data...\n");
  
  for (section_t * p = this->_gdata.sections_list; p; 
       p = p->next_section)
  {
    printf("Found new section -> ");
    
    if (p->sectype == HT_SECTION_META)
    {
      printf("metadata section.\n");
      
      smeta_t * metadata = (smeta_t *)p->sec_content;
      
      printf("Meta field (lenght=%d):\n", metadata->seclen);
  
      for (struct smeta_block * block = metadata->list; block; 
           block = block->next_field)
        printf("Name: %s\n Value:%s\n [%s]\n",
        block->field_content->propname.p,
        block->field_content->propval.p,
        _vtype[block->field_content->propval.pt]);
    }
    
    else if (p->sectype == HT_SECTION_PROPS)
    {
      ht_props_t * htp = p->sec_content;
      printf("Properties section.\n");
      
      printf(" seclen: %d\n", htp->seclen);
      printf(" ver_major: %d\n", htp->ver_major);
      printf(" ver_minor: %d\n", htp->ver_minor);
      printf(" hash_algo: %s\n", htp->hash_algo);
      printf(" hash_dl_helper: %s\n", htp->hash_dl_helper);
      printf(" hash_modulus: %d\n", htp->hash_modulus);
      printf(" has_only_key: %d\n", htp->has_only_key);
      printf(" has_fixed_key_size: %d\n", htp->has_fixed_key_size);
      printf(" key_size: %d\n", htp->key_size);
      printf(" has_full_hash_array: %d\n", htp->has_full_hash_array);
      printf(" has_fixed_size: %d\n", htp->has_fixed_size);
      printf(" fixed_size_len: %d\n", htp->fixed_size_len);
    }
    
    else if (p->sectype == HT_SECTION_DATA)
    {
      printf("Data section.\n");
      
      printf("Entries found:\n");
      
      for (int imod = 0; 
           imod < this->_htp->hash_modulus; 
           imod++)
      {
        fprintf(stderr, "modulus=%d\n", imod);
        
        hashentry_nn_t * ne = 
          (hashentry_nn_t *)(pha(this->_ht, imod)->first_entry);
        while(ne)
        {
          fprintf(stderr, "   keysize=%d\n"
                          "    key=%s\n"
                          "    datasize=%d\n"
                          "    data=%s\n",
                  ne->keysize,
                  ne->keyp.p,
                  ne->contentsize,
                  ne->contentp.p
                 );
          ne = ne->next_entry;
        }
        
      }
    }
    
    else
      printf("Section type unknown!\n");
  }
  
  return 0;
}

/* Add meta field in metadata section */
int clsm(add_meta_field,
  uint16_t name_len, uint16_t value_len,
  char * name, char * value
)
{
  smeta_t * sm = this->_metadata;
  
  struct smeta_block * block;
  struct smeta_block ** last_block;
  
  if (!sm)
  {
    sm = hcalloc(1, sizeof(smeta_t));
    this->_metadata = sm;
    CALL(this, _append_section, HT_SECTION_META, sm);
  }
  
  /* Allocate and set to zero memory for the new item pointers. */
  block = hcalloc(1, sizeof(struct smeta_block));
  
  /* Allocate and set to zero memory for the new item content. */
  block->field_content = hcalloc(1, sizeof(struct smeta_field));
  
  /* Fill field. */
  block->field_content->propname_len = name_len;
  block->field_content->propval_len = value_len;
  block->field_content->propname.p = hmalloc(name_len * sizeof(char));
  block->field_content->propname.pt = P_dyn;
  block->field_content->propval.p = hmalloc(value_len * sizeof(char));
  block->field_content->propval.pt = P_dyn;
  memcpy(block->field_content->propname.p, name, name_len);
  memcpy(block->field_content->propval.p, value, value_len);
  
  /* Push new item into list */
  last_block = &(sm->list);
  while(*last_block)
    last_block = &((*last_block)->next_field);
  *last_block = block;
  
  /* Update section lenght. */
  sm->seclen += (name_len + value_len + 
                   sizeof(block->field_content->propname_len) +
                   sizeof(block->field_content->propval_len));
  
  return 0;
}
/* */

/* Append new section */
int clsm(_append_section, uint32_t stype, void * s)
{
  section_t * new_section, ** last_section;
  
  new_section = hcalloc(1, sizeof(section_t));
  new_section->sectype = stype;
  new_section->seclen = *((uint64_t *)s);
  new_section->sec_content = s;
  
  /* Push new item into list */
  last_section = &(this->_gdata.sections_list);
  while(*last_section)
    last_section = &((*last_section)->next_section);
  *last_section = new_section;
  
  this->_gdata.num_of_sections += 1;
  
  return 0;
}
/* */

/* Load props section */
int clsm(_load_props_section, char * secaddr, uint64_t seclen)
{
  if (this->_htp)
    perrandexit("_load_props_section(): _htp is not NULL!\n");
    
  this->_htp = hcalloc(1, sizeof(ht_props_t));
  ht_props_t * htp = this->_htp;
  
  
  htp->seclen = seclen;
  DF_LOAD(htp, secaddr);
  
  CALL(this, _append_section, HT_SECTION_PROPS, htp);
  
  return 0;
}
/* */

/* Load meta section */
int clsm(_load_meta_section, char * secaddr, uint64_t seclen)
{
  uint64_t seccur = 0;
  smeta_t * new_meta = hcalloc(1, sizeof(smeta_t));
  
  new_meta->seclen = seclen;
  
  this->_metadata = new_meta;
  
  struct smeta_block ** block = &(new_meta->list);
  while (seccur < seclen)
  {
    *block = hcalloc(1, sizeof(struct smeta_block));
    (*block)->field_content = calloc(1, sizeof(struct smeta_field));
    
    (*block)->field_content->propname_len = 
      (uint16_t)(*(secaddr+seccur));
    seccur += sizeof((*block)->field_content->propname_len);
    (*block)->field_content->propval_len = 
      (uint16_t)(*(secaddr+seccur));
    seccur += sizeof((*block)->field_content->propname_len);
    (*block)->field_content->propname.p = 
      (char *)(secaddr+seccur);
    (*block)->field_content->propname.pt = P_unspec;
    seccur += (*block)->field_content->propname_len;
    (*block)->field_content->propval.p = 
      (char *)(secaddr+seccur);
    (*block)->field_content->propval.pt = P_unspec;
    seccur += (*block)->field_content->propval_len;
    
    block = &((*block)->next_field);
  }
  
  CALL(this, _append_section, HT_SECTION_META, new_meta);
  
  return 0;
}
/* */

#define _CAST_LOAD(_p_src, _dst, _offs) \
  { \
    unsigned int sz = sizeof(_dst); \
    char * _p_dst = (char *)(&(_dst)); \
    char * __p_src = (char *)(_p_src); \
    \
    for (unsigned int i = 0; i < sz; *(_p_dst+i) = *(__p_src+i), i++); \
    _offs += sz; \
  }

/* Load entry */
int _hashtable__load_entry_nn(void * _this, char * mp,
                              keysize_t * ks, char ** key,
                              uint32_t * ds, void ** data)
{
  uint64_t _mp_offs = 0;
  
  //keysize_t _ks;
  //uint32_t _ds;
  
  _CAST_LOAD(mp, *ks, _mp_offs)
  *key = mp + _mp_offs;
  _mp_offs += (uint64_t)(*ks);
  
  _CAST_LOAD(mp+_mp_offs, *ds, _mp_offs)
  *data = mp + _mp_offs;
  _mp_offs += (uint64_t)(*ds);
  
  //Debug
  //fprintf(stderr, " ****** keysz=%d, datasz=%d, addr=%x\n", 
  //        *ks, *ds, *key);
  
  return _mp_offs;
}
/* */

/* Load full hash array */
int _hashtable__load_full_ha(void * _this, char * mp)
{
  uint64_t mp_offs = 0;
  typeof(this->_num_of_entries) nl;
  typeof(this->_num_of_entries) tot_ne;
  
  /* First load _num_of_entries */
  _CAST_LOAD(mp, tot_ne, mp_offs)
  
  /* Now load entries */
  for (nl=0; nl<tot_ne; nl++)
  {
    keysize_t ks;
    uint32_t ds;
    char *pkey;
    void *pdata;
    
    typeof(this->_htp->hash_modulus) hme;
    typeof(((hasharray_t *)(this->_ht))->num_of_entries) ne;
    
    _CAST_LOAD(mp+mp_offs, hme, mp_offs)
    _CAST_LOAD(mp+mp_offs, ne, mp_offs)
    
    for (unsigned int j = 0; j < ne; j++)
    {
      mp_offs += 
        _hashtable__load_entry_nn(_this, mp+mp_offs, 
                                  &ks, &pkey, &ds, &pdata);
                                  
      CALL(this, _push, ks, P_unspec, pkey, ds, P_unspec, pdata, &hme);
    }
  }
  
  return 0;
}
/* */

/* Load data section */
int clsm(_load_data_section, char * secaddr, uint64_t seclen)
{
  sdata_t * new_data = hcalloc(1, sizeof(sdata_t));
  CALL(this, _append_section, HT_SECTION_DATA, new_data);
  
  _hashtable__load_full_ha(_this, secaddr);
  
  return 0;
}
/* */

/* Load data from file */
int clsm(_load_data_from_file, char *file_name)
{
  int fd;
  struct stat sb;
  char * fmap;
  size_t mapcur = 0;
  
  fd = open(file_name, O_RDONLY);
  if (fd<0)
    perrandexit("open()");
  
  if (fstat(fd, &sb)<0)
    perrandexit("fstat()");
  
  if (!(fmap = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0)))
    perrandexit("mmap()");
  
  /* Check for magick */
  if (memcmp(fmap, FILE_MAGICK, 3))
  {
    fprintf(stderr, "Bad magick!\n");
    return 1;
  }
  
  mapcur += 3;
  
  /* Fill Top level data section. */
  this->_gdata.ht_version_num = (uint32_t)(*(fmap+mapcur));
  mapcur += sizeof(this->_gdata.ht_version_num);
  uint32_t num_of_sections = (uint32_t)(*(fmap+mapcur));
  mapcur += sizeof(num_of_sections);
  this->_gdata.sections_global_flags = (uint32_t)(*(fmap+mapcur));
  mapcur += sizeof(this->_gdata.sections_global_flags);
  
  /* Now load sections. */
  for (int s=0; s<num_of_sections; s++)
  {
    /* Next word is section type. */
    uint32_t sectype = *((uint32_t *)(fmap+mapcur));
    mapcur += sizeof(sectype);
    uint64_t seclen = *((uint64_t *)(fmap+mapcur));
    mapcur += sizeof(seclen);
    
    
    if (sectype == HT_SECTION_PROPS)
    {
      CALL(this, _load_props_section, fmap+mapcur, seclen);
      mapcur += seclen;
      
      CALL(this, _initialize_data_sec);
    }
    
    else if (sectype == HT_SECTION_META)
    {
      CALL(this, _load_meta_section, fmap+mapcur, seclen);
      mapcur += seclen;
    }
    
    else if (sectype == HT_SECTION_DATA)
    {
      CALL(this, _load_data_section, fmap+mapcur, seclen);
      mapcur += seclen;
    }
      
    else
    {
      fprintf(stderr, "Error: unknown section type! Exit...\n");
      exit(EXIT_FAILURE);
    }
      
  }
  
  return 0;
}
/* */

/* Write data meta */
int clsm(_write_data_meta, int fd, smeta_t * metadata)
{
  for (struct smeta_block * block = metadata->list; block; 
           block = block->next_field)
      {
        write(fd, &(block->field_content->propname_len),
              sizeof(block->field_content->propname_len));
        write(fd, &(block->field_content->propval_len),
              sizeof(block->field_content->propval_len));
        write(fd, block->field_content->propname.p,
                  block->field_content->propname_len);
        write(fd, block->field_content->propval.p,
                  block->field_content->propval_len);
      }
  
  return 0;
}
/* */

/* Private _fill_buffer method macro generator: 
 * add new entry into hash array/list */
#define fungen_hashtable__fill_buff_part_ff \
  CALL(b, write, this->_htp->key_size, e->keyp.p); \
  CALL(b, write, this->_htp->fixed_size_len, e->contentp.p);
#define fungen_hashtable__fill_buff_part_fn \
  CALL(b, write, this->_htp->key_size, e->keyp.p); \
  CALL(b, write, sizeof(e->contentsize), &(e->contentsize)); \
  CALL(b, write, e->contentsize, e->contentp.p);
#define fungen_hashtable__fill_buff_part_nf \
  CALL(b, write, sizeof(e->keysize), &(e->keysize)); \
  CALL(b, write, e->keysize, e->keyp.p); \
  CALL(b, write, this->_htp->fixed_size_len, e->contentp.p);
#define fungen_hashtable__fill_buff_part_nn \
  CALL(b, write, sizeof(e->keysize), &(e->keysize)); \
  CALL(b, write, e->keysize, e->keyp.p); \
  CALL(b, write, sizeof(e->contentsize), &(e->contentsize)); \
  CALL(b, write, e->contentsize, e->contentp.p);
#define fungen_hashtable__fill_buff_part_kf \
  CALL(b, write, this->_htp->key_size, e->keyp.p);
#define fungen_hashtable__fill_buff_part_kn \
  CALL(b, write, sizeof(uint32_t), &(e->keysize)); \
  CALL(b, write, e->keysize, e->keyp.p);
#define fungen_hashtable__fill_buff(PF) \
int _hashtable__fill_buff_ ## PF \
( void * _this, buffer * b, void * entry ) \
{ \
  /* Allocate memory for keypkt and set it */ \
  hashentry_ ## PF ## _t * e = \
    (hashentry_ ## PF ## _t *)entry; \
  \
  fungen_hashtable__fill_buff_part_ ## PF \
  return 0; \
}

fungen_hashtable__fill_buff(ff)
fungen_hashtable__fill_buff(fn)
fungen_hashtable__fill_buff(nn)
fungen_hashtable__fill_buff(nf)
fungen_hashtable__fill_buff(kf)
fungen_hashtable__fill_buff(kn)

#undef fungen_hashtable__fill_buff_part_ff
#undef fungen_hashtable__fill_buff_part_fn
#undef fungen_hashtable__fill_buff_part_nn
#undef fungen_hashtable__fill_buff_part_nf
#undef fungen_hashtable__fill_buff_part_kf
#undef fungen_hashtable__fill_buff_part_kn
#undef fungen_hashtable__fill_buff
/* */

/* Create and fill data buffer with full dictionary 
 *  when has_full_hash_array is True */
buffer * _hashtable__fill_full_ha(void * _this)
{
  /* Create new buffer object */
  buffer * b = new(buffer, NULL);
  
  /* write _num_of_entries */
  CALL(b, write, sizeof(this->_num_of_entries), 
       &(this->_num_of_entries));
  
  /* Explore hash_array entries */
  for (uint32_t imod = 0; 
       imod < this->_htp->hash_modulus; 
       imod++)
  {
    fprintf(stderr, "filling modulus %d\n", imod);
    
    hashentry_nn_t * ne = 
      (hashentry_nn_t *)(pha(this->_ht, imod)->first_entry);
    
    if (pha(this->_ht, imod)->num_of_entries)
    {
      /* Since list is non empty, write modulus and num_of_entries */
      CALL(b, write, sizeof(imod), &imod);
      CALL(b, write, sizeof(pha(this->_ht, imod)->num_of_entries), 
           &(pha(this->_ht, imod)->num_of_entries));
    }
      
    while(ne)
    {
      CALL(this, _fill_buff, b, ne);
      ne = ne->next_entry;
    }
    
  }
  
  return b;
}
/* */

/* Write data */
int clsm(write_hashtable, char * file_name)
{
  printf("Writing data to %s\n", file_name);
  int fd;
  
  fd = open(file_name, O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR);
  
  printf("Writing magick...\n");
  write(fd, FILE_MAGICK, sizeof(FILE_MAGICK)-1);
  
  data_t * dt = &(this->_gdata);
  
  printf("Writing header data section...\n");
  write(fd, &(dt->ht_version_num), sizeof(dt->ht_version_num));
  write(fd, &(dt->num_of_sections), sizeof(dt->num_of_sections));
  write(fd, &(dt->sections_global_flags),
        sizeof(dt->sections_global_flags));
  
  printf("Exploring data...\n");
  
  for (section_t * p = dt->sections_list; p; p = p->next_section)
  {
    printf("Found new section -> ");
    
    if (p->sectype == HT_SECTION_META)
    {
      printf("metadata section.\n");
      printf("Writing metadata section header...\n");
      write(fd, &(p->sectype), sizeof(p->sectype));
      
      typeof(((smeta_t *)p->sec_content)->seclen) _seclen = 
        ((smeta_t *)p->sec_content)->seclen;
      
      write(fd, &_seclen, sizeof(_seclen));
      
      CALL(this, _write_data_meta, fd, (smeta_t *)p->sec_content);
    }
    
    else if (p->sectype == HT_SECTION_PROPS)
    {
      printf("properties section.\n");
      printf("Writing properties section header...\n");
      write(fd, &(p->sectype), sizeof(p->sectype));
      write(fd, &(p->seclen), sizeof(p->seclen));
      
      char * wbuf = hmalloc(p->seclen);
      DF_WRITE(p->sec_content, wbuf);
      write(fd, wbuf, p->seclen);
      free(wbuf);
      
    }
    
    else if (p->sectype == HT_SECTION_DATA)
    {
      printf("Data section.\n");
      printf("Writing data section header...\n");
      write(fd, &(p->sectype), sizeof(p->sectype));
      
      printf("Writing data...\n");
      buffer * b = CALL(this, _fill);
      
      uint64_t data_seclen = (uint64_t)(CALL(b, get_mapsz));
      fprintf(stderr, "map_size = %d\n", data_seclen);
      write(fd, &data_seclen, sizeof(data_seclen));
      write(fd, CALL(b, get_map), CALL(b, get_mapsz));
      
      CALL(b, destroy);
    }
    
    else
      printf("Section type unknown!\n");
  }
  
  close(fd);
}
/* */

/* Initialize data section */
int clsm(_initialize_data_sec)
{
  uint8_t ne_case = 0;
  ne_case = (this->_htp->has_only_key) &
            ((this->_htp->has_fixed_key_size << 1) & 0x02) &
            ((this->_htp->has_fixed_size << 2) & 0x04);
  switch(ne_case)
  {
    case 0: 
      this->_newentry  = _hashtable__newentry_nn;
      this->_fill_buff = _hashtable__fill_buff_nn;
      break;
    case 1: 
      this->_newentry  = _hashtable__newentry_kn;
      this->_fill_buff = _hashtable__fill_buff_kn;
      break;
    case 2: 
      this->_newentry  = _hashtable__newentry_fn;
      this->_fill_buff = _hashtable__fill_buff_fn;
      break;
    case 3: 
      this->_newentry  = _hashtable__newentry_kf;
      this->_fill_buff = _hashtable__fill_buff_kf;
      break;
    case 4: 
      this->_newentry  = _hashtable__newentry_nf;
      this->_fill_buff = _hashtable__fill_buff_nf;
      break;
    case 6: 
      this->_newentry  = _hashtable__newentry_ff;
      this->_fill_buff = _hashtable__fill_buff_ff;
      break;
  }
  
  /* Allocate and set hashtable */
  if (memcmp(this->_htp->hash_algo, "mod", 4) == 0)
  {
    /* Use module to compute hash */
    this->compute_hash = _hashtable_compute_hash;
    
    this->_htp->has_full_hash_array = 
      (this->_htp->hash_modulus < FULL_HASH_MAX_SZ) ? True : False;
    
    if (this->_htp->has_full_hash_array)
    {
      /* Use full hash array */
      this->_num_of_classes = this->_htp->hash_modulus;
      
      /* Allocate memory for classes */
      this->_ht = hcalloc(this->_num_of_classes, sizeof(hasharray_t));
      
      /* Link methods */
      this->_lookup_into_class = (this->_htp->has_fixed_key_size) ? 
        _hashtable__lookup_into_class_array_f : 
        _hashtable__lookup_into_class_array_n;
      this->_push = _hashtable_push_into_array;
      
      this->_fill = _hashtable__fill_full_ha;
    }
    
    else
    {
      /* Use list */
      this->_num_of_classes = 0;
      perrandexit("list table not implemented yet!");
    }
  }
  
  else
    perrandexit("hash_algo not implemented yet!");
  
  return 0;
}
/* */

/* Initialize new hashtable object */
int clsm(init, void * init_par)
{
  clsmlnk(_initialize_data_sec);
  clsmlnk(_load_data_from_file);
  clsmlnk(_load_props_section);
  clsmlnk(_load_meta_section);
  clsmlnk(_load_data_section);
  clsmlnk(_append_section);
  clsmlnk(_write_data_meta);
  clsmlnk(add_meta_field);
  clsmlnk(push);
  clsmlnk(fill_sample_data);
  clsmlnk(print_sample_data);
  clsmlnk(write_hashtable);
  clsmlnk(lookup);
  
  struct hashtable_init_params * ip = 
    (struct hashtable_init_params *)init_par;

  if (ip)
  {
    if (ip->file_name)
      CALL(this, _load_data_from_file, ip->file_name);
    else
    {
      this->_htp = hcalloc(1, sizeof(ht_props_t));
      
      this->_htp->seclen = 0;
      
      this->_htp->ver_major = 0;
      this->_htp->seclen += sizeof(this->_htp->ver_major);
      
      this->_htp->ver_minor = 1;
      this->_htp->seclen += sizeof(this->_htp->ver_minor);
      
      if (ip->hash_algo)
      {
        this->_htp->hash_algo = hmalloc(strlf(ip->hash_algo));
        memcpy(this->_htp->hash_algo, ip->hash_algo, 
          strlf(ip->hash_algo));
        this->_htp->seclen += strlf(ip->hash_algo);
      }
      else
      {
        this->_htp->hash_algo = hmalloc(sizeof(char));
        *(this->_htp->hash_algo) = '\0';
        this->_htp->seclen += 1;
      }
        
      if (ip->hash_dl_helper)
      {
        this->_htp->hash_dl_helper = hmalloc(strlf(ip->hash_dl_helper));
        memcpy(this->_htp->hash_dl_helper, ip->hash_dl_helper, 
          strlf(ip->hash_dl_helper));
        this->_htp->seclen += strlf(ip->hash_dl_helper);
      }
      else
      {
        this->_htp->hash_dl_helper = hmalloc(sizeof(char));
        *(this->_htp->hash_dl_helper) = '\0';
        this->_htp->seclen += 1;
      }
      
      this->_htp->hash_modulus = ip->hash_modulus;
      this->_htp->seclen += sizeof(this->_htp->hash_modulus);
      this->_htp->has_only_key = ip->has_only_key;
      this->_htp->seclen += 1;
      this->_htp->has_fixed_key_size = ip->has_fixed_key_size;
      this->_htp->seclen += 1;
      this->_htp->key_size = ip->key_size;
      this->_htp->seclen += sizeof(this->_htp->key_size);
      this->_htp->has_fixed_size = ip->has_fixed_size;
      this->_htp->seclen += 1;
      this->_htp->fixed_size_len = ip->fixed_size_len;
      this->_htp->seclen += sizeof(this->_htp->fixed_size_len);
      
      /* has_full_hash_array space */
      this->_htp->seclen += 1;
    
      CALL(this, _append_section, HT_SECTION_PROPS, this->_htp);
      
      CALL(this, _initialize_data_sec);
      
      sdata_t * data_section = hcalloc(1, sizeof(data_t));
      CALL(this, _append_section, HT_SECTION_DATA, data_section);
    }
  }
  
  return 0;
}

#endif
#undef _CLASS_NAME
/* hashtable object methods ends. */
/**********************************/
