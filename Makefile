CC = gcc
CFLAGS = -Wall -DERR -DLOG -DSHELLS -DFILES -DPS -D_FILE_OFFSET_BITS=64 -pthread -lutil -lfuse

.PHONY: wtalk clean

wtalk: wtalk.o wjson.o wxmpp.o wxmpp_handlers.o shells.o shells_helper.o files.o ps.o base64.o libds
	make -C libds
	$(CC) wtalk.o wjson.o wxmpp.o wxmpp_handlers.o shells.o shells_helper.o files.o ps.o base64.o libds/*.o -o wtalk \
	-ljansson -lstrophe -pthread -lutil -lfuse

wtalk.o: wtalk.c winternals/winternals.h wjson/wjson.h wxmpp/wxmpp.h
	$(CC) $(CFLAGS) -c wtalk.c

wjson.o: wjson/wjson.c wjson/wjson.h winternals/winternals.h
	$(CC) $(CFLAGS) -c wjson/wjson.c

wxmpp.o: wxmpp/wxmpp.c wxmpp/wxmpp.h wxmpp/wxmpp_handlers.h winternals/winternals.h
	$(CC) $(CFLAGS) -c wxmpp/wxmpp.c

wxmpp_handlers.o: wxmpp/wxmpp_handlers.c wxmpp/wxmpp.h wxmpp/wxmpp_handlers.h winternals/winternals.h
	$(CC) $(CFLAGS) -c wxmpp/wxmpp_handlers.c

shells.o: shells/shells.c winternals/winternals.h shells/shells.h
	$(CC) $(CFLAGS) -c shells/shells.c

shells_helper.o: shells/shells_helper.c winternals/winternals.h shells/shells_helper.h
	$(CC) $(CFLAGS) -c shells/shells_helper.c

files.o: files/files.c
	$(CC) $(CFLAGS) -c files/files.c

ps.o: ps/ps.c
	$(CC) $(CFLAGS) -c ps/ps.c

base64.o: base64/base64.c base64/base64.h
	$(CC) $(CFLAGS) -c base64/base64.c

clean:
	rm -f *.o wtalk
