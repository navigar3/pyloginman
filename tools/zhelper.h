#ifndef _ZHELPER_H

#define _ZHELPER_H


void * z_prepare_buffer(unsigned int bufflen);

void * z_get_buffer_data(void * buff);

unsigned int z_get_buffer_data_sz(void * buff);

int z_destroy_buffer(void * buff);

int z_inflate(unsigned int slen, char *src, void * dst);

int z_deflate(unsigned int slen, char * src, void * dst);

#endif
