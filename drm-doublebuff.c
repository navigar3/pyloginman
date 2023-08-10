#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "cobj.h"
#include "tools/zhelper.h"

#define _DRM_DOUBLEBUFF_H_PRIVATE

#include "drm-doublebuff.h"


/* Helper for python binding */
drmvideo * new_drmvideo(char * videocard)
{
  return new(drmvideo, videocard);
}


/*************************************/
/* Implement imagetool class methods */
#ifndef _CLASS_NAME
#define _CLASS_NAME imagetool

thisclass_creator

/* Load image from file*/
int clsm(load_from_file, char * image_file_name)
{
  int fd;
  struct stat sb;
  char * fmap;
  size_t m_off = 0;
  
  fd = open(image_file_name, O_RDONLY);
  if (fd<0)
  {
    fprintf(stderr, "open()\n");
    return 1;
  }
  
  if (fstat(fd, &sb)<0)
  {
    fprintf(stderr, "fstat()\n");
    return 1;
  }
  
  if (!(fmap = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0)))
  {
    fprintf(stderr, "mmap()\n");
    return 1;
  }
  
  /* Check for magick */
  if (memcmp(fmap, "i0i0", 4)==0)
  {
    fprintf(stderr, "Uncompressed image.\n");
    this->_immap = fmap;
    this->_immapsz = (size_t)sb.st_size;
    m_off += 4;
  }
  
  else if (memcmp(fmap, "z0z0", 4)==0)
  {
    fprintf(stderr, "Compressed image.\n");
    m_off += 4;
    uint32_t zlen = *((uint32_t *)(fmap+m_off));
    m_off += 4;
    fprintf(stderr, "Compressed image size = %d.\n", sb.st_size-m_off);
    fprintf(stderr, "Image size = %d.\n", zlen);
    
    this->_imbuff = z_prepare_buffer(zlen);
    int ret = z_inflate(sb.st_size-m_off, fmap+m_off, this->_imbuff);
    if (ret)
    {
      fprintf(stderr, "Error while inflating!\n");
      munmap(fmap, sb.st_size);
      return -1;
    }
    
    /* Successfully inflated */
    
    /* Free compressed file map */
    munmap(fmap, sb.st_size);
    
    /* Set map offset to 0 */
    m_off = 0;
    
    /* Save pointer to uncompressed map. */
    this->_immap = z_get_buffer_data(this->_imbuff);
    this->_immapsz = zlen;
    
    /* Check magick and advance map offset. */
    if (memcmp(this->_immap, "i0i0", 4))
    {
      fprintf(stderr, "File data corrupted.\n");
      z_destroy_buffer(this->_imbuff);
      return -1;
    }
    else
      m_off = 4;
  }
  
  else
  {
    fprintf(stderr, "Unkwnown image type!\n");
    munmap(fmap, sb.st_size);
    return -1;
  }
  
  this->_immode = *((uint32_t *)(this->_immap+m_off));
  m_off += 4;
  this->_w = *((uint32_t *)(this->_immap+m_off));
  m_off += 4;
  this->_h = *((uint32_t *)(this->_immap+m_off));
  m_off += 4;
  
  this->_imraw = this->_immap+m_off;
  
  fprintf(stderr, "Image info: mode=%d, w=%d, h=%d\n", 
                  this->_immode, this->_w, this->_h);
    
  fprintf(stderr, "All done.\n");
  
  return 0;
}

/* Resize image */
int clsm(resize, uint32_t out_fmt,
                 uint32_t out_w, uint32_t out_h,
                 char * out)
{
  char * in = this->_imraw;
  uint32_t in_fmt = this->_immode;
  uint32_t in_w = this->_w;
  uint32_t in_h = this->_h;
  
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
      
      r = r / 4;
      g = g / 4;
      b = b / 4;
      
      px_out = ((uint8_t *)out) + (y * out_w + x) * pxsize_out;
      *px_out     = (uint8_t)b;
      *(px_out+1) = (uint8_t)g;
      *(px_out+2) = (uint8_t)r;
      
    }
    
  return 0;
}

/* Destroy imagetool object */
int clsm(destroy)
{
  if (this->_imbuff)
    z_destroy_buffer(this->_imbuff);
  else
    munmap(this->_immap, this->_immapsz);
  
  return 0;
}

/* Initialize imagetool object */
int clsm(init, void * init_params)
{
  clsmlnk(load_from_file);
  clsmlnk(resize);
  clsmlnk(destroy);
  
  char * fname = (char *)init_params;
  
  if (!fname)
    return 1;
    
  if (CALL(this, load_from_file, fname))
  {
    fprintf(stderr, "Error while loading image file %s!\n", fname);
    return 1;
  }
   
  return 0;
}

#endif
#undef _CLASS_NAME
/* imagetool object methods ends. */
/**********************************/


/*************************************/
/* Implement videoterm class methods */
#ifndef _CLASS_NAME
#define _CLASS_NAME videoterm

#define _PARENT_CLASS_videoterm monitor

thisclass_creator

uint32_t clsm(get_curx)
{
  return this->_curx;
}

uint32_t clsm(get_cury)
{
  return this->_cury;
}

uint32_t clsm(get_nrows)
{
  return this->_rows;
}

uint32_t clsm(get_ncols)
{
  return this->_cols;
}

int clsm(move_cur, int32_t x, int32_t y)
{
  if ((x>=0) && (x<this->_cols))
    this->_curx = x;
    
  if ((y>=0) && (y<this->_rows))
    this->_cury = y;
  
  return 0;
}

int clsm(move_cur_rel, int32_t x, int32_t y)
{
  int32_t newx = this->_curx + x;
  if ((newx>=0) && (newx<this->_cols))
    this->_curx = newx;
  
  int32_t newy = this->_cury + y;
  if ((newy>=0) && (newy<this->_rows))
    this->_cury = newy;
  
  return 0;
}

int clsm(move_cur_prop, int32_t x, int32_t y)
{
  if (x>=0)
  {
    int32_t newx = (x * this->_cols) / 1000;
    if ((newx>=0) && (newx<this->_cols))
      this->_curx = newx;
  }
  
  if (y>=0)
  {
    int32_t newy = (y * this->_rows) / 1000;
    if ((newy>=0) && (newy<this->_rows))
      this->_cury = newy;
  }
  
  return 0;
}

int clsm(set_fontcolor, uint32_t fcolor)
{
  this->_fc = fcolor & 0x00ffffff;
  
  return 0;
}

/* draw 4 bits depth glyph for char key on 
 * terminal and advance cursor. */
int clsm(putc_advance_and_sync,
         uint32_t sync_now, uint32_t advance, uint32_t ks, char * key)
{
  struct modeset_buf * buf, * fbuf, * bbuf;
  uint32_t tapecur = 0;
  uint32_t pxal;
  bool front_buf_tile_fullcopy = false;
  
  /* Check if monitor is enabled */
  if (!(Parent->_enabled))
  {
    fprintf(stderr, "Cannot draw on disabled monitor!\n");
    return -1;
  }
  
  unsigned int x0;
  unsigned int y0;
  unsigned int xmax;
  unsigned int ymax;
  unsigned int off;
  
  /* Tile status struct */
  struct vt_tile_status_s * ts = 
    this->_sb + this->_cury * this->_cols + this->_curx;
  
  /* Select backward buffer and operate on it. */
  buf = &(Parent->_dev->bufs[Parent->_dev->front_buf ^ 1]);
  
  /* Get front buffer */
  fbuf = &(Parent->_dev->bufs[Parent->_dev->front_buf]);
  
  /* Get background buffer */
  bbuf = &(Parent->_dev->bufs[2]);
  
  /* Clear tile if needed */
  if (ts->written)
  {
    x0 = this->_fm->boxwidth * this->_curx;
    y0 = this->_fm->boxheight * this->_cury;
    xmax = x0 + this->_fm->boxwidth;
    ymax = y0 + this->_fm->boxheight;
    
    for (unsigned int j = y0; j < ymax; ++j)
    {
      for (unsigned int k = x0; k < xmax; ++k) 
      {
        off = buf->stride * j + k * 4;
        
        *(uint32_t*)&buf->map[off] = *(uint32_t*)&bbuf->map[off];
      }
    }
    
    front_buf_tile_fullcopy = true; /* Copy full tile on front buffer */
    ts->written = false;            /* Tile is clear. */
  }
  
  /* Search key in .glyphs table */
  struct lookup_res lr;
  if(CALL(this->_glyphs, lookup, ks, key, NULL, &lr))
  {
    front_buf_tile_fullcopy = true;
    /* Glyph not fonud! */
    goto putc_advance_and_sync__advance;
  }
  
  /* Glyph metrics */
  struct glyph_metrics_s * gm = (struct glyph_metrics_s *)lr.content;
  
  /* Get glyph map */
  char * px = ((char *)(lr.content)) + sizeof(struct glyph_metrics_s);
  
  /* Draw glyph on backward buffer */
  x0 = this->_fm->boxwidth * this->_curx + gm->offx;
  y0 = this->_fm->boxheight * this->_cury + gm->offy;
  xmax = x0 + gm->gw;
  ymax = y0 + gm->gh;
  char sw = 0;
  
  for (unsigned int j = y0; j < ymax; ++j)
  {
    for (unsigned int k = x0; k < xmax; ++k) 
    {
      off = buf->stride * j + k * 4;
      if (sw)
      {
        pxal = (uint32_t)(*px & 0x0f);
        px++;
        sw = 0;
        if (pxal == 0) /* pixel opacity 1 */
          *(uint32_t*)&buf->map[off] = 
            (*(uint32_t*)&buf->map[off] & 0xff000000 ) | this->_fc;
        else if (pxal == 15) /* pixel opacity 0 */
          continue;
        else
          *(uint32_t*)&buf->map[off] =  
            _alphacol4h(*(uint32_t*)&buf->map[off], this->_fc, pxal);
      }
      else
      {
        pxal = (uint32_t)((*px & 0xf0) >> 4);
        sw = 1;
        if (pxal == 0) /* pixel opacity 1 */
          *(uint32_t*)&buf->map[off] =
            (*(uint32_t*)&buf->map[off] & 0xff000000 ) | this->_fc;
        else if (pxal == 15) /* pixel opacity 0 */
          continue;
        else
          *(uint32_t*)&buf->map[off] =  
            _alphacol4h(*(uint32_t*)&buf->map[off], this->_fc, pxal);
      }
    }
  }
  
  ts->written = true;

putc_advance_and_sync__advance:
  if (sync_now)
  {
    /* Flip buffers and sync changes. */
    if (CALL(this, sync_term))
      return -1;
    
    /* Update tile written now */
    if (front_buf_tile_fullcopy)
    {
      /* Redraw full tile on backward buffer, now flipped. */
      x0 = this->_fm->boxwidth * this->_curx;
      y0 = this->_fm->boxheight * this->_cury;
      xmax = x0 + this->_fm->boxwidth;
      ymax = y0 + this->_fm->boxheight;
    }
    else
    {
      /* Redraw only tile written pixels on backward buffer, 
       *  now flipped. */
      x0 = this->_fm->boxwidth * this->_curx + gm->offx;
      y0 = this->_fm->boxheight * this->_cury + gm->offy;
      xmax = x0 + gm->gw;
      ymax = y0 + gm->gh;
    }
    
    /* Now redraw last tile on backward buffer, now flipped. */
    for (unsigned int j = y0; j < ymax; ++j)
    {
      for (unsigned int k = x0; k < xmax; ++k) 
      {
        off = buf->stride * j + k * 4;
        
        *(uint32_t*)&fbuf->map[off] = *(uint32_t*)&buf->map[off];
      }
    }
  }

  else
  {
    /* Dont sync. Mark this tile as to update and 
     * set _need_update true */
     
    ts->need_update = true;
    this->_need_update = true;
  }
  
  if (advance)
  {
    tapecur = (this->_cols * this->_cury) + this->_curx + 1;
    if (tapecur >= (this->_rows * this->_cols)) tapecur = 0;
    this->_cury = tapecur / this->_cols;
    this->_curx = tapecur % this->_cols;
  }
  
  return 0;
}


/* Clear current tile. */
int clsm(clear_pos, uint32_t sync_now)
{
  struct modeset_buf * buf, * fbuf, * bbuf;
  bool set_clear_tile = false;
  
  /* Check if monitor is enabled */
  if (!(Parent->_enabled))
  {
    fprintf(stderr, "Cannot draw on disabled monitor!\n");
    return -1;
  }
  
  unsigned int x0;
  unsigned int y0;
  unsigned int xmax;
  unsigned int ymax;
  unsigned int off;
  
  /* Tile status struct */
  struct vt_tile_status_s * ts = 
    this->_sb + this->_cury * this->_cols + this->_curx;
  
  /* Select backward buffer and operate on it. */
  buf = &(Parent->_dev->bufs[Parent->_dev->front_buf ^ 1]);
  
  /* Get front buffer */
  fbuf = &(Parent->_dev->bufs[Parent->_dev->front_buf]);
  
  /* Get background buffer */
  bbuf = &(Parent->_dev->bufs[2]);
  
  /* Clear tile if needed */
  if (ts->written)
  {
    x0 = this->_fm->boxwidth * this->_curx;
    y0 = this->_fm->boxheight * this->_cury;
    xmax = x0 + this->_fm->boxwidth;
    ymax = y0 + this->_fm->boxheight;
    
    for (unsigned int j = y0; j < ymax; ++j)
    {
      for (unsigned int k = x0; k < xmax; ++k) 
      {
        off = buf->stride * j + k * 4;
        
        *(uint32_t*)&buf->map[off] = *(uint32_t*)&bbuf->map[off];
      }
    }
    
    set_clear_tile = true; /* Copy full tile on front buffer */
    ts->written = false;   /* Tile is clear. */
  }
  
  if (sync_now)
  {
    /* Flip buffers and sync changes. */
    if (CALL(this, sync_term))
      return -1;
    
    /* Update tile written now */
    if (set_clear_tile)
    {
      /* Redraw full tile on backward buffer, now flipped. */
      x0 = this->_fm->boxwidth * this->_curx;
      y0 = this->_fm->boxheight * this->_cury;
      xmax = x0 + this->_fm->boxwidth;
      ymax = y0 + this->_fm->boxheight;
    
    
      /* Now redraw last tile on backward buffer, now flipped. */
      for (unsigned int j = y0; j < ymax; ++j)
      {
        for (unsigned int k = x0; k < xmax; ++k) 
        {
          off = buf->stride * j + k * 4;
          
          *(uint32_t*)&fbuf->map[off] = *(uint32_t*)&bbuf->map[off];
        }
      }
    }
  }

  else if (set_clear_tile)
  {
    /* Dont sync. Mark this tile as to update and 
     * set _need_update true */
     
    ts->need_update = true;
    this->_need_update = true;
  }
  
  return 0;
}

/* Clear terminal */
int clsm(clear_term, uint32_t do_sync)
{
  CALL(Parent, clear_scr, do_sync);
  
  struct vt_tile_status_s * vts;
    
  for (unsigned int r = 0; r<this->_rows; r++)
  {
    for (unsigned int c = 0; c<this->_cols; c++)
    {
      vts = this->_sb + r * this->_cols + c;
      vts->need_update = false;
      vts->written = false;
    }
  }
  
  this->_curx = this->_cury = 0;
  this->_need_update = false;
  
  return 0;
}

/* Sync monitor related to this terminal */
int clsm(sync_term)
{
  
  /* Select backward buffer and operate on it. */
  struct modeset_buf * buf = 
    &(Parent->_dev->bufs[Parent->_dev->front_buf ^ 1]);
  
  /* Get front buffer */
  struct modeset_buf * 
    fbuf = &(Parent->_dev->bufs[Parent->_dev->front_buf]);
  
  /* Flip buffers */
  int ret = CALL(Parent, sync_monitor);
  
  if (ret)
    return ret;
  
  /* Update all tiles written before if needed. */
  if (this->_need_update)
  {
    struct vt_tile_status_s * vts;
    
    for (unsigned int r = 0; r<this->_rows; r++)
    {
      for (unsigned int c = 0; c<this->_cols; c++)
      {
        vts = this->_sb + r * this->_cols + c;
        if (vts->need_update == false)
          continue;
          
        unsigned int x0 = this->_fm->boxwidth * c;
        unsigned int y0 = this->_fm->boxheight * r;
        unsigned int xmax = x0 + this->_fm->boxwidth;
        unsigned int ymax = y0 + this->_fm->boxheight;
        unsigned int off;
        
        for (unsigned int j = y0; j < ymax; ++j)
        {
          for (unsigned int k = x0; k < xmax; ++k) 
          {
            off = buf->stride * j + k * 4;
            
            *(uint32_t*)&fbuf->map[off] = *(uint32_t*)&buf->map[off];
          }
        }
      }
    }
    
    this->_need_update = false;
  }
  
  return ret;
}

/* destroy object */
int clsm(destroy)
{
  /* Set monitor _vt to NULL */
  Parent->_vt = NULL;
  
  /* Free tiles status */
  if (this->_sb)
    free(this->_sb);
  
  /* Free symbols table */
  if (this->_syms)
    free(this->_syms);
  
  free(this);
  
  return 0;
}

/* Initialize videoterm object */
int clsm(init, void * init_params)
{
  clsmlnk(destroy);
  clsmlnk(putc_advance_and_sync);
  clsmlnk(clear_pos);
  clsmlnk(sync_term);
  clsmlnk(clear_term);
  clsmlnk(get_nrows);
  clsmlnk(get_ncols);
  clsmlnk(get_curx);
  clsmlnk(get_cury);
  clsmlnk(move_cur);
  clsmlnk(move_cur_rel);
  clsmlnk(move_cur_prop);
  clsmlnk(set_fontcolor);
  
  struct vt_init_par_s * ip = (struct vt_init_par_s *)init_params;
  this->__Parent = ip->par;
  this->_font = (htables *)(ip->font);
  this->_fm = (struct font_metrics_s *)(ip->fmetr);
  this->_fc = 0x00000000; /* Black font color mask 
                              (set alpha bits to 0xff)*/
  
  
  this->_glyphs = CALL(this->_font, lookup, ".glyphs");
  if (!this->_glyphs)
  {
    fprintf(stderr, "Cannot load .glyphs table\n!");
    return 1;
  }
  
  this->_rows = Parent->_dev->bufs[0].height / this->_fm->boxheight;
  this->_cols = Parent->_dev->bufs[0].width / this->_fm->boxwidth;
  
  this->_sb = hcalloc(this->_rows * this->_cols, 
                      sizeof(struct vt_tile_status_s));
    
  
  fprintf(stderr, "New videoterminal for monitor %d\n"
                  "  terminal rows = %d\n"
                  "  terminal cols = %d\n", 
                  Parent->_monID, this->_rows, this->_cols);
  
  return 0;
}

#endif
#undef _CLASS_NAME
/* videoterm object methods ends. */
/**********************************/


/***********************************/
/* Implement monitor class methods */
#ifndef _CLASS_NAME
#define _CLASS_NAME monitor

#define _PARENT_CLASS_monitor drmvideo

thisclass_creator

/* Create videoterminal */
videoterm * clsm(videoterminal, uint32_t fontID)
{
  if (this->_vt)
  {
    fprintf(stderr, "Monitor %d has already a videoterminal!\n",
                    this->_monID);
    return NULL;
  }
  
  if (Parent->_num_of_fonts == 0)
  {
    fprintf(stderr, "There aren't fonts available!\n");
    return NULL;
  }
  
  struct fonts_list * it = Parent->_fonts;
  for ( ; it; it=it->next)
    if (it->fontID == fontID)
      break;
  
  if (!it)
  {
    fprintf(stderr, "Font %d not found!\n", fontID);
    return NULL;
  }
  
  struct vt_init_par_s vt_init_p = {this, it->font, &it->fm};
  
  this->_vt = new(videoterm, &vt_init_p);
  
  return this->_vt;
  
}

/* Sync monitor related to this terminal */
int clsm(sync_monitor)
{
  struct modeset_buf * buf;
  
  /* Check if monitor is enabled */
  if (!this->_enabled)
  {
    fprintf(stderr, "Cannot draw on disabled monitor!\n");
    return -1;
  }
  
  /* Get drm fd*/
  int drm_fd = Parent->_drm_fd;
  
  /* Select backward buffer. */
  buf = &(this->_dev->bufs[this->_dev->front_buf ^ 1]);
  
  /* Flip buffers */
  int ret = drmModeSetCrtc(drm_fd, 
                       this->_dev->crtc, buf->fb, 0, 0,
                       &this->_dev->conn, 1, &this->_dev->mode);
  
  /* Copy background on not refreshed front buffer, now flipped. */
  if (this->_reset_background)
  {
    uint8_t * _s = this->_dev->bufs[2].map;
    uint8_t * _d = this->_dev->bufs[this->_dev->front_buf].map;
    
    unsigned int _len = this->_width * this->_height * 4;
    
    for (unsigned int k=0; k<_len; k++)
      *(_d+k) = *(_s+k);
    
    this->_reset_background = false;
  }
  
  if (ret)
  {
    fprintf(stderr, "cannot flip CRTC for connector %u (%d): %m\n",
            this->_dev->conn, errno);
    return -1;
  }
  else
    this->_dev->front_buf ^= 1;
  
  return 0;
}

int clsm(_redraw_fb)
{
  /* Get drm fd*/
  int drm_fd = Parent->_drm_fd;
  
  /* Select front buffer. */
  struct modeset_buf * buf = &(this->_dev->bufs[this->_dev->front_buf]);
  
  /* call SetCrtc on front buffer */
  int ret = drmModeSetCrtc(drm_fd, 
                       this->_dev->crtc, buf->fb, 0, 0,
                       &this->_dev->conn, 1, &this->_dev->mode);
  if (ret)
  {
    fprintf(stderr, "cannot flip CRTC for connector %u (%d): %m\n",
            this->_dev->conn, errno);
    return -1;
  }
  
  return 0;
}

/*
 * modeset_create_fb() is mostly the same as before. Buf instead of writing the
 * fields of a modeset_dev, we now require a buffer pointer passed as @buf.
 * Please note that buf->width and buf->height are initialized by
 * modeset_setup_dev() so we can use them here.
 */
int clsm(_modeset_create_fbs)
{
	struct drm_mode_create_dumb creq;
	struct drm_mode_destroy_dumb dreq;
	struct drm_mode_map_dumb mreq;
	int ret;
  
  int fd = Parent->_drm_fd;
  
  struct modeset_buf * buf;
  
  for (int ibuf = 0; ibuf<3; ibuf++)
  {
    buf = &(this->_dev->bufs[ibuf]);
    
    /* create dumb buffers */
    memset(&creq, 0, sizeof(creq));
    creq.width = buf->width;
    creq.height = buf->height;
    creq.bpp = 32;
    ret = drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &creq);
    if (ret < 0) {
      fprintf(stderr, "cannot create dumb buffer (%d): %m\n",
        errno);
      return -errno;
    }
    buf->stride = creq.pitch;
    buf->size = creq.size;
    buf->handle = creq.handle;
  
    /* create framebuffer object for the dumb-buffer */
    ret = drmModeAddFB(fd, buf->width, buf->height, 24, 32, buf->stride,
           buf->handle, &buf->fb);
    if (ret) {
      fprintf(stderr, "cannot create framebuffer (%d): %m\n",
        errno);
      ret = -errno;
      goto err_destroy;
    }
  
    /* prepare buffer for memory mapping */
    memset(&mreq, 0, sizeof(mreq));
    mreq.handle = buf->handle;
    ret = drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &mreq);
    if (ret) {
      fprintf(stderr, "cannot map dumb buffer (%d): %m\n",
        errno);
      ret = -errno;
      goto err_fb;
    }
  
    /* perform actual memory mapping */
    buf->map = mmap(0, buf->size, PROT_READ | PROT_WRITE, MAP_SHARED,
              fd, mreq.offset);
    if (buf->map == MAP_FAILED) {
      fprintf(stderr, "cannot mmap dumb buffer (%d): %m\n",
        errno);
      ret = -errno;
      goto err_fb;
    }
  
    /* clear the framebuffer to 0 */
    memset(buf->map, 0, buf->size);
  }

	return 0;

err_fb:
	drmModeRmFB(fd, buf->fb);
err_destroy:
	memset(&dreq, 0, sizeof(dreq));
	dreq.handle = buf->handle;
	drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
	return ret;
}

/*
 * modeset_destroy_fb() is a new function. It does exactly the reverse of
 * modeset_create_fb() and destroys a single framebuffer. The modeset.c example
 * used to do this directly in modeset_cleanup().
 * We simply unmap the buffer, remove the drm-FB and destroy the memory buffer.
 */
int clsm(_modeset_destroy_fbs)
{
	struct drm_mode_destroy_dumb dreq;
  
  int fd = Parent->_drm_fd;

  for (int ibuf = 0; ibuf<3; ibuf++)
  {
    struct modeset_buf * buf = &(this->_dev->bufs[ibuf]);
    /* unmap buffer */
    munmap(buf->map, buf->size);
  
    /* delete framebuffer */
    drmModeRmFB(fd, buf->fb);
  
    /* delete dumb buffer */
    memset(&dreq, 0, sizeof(dreq));
    dreq.handle = buf->handle;
    drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
  }
  
  return 0;
}

/* Initialize monitor */
int clsm(_initialize_monitor, drmModeConnector * conn)
{
	/* check if a monitor is connected */
	if (conn->connection != DRM_MODE_CONNECTED)
  {
		fprintf(stderr, "ignoring unused connector %u\n",
			conn->connector_id);
		return -ENOENT;
	}

	/* check if there is at least one valid mode */
	if (conn->count_modes == 0)
  {
		fprintf(stderr, "no valid mode for connector %u\n",
			conn->connector_id);
		return -EFAULT;
	}
  
  /* Allocate memory for dev */
  this->_dev = hcalloc(1, sizeof(struct modeset_dev));

	/* copy the mode information into our device structure and into the 3
	 * buffers */
	memcpy(&(this->_dev->mode), &conn->modes[0], 
         sizeof(this->_dev->mode));
  this->_width = conn->modes[0].hdisplay;
  this->_height = conn->modes[0].vdisplay;
	this->_dev->bufs[0].width = conn->modes[0].hdisplay;
	this->_dev->bufs[0].height = conn->modes[0].vdisplay;
	this->_dev->bufs[1].width = conn->modes[0].hdisplay;
	this->_dev->bufs[1].height = conn->modes[0].vdisplay;
  this->_dev->bufs[2].width = conn->modes[0].hdisplay;
	this->_dev->bufs[2].height = conn->modes[0].vdisplay;
  this->_dev->conn = conn->connector_id;
  
	fprintf(stderr, "mode for connector %u is %ux%u\n",
		conn->connector_id, 
    this->_dev->bufs[0].width, this->_dev->bufs[0].height);
  
  return 0;
}

/* destroy object */
int clsm(destroy)
{
  
  CALL(Parent, _remove_monitor, this->_monID);
  
  /* restore saved CRTC configuration */
  if (this->_enabled)
  {
    drmModeSetCrtc(Parent->_drm_fd,
                   this->_dev->saved_crtc->crtc_id,
                   this->_dev->saved_crtc->buffer_id,
                   this->_dev->saved_crtc->x,
                   this->_dev->saved_crtc->y,
                   &(this->_dev->conn),
                   1,
                   &(this->_dev->saved_crtc->mode));
    drmModeFreeCrtc(this->_dev->saved_crtc);
  }

  /* destroy videoterm */
  if (this->_vt)
    CALL(this->_vt, destroy);

  /* destroy framebuffers */
  if (this->_ready)
    CALL(this, _modeset_destroy_fbs);
  
  /* Free saved _conn */
  if (this->_conn)
    drmModeFreeConnector(this->_conn);
  
  /* free _dev */
  if (this->_dev)
    free(this->_dev);
  
  /* free this */
  free(this);
  
  return 0;
}

/* Set background image from imagetool object */
int clsm(set_backg_image, void * image)
{
  /* Check if monitor is enabled */
  if (!this->_enabled)
  {
    fprintf(stderr, "Cannot draw on disabled monitor!\n");
    return -1;
  }
  
  fprintf(stderr, "Copyng background %d x %d\n", this->_width, 
                  this->_height);
  
  CALL(((imagetool *)image), 
       resize, 32, this->_width, this->_height, 
       (char *)(this->_dev->bufs[2].map));
  
  return 0;
}

/* Draw rectangle */
int clsm(draw_rectangle, uint32_t x0, uint32_t y0,
                         uint32_t rw, uint32_t rh,
                         uint32_t color)
{
  unsigned int off;
  struct modeset_buf * buf;
  
  int ret;
  
  /* Check if monitor is enabled */
  if (!this->_enabled)
  {
    fprintf(stderr, "Cannot draw on disabled monitor!\n");
    return -1;
  }
  
  /* Select backward buffer and operate on it. */
  buf = &(this->_dev->bufs[this->_dev->front_buf ^ 1]);
  
  /* Check data */
  if ((y0 >= buf->height) || (y0+rh>=buf->height) ||
      (x0 >= buf->width)  || (x0+rw>=buf->width))
  {
    fprintf(stderr, "Wrong boundaries for this monitor!\n");
    return -1;
  }
  
  /* Draw rectangle on backward buffer */
  for (unsigned int j = y0; j < y0+rh; ++j)
  {
    for (unsigned int k = x0; k < x0+rw; ++k) 
    {
      off = buf->stride * j + k * 4;
      *(uint32_t*)&buf->map[off] = color;
    }
  }
  
  /* Flip buffers */
  ret = drmModeSetCrtc(Parent->_drm_fd, 
                       this->_dev->crtc, buf->fb, 0, 0,
                       &this->_dev->conn, 1, &this->_dev->mode);
  if (ret)
  {
    fprintf(stderr, "cannot flip CRTC for connector %u (%d): %m\n",
            this->_dev->conn, errno);
    return -1;
  }
  else
    this->_dev->front_buf ^= 1;
    
  /* Select the new backward buffer and copy data on it. */
  buf = &(this->_dev->bufs[this->_dev->front_buf ^ 1]);
  
  for (unsigned int j = y0; j < y0+rh; ++j)
  {
    for (unsigned int k = x0; k < x0+rw; ++k) 
    {
      off = buf->stride * j + k * 4;
      *(uint32_t*)&buf->map[off] = color;
    }
  }
  
  return 0;
}

/* is monitor ready ? */
int clsm(is_ready)
{
  return (this->_ready) ? 1 : 0;
}

/* is monitor enabled ? */
int clsm(is_enabled)
{
  return (this->_enabled) ? 1 : 0;
}

/* width */
uint32_t clsm(get_monitor_width)
{
  return this->_width;
}

/* height */
uint32_t clsm(get_monitor_height)
{
  return this->_height;
}

int clsm(clear_scr, uint32_t do_sync)
{
  unsigned int off;
  struct modeset_buf * buf, * bbuf;
  
  int ret;
  
  /* Check if monitor is enabled */
  if (!this->_enabled)
  {
    fprintf(stderr, "Cannot draw on disabled monitor!\n");
    return -1;
  }
  
  /* Select backward buffer and operate on it. */
  buf = &(this->_dev->bufs[this->_dev->front_buf ^ 1]);
  
  /* Select background buffer. */
  bbuf = &(this->_dev->bufs[2]);
  
  /* Draw background on backward buffer */
  for (unsigned int j = 0; j < buf->height; ++j)
  {
    for (unsigned int k = 0; k < buf->width; ++k) 
    {
      off = buf->stride * j + k * 4;
      *(uint32_t*)&buf->map[off] = *(uint32_t*)&bbuf->map[off];
    }
  }
  
  if (!do_sync)
  {
    this->_reset_background = true;
    return 0;
  }
  
  /* Flip buffers */
  ret = drmModeSetCrtc(Parent->_drm_fd, 
                       this->_dev->crtc, buf->fb, 0, 0,
                       &this->_dev->conn, 1, &this->_dev->mode);
  if (ret)
  {
    fprintf(stderr, "cannot flip CRTC for connector %u (%d): %m\n",
            this->_dev->conn, errno);
    return -1;
  }
  else
    this->_dev->front_buf ^= 1;
    
  /* Select the new backward buffer and copy data on it. */
  buf = &(this->_dev->bufs[this->_dev->front_buf ^ 1]);
  
  /* Draw background on (now flipped) backward buffer */
  for (unsigned int j = 0; j < buf->height; ++j)
  {
    for (unsigned int k = 0; k < buf->width; ++k) 
    {
      off = buf->stride * j + k * 4;
      *(uint32_t*)&buf->map[off] = *(uint32_t*)&bbuf->map[off];
    }
  }
  
  return 0;
}

/* Initialize object */
int clsm(init, void * init_params)
{
  clsmlnk(_initialize_monitor);
  clsmlnk(_modeset_create_fbs);
  clsmlnk(_modeset_destroy_fbs);
  clsmlnk(_redraw_fb);
  clsmlnk(is_ready);
  clsmlnk(is_enabled);
  clsmlnk(get_monitor_width);
  clsmlnk(get_monitor_height);
  clsmlnk(videoterminal);
  clsmlnk(clear_scr);
  clsmlnk(set_backg_image);
  clsmlnk(draw_rectangle);
  clsmlnk(sync_monitor);
  clsmlnk(destroy);
  
  return 0;
}

#endif
#undef _CLASS_NAME
/* monitor object methods ends. */
/********************************/


/************************************/
/* Implement drmvideo class methods */
#ifndef _CLASS_NAME
#define _CLASS_NAME drmvideo

thisclass_creator

/*
 * modeset_cleanup() stays the same as before. But it now calls
 * modeset_destroy_fb() instead of accessing the framebuffers directly.
 */
int clsm(_modeset_cleanup)
{
	for (int i=0; i<this->_num_of_monitors; i++) 
  {
    monitor * mon = *(this->_monitors+i);
    
    if (! mon)
      continue;
    
    /* Destroy object */
    CALL(mon, destroy);

		/* Set pointer to NULL */
		*(this->_monitors+i) = NULL;
	}
  
  return 0;
}

/* Setup monitor */
int clsm(setup_monitor, uint32_t monID)
{
  int ret;
  
  /* Check if monID is available */
  if (monID >= this->_num_of_monitors)
  {
    fprintf(stderr, "monitor monID is out of range!\n");
    return -1;
  }
  
  monitor * mon = *(this->_monitors + monID);
  if (!mon)
  {
    fprintf(stderr, "monitor monID is not available!\n");
    return -1;
  }
  
  if (mon->_ready)
  {
    fprintf(stderr, "monitor monID is ready!\n");
    return -1;
  }
  
  /* create framebuffers for this CRTC */
	ret = CALL(mon, _modeset_create_fbs);
	if (ret) {
		fprintf(stderr, "cannot create framebuffers for connector %u\n",
			mon->_conn->connector_id);
		return ret;
	}
  
  mon->_ready = true;

	return 0;
}

/* Enable monitor monID */
int clsm(enable_monitor, uint32_t monID)
{
  int ret;
  
  /* Check if monID is available */
  if (monID >= this->_num_of_monitors)
  {
    fprintf(stderr, "monitor monID is out of range!\n");
    return -1;
  }
  
  monitor * mon = *(this->_monitors + monID);
  if (!mon)
  {
    fprintf(stderr, "monitor monID is not available!\n");
    return -1;
  }
  
  if(!mon->_ready)
  {
    fprintf(stderr, "monitor monID is not ready!\n");
    return -1;
  }
  
  if (mon->_enabled)
  {
    fprintf(stderr, "monitor monID is already enabled!\n");
    return -1;
  }
  
  
  mon->_dev->saved_crtc = drmModeGetCrtc(this->_drm_fd, 
                                         mon->_dev->crtc);
                                         
  struct modeset_buf * buf = &(mon->_dev->bufs[mon->_dev->front_buf]);
  ret = drmModeSetCrtc(this->_drm_fd, 
                       mon->_dev->crtc, buf->fb, 0, 0,
                       &(mon->_dev->conn), 
                       1, &(mon->_dev->mode));
  if (ret)
  {
    fprintf(stderr, "cannot set CRTC for connector %u (%d): %m\n",
            mon->_dev->conn, errno);
    return ret;
  }
  
  mon->_enabled = true;
  
  return 0;
}

/* Destroy monitor */
int clsm(_remove_monitor, uint32_t monID)
{
  if (monID >= this->_num_of_monitors)
  {
    fprintf(stderr, "monitor monID is out of range!\n");
    return -1;
  }
  
  /* Set pointer to NULL */
  *(this->_monitors+monID) = NULL;
  
  return 0;
}

/*
 * modeset_find_crtc() stays the same.
 */
int clsm(_modeset_find_crtc, monitor * mon)
{
	drmModeEncoder *enc;
	unsigned int i, j;
	int32_t crtc;
  drmModeConnector * conn = mon->_conn;

	/* first try the currently conected encoder+crtc */
	if (conn->encoder_id)
		enc = drmModeGetEncoder(this->_drm_fd, conn->encoder_id);
	else
		enc = NULL;

	if (enc)
  {
		if (enc->crtc_id)
    {
			crtc = enc->crtc_id;
			for (i=0; i<this->_num_of_monitors; i++)
      {
        monitor * cmon = *(this->_monitors+i);
        if (cmon)
          if ( cmon->_dev->crtc == crtc)
          {
            crtc = -1;
            break;
          }
			}

			if (crtc >= 0) {
				drmModeFreeEncoder(enc);
				mon->_dev->crtc = crtc;
				return 0;
			}
		}

		drmModeFreeEncoder(enc);
	}

	/* If the connector is not currently bound to an encoder or if the
	 * encoder+crtc is already used by another connector (actually unlikely
	 * but lets be safe), iterate all other available encoders to find a
	 * matching CRTC. */
	for (i = 0; i < conn->count_encoders; ++i)
  {
		enc = drmModeGetEncoder(this->_drm_fd, conn->encoders[i]);
		if (!enc)
    {
			fprintf(stderr, "cannot retrieve encoder %u:%u (%d): %m\n",
				i, conn->encoders[i], errno);
			continue;
		}

		/* iterate all global CRTCs */
		for (j = 0; j < this->_res->count_crtcs; ++j) {
			/* check whether this CRTC works with the encoder */
			if (!(enc->possible_crtcs & (1 << j)))
				continue;

			/* check that no other device already uses this CRTC */
			crtc = this->_res->crtcs[j];
      for (int k=0; k<this->_num_of_monitors; k++)
      {
        monitor * cmon = *(this->_monitors+k);
        if (cmon)
          if ( cmon->_dev->crtc == crtc)
          {
            crtc = -1;
            break;
          }
			}
      

			/* we have found a CRTC, so save it and return */
			if (crtc >= 0) {
				drmModeFreeEncoder(enc);
				mon->_dev->crtc = crtc;
				return 0;
			}
		}

		drmModeFreeEncoder(enc);
	}

	fprintf(stderr, "cannot find suitable CRTC for connector %u\n",
		conn->connector_id);
	return -ENOENT;
}

/*
 * modeset_prepare() stays the same.
 */
int clsm(_modeset_prepare)
{
	drmModeRes * res;
	drmModeConnector * conn;
	unsigned int i;

	/* retrieve resources */
	res = drmModeGetResources(this->_drm_fd);
	if (!res) {
		fprintf(stderr, "cannot retrieve DRM resources (%d): %m\n",
			errno);
		return -errno;
	}
  
  /* allocate memory for monitors objects array */
  this->_monitors = hcalloc(res->count_connectors, sizeof(monitor *));
  
  /* store _num_of_monitors */
  this->_num_of_monitors = res->count_connectors;
  
	/* iterate all connectors */
	for (i = 0; i < res->count_connectors; ++i)
  {
		/* get information for each connector */
		conn = drmModeGetConnector(this->_drm_fd, res->connectors[i]);
		if (!conn)
    {
			fprintf(stderr, "cannot retrieve DRM connector %u:%u (%d): %m\n",
				i, res->connectors[i], errno);
			continue;
		}
    
    monitor * mon = new(monitor, (void *)conn);
    if (CALL(mon, _initialize_monitor, conn))
      continue;
    
    /* Store monitor ID */
    mon->_monID = i;
    /* Save conn for future operation and do not free it */
    mon->_conn = conn;
    /* Link __Parent */
    mon->__Parent = (void *)this;
    /* Store object in array */
    *(this->_monitors+i) = mon;
    
    if (CALL(this, _modeset_find_crtc, mon))
    {
      *(this->_monitors+i) = NULL;
      continue;
    }
    
		/* free connector data */
		//drmModeFreeConnector(conn);
	}
  
  /* Save res for future operation and do not free it */
  this->_res = res;
	/* free resources again */
	//drmModeFreeResources(res);
	return 0;
}

/*
 * modeset_open() stays the same as before.
 */
int clsm(_modeset_open)
{
	int fd, ret;
	uint64_t has_dumb;

	fd = open(this->_card, O_RDWR | O_CLOEXEC);
	if (fd < 0) {
		ret = -errno;
		fprintf(stderr, "cannot open '%s': %m\n", this->_card);
		return ret;
	}

	if (drmGetCap(fd, DRM_CAP_DUMB_BUFFER, &has_dumb) < 0 ||
	    !has_dumb) {
		fprintf(stderr, "drm device '%s' does not support dumb buffers\n",
			this->_card);
		close(fd);
		return -EOPNOTSUPP;
	}

	this->_drm_fd = fd;
  
	return 0;
}

/* get monitor object by ID */
monitor * clsm(get_monitor_by_ID, uint32_t monID)
{
  if (monID >= this->_num_of_monitors)
  {
    fprintf(stderr, "monitor ID out of range!\n");
    return NULL;
  }
  
  monitor * mon = *(this->_monitors + monID);
  
  return mon;
}

/* destroy object */
int clsm(destroy)
{
  /* Cleanup monitors */
  CALL(this, _modeset_cleanup);
  
  /* Free resource data */
  if (this->_res)
    drmModeFreeResources(this->_res);
  
  /* Free _monitors */
  if (this->_monitors)
    free (this->_monitors);
  
  /* Free _fonts */
  struct fonts_list * to_free;
  struct fonts_list * node = this->_fonts;
  for ( ; node; )
  {
    to_free = node;
    node = node->next;
    
    /* Need to Destroy fonts */
    
    /* free node */
    free(to_free);
  }
  
  /* Close _drm_fd*/
  close(this->_drm_fd);
  
  /* Free this */
  free(this);
  
  return 0;
}

/* Load font from file */
int clsm(load_font_from_file, char * font_file_name)
{
  htables * f = new(htables, NULL);
  
  if (!f)
  {
    fprintf(stderr, "Error while creating new font object!\n");
    return 1;
  }
  
  if (CALL(f, loadtables, font_file_name))
  {
    fprintf(stderr, "Cannot load font from %s!\n", font_file_name);
    return 1;
  }
  
  hashtable * metrics;
  if (!(metrics = CALL(f, lookup, ".metrics")))
  {
    fprintf(stderr, "Cannot find metrics table!\n");
    return 1;
  }
  
  /* Load metrics data */
  char * metrics_data_key[] = {
      "boxwidth", "boxheight", "ascent", "descent" 
    };
  uint32_t metrics_data[4];
  struct lookup_res lr;
  for (unsigned int i=0; i<4; i++)
  {
    if (CALL(metrics, lookup, 
             strlen(metrics_data_key[i]), metrics_data_key[i],
             NULL, &lr)
       )
    {
      fprintf(stderr, "Cannot find metrics key %s!\n",
              metrics_data_key[i]);
      return 1;
    }
    metrics_data[i] = *((uint32_t *)(lr.content));
  }
  
  
  /* Create list node, fill it and append to list */
  struct fonts_list * fl = hcalloc(1, sizeof(struct fonts_list));
  fl->fontID = this->_num_of_fonts;
  fl->font = f;
  
  uint32_t * p = (uint32_t *)&(fl->fm);
  for (unsigned int i=0; i<4; *(p+i) = metrics_data[i], i++);
  
  struct fonts_list ** iter = &this->_fonts;
  for ( ; *iter; iter=&(*iter)->next);
  *iter = fl;
  
  this->_num_of_fonts++;
  
  fprintf(stderr, "Font %s loaded.\n"
                  "  Box width = %d\n"
                  "  Box height = %d\n"
                  "  Ascent = %d\n"
                  "  Descent = %d\n", font_file_name,
                  fl->fm.boxwidth, fl->fm.boxheight, 
                  fl->fm.ascent, fl->fm.descent);
  
  return 0;
}

/* Get num of monitors*/
uint32_t clsm(get_num_of_monitors)
{
  return this->_num_of_monitors;
}

/* Set/Drop Master Mode */
int clsm(set_or_drop_master_mode, uint32_t setdrop)
{
  if (setdrop)
    /* Set Master Mode */
    return drmSetMaster(this->_drm_fd);
  else
    /* Drop Master Mode */
    return drmDropMaster(this->_drm_fd);
}

/* Redraw all active monitors */
int clsm(redraw)
{
  for (int i=0; i<this->_num_of_monitors; i++)
    if (*(this->_monitors+i))
      if ((*(this->_monitors+i))->_enabled)
        CALL((*(this->_monitors+i)), _redraw_fb);
  
  return 0;
}

/* Set backgrounds on all monitors. */
int clsm(set_backgrounds)
{
  if (!this->_im)
    return 1;
  
  bool _resize_img = true;
  
  /* Set background on first monitor */
  if (*(this->_monitors))
    if ((*(this->_monitors))->_enabled)
      CALL((*(this->_monitors)), set_backg_image, this->_im);
  
  /* Set background on all others monitors */
  for (int i=1; i<this->_num_of_monitors; i++)
    if (*(this->_monitors+i))
      if ((*(this->_monitors+i))->_enabled)
      {
        /* Check if there is another monitor having same sizes 
         *  and avoid resizing image again. */
        for (int j=0; j<this->_num_of_monitors; j++)
        {
          if (*(this->_monitors+j))
            if ((*(this->_monitors+j))->_enabled)
              if ( ( (*(this->_monitors+j))->_width  == 
                     (*(this->_monitors+i))->_width ) &&
                   ( (*(this->_monitors+j))->_height == 
                     (*(this->_monitors+i))->_height ) )
              {
                unsigned int _len = 
                  (*(this->_monitors+j))->_width *
                  (*(this->_monitors+j))->_height * 4;
                
                uint8_t * _d = 
                  (*(this->_monitors+i))->_dev->bufs[2].map;
                
                uint8_t * _s = 
                  (*(this->_monitors+j))->_dev->bufs[2].map;
                
                for (unsigned int k=0; k<_len; k++)
                  *(_d+k) = *(_s+k);
                
                _resize_img = false;
                
                break;
              }
        }
        
        if (_resize_img)
          CALL((*(this->_monitors+i)), set_backg_image, this->_im);
        else
          _resize_img = true;
      }
      
  /* Now destroy imagetool object */
  CALL(this->_im, destroy);
  this->_im = NULL;
  
  return 0;
}

/* Move cursors on all available terminals */
int clsm(move_cur, int32_t x, int32_t y)
{
  for (unsigned int i=0; i<this->_num_of_monitors; i++)
    if (*(this->_monitors+i))
      if (((*(this->_monitors+i))->_enabled) && 
          ((*(this->_monitors+i))->_vt))
        CALL(((*(this->_monitors+i))->_vt), move_cur, x, y);
  
  return 0;
}

/* Move cursors (relative motion) on all available terminals */
int clsm(move_cur_rel, int32_t x, int32_t y)
{
  for (unsigned int i=0; i<this->_num_of_monitors; i++)
    if (*(this->_monitors+i))
      if (((*(this->_monitors+i))->_enabled) && 
          ((*(this->_monitors+i))->_vt))
        CALL(((*(this->_monitors+i))->_vt), move_cur_rel, x, y);
  
  return 0;
}

/* Move cursors (prop pos) on all available terminals */
int clsm(move_cur_prop, int32_t x, int32_t y)
{
  for (unsigned int i=0; i<this->_num_of_monitors; i++)
    if (*(this->_monitors+i))
      if (((*(this->_monitors+i))->_enabled) && 
          ((*(this->_monitors+i))->_vt))
        CALL(((*(this->_monitors+i))->_vt), move_cur_prop, x, y);
  
  return 0;
}

/* Set font color on all available terminals */
int clsm(set_vts_fontcolor, uint32_t fcolor)
{
  for (unsigned int i=0; i<this->_num_of_monitors; i++)
    if (*(this->_monitors+i))
      if (((*(this->_monitors+i))->_enabled) && 
          ((*(this->_monitors+i))->_vt))
        CALL(((*(this->_monitors+i))->_vt), 
             set_fontcolor, fcolor);
  
  return 0;
}

/* Put char on all available terminals */
int clsm(vputc, uint32_t do_sync, uint32_t advance, 
                uint32_t csize, char * charkey)
{
  for (unsigned int i=0; i<this->_num_of_monitors; i++)
    if (*(this->_monitors+i))
      if (((*(this->_monitors+i))->_enabled) && 
          ((*(this->_monitors+i))->_vt))
        CALL(((*(this->_monitors+i))->_vt), putc_advance_and_sync, 
             do_sync, advance, csize, charkey);
  
  return 0;
}

/* Clear tile on all available terminals */
int clsm(clear_pos, uint32_t do_sync)
{
  for (unsigned int i=0; i<this->_num_of_monitors; i++)
    if (*(this->_monitors+i))
      if (((*(this->_monitors+i))->_enabled) && 
          ((*(this->_monitors+i))->_vt))
        CALL(((*(this->_monitors+i))->_vt), clear_pos, do_sync);
  
  return 0;
}

int clsm(clear_terms, uint32_t do_sync)
{
  for (unsigned int i=0; i<this->_num_of_monitors; i++)
    if (*(this->_monitors+i))
      if (((*(this->_monitors+i))->_enabled) && 
          ((*(this->_monitors+i))->_vt))
        CALL(((*(this->_monitors+i))->_vt), clear_term, do_sync);
  
  return 0;
}

/* Sync all available terminals */
int clsm(sync_terms)
{
  for (unsigned int i=0; i<this->_num_of_monitors; i++)
    if (*(this->_monitors+i))
      if (((*(this->_monitors+i))->_enabled) && 
          ((*(this->_monitors+i))->_vt))
        CALL(((*(this->_monitors+i))->_vt), sync_term);
  
  return 0;
}

/* Setup all monitors */
int clsm(setup_all_monitors)
{
  int ret;
  
  for (unsigned int i=0; i<1; i++)
  {
    ret = CALL(this, setup_monitor, i);
    if (ret) return ret;
  }
  
  return 0;
}

/* Enable all monitors */
int clsm(enable_all_monitors)
{
  int ret;
  
  for (unsigned int i=0; i<1; i++)
  {
    ret = CALL(this, enable_monitor, i);
    if (ret) return ret;
  }
  
  return 0;
}

/* Activate all video terminals */
int clsm(activate_vts, uint32_t fontID)
{
  for (unsigned int i=0; i<this->_num_of_monitors; i++)
    if (*(this->_monitors+i))
      if (((*(this->_monitors+i))->_enabled) && 
          (!((*(this->_monitors+i))->_vt)))
        CALL((*(this->_monitors+i)), videoterminal, fontID);
  
  return 0;
}

/* Load image from file */
int clsm(load_image_from_file, char * image_file_name)
{
  this->_im = new(imagetool, image_file_name);
  
  if (!this->_im)
    return 1;
    
  return 0;
}

/* initialize object */
int clsm(init, void * init_params)
{
  clsmlnk(_modeset_open);
  clsmlnk(_modeset_prepare);
  clsmlnk(_modeset_find_crtc);
  clsmlnk(_modeset_cleanup);
  clsmlnk(_remove_monitor);
  clsmlnk(set_or_drop_master_mode);
  clsmlnk(get_num_of_monitors);
  clsmlnk(setup_monitor);
  clsmlnk(enable_monitor);
  clsmlnk(enable_all_monitors);
  clsmlnk(setup_all_monitors);
  clsmlnk(activate_vts);
  clsmlnk(set_vts_fontcolor);
  clsmlnk(get_monitor_by_ID);
  clsmlnk(redraw);
  clsmlnk(set_backgrounds);
  clsmlnk(move_cur);
  clsmlnk(move_cur_rel);
  clsmlnk(move_cur_prop);
  clsmlnk(sync_terms);
  clsmlnk(vputc);
  clsmlnk(clear_pos);
  clsmlnk(clear_terms);
  clsmlnk(load_font_from_file);
  clsmlnk(load_image_from_file);
  clsmlnk(destroy);
  
  this->_card = (char *)init_params;
  
  fprintf(stderr, "using card '%s'\n", this->_card);
  
  int ret = CALL(this, _modeset_open);
  if (ret)
		goto out_return;
    
  ret = CALL(this, _modeset_prepare);
	if (ret)
		goto out_close;
  
out_close:
  if (ret)
    close(this->_drm_fd);

out_return:
	if (ret) {
		errno = -ret;
		fprintf(stderr, "modeset failed with error %d: %m\n", errno);
  }
	
  return ret;
}

#endif
#undef _CLASS_NAME
/* drmvideo object methods ends. */
/*******************************/
