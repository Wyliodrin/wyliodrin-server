
#include "xmpp.h"

#include <stdio.h>
#include <strophe.h>
#include <string.h>

#include "libds/ds.h"

#define MAX_JID_STR					1000
#define TAGS_SIZE					100
#define WYLIODRIN_NAMESPACE			"wyliodrin"

static xmpp_ctx_t *context;
static xmpp_conn_t *connection;

static hashmap_p tags = NULL;

static pthread_t xmpp_thread;

void wylio (const char *from, const char *to, int error, xmpp_stanza_t *stanza)
{
	printf ("wylio error: %d from: %s to: %s\n", error, from, to); 
}

int wxmpp_tag_add (char *tag, tag_function function)
{
	if (context==NULL || tags==NULL) return XMPP_E_NOT_CONNECTED;
	int v = (int)function;
	// printf ("v: %d\n", v);
	hashmap_put (tags, tag, &v, sizeof (v));
	return XMPP_OK;		
}

int wxmpp_ping_handler (xmpp_conn_t *const conn, xmpp_stanza_t *const stanza, void *const userdata)
{
	xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata;
	xmpp_stanza_t *pong = xmpp_stanza_new (ctx);
	xmpp_stanza_set_name (pong, "iq");
	xmpp_stanza_set_attribute (pong, "to", xmpp_stanza_get_attribute (stanza, "from"));
	xmpp_stanza_set_id (pong, xmpp_stanza_get_id (stanza));
	xmpp_stanza_set_type (pong, "result");
	xmpp_send (conn, pong);
	xmpp_stanza_release (pong);
	return 1;
}

int wxmpp_wyliodrin_handler (xmpp_conn_t *const conn, xmpp_stanza_t *const stanza, void *const userdata)
{
	char *type = xmpp_stanza_get_type (stanza);
	int error = 0;
	if (type!=NULL && strncasecmp (type, "error", 5)==0)
	{
		// TODO
		error = 1;
	}
	
	xmpp_stanza_t *tag = xmpp_stanza_get_children (stanza);
	while (tag!=NULL)
	{
		char *ns = xmpp_stanza_get_ns (tag);
		if (ns!=NULL && strncasecmp (ns, WYLIODRIN_NAMESPACE, strlen (WYLIODRIN_NAMESPACE))==0)
		{
			char *name = xmpp_stanza_get_name (tag);
			// printf ("tag %s\n", name);
			tag_function *function = hashmap_get (tags, name);
			if (function != NULL && *function != NULL) (*function)(xmpp_stanza_get_attribute(stanza, "from"), xmpp_stanza_get_attribute(stanza, "to"), error, tag);
			tag = xmpp_stanza_get_next (tag);
		}
	}
	
	// printf ("%s\n", xmpp_stanza_get_name (stanza));
	
	return 1;
}

int wxmpp_messages_handler(xmpp_conn_t *const conn, xmpp_stanza_t *const stanza, void *const userdata)
{
	xmpp_stanza_t *body = xmpp_stanza_get_child_by_name (stanza, "body");
	if (body!=NULL)
	{
		printf ("%s\n", xmpp_stanza_get_text (body));
	}
	else
	{
		printf ("NULL body\n");
	}
	return 1;
}

void wxmpp_connection_handler(xmpp_conn_t * const conn, const xmpp_conn_event_t status, 
		  const int error, xmpp_stream_error_t * const stream_error,
		  void * const userdata)
{
    xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;
    if (status == XMPP_CONN_CONNECT)
    {
    	//xmpp_handler_add (conn, wxmpp_messages_handler, NULL, "message", NULL, ctx);
    	// PING
    	xmpp_handler_add (conn, wxmpp_ping_handler, "urn:xmpp:ping", "iq", "get", ctx);
    	xmpp_handler_add (conn, wxmpp_wyliodrin_handler, WYLIODRIN_NAMESPACE, "message", NULL, ctx);
    	
    	xmpp_stanza_t *pres = xmpp_stanza_new(ctx);
		xmpp_stanza_set_name(pres, "presence");
		xmpp_stanza_t *priority = xmpp_stanza_new (ctx);
		xmpp_stanza_set_name (priority, "priority");
		xmpp_stanza_add_child (pres, priority);
		xmpp_stanza_t *value = xmpp_stanza_new (ctx);
		xmpp_stanza_set_text (value, "50");
		xmpp_stanza_add_child (priority, value);
		xmpp_stanza_release (value);
		xmpp_stanza_release (priority);
		xmpp_send(conn, pres);
		xmpp_stanza_release(pres);
    }
    else if (status == XMPP_CONN_DISCONNECT)
    {
    	wxmpp_disconnect ();
    }
    else if (status == XMPP_CONN_FAIL)
    {
    	wxmpp_disconnect ();
    }
}

void * wxmpp_start (void *data)
{
	xmpp_run (context);
}

int wxmpp_connect (const char *jid, const char *password, const char *resource, const char *domain, int port)
{
	char xmpp_jid[MAX_JID_STR];
	
	// TODO MUTEX
	if (jid == NULL || password == NULL) return XMPP_E_JID_PASSWORD_NULL;
	if (context!=NULL || connection!=NULL) return XMPP_E_ALREADY_CONNECTED;
	
	// TAGS
	tags = create_hashmap ();
	
	/* initialize lib */
    xmpp_initialize();
    
    xmpp_log_t *log = xmpp_get_default_logger (XMPP_LEVEL_DEBUG);

    /* create a context */
    context = xmpp_ctx_new(NULL, log);
    // context = xmpp_ctx_new(NULL, NULL);

    /* create a connection */
    connection = xmpp_conn_new(context);
    
    if (resource != NULL)
    {
    	snprintf (xmpp_jid, MAX_JID_STR, "%s/%s", jid, resource);
    }
    else
    {
    	snprintf (xmpp_jid, MAX_JID_STR, "%s", jid);
    }
    
    /* setup authentication information */
    xmpp_conn_set_jid(connection, xmpp_jid);
    xmpp_conn_set_pass(connection, password);
    
    /* initiate connection */
    xmpp_connect_client(connection, domain, port, wxmpp_connection_handler, context);
    
    wxmpp_tag_add ("wylio", wylio);
    
    if (pthread_create (&xmpp_thread, NULL, wxmpp_start, NULL))
    {
    	wxmpp_disconnect ();
    	return XMPP_E_THREAD;
    }
    return XMPP_OK;
}

int wxmpp_disconnect ()
{
	// TODO MUTEX
	
	// TAGS
	destroy_hashmap (tags);
	tags = NULL;
	
	xmpp_stop (context);
	/* release our connection and context */
    xmpp_conn_release(connection);
    connection = NULL;
    xmpp_ctx_free(context);
    context = NULL;

    /* shutdown lib */
    xmpp_shutdown();
    return 0;
}

int wxmpp_send (char *to, xmpp_stanza_t *stanza)
{
	if (context==NULL || connection==NULL) return XMPP_E_NOT_CONNECTED;
	if (stanza == NULL) return XMPP_E_STANZA;
	xmpp_stanza_t *message = xmpp_stanza_new(context);
	xmpp_stanza_set_name(message, "message");
	xmpp_stanza_set_attribute (message, "to", to);
	xmpp_stanza_add_child (message, stanza);
	xmpp_send (connection, message);
	xmpp_stanza_release (message);
	return XMPP_OK;
}

xmpp_ctx_t *wxmpp_get_context ()
{
	return context;
}

int wxmpp_wait ()
{
	int rc = pthread_join (xmpp_thread, NULL);
	if (rc == 0) return XMPP_OK;
	else return XMPP_E_THREAD;
}

/*
int main(int argc, char **argv)
{
    wxmpp_connect ("wylio.project@gmail.com", "wylio123drin", "raspberry", NULL, 0);
    return 0;
}
*/
