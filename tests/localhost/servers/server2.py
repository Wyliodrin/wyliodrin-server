# Test first log is info about startup

import BaseHTTPServer, SimpleHTTPServer
import SocketServer
import ssl
import os
import json

class ServerHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):

    def do_POST(self):
        length = int(self.headers['Content-Length'])
        r_json = json.loads(self.rfile.read(length).decode('utf-8'))

        if r_json["str"].startswith("[INFO: ") and "Starting wyliodrin-server" in r_json["str"]:
            print "First log is INFO about startup"
            os._exit(0)
        else:
            print "First log is not INFO about startup"
            os._exit(1)


httpd = BaseHTTPServer.HTTPServer(('localhost', 443), ServerHandler)
httpd.socket = ssl.wrap_socket(httpd.socket, certfile='./certificate/server.pem', server_side=True)

print "Test server up and running"
httpd.serve_forever()
