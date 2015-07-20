/**************************************************************************************************
 * Download files from cloud to device.
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: July 2015
 *************************************************************************************************/

#ifdef DOWNLOAD

/**
 *  <message to="<jid"> from="<owner>">
 *    <download xmlns="wyliodrin">
 *       <msgpack>X</msgpack>
 *    </upload>
 *  </message>
 */
void download(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
  xmpp_conn_t *const conn, void *const userdata)
{

#endif /* DOWNLOAD */
