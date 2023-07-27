#ifndef _DRM_DOUBLEBUFF_H

#define _DRM_DOUBLEBUFF_H

#include <stdint.h>
#include <stdbool.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "cobj.h"

#include "tools/hashtable.h"


typedef struct
{
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
} color_t;


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
  
  htables * _font;
  
  #endif
  
} videoterm;

newclass_h(videoterm)
/* */

/* CRTC Monitor class */
typedef struct 
{
  int (*init)(void *, void *);
  
  int (* draw_rectangle)(void *, 
                         uint32_t x0, uint32_t y0,
                         uint32_t rw, uint32_t rh,
                         uint32_t color);
  videoterm * (* videoterminal)(void *, uint32_t fontID);
  int (*destroy)(void *);

  #ifdef _DRM_DOUBLEBUFF_H_PRIVATE
  
  void * __Parent;
  
  bool _enabled;
  bool _ready;
  uint32_t _monID;
  struct modeset_dev * _dev;
  videoterm * _vt;
  
  drmModeConnector * _conn;
  
  int (* _initialize_monitor)(void *, drmModeConnector * conn);
  int (* _modeset_create_fbs)(void *);
  int (* _modeset_destroy_fbs)(void *);
  
  #endif
  
} monitor;

newclass_h(monitor)
/* */

/* drmvideo class */
struct monitors_list
{
  struct monitors_list * next;
  
  uint32_t monID;
  uint32_t ready;
  uint32_t enabled;
  
  uint32_t width;
	uint32_t height;
	uint32_t stride;
	uint32_t size;
};

#ifdef _DRM_DOUBLEBUFF_H_PRIVATE
struct fonts_list
{
  struct fonts_list * next;
  
  uint32_t fontID;
  
  uint32_t boxwidth;
  uint32_t boxheight;
  uint32_t ascent;
  uint32_t descent;
  htables * font;
}; 
#endif

struct font_info
{
  uint32_t boxwidth;
  uint32_t boxheight;
  uint32_t ascent;
  uint32_t descent;
};

typedef struct 
{
  int (*init)(void *, void *);
  
  int (* get_drm_monitors_info)(void *, struct monitors_list ** list);
  monitor * (* get_monitor_by_ID)(void *, uint32_t monID);
  int (* setup_monitor)(void *, uint32_t monID);
  int (* enable_monitor)(void *, uint32_t monID);
  
  int (* load_font_from_file)(void *, char * font_file_name);
  int (* get_num_of_loaded_fonts)(void *);
  int (* get_font_info)(void *, uint32_t fontID, struct font_info *);
  
  int (* destroy)(void *);
  
  
  #ifdef _DRM_DOUBLEBUFF_H_PRIVATE
  
  char * _card;
  int _drm_fd;
  uint32_t _num_of_monitors;
  monitor ** _monitors;
  
  drmModeRes * _res;
  
  uint32_t _num_of_fonts;
  struct fonts_list * _fonts;
  
  int (* _modeset_open)(void *);
  int (* _modeset_prepare)(void *);
  int (* _modeset_find_crtc)(void *, monitor * mon);
  int (* _modeset_cleanup)(void *);
  
  #endif
  
} drmvideo;

newclass_h(drmvideo)
/* */

#endif
