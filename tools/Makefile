CFLAGS+=-I..

.PHONY: clean

all: main

hashtable.o: hashtable.c hashtable.h ../cobj.h
	$(CC) -c $(CFLAGS) -o $@ $<

main.o: main.c hashtable.h ../cobj.h
	$(CC) -c $(CFLAGS) -o $@ $<

main: main.o hashtable.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f *.o main
