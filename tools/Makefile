CFLAGS+=-I.. -fPIC

.PHONY: clean

all: main

hashtable.o: hashtable.c hashtable.h ../cobj.h
	$(CC) -c $(CFLAGS) -o $@ $<

main.o: main.c hashtable.h ../cobj.h
	$(CC) -c $(CFLAGS) -o $@ $<

main: main.o hashtable.o
	$(CC) $(CFLAGS) -o $@ $^
	
ht_dict.o: ht_dict.c hashtable.h ../cobj.h
	$(CC) -c $(CFLAGS) -o $@ $<

ht_dict.so: ht_dict.o hashtable.o
	$(CC) -shared -o $@ $^

clean:
	rm -f *.o main
