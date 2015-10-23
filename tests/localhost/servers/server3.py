# Test connection error logs

import BaseHTTPServer, SimpleHTTPServer
import SocketServer
import ssl
import os
import json


class ServerHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):
  l = []

  def do_POST(self):
    length = int(self.headers['Content-Length'])
    r_json = json.loads(self.rfile.read(length).decode('utf-8'))
    ServerHandler.l += r_json["str"].split("[")[1:]

    if len(ServerHandler.l) >= 3:
      if ServerHandler.l[1].startswith("ERROR: ") and "XMPP connection error" in ServerHandler.l[1]:
        print "Second log is ERROR about XMPP connection error"
      else:
        print "Second log is not ERROR about XMPP connection error"
        os._exit(1)

      if ServerHandler.l[2].startswith("ERROR: ") and "Retrying to connect" in ServerHandler.l[2]:
        print "Third log is ERROR about XMPP connection retry"
        os._exit(0)
      else:
        print "Third log is not ERROR about XMPP connection retry"
        os._exit(1)

httpd = BaseHTTPServer.HTTPServer(('localhost', 443), ServerHandler)
httpd.socket = ssl.wrap_socket(httpd.socket, certfile='./certificate/server.pem', server_side=True)

print "Test server up and running"
httpd.serve_forever()
