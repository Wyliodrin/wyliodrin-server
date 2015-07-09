*************
XMPP handling
*************

.. highlight:: c

The device communicates with its owner via XMPP. In order to integrate the XMPP
protocol, wyliodrin-server uses the libstrophe_ library.



Connection
==========
The credentials for the XMPP connection (jid and password) are found in
``wyliodrin.json``. The function responsible for the XMPP connection is
``wxmpp_connect`` from `wxmpp/wxmpp.c`_.



Handlers
========
The handlers are implemented in `wxmpp/wxmpp_handlers.c`_.


Connection handler
------------------
The connection callback, on successful connection, adds the other handlers
via xmpp_handler_add_ and sends two presence stanzas:
::
  <presence><priority>50</priority></presence>

and
::
  <presence type="subscribe" to=<owner>/>

The ``<owner>`` value is retrieved from ``wyliodrin.json``.

In case of a connection error, the error must be signaled to the ``files``
module in order to avoid a deadlock.


Ping handler
------------
Ping stanza:
::
  <iq id=<id> to=<jid> type="get" from="wyliodrin.com">
    <ping xmlns="urn:xmpp:ping"/>
  </iq>

Pong stanza:
::
  <iq id=<id> type="result" to="wyliodrin.com"/>


Presence handler
----------------
This is an example of a presence stanza when the owner is online:
::
  <presence to=<jid> from=<owner>>
    <priority>50</priority>
    <status>Happily echoing your &lt;message/&gt; stanzas</status>
  </presence>

When running wyliodrin-server on a board for a first time, a subscription
will be required. This is done via the following stanza:
::
  <presence to=<jid> type="subscribe" from=<owner>/>

In order to establish the subscription, the following stanza is sent:
::
  <presence type="subscribed" to=<owner>/>

When the owner becomes unavailable, the following stanza is received:
::
  <presence to=<jid> type="unavailable" from=<owner>/>


Message handler
---------------
Via this handler the device communicates with his owner. The stanza named
``message`` contains as its children other stanzas whose names represent the
keys in the ``modules`` hashmap.

The ``modules`` hashmap associates some modules names with their corresponding
handlers.

The module names are retrieved from the children stanzas, and their handlers
are retrieved from the module names.

Example of such a stanza:
::
  <message to=<jid> from=<owner>>
    <shells height="21" gadgetid=<jid> xmlns="wyliodrin" action="open" request=<request> width="90"/>
  </message>

In the example above, is requested to invoke the ``shells`` handler. The
handler of "shells" is retrieved from the ``modules`` hashmap and called
with its corresponding arguments.



.. _libstrophe: http://strophe.im/libstrophe/
.. _`wxmpp/wxmpp.c`: https://github.com/alexandruradovici/wyliodrin-server/blob/clean/wxmpp/wxmpp.c
.. _`wxmpp/wxmpp_handlers.c`: https://github.com/alexandruradovici/wyliodrin-server/blob/clean/wxmpp/wxmpp_handlers.c
.. _xmpp_handler_add: http://strophe.im/libstrophe/doc/0.8-snapshot/group___handlers.html#gad307e5a22d16ef3d6fa18d503b68944f
.. _`wxmpp/wxmpp.h`: https://github.com/alexandruradovici/wyliodrin-server/blob/clean/wxmpp/wxmpp.h
