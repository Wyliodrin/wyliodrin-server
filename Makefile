.PHONY: libds clean

WYLIODRIN_LIBS		=	-lpthread
XMPP_LIBS	=	-lstrophe -lexpat -lssl -lresolv -lds
XMPP_LIBS_PATH	=	-L./libds/
SHELL_LIBS		=	-lutil

wyliodrind:main.o shell.o xmpp.o base64.o libds
		gcc main.o shell.o xmpp.o base64.o $(WYLIODRIN_LIBS) $(SHELL_LIBS) $(XMPP_LIBS) $(XMPP_LIBS_PATH) -o $@ 

#main.main.o

#shell:shell.o

#xmpp:xmpp.o libds
#		gcc xmpp.o $(XMPP_LIBS) $(XMPP_LIBS_PATH) -o $@ 
files: files.o
	gcc files.o $(XMPP_LIBS) $(XMPP_LIBS_PATH)-o files
f_make:	make_options.o
	gcc make_options.o $(XMPP_LIBS) $(XMPP_LIBS_PATH) -o make_options
libds:
		cd libds; make

clean:
		rm *.o
		rm wyliodrind
		cd libds; rm *.o; rm *.a



