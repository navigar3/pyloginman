CFLAGS+=-pedantic -Wall -Wno-format -I. -I/usr/include/libdrm
LDFLAGS+=-ldrm

.PHONY: clean

all: drm-splash4slack

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

drmobjtest: drmobjtest.o drm-doublebuff.o tools/hashtable.o
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o drm-splash4slack
