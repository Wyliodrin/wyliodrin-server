*************
XMPP handling
*************

.. highlight:: c

The device communicates with its owner via XMPP. In order to integrate the XMPP
protocol, wyliodrin-server uses he libstrophe_ library.



Connection
==========

The credentials for the XMPP connection (jid and password) are found in
``wyliodrin.json``. The function responsible for the XMPP connection is
``wxmpp_connect`` from `wxmpp/wxmpp.c`_.



Presence
========



Pinging
=======



Messages
========



.. _libstrophe: http://strophe.im/libstrophe/
.. _`wxmpp/wxmpp.c`: https://github.com/alexandruradovici/wyliodrin-server/blob/clean/wxmpp/wxmpp.c
