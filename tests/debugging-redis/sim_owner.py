#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
  Owner
"""

import sys
import logging
import getpass
import ssl
from optparse import OptionParser
from time import sleep

import sleekxmpp
from sleekxmpp import Message, Presence
from sleekxmpp.xmlstream import ElementBase
from sleekxmpp.xmlstream import register_stanza_plugin
from sleekxmpp.xmlstream.handler import Callback
from sleekxmpp.xmlstream.matcher import StanzaPath

import msgpack
import base64



# Python versions before 3.0 do not use UTF-8 encoding
# by default. To ensure that Unicode is handled properly
# throughout SleekXMPP, we will set the default encoding
# ourselves to UTF-8.
if sys.version_info < (3, 0):
  from sleekxmpp.util.misc_ops import setdefaultencoding
  setdefaultencoding('utf8')
else:
  raw_input = input



class D(ElementBase):
  """
  <d xmlns="wyliodrin" d="<msgpack_data>" n="<irrelevant>"/>
  """

  name = 'd'
  namespace = 'wyliodrin'
  plugin_attrib = 'd'
  interfaces = set(('d', 'n'))



class SimOwner(sleekxmpp.ClientXMPP):
  def __init__(self, jid, password, recipient):
    sleekxmpp.ClientXMPP.__init__(self, jid, password)

    self.recipient = recipient

    self.add_event_handler("session_start", self.start, threaded=True)

    self.register_handler(
      Callback('Some custom message',
        StanzaPath('message/d'),
        self._handle_action))

    self.add_event_handler('custom_action',
      self._handle_action_event,
      threaded=True)

    register_stanza_plugin(Message, D)

    self.last_id = 0

    # Build messages list
    self.messages = [
      {
        "s" : "test",
        "i" : 0
      },
      {
        "p" : "test",
        "i" : 1,
        "d" : ["f", "main"]
      },
      {
        "p" : "test",
        "i" : 2,
        "b" : ["main", "12"]
      },
      {
        "p" : "test",
        "i" : 3,
        "c" : "r"
      },
      {
        "p" : "test",
        "i" : 4,
        "w" : ["a", "b"]
      },
      {
        "p" : "test",
        "i" : 5,
        "c" : "n"
      },
      {
        "p" : "test",
        "i" : 6,
        "c" : "n"
      },
      {
        "p" : "test",
        "i" : 7,
        "c" : "c"
      },
      {
        "p" : "test",
        "i" : 8,
        "c" : "q"
      }
    ]


  def start(self, event):
    # Send priority
    prio = self.Presence()
    prio['lang'] = None
    prio['to'] = None
    prio['priority'] = '50'
    prio.send()

    # Send subscription
    sub = self.Presence()
    sub['lang'] = None
    sub['to'] = self.recipient
    sub['type'] = 'subscribe'
    sub.send()

    # Send status
    stat = self.Presence()
    stat['lang'] = None
    stat['to'] = self.recipient
    stat['status'] = 'Happy'
    stat.send()
    sleep(1)

    msg = self.Message()
    msg['lang'] = None
    msg['to'] = self.recipient
    msg['d']['n'] = "n"
    msg.send()
    sleep(3)

    """
    <message to="<self.recipient>">
      <d xmlns="wyliodrin" d="<msgpack_data>"/>
    </message>
    """

    # Send first message
    msg = self.Message()
    msg['lang'] = None
    msg['to'] = self.recipient
    msg['d']['d'] = base64.b64encode(msgpack.packb(self.messages[self.last_id])).decode("utf-8")
    msg.send()

    self.last_id += 1


  def _handle_action(self, msg):
    self.event('custom_action', msg)


  def _handle_action_event(self, msg):
    decoded = msgpack.unpackb(base64.b64decode(msg['d']['d']))
    logging.info(decoded)

    # Send next message
    msg = self.Message()
    msg['lang'] = None
    msg['to'] = self.recipient
    msg['d']['d'] = base64.b64encode(msgpack.packb(self.messages[self.last_id])).decode("utf-8")
    msg.send()

    self.last_id += 1



if __name__ == '__main__':
  # Setup the command line arguments.
  optp = OptionParser()

  # Output verbosity options.
  optp.add_option('-q', '--quiet', help='set logging to ERROR',
          action='store_const', dest='loglevel',
          const=logging.ERROR, default=logging.INFO)
  optp.add_option('-d', '--debug', help='set logging to DEBUG',
          action='store_const', dest='loglevel',
          const=logging.DEBUG, default=logging.INFO)
  optp.add_option('-v', '--verbose', help='set logging to COMM',
          action='store_const', dest='loglevel',
          const=5, default=logging.INFO)

  # JID and password options.
  optp.add_option("-j", "--jid", dest="jid",
          help="JID to use")
  optp.add_option("-p", "--password", dest="password",
          help="password to use")
  optp.add_option("-t", "--to", dest="to",
          help="JID to send the message to")

  opts, args = optp.parse_args()

  # Setup logging.
  logging.basicConfig(level=opts.loglevel,
            format='%(levelname)-8s %(message)s')

  if opts.jid is None:
    opts.jid = raw_input("jid: ")
  if opts.password is None:
    opts.password = getpass.getpass("password: ")
  if opts.to is None:
    opts.to = raw_input("recipient: ")

  xmpp = SimOwner(opts.jid, opts.password, opts.to)
  xmpp.register_plugin('xep_0030') # Service Discovery
  xmpp.register_plugin('xep_0199') # XMPP Ping

  xmpp.ssl_version = ssl.PROTOCOL_SSLv3
  xmpp.auto_authorize = True
  xmpp.auto_subscribe = True

  # Connect to the XMPP server and start processing XMPP stanzas.
  if xmpp.connect():
    xmpp.process(block=True)
    print("Done")
  else:
    print("Unable to connect.")
