*************
Logs handling
*************

.. highlight:: c

Protocol
========

The macros in ``winternals.h`` push the log messages in a queue and every
second in a separate threads, the entries in the log queue are popped and
send using the ``libcurl`` library to
``https://wyliodrin.com/gadgets/logs/<jid>``.
