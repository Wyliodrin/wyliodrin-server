
#include "shell.h"
#include "xmpp.h"
#include "base64.h"
#include <string.h>
#include <stdlib.h>

#define SHELLS_TAG	"shells"

// TODO
int shellid;

void function_keys_pressed (int id, char *buffer, int n)
{
	if (wxmpp_get_context()==NULL)
	{
		int i;
		for (i=0; i<n; i++)
		{
			putc (buffer[i], stdout);
		}
		return;
	}
	size_t nokeys;
	char *keys = wxmpp_base64_encode (buffer, n, &nokeys);
	// char *keys = x16_encode (buffer, n);
	printf ("%s\n", keys);
	xmpp_stanza_t *shells = xmpp_stanza_new (wxmpp_get_context ());
	xmpp_stanza_set_name (shells, "shells");
	xmpp_stanza_set_ns (shells, "wyliodrin");
	xmpp_stanza_set_attribute (shells, "action", "keys");
	xmpp_stanza_t *value = xmpp_stanza_new (wxmpp_get_context());
	xmpp_stanza_set_text (value, keys);
	xmpp_stanza_add_child (shells, value);
	xmpp_stanza_release (value);
	wxmpp_send("alex@wyliodrin.com", shells);
	xmpp_stanza_release(shells);
	free (keys);
}

void shell_tag (const char *from, const char *to, int error, xmpp_stanza_t *stanza)
{
	// printf ("error: %d from: %s to: %s\n", error, from, to); 
	if (error == 1) return;
	char *action = xmpp_stanza_get_attribute (stanza, "action");
	// printf ("%s\n", action);
	if (action != NULL)
	{
		if (strncasecmp (action, "open", 4)==0)
		{
			shellid = create_shell ();
		}
		else
		if (strncasecmp (action, "keys", 4)==0)
		{
			char *keys = xmpp_stanza_get_text (stanza);
			size_t nobuffer;
			char *buffer = wxmpp_base64_decode (keys, strlen (keys), &nobuffer);
			// char *buffer = x16_decode (keys, &nobuffer);
			send_keys_to_shell (shellid, buffer, nobuffer);
			free (buffer);
		}
	}
}

int main ()
{
	init_shells ();
	set_keys_pressed (function_keys_pressed);
	//wxmpp_connect ("pi@wyliodrin.com", "123456", "raspberry", NULL, 0);
	//wxmpp_tag_add (SHELLS_TAG, shell_tag);
	run_shell ();

	int id = create_shell ();
	sleep (10);
	close_shell (id);	

	//wxmpp_wait ();

}
