CFLAGS = -Wall -g -DSTANDALONE=1

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

simpleserver: simpleserver.c icons.c minizip_zip.c minizip_unzip.c minizip_ioapi.c
	$(CC) $(CFLAGS) -DNOCRYPT -DNOUNCRYPT -o simpleserver simpleserver.c icons.c minizip_zip.c minizip_unzip.c minizip_ioapi.c -lz -lpthread

icons.c: icon2c icon2c.conf
	./icon2c

icon2c: icon2c.c
	env -i /usr/bin/cc -o icon2c icon2c.c

clean: FORCE
	rm -rf icon2c icon2c.exe icons.c simpleserver simpleserver.exe \
		core.* *.stackdump icon2c.dSYM simpleserver.dSYM

FORCE:
