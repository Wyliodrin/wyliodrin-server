#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
  Test file upload from device to cloud.
"""

import sys
import logging
import getpass
import ssl
from optparse import OptionParser

import sleekxmpp
from sleekxmpp import Message
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



class ActionCode(object):
  ATTRIBUTES = 0
  LIST       = 1
  READ       = 2



class FileType(object):
  DIRECTORY = 0
  REGULAR   = 1
  OTHER     = 2



class W(ElementBase):

    """
    <w xmlns="wyliodrin" d="<msgpack_data>"/>
    """

    name = 'w'
    namespace = 'wyliodrin'
    plugin_attrib = 'w'
    interfaces = set(('d',))



class TestUploadBot(sleekxmpp.ClientXMPP):

  def __init__(self, jid, password, recipient, path):
    sleekxmpp.ClientXMPP.__init__(self, jid, password)

    self.recipient = recipient
    self.path = path

    self.add_event_handler("session_start", self.start, threaded=True)

    self.register_handler(
      Callback('Some custom message',
        StanzaPath('message/w'),
        self._handle_action))

    self.add_event_handler('custom_action',
      self._handle_action_event,
      threaded=True)

    register_stanza_plugin(Message, W)


  def start(self, event):
    self.send_presence()
    self.get_roster()

    """
    <message to="<self.recipient>">
      <w xmlns="wyliodrin" d="<msgpack_data>"/>
    </message>
    """
    msg = self.Message()
    msg['lang'] = None
    msg['to'] = self.recipient

    # f = function
    # u = upload
    # c = code
    # p = path
    msg['w']['d'] = base64.b64encode(msgpack.packb(
      {'sf':'u', 'nc':ActionCode.ATTRIBUTES, 'sp':self.path}))
    msg.send()


  def _handle_action(self, msg):
    self.event('custom_action', msg)


  def _handle_action_event(self, msg):
    if msg['w']['d'] == '':
      logging.error("No data")
      self.disconnect(wait=True)
      return

    decoded = msgpack.unpackb(base64.b64decode(msg['w']['d']))
    # logging.info(decoded)

    if decoded['c'] == ActionCode.ATTRIBUTES:
      logging.info(decoded)
      if decoded['t'] == FileType.DIRECTORY:
        msg = self.Message()
        msg['lang'] = None
        msg['to'] = self.recipient
        msg['w']['d'] = base64.b64encode(msgpack.packb(
          {"nc":ActionCode.LIST, "sp":self.path}))
        msg.send()
      elif decoded['t'] == FileType.REGULAR:
        msg = self.Message()
        msg['lang'] = None
        msg['to'] = self.recipient
        msg['w']['d'] = base64.b64encode(msgpack.packb(
          {"nc":ActionCode.READ, "sp":self.path}))
        msg.send()

    elif decoded['c'] == ActionCode.READ:
      logging.info("READ")
      if decoded['o'] != '':
        if decoded['o'] == 0:
          f = open(decoded['p'] + "_", "w")
        else:
          f = open(decoded['p'] + "_", "a")
        f.seek(decoded['o'], 0)
        f.write(decoded['d'])
        f.close()
        if (decoded['e'] == 1):
          self.disconnect(wait=True)
      else:
        logging.error("Not a valid file")
        self.disconnect(wait=True)

    else:
      self.disconnect(wait=True)


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
  optp.add_option("--path", dest="path",
          help="file path to get from device")

  opts, args = optp.parse_args()

  # Setup logging.
  logging.basicConfig(level=opts.loglevel,
            format='%(levelname)-8s %(message)s')

  if opts.jid is None:
    opts.jid = raw_input("Username: ")
  if opts.password is None:
    opts.password = getpass.getpass("Password: ")
  if opts.to is None:
    opts.to = raw_input("Send To: ")
  if opts.path is None:
    opts.path = raw_input("Path: ")

  xmpp = TestUploadBot(opts.jid, opts.password, opts.to, opts.path)
  xmpp.register_plugin('xep_0030') # Service Discovery
  xmpp.register_plugin('xep_0199') # XMPP Ping

  xmpp.ssl_version = ssl.PROTOCOL_SSLv3

  # Connect to the XMPP server and start processing XMPP stanzas.
  if xmpp.connect():
    xmpp.process(block=True)
    print("Done")
  else:
    print("Unable to connect.")
