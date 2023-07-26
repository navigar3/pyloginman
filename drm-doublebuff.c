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


/***********************************/
/* Implement monitor class methods */
#ifndef _CLASS_NAME
#define _CLASS_NAME monitor

thisclass_creator

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

	/* copy the mode information into our device structure and into both
	 * buffers */
	memcpy(&(this->_dev->mode), &conn->modes[0], 
         sizeof(this->_dev->mode));
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

int clsm(init, void * init_params)
{
  clsmlnk(_initialize_monitor);
  
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
 * modeset_create_fb() is mostly the same as before. Buf instead of writing the
 * fields of a modeset_dev, we now require a buffer pointer passed as @buf.
 * Please note that buf->width and buf->height are initialized by
 * modeset_setup_dev() so we can use them here.
 */
int clsm(_modeset_create_fbs, monitor * mon)
{
	struct drm_mode_create_dumb creq;
	struct drm_mode_destroy_dumb dreq;
	struct drm_mode_map_dumb mreq;
	int ret;
  
  int fd = this->_drm_fd;
  
  struct modeset_buf * buf;
  
  for (int ibuf = 0; ibuf<3; ibuf++)
  {
    buf = &(mon->_dev->bufs[ibuf]);
    
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
int clsm(_modeset_destroy_fbs, monitor * mon)
{
	struct drm_mode_destroy_dumb dreq;
  
  int fd = this->_drm_fd;

  for (int ibuf = 0; ibuf<3; ibuf++)
  {
    struct modeset_buf * buf = &(mon->_dev->bufs[ibuf]);
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

int clsm(modeset_prepare_monitor, uint32_t monID)
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
  
  /* create framebuffers for this CRTC */
	ret = CALL(this, _modeset_create_fbs, mon);
	if (ret) {
		fprintf(stderr, "cannot create framebuffers for connector %u\n",
			mon->_conn->connector_id);
		return ret;
	}

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
  
  
  mon->_dev->saved_crtc = drmModeGetCrtc(this->_drm_fd, 
                                         mon->_dev->crtc);
  struct modeset_buf * buf = &(mon->_dev->bufs[mon->_dev->front_buf]);
  ret = drmModeSetCrtc(this->_drm_fd, mon->_dev->crtc, buf->fb, 0, 0,
				     &(mon->_dev->conn), 1, &(mon->_dev->mode));
  if (ret)
    fprintf(stderr, "cannot set CRTC for connector %u (%d): %m\n",
            mon->_dev->conn, errno);
  
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

/* initialize object */
int clsm(init, void * init_params)
{
  clsmlnk(_modeset_open);
  clsmlnk(_modeset_prepare);
  clsmlnk(_modeset_find_crtc);
  clsmlnk(_modeset_create_fbs);
  clsmlnk(_modeset_destroy_fbs);
  clsmlnk(modeset_prepare_monitor);
  clsmlnk(enable_monitor);
  
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
	} else {
		fprintf(stderr, "exiting\n");
	}
	
  return ret;
}

#endif
#undef _CLASS_NAME
/* drmvideo object methods ends. */
/*******************************/
