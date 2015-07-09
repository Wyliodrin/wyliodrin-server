***********
Make module
***********

.. highlight:: c

Protocol
========
The owner asks for the project ``projectid`` to be built:
::
  <message to="<jid>" from="<owner>">
    <make xmlns="wyliodrin" gadgetid=<jid> projectid="<projectid>" address="<download_address>" action="build" request="<id>"/>
  </message>

The ``files`` protocol begins:
::
  <message to="<owner>">
    <files xmlns="wyliodrin" action="attributes" path="/<projectid>"/>
  </message>

Once the files protocol ends, the project is built and the output is sent to
owner:
::
  <message to="<jid>" from="<owner>">
    <make xmlns="wyliodrin" action="build" response="working" source="stdout/stderr" request="<id>">
      "<stdout/stderr data encoded in base64>"
    </make>
  </message>

Once the buildind process ends, the build status is send to the owner:
::
  <message to="<owner>">
    <make xmlns="wyliodrin" action="build" response="done" code="<status_code>" request="<id>"/>
  </message>
