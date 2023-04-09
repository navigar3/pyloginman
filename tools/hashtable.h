#ifndef _HASHTABLE_H

#define _HASHTABLE_H


#include <stdint.h>


#define HT_VERSION 0x00000001
#define FILE_MAGICK "htd"

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


/* Metadata section:
 *  define metadata proprerties for the file. */
#define  HT_SECTION_META  0x00000001

struct smeta_field
{
  uint16_t propname_len;
  uint16_t propval_len;
  char * propname;
  char * propval;
};

struct smeta_block
{
  struct smeta_field * field_content;
  struct smeta_block * next_field;
};

typedef struct
{
  uint16_t seclen;
  
  struct smeta_block * list;
} smeta_t;

/* Add metadata field in metadata section */
int add_meta_field(
  uint16_t name_len,
  uint16_t value_len,
  char * name,
  char * value
);

/* Append metadata section to sections' list. */
int append_meta_section(smeta_t * smeta);

/* Save metadata section into file. */
int write_data_meta(int fd, smeta_t * metadata);

/* Metadata defs ends. */


#endif
