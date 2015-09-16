#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
  Board
"""

import gdb
import sys
import logging
import getpass
import ssl
import os
import signal
import threading
import time

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



JID  = "wyliodrin_board@wyliodrin.org"
PASS = "wyliodrin"
loaded_projects = []

PROJECT = ""
COMMAND = ""
MESSAGE = None
ID = None
QUIT = False

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



class Priority(ElementBase):
  name = 'priority'
  plugin_attrib = 'priority'



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

    MESSAGE = self.Message()
    MESSAGE['lang'] = None
    MESSAGE['to'] = "wyliodrin_test@wyliodrin.org"


  def _handle_action(self, msg):
    self.event('custom_action', msg)


  def _handle_action_event(self, msg):

    global PROJECT
    global COMMAND
    global MESSAGE
    global ID

    # data = msgpack.unpackb(base64.b64decode())

    self.pipeout.write(msg['w']['d'])
    self.pipeout.flush()


    # if b'p' not in data:
    #   logging.info("No project in data")
    #   return

    # project = data[b'p'].decode("utf-8")
    # PROJECT = project
    # if project not in loaded_projects:
    #   gdb.execute('file ' + project)
    #   loaded_projects.append(project)

    # if b'd' in data:
    #   disassemble_func = data[b'd']

    #   result = {}
    #   for func in disassemble_func:
    #     o = gdb.execute('disassemble ' + func.decode("utf-8"), to_string=True)
    #     result[func.decode("utf-8")] = o

    #   response = self.Message()
    #   response['lang'] = None
    #   response['to'] = msg['from']
    #   response['w']['d'] = base64.b64encode(msgpack.packb(
    #     {
    #     "p" : project,
    #     "d" : result
    #     })).decode("utf-8")
    #   response.send()

    # if b'b' in data:
    #   breakpoints = data[b'b']

    #   for breakpoint in breakpoints:
    #     gdb.execute("break " + breakpoint.decode("utf-8"))

    # if b'w' in data:
    #   watchpoints = data[b'w']

    #   for watchpoint in watchpoints:
    #     gdb.execute("watch " + watchpoint.decode("utf-8"))

    # if b'c' in data:
    #   cmd = data[b'c'].decode("utf-8")
    #   cid = data[b'i'].decode("utf-8")

    #   fcntl.flock(self.fd, fcntl.LOCK_EX)
    #   fd.write(cmd)
    #   fcntl.flock(self.fd, fcntl.LOCK_UN)




class Worker(threading.Thread):
  def __init__(self, pipein):
    threading.Thread.__init__(self)
    self.pipein = pipein

  def run(self):
    global MESSAGE

    while True:
      content = os.read(self.pipein.fileno(), 3 * 1024).decode("utf-8")

      MESSAGE['w']['d'] = content
      MESSAGE.send()



if __name__ == '__main__':
  # Create the commands and results pipes
  if not os.path.exists(gdb_commands_pipe_name):
    os.mkfifo(gdb_commands_pipe_name)
  if not os.path.exists(gdb_results_pipe_name):
    os.mkfifo(gdb_results_pipe_name)

  gdb_commands_pipe_fd = open(gdb_commands_pipe_name, 'w')
  gdb_results_pipe_fd  = open(gdb_results_pipe_name,  'r')

  worker = Worker(gdb_results_pipe_fd)
  worker.start()

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
