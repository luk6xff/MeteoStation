import socket
import struct
import sys
import time

def get_time_from_ntp(ntp_server_addr='time.nist.gov'):
    #time gap in seconds from 01.01.1900 (NTP time) to 01.01.1970 (UNIX time)
    TIME_DIFF_1900_TO_1970 = 2208988800
    client = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) #UDP
    data = '\x1b' + 47 * '\0'
    client.sendto(data.encode(), (ntp_server_addr, 123))
    data, address = client.recvfrom( 1024 )
    if data:
        print('Response received from address:', address)
        print(data)
        print("DATA LEN: %d"%len(data))
        epoch = data[40]<<24 | data[41]<<16 | data[42]<<8 | data[43] # manual extracting
        print('\tepoch=%d' % (epoch))
        epoch = epoch - TIME_DIFF_1900_TO_1970
        print('\t@Unix epoch=%d, Time=%s' % (epoch,time.ctime(epoch)))
        
        t = struct.unpack('!12I', data) # extracting using struct
        t = t[10] - TIME_DIFF_1900_TO_1970
        print('\t@@Unix epoch=%d, Time=%s' % (t,time.ctime(t)))
        
get_time_from_ntp()#'pool.ntp.org')