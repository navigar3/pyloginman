#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "cobj.h"

#define _DRM_DOUBLEBUFF_H_PRIVATE

#include "drm-doublebuff.h"


/* Helper for python binding */
drmvideo * new_drmvideo(char * videocard)
{
  return new(drmvideo, videocard);
}


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

int clsm(move_cur, uint32_t x, uint32_t y)
{
  if ((x>=this->_cols) || (y>=this->_rows))
    return -1;
  
  this->_curx = x;
  this->_cury = y;
  
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
         uint32_t sync_now, uint32_t ks, char * key)
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

  tapecur = (this->_cols * this->_cury) + this->_curx + 1;
  if (tapecur >= (this->_rows * this->_cols)) tapecur = 0;
  this->_cury = tapecur / this->_cols;
  this->_curx = tapecur % this->_cols;

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
  clsmlnk(sync_term);
  clsmlnk(get_nrows);
  clsmlnk(get_ncols);
  clsmlnk(get_curx);
  clsmlnk(get_cury);
  clsmlnk(move_cur);
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
  
  return new(videoterm, &vt_init_p);
  
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
  
  /* Select backward buffer and operate on it. */
  buf = &(this->_dev->bufs[this->_dev->front_buf ^ 1]);
  
  /* Flip buffers */
  int ret = drmModeSetCrtc(drm_fd, 
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
  clsmlnk(get_monitor_by_ID);
  clsmlnk(redraw);
  clsmlnk(load_font_from_file);
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
