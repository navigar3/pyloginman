CFLAGS+=-pedantic -fPIC -Wall -Wno-format -I. -I/usr/include/libdrm
LDFLAGS+=

.PHONY: clean tools/hashtable_dyn.o

all: modules/drmhandler/drm_handler.so modules/kbdhandler/kbd_handler.so

tools/hashtable_dyn.o:
	$(MAKE) -C tools hashtable_dyn.o

tools/zhelper_dyn.o:
	$(MAKE) -C tools zhelper_dyn.o

kbd-handler.o: kbd-handler.c
	$(CC) -c $(CFLAGS) -o $@ $?

drm-splash4slack.o: drm-splash4slack.c
	$(CC) -c $(CFLAGS) -o $@ $?

pipeline.o: pipeline.c
	$(CC) -c $(CFLAGS) -o $@ $?

main.o: main.c
	$(CC) -c $(CFLAGS) -o $@ $?

drm-splash4slack: tools/hashtable.o drm-splash4slack.o kbd-handler.o pipeline.o main.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

drmobjtest.o: drmobjtest.c
	$(CC) -c $(CFLAGS) -o $@ $?

drm-doublebuff.o: drm-doublebuff.c
	$(CC) -c $(CFLAGS) -o $@ $?

kbd-handler.o: kbd-handler.c
	$(CC) -c $(CFLAGS) -o $@ $?

drmobjtest: drmobjtest.o drm-doublebuff.o tools/hashtable.o
	$(CC) -o $@ $^ $(LDFLAGS)
	
modules/drmhandler/drm_handler.so: drm-doublebuff.o tools/hashtable_dyn.o tools/zhelper_dyn.o
	$(CC) -shared -o $@ $^ $(LDFLAGS) -ldrm -lz

modules/kbdhandler/kbd_handler.so: kbd-handler.o
	$(CC) -shared -o $@ $^ $(LDFLAGS)

clean:
	find . -name '*.o' -exec rm {} +
	find . -name '*.so' -exec rm {} +
