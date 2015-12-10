/**************************************************************************************************
 * Shells module implementation
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: November 2015
 *************************************************************************************************/

#ifdef SHELLS



/*** INCLUDES ************************************************************************************/

#include <stdlib.h> /* memory handling */
#include <time.h>   /* hypervisor time */
#include <unistd.h> /**/

#include "../libds/ds.h" /* hashmap */

#include "../winternals/winternals.h"       /* logs and errs */
#include "../wxmpp/wxmpp.h"                 /* xmpp handling */
#include "../cmp/cmp.h"                     /* msgpack       */
#include "../communication/communication.h" /* redis         */

#include "shells.h" /* API */

/*************************************************************************************************/



/*** UNDEFINED ***********************************************************************************/

int xmpp_stanza_get_attribute_count(xmpp_stanza_t *const stanza);
int xmpp_stanza_get_attributes(xmpp_stanza_t *const stanza, const char **attr, int attrlen);

/*************************************************************************************************/



/*** STATIC VARIABLES ****************************************************************************/

static hashmap_p hm;
static hashmap_p action_hm;

/*************************************************************************************************/



/*** EXTERN VARIABLES ****************************************************************************/

extern const char *owner;
extern time_t time_of_last_hypervior_msg;
extern xmpp_ctx_t *global_ctx;
extern xmpp_conn_t *global_conn;
extern int pong_timeout;

/*************************************************************************************************/




/*** API IMPLEMENTATION **************************************************************************/

void init_shells() {
  hm = create_hashmap();

  hashmap_put(hm, "width",     "w", strlen("w") + 1);
  hashmap_put(hm, "height",    "h", strlen("h") + 1);
  hashmap_put(hm, "request",   "r", strlen("r") + 1);
  hashmap_put(hm, "action",    "a", strlen("a") + 1);
  hashmap_put(hm, "projectid", "p", strlen("p") + 1);
  hashmap_put(hm, "shellid",   "s", strlen("s") + 1);
  hashmap_put(hm, "userid",    "u", strlen("u") + 1);

  action_hm = create_hashmap();

  hashmap_put(action_hm, "open",       "o", strlen("o") + 1);
  hashmap_put(action_hm, "close",      "c", strlen("c") + 1);
  hashmap_put(action_hm, "keys",       "k", strlen("k") + 1);
  hashmap_put(action_hm, "status",     "s", strlen("s") + 1);
  hashmap_put(action_hm, "poweroff",   "p", strlen("p") + 1);
  hashmap_put(action_hm, "disconnect", "d", strlen("d") + 1);
}


void shells(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
            xmpp_conn_t *const conn, void *const userdata) {
  werr2(time(NULL) - time_of_last_hypervior_msg >= pong_timeout + 1, return, "Hypervisor is dead");

  werr2(strncasecmp(owner, from, strlen(owner)) != 0, return,
        "Ignore shells stanza received from %s", from);

  /* Convert stanza to text in order to get its size */
  char *stanza_to_text;
  size_t stanza_to_text_len;
  int xmpp_stanza_to_text_rc = xmpp_stanza_to_text(stanza, &stanza_to_text,
                                                   &stanza_to_text_len);
  werr2(xmpp_stanza_to_text_rc < 0, return, "Could not convert stanza to text");

  /* Allocate memory for the msgpack buffer */
  char *msgpack_buf = malloc(stanza_to_text_len * sizeof(char));
  werr2(msgpack_buf == NULL, return, "Could not allocate memory for msgpack_buf");

  /* Init msgpack */
  cmp_ctx_t cmp;
  cmp_init(&cmp, msgpack_buf, stanza_to_text_len);

  /* Get attributes of stanza */
  int num_attrs = xmpp_stanza_get_attribute_count(stanza);
  char **attrs = malloc(2 * num_attrs * sizeof(char *));
  werr2(attrs == NULL, return, "Could not allocate memory for attrs");

  xmpp_stanza_get_attributes(stanza, (const char **)attrs, 2 * num_attrs);

  /* Get text from stanza */
  char *text = xmpp_stanza_get_text(stanza);

  /* Write map */
  werr2(!cmp_write_map(&cmp, 2 +                  /* text */
                             2 * num_attrs - 4 ), /* attributes without gadgetid and xmlns */
        return,
        "cmp_write_map error: %s", cmp_strerror(&cmp));

  /* Write text */
  werr2(!cmp_write_str(&cmp, "t", 1),
        return,
        "cmp_write_str error: %s", cmp_strerror(&cmp));
  werr2(!cmp_write_str(&cmp, text, text != NULL ? strlen(text) : 0),
        return,
        "cmp_write_str error: %s", cmp_strerror(&cmp));

  /* Write attributes */
  int i;
  for (i = 0; i < 2 * num_attrs; i += 2) {
    if ((strncmp(attrs[i], "gadgetid", strlen("gadgetid")) == 0) ||
        (strncmp(attrs[i], "xmlns",    strlen("xmlns"))    == 0)) {
      continue;
    }

    char *replacement = (char *)hashmap_get(hm, attrs[i]);
    werr2(replacement == NULL, return, "No entry named %s in attribute hashmap", attrs[i]);

    werr2(!cmp_write_str(&cmp, replacement, strlen(replacement)),
          return,
          "cmp_write_str error: %s", cmp_strerror(&cmp));

    if (strncmp(attrs[i], "action", strlen("action")) == 0) {
      char *action_replacement = (char *)hashmap_get(action_hm, attrs[i+1]);
      werr2(action_replacement == NULL, return, "No entry named %s in attribute action hashmap",
            attrs[i+1]);

      werr2(!cmp_write_str(&cmp, action_replacement, strlen(action_replacement)),
          return,
          "cmp_write_str error: %s", cmp_strerror(&cmp));
    } else {
      werr2(!cmp_write_str(&cmp, attrs[i+1], strlen(attrs[i+1])),
            return,
            "cmp_write_str error: %s", cmp_strerror(&cmp));
    }
  }

  /* Send msgpack buffer to hypervisor via redis */
  publish(HYPERVISOR_PUB_CHANNEL, msgpack_buf, cmp.writer_offset);
  /* Clean */
  free(msgpack_buf);
}

/*************************************************************************************************/



#endif /* SHELLS */
