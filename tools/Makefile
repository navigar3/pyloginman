.PHONY: clean

all: hashtable

hashtable.o: hashtable.c
	$(CC) -c $(CFLAGS) -o $@ $?

hashtable: hashtable.o
	$(CC) $(CFLAGS) -o $@ $?

clean:
	rm -f *.o drm-splash4slack
