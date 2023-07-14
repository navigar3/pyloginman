#include <stdio.h>

#include "cobj.h"
#include "hashtable.h"

int main(void)
{
  struct hashtable_init_params ip = 
    {"ver_0_1_ciao.baf", NULL, NULL, 0, False, False, 0, False, 0};
    
  hashtable * t = new(hashtable, &ip);
  
  //CALL(t, push, 5, P_unspec, "ELAV", 6, P_unspec, "VALUE", NULL);
  //CALL(t, push, 5, P_unspec, "ILAV", 6, P_unspec, "VALUE", NULL);
  //CALL(t, push, 5, P_unspec, "OLAV", 6, P_unspec, "VILUE", NULL);
  //CALL(t, push, 5, P_unspec, "ELAM", 6, P_unspec, "VALUE", NULL);
  
  //CALL(t, push, 9, P_unspec, "LA_NUOVE", 11, P_unspec, "QUO_VADIS?", NULL);
  
  CALL(t, print_sample_data);
  
  struct lookup_res lr;
  
  if (CALL(t, lookup, 5, "OLAV", NULL, &lr) == 0)
    fprintf(stderr, "Key found! Data: %s\n", lr.content);
  else
    fprintf(stderr, "Key NOT found.\n");
    
  //Debug
  //uint16_t k = 1035;
  //fprintf(stderr, "Computed hash for %d is %d\n", k, CALL(t, compute_hash, 2, &k));
  
  //CALL(t, write_hashtable, "ver_0_1_ciao.baf");
  
  return 0;
}
