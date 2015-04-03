__author__ = 'amaurial'

# !/usr/bin/python3

import socketserver
from datetime import datetime

class MyTCPHandler(socketserver.BaseRequestHandler):
    """
    The RequestHandler class for our server.

    It is instantiated once per connection to the server, and must
    override the handle() method to implement communication to the
    client.
    """

    def handle(self):
        # self.request is the TCP socket connected to the client
        while True:
            self.data = self.request.recv(1024).strip()
            print("{} wrote:".format(self.client_address[0]))
            print(self.data)
            # just send back the same data, but upper-cased
            #send the time back
            tnow=str(datetime.now()).encode()
            tnow=tnow+"\n".encode()
            self.request.sendall(tnow)
            if str(self.data).find("quit")>0:
                break

if __name__ == "__main__":
    HOST, PORT = "192.168.1.119", 9999

    # Create the server, binding to localhost on port 9999
    server = socketserver.TCPServer((HOST, PORT), MyTCPHandler)

    # Activate the server; this will keep running until you
    # interrupt the program with Ctrl-C
    try:
        server.serve_forever()
    except Exception as e:
        server.shutdown()
