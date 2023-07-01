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

/* Top level section. */
//data_t data = {HT_VERSION, 0, 0, NULL};

/* Only one metadata field is allowed. */
//smeta_t smeta = {0, (struct smeta_block *)NULL};


/* ************************* */
/* Metadata section methods. */
int add_meta_field(
  smeta_t * sm,
  uint16_t name_len,
  uint16_t value_len,
  char * name,
  char * value
)
{
  struct smeta_block * block;
  struct smeta_block ** last_block;
  
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

/* Metadata section methods ends. */
/* ****************************** */


/* **********************/
/* Prop section methods */

/* Prop section methods ends. */
/* ****************************/


/**************/
/* New object */
void * newobj(int (*init_fun)(void*, void*),
              size_t osz, 
              void * init_par)
{
  genobj * newo;
  
  newo = hcalloc(1, osz);
  
  newo->init = init_fun;
  
  if(newo->init(newo, init_par))
    perrandexit("Failed to create object.");
  
  return newo;
}
/* */
/***/

/***********************************/
/* Implement buffer object methods */
int _buffer_realloc(buffer *this, size_t newsz)
{
  size_t newsize = (newsz) ? newsz : this->msize + this->newwinsz;
  
  if (!(this->addr = 
          mremap(this->addr, this->msize, newsize,  MREMAP_MAYMOVE)))
    perrandexit("mremap()");
  
  this->msize = newsize;
  
  return 0;
}

int _buffer_write(buffer *this, size_t bsz, char * p)
{
  if (this->used + bsz > this->msize)
    CALL(this, _realloc, 0);
  
  char * sp = this->addr + this->used;
  
  for (size_t i = 0; i<bsz; i++, sp++, p++)
    *sp = *p;
  
  this->used += bsz;
  
  return 0;
}

int _buffer_destroy(buffer *this)
{
  if (munmap(this->addr, this->msize))
    perrandexit("munmap()");
  
  free(this);
  
  return 0;
}

int _buffer_init(void * t, void * init_params)
{
  buffer * this = (buffer *)t;
   
  size_t sz = (init_params) ? *((int *)init_params) : 4096;
  
  if (!(this->addr = 
          mmap(NULL, sz, 
               PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 
               -1, 0)
       )
     )
    return 1;
  
  this->_realloc = _buffer_realloc;
  this->write = _buffer_write;
  this->destroy = _buffer_destroy;
    
  this->msize = sz;
  this->used = 0;
  this->newwinsz = 4096;
  
  return 0;
}
/* buffer object methods ends. */
/*******************************/

/**************************************/
/* Implement hashtable object methods */

uint32_t _hashtable_compute_hash(hashtable * this, uint32_t keysize,
                                 char * key)
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
int _hashtable_push_into_list(hashtable * this, uint32_t keysize,
                              char * key, void * data)
{
  
  return 0;
}
/* */

/* Private _newentry method macro generator: 
 * add new entry into hash array/list */
#define fungen_hashtable__newentry_part_ff
#define fungen_hashtable__newentry_part_fn \
  newentry->contentsize = datasize;
#define fungen_hashtable__newentry_part_nf \
  newentry->keysize = keysize;
#define fungen_hashtable__newentry_part_nn \
  newentry->keysize = keysize; \
  newentry->contentsize = datasize;
#define fungen_hashtable__newentry(PF) \
void * _hashtable__newentry_ ## PF \
( \
  hashtable * this, \
  uint32_t keysize, pointer_type kt, char * key, \
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
  newentry->contentp.p = data; \
  newentry->contentp.pt = dt; \
  \
  return newentry; \
}

fungen_hashtable__newentry(ff)
fungen_hashtable__newentry(fn)
fungen_hashtable__newentry(nn)
fungen_hashtable__newentry(nf)

#undef fungen_hashtable__newentry_part_ff
#undef fungen_hashtable__newentry_part_fn
#undef fungen_hashtable__newentry_part_nn
#undef fungen_hashtable__newentry_part_nf
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
  hashtable * this, 
  void * modclass,
  uint32_t keysize, char * key,
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
          ls->content = (*entry)->contentp.p;
          ls->keysize = (*entry)->keysize;
          if (this->_htp->has_fixed_size)
            ls->contentsize = (*entry)->contentsize;
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
  hashtable * this, 
  void * modclass,
  uint32_t keysize, char * key,
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
      ls->content = (*entry)->contentp.p;
      ls->keysize = (*entry)->keysize;
      if (this->_htp->has_fixed_size)
        ls->contentsize = (*entry)->contentsize;
    }
    
    return 0;
  }
      
  
  if (resptr)
    *((hashentry_nn_t ***)resptr) = entry;
  
  return -1;
}
/* */

/* Push element into hashtable */
int _hashtable_push_into_array(
  hashtable * this, uint32_t keysize, 
  pointer_type kt, char * key, uint32_t datasize, 
  pointer_type dt, void * data)
{ 
  /* Compute key hash */ 
  uint32_t keyhash = CALL(this, compute_hash, 
                          (this->_htp->has_fixed_key_size) ? 
                          this->_htp->key_size : keysize, key);
  
  hashentry_t ** he, * ne;
  
  /* Allocate new item and set it */
  ne = CALL(this, _newentry, 
            keysize, P_unspec, key, 
            datasize, P_unspec, data);
  
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

/* Fill sample data (debug purpose) */
int _hashtable_fill_sample_data(hashtable * this)
{
  if (this->_htp)
    perrandexit("fill_sample_data(): htp struct is not NULL!\n");
  
  this->_htp = hcalloc(1, sizeof(ht_props_t));
  
  ht_props_t * htp = this->_htp;
  
  htp->seclen = 0;
  
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
  
  htp->has_key = True;
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
    add_meta_field(sm,
                   strlen(meta_fields_test[i][0])+1,
                   strlen(meta_fields_test[i][1])+1,
                   (char *)meta_fields_test[i][0],
                   (char *)meta_fields_test[i][1]);
  
  CALL(this, _append_section, HT_SECTION_META, sm);
  
  return 0;
}
/* */

/* Print sample data (debug purpose) */
int _hashtable_print_sample_data(hashtable * this)
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
      printf(" hash_algo: %s\n", htp->hash_algo);
      printf(" hash_dl_helper: %s\n", htp->hash_dl_helper);
      printf(" hash_modulus: %d\n", htp->hash_modulus);
      printf(" has_key: %d\n", htp->has_key);
      printf(" has_fixed_key_size: %d\n", htp->has_fixed_key_size);
      printf(" key_size: %d\n", htp->key_size);
      printf(" has_full_hash_array: %d\n", htp->has_full_hash_array);
      printf(" has_fixed_size: %d\n", htp->has_fixed_size);
      printf(" fixed_size_len: %d\n", htp->fixed_size_len);
    }
    
    else
      printf("Section type unknown!\n");
  }
  
  return 0;
}

/* Append new section */
int _hashtable__append_section(hashtable * this, 
                               uint32_t stype, void * s)
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
int _hashtable__load_props_section(hashtable * this, 
                                   char * secaddr, uint64_t seclen)
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
int _hashtable__load_meta_section(hashtable * this, 
                                  char * secaddr, uint64_t seclen)
{
  uint64_t seccur = 0;
  smeta_t * new_meta = hcalloc(1, sizeof(smeta_t));
  
  new_meta->seclen = seclen;
  
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

/* Load data from file */
int _hashtable__load_data_from_file(hashtable * this, char *file_name)
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
    
    if (sectype == HT_SECTION_META)
    {
      CALL(this, _load_meta_section, fmap+mapcur, seclen);
      mapcur += seclen;
    }
    
    else if (sectype == HT_SECTION_PROPS)
    {
      CALL(this, _load_props_section, fmap+mapcur, seclen);
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
int _hashtable__write_data_meta(hashtable * this, 
                                int fd, smeta_t * metadata)
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

/* Write data */
int _hashtable_write_hashtable(hashtable * this, char * file_name)
{
  printf("Writing data to %s\n", file_name);
  int fd;
  
  fd = open(file_name, O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR);
  
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
      write(fd, &(p->seclen), sizeof(p->seclen));
      
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
    
    else
      printf("Section type unknown!\n");
  }
  
  close(fd);
}
/* */

/* Create new hashtable object */
int _hashtable_init(void * t, void * init_par)
{
  hashtable * this = (hashtable *)t;
  
  this->_load_data_from_file = _hashtable__load_data_from_file;
  this->_load_props_section = _hashtable__load_props_section;
  this->_load_meta_section = _hashtable__load_meta_section;
  this->fill_sample_data = _hashtable_fill_sample_data;
  this->print_sample_data = _hashtable_print_sample_data;
  this->_append_section = _hashtable__append_section;
  this->write_hashtable = _hashtable_write_hashtable;
  this->_write_data_meta = _hashtable__write_data_meta;

  if (init_par)
  {
    if (((struct hashtable_init_params *)init_par)->file_name)
      CALL(this, _load_data_from_file, 
           ((struct hashtable_init_params *)init_par)->file_name);
  }
  else
    CALL(this, fill_sample_data);
  
  //if (init_par)
  //  this->_htp = (ht_props_t *)init_par;
  //else
  //  CALL(this, fill_sample_data);
    
  if (this->_htp->has_fixed_key_size)
    this->_newentry = (this->_htp->has_fixed_size) ? 
      _hashtable__newentry_ff : _hashtable__newentry_fn;
  else
    this->_newentry = (this->_htp->has_fixed_size) ? 
      _hashtable__newentry_nf : _hashtable__newentry_nn;
  
  /* Allocate and set hashtable */
  if (memcmp(this->_htp->hash_algo, "mod", 4) == 0)
  {
    /* Use module to compute hash */
    this->compute_hash = _hashtable_compute_hash;
    
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
      this->push = _hashtable_push_into_array;
      
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
/* hashtable object methods ends. */
/**********************************/


int main(void)
{
  /*
  buffer * b = new(buffer, NULL);
  
  CALL(b, write, strlf("ciao belli"), "ciao belli");
  CALL(b, write, strlf("ancora 1"), "ancora 1");
  
  fprintf(stderr, "b->msize=%d, b->used = %d\n", b->msize, b->used);
  
  for (int i=0; i<b->used; i++)
    fprintf(stderr, "%c - %d\n", *(b->addr + i), *(b->addr + i) );
  
  CALL(b, destroy);
  */
  
  //struct hashtable_init_params ip = {"nuova.baf", NULL};
  
  hashtable * t = new(hashtable, NULL);
  
  CALL(t, print_sample_data);
  //CALL(t, write_hashtable, "nuova.baf");
  
  /*
  CALL(t, push, 4, P_unspec, "ELAV", 6, P_unspec, "VALUE");
  CALL(t, push, 4, P_unspec, "ALAV", 6, P_unspec, "VALUE");
  CALL(t, push, 4, P_unspec, "EPEP", 6, P_unspec, "VALUE");
  
  for (int imod = 0; 
       imod < t->_htp->hash_modulus; 
       imod++)
  {
    fprintf(stderr, "modulus=%d\n", imod);
    
    hashentry_nn_t * ne = 
      (hashentry_nn_t *)(pha(t->_ht, imod)->first_entry);
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
  */
  
  return 0;
}
