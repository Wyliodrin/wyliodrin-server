*********
Ps module
*********

.. highlight:: c

Protocol
========
The owner asks to list all the running processes:
::
  <message to="<jid>" from="<owner>">
    <ps xmlns="wyliodrin" gadgetid="<jid>" action="send" request="<id>"/>
  </message>

Response:
::
  <message to="<owner>">
    <info xmlns="wyliodrin" data="ps">
      <ps pid="<pid_1>" cpu="<cpu_load_1>" mem="<mem_load_1>" name="<name_1>"/>
      ...
      <ps pid="<pid_n>" cpu="<cpu_load_n>" mem="<mem_load_n>" name="<name_n>"/>
    </info>
  </message>
