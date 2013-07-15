
#ifndef XMPP_H_
#define XMPP_H_

#include <strophe.h>

#define XMPP_OK						0
#define XMPP_E_CONNECTION_FAILED	-1
#define XMPP_E_JID_PASSWORD_NULL	-2
#define XMPP_E_ALREADY_CONNECTED	-3
#define XMPP_E_NOT_CONNECTED		-4
#define XMPP_E_THREAD				-5
#define XMPP_E_STANZA				-6

typedef void (*tag_function)(const char *from, const char *to, int error, xmpp_stanza_t *stanza);

int wxmpp_tag_add (char *tag, tag_function function);
int wxmpp_connect (const char *jid, const char *password, const char *resource, const char *domain, int port);
int wxmpp_send (char *to, xmpp_stanza_t *stanza);
int wxmpp_disconnect ();
xmpp_ctx_t *wxmpp_get_context ();

int wxmpp_wait ();

#endif
