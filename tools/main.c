#include <stdio.h>

#include "cobj.h"
#include "hashtable.h"

int main(void)
{
  //struct hashtable_init_params ip = 
  //  {"ver_0_1_ciao.baf", NULL, NULL, 0, False, False, 0, False, 0};
  
  struct hashtable_init_params ip = 
    {NULL, "mod", NULL, 11, False, False, 0, False, 0};
    
  //struct hashtable_init_params ip = 
  //  {"ver_0_1_prova2.baf", NULL, NULL, 0, False, False, 0, False, 0};
    
  //hashtable * t = new(hashtable, &ip); //<--
  
  //CALL(t, add_meta_field, 5, 7, "nome", "valore");
  //CALL(t, add_meta_field, 5, 7, "papa", "mamama");
  //CALL(t, add_meta_field, 5, 6, "cane", "gatto");
  
  //CALL(t, push, 5, "NAME", 9, "MAVERICK");
  //CALL(t, push, 5, "nome", 11, "fiordaliso");
  
  
  //CALL(t, print_sample_data); //<--
  
  
  htables * tt = new(htables, NULL);
  
  CALL(tt, loadtables, "multi3.baf");
  
  hashtable * t = CALL(tt, lookup, "test");
  
  //CALL(tt, newtable, "piro", &ip);
  
  //CALL(t, push, 5, "Kook", 6, "12345");
  CALL(t, print_sample_data);
  
  //CALL(tt, savetables, "multi3.baf");
  
  /*
  hashtable * t = CALL(tt, newtable, "test", &ip);
  
  CALL(t, push, 5, "NAME", 9, "MAVERICK");
  CALL(t, push, 5, "nome", 11, "fiordaliso");
  
  CALL(t, print_sample_data);
  
  ip.hash_modulus = 31;
  hashtable * t2 = CALL(tt, newtable, "nuovanuova", &ip);
  CALL(t2, push, 3, "Ok", 10, "1MAVERICK");
  CALL(t2, push, 6, "emonu", 10, "iordaliso");
  
  CALL(t2, print_sample_data);
  
  CALL(tt, savetables, "multi1.baf");
  */
  
  /*
  struct lookup_res lr;
  
  if (CALL(t, lookup, 5, "OLAV", NULL, &lr) == 0)
    fprintf(stderr, "Key found! Data: %s\n", lr.content);
  else
    fprintf(stderr, "Key NOT found.\n");
  */
    
  //Debug
  //uint16_t k = 1035;
  //fprintf(stderr, "Computed hash for %d is %d\n", k, CALL(t, compute_hash, 2, &k));
  
  //CALL(t, write_hashtable, "ver_0_1_prova2.baf");
  
  return 0;
}
