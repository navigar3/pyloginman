#include <stdio.h>

#include "cobj.h"
#include "hashtable.h"

hashtable * new_ht_dict(
  char *    file_name, 
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
  
  ip.file_name = file_name;
  ip.hash_algo = hash_algo;
  ip.hash_dl_helper = hash_dl_helper;
  ip.hash_modulus = hash_modulus;
  ip.has_only_key = (has_only_key) ? True: False;
  ip.has_fixed_key_size = (has_fixed_key_size) ? True : False;
  ip.key_size = key_size;
  ip.has_fixed_size = (has_fixed_size) ? True : False;
  ip.fixed_size_len = fixed_size_len;
  
  fprintf(stderr, "Going on with %s\n", file_name);
  
  hashtable * t = new(hashtable, &ip);
  
  return t;
}

int ht_print_data(void * t)
{
  hashtable * tt = (hashtable *) t;
  
  CALL(tt, print_sample_data);
  
  return 0;
}
