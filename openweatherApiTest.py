# import urllib.request
# try:
    # r = urllib.request.urlopen("http://api.openweathermap.org/data/2.5/weather?units=metric&q=NowySacz&appid=e95bbbe9f7314ea2a5ca1f60ee138eef\r\n").read()
    # #"http://api.openweathermap.org/data/2.5/weather?units=metric&q=NowySacz&appid=e95bbbe9f7314ea2a5ca1f60ee138eef\r\nHTTP/1.1\r\nHost: api.openweathermap.org:80\r\n"
    # print(r)
# except:
    # pass
import socket
import os

CRLF = "\r\n\r\n"
def getRequest(host, port, path=None, raw=False):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(0.30)
    s.connect((host, port))
    if raw:
        s.send(path.encode())
    else:
        s.send("GET /".encode()+path.encode()+"HTTP/1.1".encode()+CRLF.encode())       
    data = s.recv(100000);
    print(data)
    s.close()
    #print('Received', repr(data))

#getRequest("api.openweathermap.org", 80, "data/2.5/weather?units=metric&q=NowySacz&appid=e95bbbe9f7314ea2a5ca1f60ee138eef\r\n") #working
#getRequest("api.openweathermap.org", 80, "GET /data/2.5/weather?units=metric&q=NowySacz&appid=e95bbbe9f7314ea2a5ca1f60ee138eef\r\nHTTP/1.1\r\n\r\n", True) #working
getRequest("api.openweathermap.org", 80, "GET /data/2.5/weather?q=paris&appid=e95bbbe9f7314ea2a5ca1f60ee138eef\r\n", True)