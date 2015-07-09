********************
Communication module
********************

.. highlight:: c

Protocol
========
When a ``communication`` message is received:
::
  <message to="<jid>" from="<owner>">
    <communication xmlns="wyliodrin" gadgetid="jid" port="<port>">
      "<data encoded in base64>"
    </communication>
  </message>
the data is docoded, packed in a JSON or msgpack:
::
  {
    "from":"message sender",
    "data":"decoded data"
  }
and the packed message is published on ``communication_client:<port>``.

The ``communication`` module is subscribed on the ``communication_server:*``
channels. When the module receives a message on one of the channels, it is
loaded from its JSON or msgpack form, and the ``data`` attribute is encoded
in base64 and sent via XMPP:
::
  <message to="<jid>">
    <communication xmlns="wyliodrin" port="<port>
      "<data encoded in base64>"
    </communication>
  </message>

.. note::
  The publisher is ``libwyliodrin``.

.. note::
  The ``port`` attribute is got from the subscribed channel
  ``communication_server:<port>``
