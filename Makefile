CC = gcc
CFLAGS = -DERR -DLOG

.PHONY: init clean

init: init.o wyliodrin_json.o xmpp.o xmpp_handlers.o
	$(CC) init.o wyliodrin_json.o xmpp.o xmpp_handlers.o -o init -ljansson -lstrophe

init.o: init.c internals/internals.h wyliodrin_json/wyliodrin_json.h xmpp/xmpp.h
	$(CC) $(CFLAGS) -c init.c

wyliodrin_json.o: wyliodrin_json/wyliodrin_json.c wyliodrin_json/wyliodrin_json.h internals/internals.h
	$(CC) $(CFLAGS) -c wyliodrin_json/wyliodrin_json.c

xmpp.o: xmpp/xmpp.c xmpp/xmpp.h internals/internals.h
	$(CC) $(CFLAGS) -c xmpp/xmpp.c

xmpp_handlers.o: xmpp/xmpp_handlers.c xmpp/xmpp.h internals/internals.h
	$(CC) $(CFLAGS) -c xmpp/xmpp_handlers.c

clean:
	rm -f *.o init
