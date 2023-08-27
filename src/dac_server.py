import socket
import array
import time
import matplotlib.pyplot as plt
import numpy as np

# adress of server
host = '192.168.178.55'
port = 12345

# get 3 seconds data
numOfsamples = 48000
sizeOfSample = 2 # 2 byte

hostname=socket.gethostname()
IPAddr=socket.gethostbyname(hostname)
print("Your Computer Name is:"+hostname)
print("Your Computer IP Address is:"+IPAddr)

mySock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
mySock.connect((host, port))
buf = mySock.recv(1)
# just close to trigger receiving mode
mySock.close()


############### send
# send back
for i in range(3):
    time.sleep(1)
    print("Sleep: " + str(i))

#fD = open("Test_UnSigned8bit_PCM.raw","rb")
#fD = open("random_16kHz_uint8_t.raw","rb")
fD = open("Test_Signed16bit_PCM.raw","rb")


mySock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
mySock.connect((host, port))


# send data in binary
try:
    while(True):
        #mySock.recv(1)
        binaryData = fD.read(1024)
        if not binaryData:
            break
        mySock.send(binaryData)
        print("Send " + str( len(binaryData) ) + " bytes done")
except Exception as e:
    print("Exception: " + str(e))


fD.close()
mySock.close()
