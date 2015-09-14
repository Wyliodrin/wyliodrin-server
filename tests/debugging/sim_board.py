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
  def __init__(self, jid, password, condition):
    sleekxmpp.ClientXMPP.__init__(self, jid, password)

    self.condition = condition

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
    # Send priority
    prio = self.Presence()
    prio['lang'] = None
    prio['to'] = None
    prio['priority'] = '50'
    prio.send()


  def _handle_action(self, msg):
    self.event('custom_action', msg)


  def _handle_action_event(self, msg):
    global PROJECT
    global COMMAND
    global MESSAGE
    global ID

    data = msgpack.unpackb(base64.b64decode(msg['w']['d']))

    if b'project' not in data:
      logging.info("No project in data")
      return

    project = data[b'project'].decode("utf-8")
    PROJECT = project
    if project not in loaded_projects:
      gdb.execute('file ' + project)
      loaded_projects.append(project)

    if b'disassemble_func' in data:
      disassemble_func = data[b'disassemble_func']

      result = {}
      for func in disassemble_func:
        o = gdb.execute('disassemble ' + func.decode("utf-8"), to_string=True)
        result[func.decode("utf-8")] = o

      response = self.Message()
      response['lang'] = None
      response['to'] = msg['from']
      response['w']['d'] = base64.b64encode(msgpack.packb(
        {
        "project"          : project,
        "disassemble_func" : result
        })).decode("utf-8")
      response.send()

    if b'breakpoints' in data:
      breakpoints = data[b'breakpoints']

      for breakpoint in breakpoints:
        gdb.Breakpoint(breakpoint.decode("utf-8"))

    if b'command' in data:
      os.system("truncate -s 0 out.log")
      os.system("truncate -s 0 err.log")

      cmd = data[b'command'].decode("utf-8")
      cid = data[b'id'].decode("utf-8")

      self.condition.acquire()

      COMMAND = cmd
      ID = cid
      MESSAGE = self.Message()
      MESSAGE['lang'] = None
      MESSAGE['to'] = msg['from']

      self.condition.notify()
      self.condition.release()



class Worker(threading.Thread):
  def __init__(self, condition):
    threading.Thread.__init__(self)
    self.condition = condition

  def run(self):
    global PROJECT
    global COMMAND
    global MESSAGE
    global ID

    while True:
      self.condition.acquire()
      while True:
        if COMMAND != "":
          if COMMAND == "run":
            o = gdb.execute("run > out.log 2> err.log")
          else:
            o = gdb.execute(COMMAND, to_string=True)
            gdb.execute('call fflush(0)')

          MESSAGE['w']['d'] = base64.b64encode(msgpack.packb(
            {
            "project" : PROJECT,
            "id"      : ID,
            "result"  : o,
            "stdout"  : open("out.log").read(),
            "stderr"  : open("err.log").read()
            })).decode("utf-8")
          MESSAGE.send()
          COMMAND = ""
          break
        self.condition.wait()
      self.condition.release()



if __name__ == '__main__':
  # Setup logging.
  logging.basicConfig(level=logging.DEBUG,
            format='%(levelname)-8s %(message)s')

  cond = threading.Condition()
  worker = Worker(cond)
  worker.start()

  xmpp = SimBoard(JID, PASS, cond)
  xmpp.register_plugin('xep_0030') # Service Discovery
  xmpp.register_plugin('xep_0199') # XMPP Ping

  xmpp.ssl_version = ssl.PROTOCOL_SSLv3

  # Connect to the XMPP server and start processing XMPP stanzas.
  if xmpp.connect():
    xmpp.process(block=True)
    print("Done")
  else:
    print("Unable to connect.")
