import socket
import struct
import sys
import time

def get_time_from_ntp(ntp_server_addr='time.nist.gov'):
    #time gap in seconds from 01.01.1900 (NTP time) to 01.01.1970 (UNIX time)
    TIME_DIFF_1900_TO_1970 = 2208988800
    client = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) #UDP
    data = '\x1b' + 47 * '\0'
    print(str(data))
    client.sendto(data.encode(), (ntp_server_addr, 123))
    data, address = client.recvfrom( 1024 )
    if data:
        #print(data)
        t = struct.unpack('!12I', data)
        print(t)
        t = t[10] - TIME_DIFF_1900_TO_1970
        print('Response received from address:', address)
        print('\tUnix epoch=%d, Time=%s' % (t,time.ctime(t)))
        
get_time_from_ntp('0.pool.ntp.org')