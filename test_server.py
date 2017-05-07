from http.server import BaseHTTPRequestHandler,HTTPServer

PORT_NUMBER = 80
 
class HttpRequestHandler(BaseHTTPRequestHandler):

    #Handler for the GET requests
    def do_GET(self):
        print ('GET request received')
        self.send_response(200)
        self.send_header('Content-type','text/html')
        self.end_headers()
        # Send the html message
        self.wfile.write("Hello World !")
        return

try:
    #Create a web server and define the handler to manage the incoming request
    server = HTTPServer(('', PORT_NUMBER), HttpRequestHandler)
    print ('Started httpserver on port ' , PORT_NUMBER)
    #Wait forever for incoming http requests
    server.serve_forever()
except KeyboardInterrupt:
    pass