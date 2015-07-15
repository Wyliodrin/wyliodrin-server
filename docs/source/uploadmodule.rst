*************
Upload module
*************

.. highlight:: c

XMPP Protocol
=============
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
``<action_code>`` values:
  - 0: ATTRIBUTES
  - 1: LIST
  - 2: READ

``<filetype>`` values:
  - 0: DIRECTORY
  - 1: REGULAR
  - 2: OTHER


Request
-------
Request msgpack array:
::
  [<action_code>, "<path"]


Response
--------
Attributes response:
::
  [0, "<path>"] when path is not valid
  [0, "<path>", <filetype>, <filesize>] when path is valid

List response:
::
  [1, "<path>"] when path is a valid directory
  [1, "<path>", "<filename_1>", "<filename_2>", ... ,"<filename_n>"] when path is a valid directory
