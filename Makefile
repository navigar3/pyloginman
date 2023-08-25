CFLAGS+=-pedantic -fPIC -Wall -Wno-format -I. -I/usr/include/libdrm
LDFLAGS+=

.PHONY: clean

all: modules/drmhandler/drm_handler.so modules/lmutils/utils.so modules/lmlog/log.so modules/lmutils/pamsession.so

tools/hashtable_dyn.o:
	$(MAKE) -C tools hashtable_dyn.o

tools/zhelper_dyn.o:
	$(MAKE) -C tools zhelper_dyn.o

drm-doublebuff.o: drm-doublebuff.c
	$(CC) -c $(CFLAGS) -o $@ $?

log.o: log.c
	$(CC) -c $(CFLAGS) -o $@ $?

utils.o: utils.c
	$(CC) -c $(CFLAGS) -o $@ $?

pamsession.o: pamsession.c
	$(CC) -c $(CFLAGS) -DHAVE_SECURITY_PAM_MISC_H -o $@ $?

modules/lmlog/log.so: log.o
	$(CC) -shared -o $@ $^ $(LDFLAGS)

modules/drmhandler/drm_handler.so: drm-doublebuff.o tools/hashtable_dyn.o tools/zhelper_dyn.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -ldrm -lz

modules/lmutils/utils.so: utils.o
	$(CC) -shared -o $@ $^ $(LDFLAGS)

modules/lmutils/pamsession.so: pamsession.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -lpam -lpam_misc

clean:
	find . -name '*.o' -exec rm {} +
	find . -name '*.so' -exec rm {} +
	find . -name '__pycache__' -type d -exec rm -r {} +
