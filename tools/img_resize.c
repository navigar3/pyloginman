#include "img_resize.h"

#include <stdio.h>

int img_resize(uint32_t in_fmt, uint32_t out_fmt,
               uint32_t in_w, uint32_t in_h,
               uint32_t out_w, uint32_t out_h,
               void * in, void * out)
{
  uint32_t x, y;
  
  uint32_t xi[2], yi[2];
 
  uint32_t w0 = in_w - 1;
  uint32_t h0 = in_h - 1;
  
  uint32_t w1 = out_w - 1;
  uint32_t h1 = out_h - 1;
  
  uint32_t r, g, b;
  uint8_t * px_in, * px_out;
  
  uint32_t pxsize_in, pxsize_out;
  
  if (in_fmt == F_RGB24) pxsize_in = 3;
  else if (in_fmt == F_RGBA32) pxsize_in = 4;
  else return -1;
  
  if (out_fmt == F_RGB24) pxsize_out = 3;
  else if (out_fmt == F_RGBA32) pxsize_out = 4;
  else return -1;
  
  for (y=0; y<out_h; y++)
    for (x=0; x<out_w; x++)
    {
      xi[0] = (w0 * x) / w1;
      xi[1] = xi[0] + 1;
      if (xi[1] >= in_w) xi[1] = xi[0];
      
      yi[0] = (h0 * y) / h1;
      yi[1] = yi[0] + 1;
      if (yi[1] >= in_h) yi[1] = yi[0];
      
      r = g = b = 0;
      
      for (int i=0; i<2; i++)
        for(int j=0; j<2; j++)
        {
          px_in = ((uint8_t *)in) + (yi[j] * in_w + xi[i]) * pxsize_in;
          r += (uint32_t)(*px_in);
          g += (uint32_t)(*(px_in+1));
          b += (uint32_t)(*(px_in+2));
        }
      
      r = (r) ? (r / 4): 0;
      g = (g) ? (g / 4): 0;
      b = (b) ? (b / 4): 0;
      
      px_out = ((uint8_t *)out) + (y * out_w + x) * pxsize_out;
      *px_out     = (uint8_t)r;
      *(px_out+1) = (uint8_t)g;
      *(px_out+2) = (uint8_t)b;
      
    }
  
  return 0;
}
