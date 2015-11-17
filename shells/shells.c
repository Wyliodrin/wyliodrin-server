/**************************************************************************************************
 * Shells module implementation
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: November 2015
 *************************************************************************************************/

#ifdef SHELLS



/*** INCLUDES ************************************************************************************/

#include <stdlib.h> /* memory handling */

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



/*** EXTERN VARIABLES ****************************************************************************/

extern const char *owner;

/*************************************************************************************************/




/*** API IMPLEMENTATION **************************************************************************/

void shells(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
            xmpp_conn_t *const conn, void *const userdata) {
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
  werr2(!cmp_write_map(&cmp, 2 +             /* text */
                             2 * num_attrs), /* attributes */
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
  for (i = 0; i < 2 * num_attrs; i++) {
    werr2(!cmp_write_str(&cmp, attrs[i], strlen(attrs[i])),
          return,
          "cmp_write_str error: %s", cmp_strerror(&cmp));
  }

  /* Send msgpack buffer to hypervisor via redis */
  winfo("publish");
  publish(HYPERVISOR_PUB_CHANNEL, msgpack_buf, cmp.writer_offset);
  winfo("done message");
  /* Clean */
  free(msgpack_buf);
}

/*************************************************************************************************/



#endif /* SHELLS */
