#include <strophe.h> /* Strophe XMPP stuff */

#include "../winternals/winternals.h" /* logs and errs */
#include "shells.h"                   /* shells module api */

#ifdef SHELLS

void shells(const char *from, const char *to, int error, xmpp_stanza_t *stanza) {
  wlog("shells(%s, %s, %d, stanza)", from, to, error);
}

#endif /* SHELLS */
