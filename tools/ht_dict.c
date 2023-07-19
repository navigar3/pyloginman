#include <stdio.h>

#include "cobj.h"
#include "hashtable.h"

hashtable * new_ht_table(
  void *    _ht_dicts,
  char *    table_name,
  char *    hash_algo,
  char *    hash_dl_helper,
  uint32_t  hash_modulus,
  uint32_t  has_only_key,
  uint32_t  has_fixed_key_size,
  uint32_t  key_size,
  uint32_t  has_fixed_size,
  uint32_t  fixed_size_len
)
{
  struct hashtable_init_params ip;
  
  ip.map = NULL;
  ip.hash_algo = hash_algo;
  ip.hash_dl_helper = hash_dl_helper;
  ip.hash_modulus = hash_modulus;
  ip.has_only_key = (has_only_key) ? True: False;
  ip.has_fixed_key_size = (has_fixed_key_size) ? True : False;
  ip.key_size = key_size;
  ip.has_fixed_size = (has_fixed_size) ? True : False;
  ip.fixed_size_len = fixed_size_len;
  
  htables * ht_dicts = (htables *)_ht_dicts;
  
  hashtable * t = CALL(ht_dicts, newtable, table_name, &ip);
  
  return t;
}

int ht_table_push(
  void *    _ht_dict,
  uint32_t  ks, 
  char *    key, 
  uint32_t  ds, 
  char *    data
)
{
  hashtable * t = (hashtable *)_ht_dict;
  
  CALL(t, push, (keysize_t)ks, key, ds, data);
  
  return 0;
}

int ht_table_lookup(
  void *    _ht_dict, 
  uint32_t  ks, 
  char *    key,
  void *    res
)
{
  hashtable * ht_dict = (hashtable *)_ht_dict;
  
  int ret = CALL(ht_dict, lookup, (keysize_t)ks, key, NULL, res);
  
  return ret;
}

htables * new_ht_dicts(void)
{
  return new(htables, NULL);
}

int ht_save_dicts(void * t, char * file_name)
{
  htables * tt = (htables *)t;
  
  CALL(tt, savetables, file_name);
  
  return 0;
}

int ht_load_from_file(void * t, char * file_name)
{
  htables * tt = (htables *)t;
  
  return CALL(tt, loadtables, file_name);
}

uint32_t ht_get_num_of_tables(void * t)
{
  htables * tt = (htables *)t;
  
  return CALL(tt, get_num_of_tables);
}

void * ht_get_first_table_ref(void * t)
{
  htables * tt = (htables *)t;
  
  return CALL(tt, get_first_table_ref);
}

void * ht_get_next_table_ref(void * t, void * cref)
{
  htables * tt = (htables *)t;
  
  return CALL(tt, get_next_table_ref, cref);
}

void * ht_get_table_by_ref(void * t, void * ref)
{
  htables * tt = (htables *)t;
  
  return CALL(tt, get_table_by_ref, ref);
}

char * ht_get_table_name(void * t)
{
  hashtable * tt = (hashtable *)t;
  
  char * r = CALL(tt, get_table_name);
  
  return r;
}

int ht_print_data(void * t)
{
  hashtable * tt = (hashtable *) t;
  
  CALL(tt, print_sample_data);
  
  return 0;
}
