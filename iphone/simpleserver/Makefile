VERSION = "\"$(shell cat ../../VERSION)\""
CFLAGS = -Wall -g -DVERSION=$(VERSION) -DSTANDALONE=1

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

simpleserver: simpleserver.c icons.c

icons.c: icon2c icon2c.conf
	./icon2c

icon2c: icon2c.c

clean: FORCE
	rm -rf icon2c icon2c.exe icons.c simpleserver simpleserver.exe \
		icon2c.dSYM simpleserver.dSYM

FORCE:
