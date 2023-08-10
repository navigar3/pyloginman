CFLAGS+=-pedantic -fPIC -Wall -Wno-format -I. -I/usr/include/libdrm
LDFLAGS+=

.PHONY: clean

all: modules/drmhandler/drm_handler.so modules/lmtermutils/term_utils.so

tools/hashtable_dyn.o:
	$(MAKE) -C tools hashtable_dyn.o

tools/zhelper_dyn.o:
	$(MAKE) -C tools zhelper_dyn.o

drm-doublebuff.o: drm-doublebuff.c
	$(CC) -c $(CFLAGS) -o $@ $?

term_utils.o: term_utils.c
	$(CC) -c $(CFLAGS) -o $@ $?
	
modules/drmhandler/drm_handler.so: drm-doublebuff.o tools/hashtable_dyn.o tools/zhelper_dyn.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -ldrm -lz

modules/lmtermutils/term_utils.so: term_utils.o
	$(CC) -shared -o $@ $^ $(LDFLAGS)

clean:
	find . -name '*.o' -exec rm {} +
	find . -name '*.so' -exec rm {} +
	find . -name '__pycache__' -type d -exec rm -r {} +
