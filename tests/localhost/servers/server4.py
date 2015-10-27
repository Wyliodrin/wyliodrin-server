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

    if len(ServerHandler.l) >= 2:
      if ServerHandler.l[1].startswith("INFO: ") and "XMPP connection established" in ServerHandler.l[1]:
        print "Second log is INFO about XMPP connection success"
        os._exit(0)
      else:
        print "Second log is not INFO about XMPP connection success"
        os._exit(1)


httpd = BaseHTTPServer.HTTPServer(('localhost', 443), ServerHandler)
httpd.socket = ssl.wrap_socket(httpd.socket, certfile='./certificate/server.pem', server_side=True)

print "Test server up and running"
httpd.serve_forever()
