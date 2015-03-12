CC = gcc
CFLAGS = -DERR -DLOG

.PHONY: init clean

init: init.o wjson.o wxmpp.o wxmpp_handlers.o
	$(CC) init.o wjson.o wxmpp.o wxmpp_handlers.o -o init -ljansson -lstrophe

init.o: init.c winternals/winternals.h wjson/wjson.h wxmpp/wxmpp.h
	$(CC) $(CFLAGS) -c init.c

wjson.o: wjson/wjson.c wjson/wjson.h winternals/winternals.h
	$(CC) $(CFLAGS) -c wjson/wjson.c

wxmpp.o: wxmpp/wxmpp.c wxmpp/wxmpp.h winternals/winternals.h
	$(CC) $(CFLAGS) -c wxmpp/wxmpp.c

wxmpp_handlers.o: wxmpp/wxmpp_handlers.c wxmpp/wxmpp.h winternals/winternals.h
	$(CC) $(CFLAGS) -c wxmpp/wxmpp_handlers.c

clean:
	rm -f *.o init
