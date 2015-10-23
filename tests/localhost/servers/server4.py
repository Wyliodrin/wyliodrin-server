# Test connection error logs

import BaseHTTPServer, SimpleHTTPServer
import SocketServer
import ssl
import os
import json


class ServerHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):
  post_index = 0

  def do_POST(self):
    print "POST"
    if ServerHandler.post_index != 0:
      length = int(self.headers['Content-Length'])
      r_json = json.loads(self.rfile.read(length).decode('utf-8'))

      if ServerHandler.post_index == 1:
        if r_json["str"].startswith("[INFO: ") and "XMPP connection established" in r_json["str"]:
          print "Second log is INFO about XMPP connection success"
          os._exit(0)
        else:
          print "Second log is not INFO about XMPP connection success"
          os._exit(1)

    ServerHandler.post_index += 1


httpd = BaseHTTPServer.HTTPServer(('localhost', 443), ServerHandler)
httpd.socket = ssl.wrap_socket(httpd.socket, certfile='./certificate/server.pem', server_side=True)

print "Test server up and running"
httpd.serve_forever()
