*************
Shells module
*************

.. highlight:: c

The communication protocol for the shells module is done via the
``shells`` stanza as the child of the ``message`` stanza:
::
  <message to="<jid>" from="<owner>">
    <w xmlns="wyliodrin" d="<msgpack_map>"/>
  </message>

The ``msgpack_map``:
::
  {
    "sm" : "s",
    "sa" : "<action>",
    "sp" : "<projectid>", // if a make shell must be open
    "su" : "<userid>", // if a make shell must be open
    ...
  }

The ``action`` value can be ``o`` (``open``), ``k`` (``keys``), ``c`` (``close``)
or ``s`` (``status``).


Open
====
When a new shell request arrives via the ``open`` action, a new ``bash``
process is created and its output is redirected in a pipe. Everything that
comes out on the other end of the pipe is send back via ``keys``.

Example of ``open`` map:
::
  {
    "sm" : "s",
    "sa" : "o",
    "nw" : <width>,
    "nh" : <height>,
    "nr" : <request>
  }

Response stanza when a new shell could be successfully opened:
::
  <message to=<owner>>
    <shells xmlns="wyliodrin" request=<id> action="open" response="done" shellid=<shellid>/>
  </message>

::
  {
    "m" : "s",
    "a" : "o",
    "r" : <request>,
    "s" : <success>, // 0 or 1
    "i" : <id_of_shell> // if success == 1
    "w" : <work_in_progress> // 0 or 1; if a make shell must be open; if success == 1
  }
