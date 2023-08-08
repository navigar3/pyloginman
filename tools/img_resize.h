#ifndef _IMG_RESIZE_H

#define _IMG_RESIZE_H

#include <stdint.h>


#define  F_RGB24   0x00000018
#define  F_RGBA32  0x00000020

struct pxcolor24_s
{
  char rgb[3];
};

struct pxcolor32_s
{
  char rgba[4];
};

#endif
