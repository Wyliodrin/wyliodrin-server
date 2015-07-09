****************
Signals handling
****************

.. highlight:: c

Protocol
========
The signals sent while running the ``projectid`` project are pushed
in a ``redis`` list named ``projectid``. Then, ``libwyliodrin`` publishes
on the ``wyliodrin`` channel the message ``signal:<projectid>`` if the
list contains data encoded in ``JSON`` format or ``signalmp:<projectid>`` if
the list contains data encoded in ``msgpack`` format.

``wyliodrin-server`` is subscribed on the ``wyliodrin`` channel. When a message
is received on this channel, the values from the ``projectid`` list are
retrieved, packed in a JSON or msgpack message:
::
  {
    "projectid":
    "gadgetid":
    "data": [
      "<values retrieved from the projectid list>"
    ]
  }
and sent using the ``libcurl`` library to
``http://projects.wyliodrin.com/signals/send``.
