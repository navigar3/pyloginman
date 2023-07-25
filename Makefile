CFLAGS+=-pedantic -Wall -I. -I/usr/include/libdrm
LDFLAGS+=-ldrm

.PHONY: clean

all: drm-splash4slack

kbd-handler.o: kbd-handler.c
	$(CC) -c $(CFLAGS) -o $@ $?

drm-splash4slack.o: drm-splash4slack.c
	$(CC) -c $(CFLAGS) -o $@ $?

main.o: main.c
	$(CC) -c $(CFLAGS) -o $@ $?

drm-splash4slack: tools/hashtable.o drm-splash4slack.o kbd-handler.o main.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o drm-splash4slack
