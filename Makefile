#?CC=diet gcc
#?CFLAGS=-Wall -Os -g -D_BSD_SOURCE
#?LDFLAGS=-static
#?all:	iocethtool
#?iocethtool:	iocethtool.o jelopt.o jelist.o
#?install:	iocethtool
#?	strip iocethtool
#?	mkdir -p $(DESTDIR)/usr/sbin
#?	cp iocethtool $(DESTDIR)/usr/sbin
#?clean:	
#?	rm -f *.o iocethtool
CC=diet gcc
CFLAGS=-Wall -Os -g -D_BSD_SOURCE
LDFLAGS=-static
all:	iocethtool
iocethtool:	iocethtool.o jelopt.o jelist.o
install:	iocethtool
	strip iocethtool
	mkdir -p $(DESTDIR)/usr/sbin
	cp iocethtool $(DESTDIR)/usr/sbin
clean:	
	rm -f *.o iocethtool
