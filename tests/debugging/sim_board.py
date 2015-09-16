#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
  Board
"""

import getpass
import logging
import os
import signal
import sleekxmpp
import ssl
import sys
import threading
import time

from sleekxmpp import Message, Presence
from sleekxmpp.xmlstream import ElementBase
from sleekxmpp.xmlstream import register_stanza_plugin
from sleekxmpp.xmlstream.handler import Callback
from sleekxmpp.xmlstream.matcher import StanzaPath



# Python versions before 3.0 do not use UTF-8 encoding
# by default. To ensure that Unicode is handled properly
# throughout SleekXMPP, we will set the default encoding
# ourselves to UTF-8.
if sys.version_info < (3, 0):
  from sleekxmpp.util.misc_ops import setdefaultencoding
  setdefaultencoding('utf8')
else:
  raw_input = input



JID  = "wyliodrin_board@wyliodrin.org"
PASS = "wyliodrin"

MESSAGE = None

gdb_commands_pipe_name = "/tmp/gdb_commands"
gdb_results_pipe_name  = "/tmp/gdb_results"



class W(ElementBase):
  """
  <w xmlns="wyliodrin" d="<msgpack_data>"/>
  """

  name = 'w'
  namespace = 'wyliodrin'
  plugin_attrib = 'w'
  interfaces = set(('d',))



class SimBoard(sleekxmpp.ClientXMPP):
  def __init__(self, jid, password, pipeout):
    sleekxmpp.ClientXMPP.__init__(self, jid, password)

    self.pipeout = pipeout

    self.add_event_handler("session_start", self.start, threaded=False)

    self.register_handler(
      Callback('Some custom message',
        StanzaPath('message/w'),
        self._handle_action))

    self.add_event_handler('custom_action',
      self._handle_action_event,
      threaded=True)

    register_stanza_plugin(Message, W)


  def start(self, event):
    global MESSAGE

    # Send priority
    prio = self.Presence()
    prio['lang'] = None
    prio['to'] = None
    prio['priority'] = '50'
    prio.send()

    # Save message
    MESSAGE = self.Message()
    MESSAGE['lang'] = None
    MESSAGE['to'] = "wyliodrin_test@wyliodrin.org"


  def _handle_action(self, msg):
    self.event('custom_action', msg)


  def _handle_action_event(self, msg):
    self.pipeout.write(msg['w']['d'])
    self.pipeout.flush()



class Listener(threading.Thread):
  def __init__(self, pipein):
    threading.Thread.__init__(self)
    self.pipein = pipein

  def run(self):
    global MESSAGE

    while True:
      # Get result
      content = os.read(self.pipein.fileno(), 3 * 1024).decode("utf-8")

      MESSAGE['w']['d'] = content
      MESSAGE.send()



if __name__ == '__main__':
  # Create the commands and results pipes
  if not os.path.exists(gdb_commands_pipe_name):
    os.mkfifo(gdb_commands_pipe_name)
  if not os.path.exists(gdb_results_pipe_name):
    os.mkfifo(gdb_results_pipe_name)

  # Open pipes
  gdb_commands_pipe_fd = open(gdb_commands_pipe_name, 'w')
  gdb_results_pipe_fd  = open(gdb_results_pipe_name,  'r')

  listener = Listener(gdb_results_pipe_fd)
  listener.start()

  # Setup logging.
  logging.basicConfig(level=logging.DEBUG,
            format='%(levelname)-8s %(message)s')

  xmpp = SimBoard(JID, PASS, gdb_commands_pipe_fd)
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
