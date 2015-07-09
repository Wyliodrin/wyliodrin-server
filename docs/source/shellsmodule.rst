*************
Shells module
*************

.. highlight:: c

The communication protocol for the shells module is done via the
``shells`` stanza as the child of the ``message`` stanza:
::
  <message to=<jid> from=<owner>>
    <shells xmlns="wyliodrin" gadgetid=<jid> request=<id> action=<action>/>
  </message>

The ``action`` attribute can be ``open``, ``keys`` or ``close``.


Open
====
When a new shell request arrives via the ``open`` action, a new ``bash``
process is created and its output is redirected in a pipe. Everything that
comes out on the other end of the pipe is send back via ``keys``.

Example of ``open`` stanza:
::
  <message to=<jid> from=<owner>>
    <shells xmlns="wyliodrin" gadgetid=<jid> request=<id> action="open" height="21" width="90"/>
  </message>

Response stanza when a new shell could be successfully opened:
::
  <message to=<owner>>
    <shells xmlns="wyliodrin" request=<id> action="open" response="done" shellid=<shellid>/>
  </message>

Response stanza when a new shell could not be opened:
::
  <message to=<owner>>
    <shells xmlns="wyliodrin" request=<id> action="open" response="error"/>
  </message>



Keys
====
The shell takes its input and spits its output in a pipe. Both input and output
are encoded in base64.

The input from the owner comes via the ``keys`` action, it is decoded and
pushed in the write end of the pipe. A separate thread is constantly reading
from the read end of the pipe and the read data is econded and sent to the
owner.

Example of input data:
::
  <message to=<jid> from=<owner>>
    <shells xmlns="wyliodrin" gadgetid=<jid> request=<id> action="keys" shellid=<shellid>>
      <data encoded in base64>
    </shells>
  </message>

Example of output data:
::
  <message to=<owner>>
    <shells xmlns="wyliodrin" action="keys" shellid=<shellid>>
      <data encoded in base64>
    </shells>
  </message>



Close
=====
A shell can be closed deliberately or the bash process dies.

Example of ``close`` request:
::
  <message to=<jid> from=<owner>>
    <shells xmlns="wyliodrin" gadgetid=<jid> action="close" request=<id> shellid=<shellid>/>
  </message>

Response of ``close`` request or death of shell:
::
  <message to=<owner>>
    <shells xmlns="wyliodrin" action="close" shellid=<shellid> code=<exit_code> request=<id>/>
  </message>

.. note::
  The ``request`` attribute is needed only when a ``close`` request has been
  received for that shell.
