#ifndef _DRM_SPLASH4SLACK_H

#define _DRM_SPLASH4SLACK_H

#include <stdbool.h>
#include <stdint.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

int drm_init(char *);
void modeset_draw(int);
void modeset_draw_once(int);
void modeset_cleanup(int);

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
	struct modeset_dev *next;

	unsigned int front_buf;
	struct modeset_buf bufs[2];

	drmModeModeInfo mode;
	uint32_t conn;
	uint32_t crtc;
	drmModeCrtc *saved_crtc;
};


#ifndef __DRM_SPLASH4SLACK_GLOBALS

extern struct modeset_dev *modeset_list;
extern int drm_fd;

#endif


#endif
