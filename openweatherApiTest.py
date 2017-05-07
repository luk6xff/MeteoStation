import urllib.request
r = urllib.request.urlopen("http://api.openweathermap.org/data/2.5/weather?units=metric&q=NowySacz&appid=e95bbbe9f7314ea2a5ca1f60ee138eef\r\nHTTP/1.1\r\nHost: api.openweathermap.org:80\r\n").read()

print(r)