************
Files module
************

.. highlight:: c

Protocol
========
The owner is asked for file attributes:
::
  <message to=<owner>>
    <files xmlns="wyliodrin" action="attributes" path=<file_path>/>
  </message>
.. note:
  The ``path`` attribute is the ``projectid`` attribute received in ``make``
  or its subfiles and subdirectories.

File attributes response:
::
  <message to=<jid> from=<owner>>
    <files xmlns="wyliodrin" action="attributes" path=<file_path> type="directory" size=<size> error="0"/>
  </message>

The owner is asked to list the directory content:
::
  <message to=<owner>>
    <files xmlns="wyliodrin" action="list" path=<file_path>/>
  </message>

List directory response:
::
  <message to=<jid> from=<owner>>
    <files xmlns="wyliodrin" action="list" path=<file_path> error="0">
      <file filename=<filename1> size=<size1>/>
      ...
      <directory filename=<filename2> size=<size2>/>
    </files>
  </message>

For every file or directory the owner is asked to read it and send the result:
::
  <message to=<owner>>
    <files xmlns="wyliodrin" action="read" path=<file_path>/>
  </message>

The content of the requested file is send back encoded in base64:
::
  <message to=<jid> from=<owner>>
    <files xmlns="wyliodrin" action="read" path=<file_path> length=<length> error="0">
      <file content encoded in base64>
    </files>
  </message>
