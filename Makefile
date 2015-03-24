CC = gcc
CFLAGS = -g -Wall -DERR -DLOG -DSHELLS

.PHONY: init clean

init: init.o wjson.o wxmpp.o wxmpp_handlers.o shells.o libds
	make -C libds
	$(CC) init.o wjson.o wxmpp.o wxmpp_handlers.o shells.o libds/*.o -o init -ljansson -lstrophe

init.o: init.c winternals/winternals.h wjson/wjson.h wxmpp/wxmpp.h
	$(CC) $(CFLAGS) -c init.c

wjson.o: wjson/wjson.c wjson/wjson.h winternals/winternals.h
	$(CC) $(CFLAGS) -c wjson/wjson.c

wxmpp.o: wxmpp/wxmpp.c wxmpp/wxmpp.h wxmpp/wxmpp_handlers.h winternals/winternals.h
	$(CC) $(CFLAGS) -c wxmpp/wxmpp.c

wxmpp_handlers.o: wxmpp/wxmpp_handlers.c wxmpp/wxmpp.h wxmpp/wxmpp_handlers.h winternals/winternals.h
	$(CC) $(CFLAGS) -c wxmpp/wxmpp_handlers.c

shells.o: shells/shells.c winternals/winternals.h shells/shells.h
	$(CC) $(CFLAGS) -c shells/shells.c

clean:
	rm -f *.o init
