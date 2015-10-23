# Test connection error logs

import BaseHTTPServer, SimpleHTTPServer
import SocketServer
import ssl
import os
import json


class ServerHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):
  post_index = 0

  def do_POST(self):
    if ServerHandler.post_index != 0:
      length = int(self.headers['Content-Length'])
      r_json = json.loads(self.rfile.read(length).decode('utf-8'))

      if ServerHandler.post_index == 1:
        if r_json["str"].startswith("[ERROR: ") and "XMPP connection error" in r_json["str"]:
          print "Second log is ERROR about XMPP connection error"
        else:
          print "Second log is not ERROR about XMPP connection error"
          os._exit(1)

      if ServerHandler.post_index == 2:
        if r_json["str"].startswith("[ERROR: ") and "Retrying to connect" in r_json["str"]:
          print "Third log is ERROR about XMPP connection retry"
          os._exit(0)
        else:
          print "Third log is not ERROR about XMPP connection retry"
          os._exit(1)

    ServerHandler.post_index += 1


httpd = BaseHTTPServer.HTTPServer(('localhost', 443), ServerHandler)
httpd.socket = ssl.wrap_socket(httpd.socket, certfile='./certificate/server.pem', server_side=True)

print "Test server up and running"
httpd.serve_forever()
