# Test logs are sent to <domain>/gadgets/logs/<jid>

import BaseHTTPServer, SimpleHTTPServer
import SocketServer
import ssl
import os
import sys

if len(sys.argv) != 2:
    print "usage: %s <jid>" % (sys.argv[0])
    os._exit(1)

jid = sys.argv[1]

class ServerHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):

    def do_POST(self):
        print "path %s" % (self.path)
        if self.path == "/gadgets/logs/" + jid:
            print "Path matches expected path"
            os._exit(0)
        else:
            print "Path does not match expected path"
            os._exit(1)


httpd = BaseHTTPServer.HTTPServer(('localhost', 443), ServerHandler)
httpd.socket = ssl.wrap_socket(httpd.socket, certfile='./certificate/server.pem', server_side=True)

print "Test server up and running"
httpd.serve_forever()
