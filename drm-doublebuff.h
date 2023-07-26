#ifndef _DRM_DOUBLEBUFF_H

#define _DRM_DOUBLEBUFF_H

#include <stdint.h>
#include <stdbool.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "cobj.h"


#ifdef _DRM_DOUBLEBUFF_H_PRIVATE

void * hcalloc(size_t n, size_t s);
void * hmalloc(size_t s);

struct modeset_buf {
	uint32_t width;
	uint32_t height;
	uint32_t stride;
	uint32_t size;
	uint32_t handle;
	uint8_t *map;
	uint32_t fb;
};

struct modeset_dev {
	unsigned int front_buf;
	struct modeset_buf bufs[3];

	drmModeModeInfo mode;
	uint32_t conn;
	uint32_t crtc;
	drmModeCrtc * saved_crtc;
};

#endif


/* Video terminal class */
typedef struct 
{
  int (*init)(void *, void *);

  #ifdef _DRM_DOUBLEBUFF_H_PRIVATE
  
  uint32_t _rows;
  uint32_t _cols;
  uint32_t _curx;
  uint32_t _cury;
  
  #endif
  
} videoterm;

newclass_h(videoterm)
/* */

/* CRTC Monitor class */
typedef struct 
{
  int (*init)(void *, void *);

  #ifdef _DRM_DOUBLEBUFF_H_PRIVATE
  
  bool _active;
  bool _usable;
  uint32_t _monID;
  struct modeset_dev * _dev;
  videoterm * _vt;
  
  drmModeConnector * _conn;
  
  int (*_initialize_monitor)(void *, drmModeConnector * conn);
  
  #endif
  
} monitor;

newclass_h(monitor)
/* */

/* drmvideo class */
struct monitors_list
{
  struct monitors_list * next;
  
  uint32_t monID;
  bool     active;
  
  uint32_t width;
	uint32_t height;
	uint32_t stride;
	uint32_t size;
};

typedef struct 
{
  int (*init)(void *, void *);
  
  int (* get_drm_monitors_info)(void *, struct monitors_list ** list);
  int (* modeset_prepare_monitor)(void *, uint32_t monID);
  int (* enable_monitor)(void *, uint32_t monID);
  
  
  #ifdef _DRM_DOUBLEBUFF_H_PRIVATE
  
  char * _card;
  int _drm_fd;
  uint32_t _num_of_monitors;
  monitor ** _monitors;
  
  drmModeRes * _res;
  
  int (* _modeset_open)(void *);
  int (* _modeset_prepare)(void *);
  int (* _modeset_find_crtc)(void *, monitor * mon);
  int (* _modeset_create_fbs)(void *, monitor * mon);
  int (* _modeset_destroy_fbs)(void *, monitor * mon);
  
  #endif
  
} drmvideo;

newclass_h(drmvideo)
/* */

#endif
