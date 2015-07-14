*********
Upload module
*********

.. highlight:: c

XMPP Protocol
========
Request:
::
  <message to="<jid>" from="<owner>">
    <upload xmlns="wyliodrin">
      <msgpack>X</msgpack>
    </upload>
  </message>

Response:
::
  <message to="<owner>">
    <upload xmlns="wyliodrin">
      <msgpack>X</msgpack>
    </upload>
  </message>



MSGPACK Protocol
================
Request:
::
  [<action_code>, "<path"]

``<action_code>`` values:
  - 0: ATTRIBUTES
  - 1: LIST
  - 2: READ

Response:
::
  [0, "<path>"] when path is not valid
  [0, "<path>", <filetype>, <filesize>] when path is valid

``<filetype>`` values:
  - 0: DIRECTORY
  - 1: REGULAR
  - 2: OTHER
