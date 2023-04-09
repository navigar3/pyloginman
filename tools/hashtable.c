#include <stdio.h>
#include <malloc.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "hashtable.h"

data_t data = {HT_VERSION, 0, 0, NULL};

/* Only one metadata field is allowed. */
smeta_t smeta = {0, (struct smeta_block *)NULL};


/* Metadata section methods. */
/* ************************* */
int add_meta_field(
  uint16_t name_len,
  uint16_t value_len,
  char * name,
  char * value
)
{
  struct smeta_block * block;
  struct smeta_block ** last_block;
  
  /* Allocate and set to zero memory for the new item pointers. */
  block = calloc(1, sizeof(struct smeta_block));
  
  /* Allocate and set to zero memory for the new item content. */
  block->field_content = calloc(1, sizeof(struct smeta_field));
  
  /* Fill field. */
  block->field_content->propname_len = name_len;
  block->field_content->propval_len = value_len;
  block->field_content->propname = malloc(name_len * sizeof(char));
  block->field_content->propval = malloc(value_len * sizeof(char));
  memcpy(block->field_content->propname, name, name_len);
  memcpy(block->field_content->propval, value, value_len);
  
  /* Push new item into list */
  last_block = &(smeta.list);
  while(*last_block)
    last_block = &((*last_block)->next_field);
  *last_block = block;
  
  /* Update section lenght. */
  smeta.seclen += (name_len + value_len + 
                   sizeof(block->field_content->propname_len) +
                   sizeof(block->field_content->propval_len));
  
  return 0;
}

int append_meta_section(smeta_t * smeta)
{
  section_t * new_section, ** last_section;
  
  new_section = calloc(1, sizeof(section_t));
  new_section->sectype = HT_SECTION_META;
  new_section->seclen = smeta->seclen;
  new_section->sec_content = (void *)smeta;
  
  /* Push new item into list */
  last_section = &(data.sections_list);
  while(*last_section)
    last_section = &((*last_section)->next_section);
  *last_section = new_section;
  
  data.num_of_sections += 1;
  
  return 0;
}

int write_data_meta(int fd, smeta_t * metadata)
{
  for (struct smeta_block * block = metadata->list; block; 
           block = block->next_field)
      {
        write(fd, &(block->field_content->propname_len),
              sizeof(block->field_content->propname_len));
        write(fd, &(block->field_content->propval_len),
              sizeof(block->field_content->propval_len));
        write(fd, block->field_content->propname,
                  block->field_content->propname_len);
        write(fd, block->field_content->propval,
                  block->field_content->propval_len);
      }
  
  return 0;
}
/* ****************************** */
/* Metadata section methods ends. */


int write_data(int fd)
{
  printf("Writing magick...\n");
  write(fd, FILE_MAGICK, sizeof(FILE_MAGICK)-1);
  
  printf("Writing header data section...\n");
  write(fd, &(data.ht_version_num), sizeof(data.ht_version_num));
  write(fd, &(data.num_of_sections), sizeof(data.num_of_sections));
  write(fd, &(data.sections_global_flags),
        sizeof(data.sections_global_flags));
  
  printf("Exploring data...\n");
  
  for (section_t * p = data.sections_list; p; p = p->next_section)
  {
    printf("Found new section -> ");
    
    if (p->sectype == HT_SECTION_META)
    {
      printf("metadata section.\n");
      printf("Writing metadata section header...\n");
      write(fd, &(p->sectype), sizeof(p->sectype));
      write(fd, &(p->seclen), sizeof(p->seclen));
      
      write_data_meta(fd, (smeta_t *)p->sec_content);
    }
    
    else
      printf("Section type unknown!\n");
  }
}

int load_data_from_stream()
{
  int fd;
  struct stat sb;
  char * fmap;
  int mapcur = 0;
  
  fd = open("out.baf", O_RDONLY);
  fstat(fd, &sb);
  
  fmap = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  
  //for (int j=0; j<sb.st_size; j++)
  //  fprintf(stderr, " --> %x\n", fmap[j]);
  
  //return 0;
  
  /* Check for magick */
  if (memcmp(fmap, FILE_MAGICK, 3))
  {
    fprintf(stderr, "Bad magick!\n");
    return 1;
  }
  
  mapcur += 3;
  
  /* Now fill Top level data section. */
  data.ht_version_num = (uint32_t)(*(fmap+mapcur));
  mapcur += 4;
  data.num_of_sections = (uint32_t)(*(fmap+mapcur));
  mapcur += 4;
  data.sections_global_flags = (uint32_t)(*(fmap+mapcur));
  mapcur += 4;
  
  /* Now load sections. */
  for (int s=0; s<data.num_of_sections; s++)
  {
    /* Next word is section type. */
    uint32_t sectype = (uint32_t)(*(fmap+mapcur));
    mapcur += 4;
    uint32_t seclen = (uint64_t)(*(fmap+mapcur));
    mapcur += 8;
    
    if (sectype == HT_SECTION_META)
    {
      char * secaddr = fmap+mapcur;
      int seccur = 0;
      smeta_t * new_meta = calloc(1, sizeof(smeta_t));
      
      new_meta->seclen = seclen;
      
      struct smeta_block ** block = &(new_meta->list);
      while (seccur < seclen)
      {
        *block = calloc(1, sizeof(struct smeta_block));
        (*block)->field_content = calloc(1, sizeof(struct smeta_field));
          //(struct smeta_field *)(secaddr+seccur);
        
        (*block)->field_content->propname_len = 
          (uint16_t)(*(secaddr+seccur));
        seccur += sizeof((*block)->field_content->propname_len);
        (*block)->field_content->propval_len = 
          (uint16_t)(*(secaddr+seccur));
        seccur += sizeof((*block)->field_content->propname_len);
        (*block)->field_content->propname = 
          (char *)(secaddr+seccur);
        seccur += (*block)->field_content->propname_len;
        (*block)->field_content->propval = 
          (char *)(secaddr+seccur);
        seccur += (*block)->field_content->propval_len;
                  
        fprintf(stderr, 
               "load pname = %s / pval = %s\n", 
               (*block)->field_content->propname,
               (*block)->field_content->propval);
        
        block = &((*block)->next_field);
      }
      
      append_meta_section(new_meta);
    }
    
    
  }
}

void print_sample_data(void)
{
  printf("Exploring data...\n");
  
  for (section_t * p = data.sections_list; p; p = p->next_section)
  {
    printf("Found new section -> ");
    
    if (p->sectype == HT_SECTION_META)
    {
      printf("metadata section.\n");
      
      smeta_t * metadata = (smeta_t *)p->sec_content;
      
      printf("Meta field (lenght=%d):\n", metadata->seclen);
  
      for (struct smeta_block * block = metadata->list; block; 
           block = block->next_field)
        printf("Name: %s\n Value:%s\n",
        block->field_content->propname,
        block->field_content->propval);
    }
    
    else
      printf("Section type unknown!\n");
  }
}

void fill_sample_data(void)
{
  const char * meta_fields_test[][2] = {
      {"nome1", "valore1"}, 
      {"pappa", "ciccia"},
      {"cane", "gatto"}, 
      {"ciao", "come stai?"}, 
      {"ancora tu", "ma non dovevamo vederci piu'"},
      NULL
    };
  
  for (int i=0; *meta_fields_test[i]; i++)
    add_meta_field(strlen(meta_fields_test[i][0])+1,
                   strlen(meta_fields_test[i][1])+1,
                   (char *)meta_fields_test[i][0],
                   (char *)meta_fields_test[i][1]);
  
  append_meta_section(&smeta);
  
  
  printf("Writing data to out.baf");
  int fd;
  
  fd = open("out.baf", O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR);
  
  write_data(fd);
  
  close(fd);
}

int main(void)
{
  //fill_sample_data();
  
  load_data_from_stream();
  
  print_sample_data();

  return 0;
}
