CFLAGS+=-pedantic -Wall -I /usr/include/libdrm
LDFLAGS+=-ldrm

all: drm-splash4slack

drm-splash4slack: drm-splash4slack.c
	 $(CC) $(CFLAGS) -o $@ $? $(LDFLAGS)
