CC = gcc
CFLAGS = -DERR -DLOG

.PHONY: init clean

init: init.o wyliodrin_json.o wxmpp.o wxmpp_handlers.o
	$(CC) init.o wyliodrin_json.o wxmpp.o wxmpp_handlers.o -o init -ljansson -lstrophe

init.o: init.c internals/internals.h wyliodrin_json/wyliodrin_json.h wxmpp/wxmpp.h
	$(CC) $(CFLAGS) -c init.c

wyliodrin_json.o: wyliodrin_json/wyliodrin_json.c wyliodrin_json/wyliodrin_json.h internals/internals.h
	$(CC) $(CFLAGS) -c wyliodrin_json/wyliodrin_json.c

wxmpp.o: wxmpp/wxmpp.c wxmpp/wxmpp.h internals/internals.h
	$(CC) $(CFLAGS) -c wxmpp/wxmpp.c

wxmpp_handlers.o: wxmpp/wxmpp_handlers.c wxmpp/wxmpp.h internals/internals.h
	$(CC) $(CFLAGS) -c wxmpp/wxmpp_handlers.c

clean:
	rm -f *.o init
