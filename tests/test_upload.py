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



class Upload(ElementBase):

    """
    <upload xmlns="wyliodrin">
      <msgpack>X</msgpack>
    </upload>
    """

    name = 'upload'
    namespace = 'wyliodrin'
    plugin_attrib = 'upload'
    interfaces = set(('msgpack',))
    sub_interfaces = interfaces



class TestUploadBot(sleekxmpp.ClientXMPP):

  def __init__(self, jid, password, recipient, path):
    sleekxmpp.ClientXMPP.__init__(self, jid, password)

    self.recipient = recipient
    self.path = path

    self.add_event_handler("session_start", self.start, threaded=True)

    self.register_handler(
      Callback('Some custom message',
        StanzaPath('message/upload'),
        self._handle_action))

    self.add_event_handler('custom_action',
      self._handle_action_event,
      threaded=True)

    register_stanza_plugin(Message, Upload)


  def start(self, event):
    self.send_presence()
    self.get_roster()

    # <message to="wyliodrin_test@wyliodrin.org">
    #   <upload xmlns="wyliodrin">
    #     <msgpack>X</msgpack>
    #   </upload>
    # </message>
    msg = self.Message()
    msg['lang'] = None
    msg['to'] = self.recipient

    msg['upload']['msgpack'] = base64.b64encode(msgpack.packb(
      [ActionCode.ATTRIBUTES, self.path]))
    msg.send()


  def _handle_action(self, msg):
    self.event('custom_action', msg)


  def _handle_action_event(self, msg):
    if msg['upload']['msgpack'] != '':
      logging.info(msgpack.unpackb(base64.b64decode(msg['upload']['msgpack'])))

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
