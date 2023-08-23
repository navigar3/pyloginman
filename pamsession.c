#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <security/pam_appl.h>
#ifdef HAVE_SECURITY_PAM_MISC_H
# include <security/pam_misc.h>
#elif defined(HAVE_SECURITY_OPENPAM_H)
# include <security/openpam.h>
#endif

#include "log.h"

pam_handle_t * pamh = NULL;

int initialize_pam(char * username, char * tty_name)
{
  #ifdef HAVE_SECURITY_PAM_MISC_H
		struct pam_conv conv = { misc_conv, NULL };	  /* Linux-PAM conversation function */
  #elif defined(HAVE_SECURITY_OPENPAM_H)
		struct pam_conv conv = { openpam_ttyconv, NULL }; /* OpenPAM conversation function */
  #endif
  
  int res = pam_start("login", username, &conv, &pamh);
  if (res != PAM_SUCCESS)
  {
    LogErr("pam_start() failed!")
    return 1;
  }
  
  res = pam_set_item(pamh, PAM_RHOST, NULL);
  if (res != PAM_SUCCESS)
  {
    LogErr("pam_set_item() PAM_RHOST failed!")
    return 1;
  }
  
  res = pam_set_item(pamh, PAM_TTY, tty_name);
  if (res != PAM_SUCCESS)
  {
    LogErr("pam_set_item() PAM_TTY failed!")
    return 1;
  }
  
  return 0;
}


int pam_start_session(void)
{
  int res = pam_setcred(pamh, PAM_ESTABLISH_CRED);
  if (res != PAM_SUCCESS)
  {
    LogErr("pam_setcred() PAM_ESTABLISH_CRED failed!")
    return 1;
  }
  
  res = pam_open_session(pamh, 0);
  if (res != PAM_SUCCESS)
  {
    LogErr("pam_open_session() failed!")
    return 1;
  }
  
  res = pam_setcred(pamh, PAM_REINITIALIZE_CRED);
  if (res != PAM_SUCCESS)
  {
    LogErr("pam_setcred() PAM_REINITIALIZE_CRED failed!")
    return 1;
  }
  
  return 0;
}
