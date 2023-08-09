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

struct font_metrics_s
{
  uint32_t boxwidth;
  uint32_t boxheight;
  uint32_t ascent;
  uint32_t descent;
};

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

struct vt_init_par_s
{
  void * par;
  void * font;
  void * fmetr;
};

struct glyph_metrics_s 
{
  uint32_t offx;
  uint32_t offy;
  uint32_t gw;
  uint32_t gh;
};

struct vt_tile_status_s
{
  bool written;
  bool need_update;
};

struct vt_sym_status_s
{
  uint8_t sym[5];
};

#define  F_RGB24   0x00000018
#define  F_RGBA32  0x00000020

#define _rch4(px) ((px & 0x00ff0000) >> 16)
#define _gch4(px) ((px & 0x0000ff00) >>  8)
#define _bch4(px) (px & 0x000000ff)

#define _alphacol4h(b, f, a) \
  ( ( b & 0xff000000 ) | \
    ( ( ( (_rch4(b) * a + _rch4(f) * (15-a)) >> 4 ) << 16 ) | \
      ( ( (_gch4(b) * a + _gch4(f) * (15-a)) >> 4 ) <<  8 ) | \
        ( (_bch4(b) * a + _bch4(f) * (15-a)) >> 4 )  )  )
        
/* Image manipulation class */
typedef struct 
{
  int (*init)(void *, void *);
  int (* load_from_file)(void *, char * filename);
  int (* resize)(void *, uint32_t dmode,
                         uint32_t new_w,
                         uint32_t new_h,
                         char * dmap);
  
  int (* destroy)(void *);
  
  uint32_t _immode;
  uint32_t _w;
  uint32_t _h;
  void * _imbuff;
  char * _immap;
  char * _imraw;
  size_t _immapsz;
  
} imagetool;

newclass_h(imagetool)

#endif


/* Video terminal class */
typedef struct 
{
  int (*init)(void *, void *);
  
  int (* putc_advance_and_sync)(void *, 
                                uint32_t do_sync, uint32_t advance, 
                                uint32_t csize, char * charkey);
  int (* move_cur)(void *, int32_t x, int32_t y);
  int (* move_cur_rel)(void *, int32_t rx, int32_t ry);
  int (* move_cur_prop)(void *, int32_t px, int32_t py);
  int (* clear_pos)(void *, uint32_t do_sync);
  int (* clear_term)(void *);
  uint32_t (* get_curx)(void *);
  uint32_t (* get_cury)(void *);
  uint32_t (* get_nrows)(void *);
  uint32_t (* get_ncols)(void *);
  int (* set_fontcolor)(void *, uint32_t fcolor);
  int (* sync_term)(void *);
  int (* destroy)(void *);

  #ifdef _DRM_DOUBLEBUFF_H_PRIVATE
  
  void * __Parent;
  
  uint32_t _rows;
  uint32_t _cols;
  uint32_t _curx;
  uint32_t _cury;
  
  uint32_t _fc;                /* font color (default: black) */
  struct font_metrics_s * _fm; /* font metrics. */
  hashtable * _glyphs;
  htables * _font;             /* Inherited by drmvideo. */
  
  bool _need_update;
  struct vt_tile_status_s * _sb;
  bool _vt_sym_buffer_enabled;
  struct vt_sym_status_s * _syms;
  
  #endif
  
} videoterm;

newclass_h(videoterm)
/* */

/* CRTC Monitor class */
typedef struct 
{
  int (*init)(void *, void *);
  
  int (* is_ready)(void *);
  int (* is_enabled)(void *);
  uint32_t (* get_monitor_width)(void *);
  uint32_t (* get_monitor_height)(void *);
  
  int (* clear_scr)(void *);
  int (* draw_rectangle)(void *, 
                         uint32_t x0, uint32_t y0,
                         uint32_t rw, uint32_t rh,
                         uint32_t color);
  int (* set_backg_image)(void *, void * image);
  
  int (* sync_monitor)(void *);
  
  videoterm * (* videoterminal)(void *, uint32_t fontID);
  int (*destroy)(void *);

  #ifdef _DRM_DOUBLEBUFF_H_PRIVATE
  
  void * __Parent;
  
  bool _enabled;
  bool _ready;
  uint32_t _monID;
  uint32_t _width;
  uint32_t _height;
  uint32_t _stride;
  struct modeset_dev * _dev;
  videoterm * _vt;
  
  drmModeConnector * _conn;
  
  int (* _initialize_monitor)(void *, drmModeConnector * conn);
  int (* _modeset_create_fbs)(void *);
  int (* _modeset_destroy_fbs)(void *);
  int (* _redraw_fb)(void *);
  
  #endif
  
} monitor;

newclass_h(monitor)
/* */

#ifdef _DRM_DOUBLEBUFF_H_PRIVATE
struct fonts_list
{
  struct fonts_list * next;
  
  uint32_t fontID;
  
  struct font_metrics_s fm;
  htables * font;
}; 
#endif

typedef struct 
{
  int (*init)(void *, void *);
  
  uint32_t (* get_num_of_monitors)(void *);
  monitor * (* get_monitor_by_ID)(void *, uint32_t monID);
  int (* setup_monitor)(void *, uint32_t monID);
  int (* enable_monitor)(void *, uint32_t monID);
  int (* setup_all_monitors)(void *);
  int (* enable_all_monitors)(void *);
  
  int (* set_or_drop_master_mode)(void *, uint32_t setdrop);
  
  int (* load_font_from_file)(void *, char * font_file_name);
  int (* load_image_from_file)(void *, char * image_file_name);
  int (* get_num_of_loaded_fonts)(void *);
  int (* get_font_info)(void *, uint32_t fontID, struct font_metrics_s *);
  int (* redraw)(void *);
  
  int (* set_backgrounds)(void *);
  
  int (* activate_vts)(void *, uint32_t fontID);
  int (* set_vts_fontcolor)(void *, uint32_t fcolor);
  
  int (* vputc)(void *, uint32_t do_sync, uint32_t advance, 
                        uint32_t csize, char * charkey);
  int (* clear_terms)(void *);
  int (* clear_pos)(void *, uint32_t do_sync);
  int (* move_cur)(void *, int32_t x, int32_t y);
  int (* move_cur_rel)(void *, int32_t x, int32_t y);
  int (* move_cur_prop)(void *, int32_t x, int32_t y);
  int (* sync_terms)(void *);
  
  
  int (* destroy)(void *);
  
  
  #ifdef _DRM_DOUBLEBUFF_H_PRIVATE
  
  char * _card;
  int _drm_fd;
  uint32_t _num_of_monitors;
  monitor ** _monitors;
  
  drmModeRes * _res;
  
  uint32_t _num_of_fonts;
  struct fonts_list * _fonts;
  
  imagetool * _im;
  
  int (* _modeset_open)(void *);
  int (* _modeset_prepare)(void *);
  int (* _modeset_find_crtc)(void *, monitor * mon);
  int (* _modeset_cleanup)(void *);
  
  int (* _remove_monitor)(void *, uint32_t monID);
  
  #endif
  
} drmvideo;

newclass_h(drmvideo)
/* */

#endif
