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
sizeOfSample = 2 # 1 byte

hostname=socket.gethostname()
IPAddr=socket.gethostbyname(hostname)
print("Your Computer Name is:"+hostname)
print("Your Computer IP Address is:"+IPAddr)

mySock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
mySock.connect((host, port))

n = numOfsamples * sizeOfSample
data = bytearray()
while len(data) < n:
    packet = mySock.recv(n - len(data))
    print ( "Number of received: " + str(len(packet)))
    if not packet:
        break
    data.extend(packet)
dataInt = array.array('h', data)
print("Received len: " + str(len(data)))
for d in dataInt:
    print("First val: " + str(d))
    break
mySock.close()


# send back
for i in range(5):
    time.sleep(1)
    print("Sleep: " + str(i))

mySock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
mySock.connect((host, port))

# send data in binary
try:
    mySock.send(data)
    print("Send all done")
except Exception as e:
    print("Exception: " + str(e))

mySock.close()
time.sleep(2)


# fft params
Fs = 16000  # samplerate
Ts = 1.0 / Fs
t = np.arange(0,3,Ts)
y = dataInt
n = len(y)

# calculate fft
k = np.arange(n)
T = n/Fs
frq = k/T # two sides frequency range
frq = frq[range(int(n/2))] # one side frequency range
Y = np.fft.fft(y)/n # fft computing and normalization
Y = Y[range(int(n/2))]


fD = open("random_16kHz_int16_t.raw","wb")
fD.write(dataInt)
fD.close()

# plot
fig, ax = plt.subplots(3,1)
ax[0].plot(t,y,'b.-')
ax[0].set_xlabel('Time')
ax[0].set_ylabel('Amplitude')
ax[1].plot(frq,abs(Y),'r') #plotting spectrum
ax[1].set_xlabel('Freq (Hz)')
ax[1].set_ylabel('|Y(freq)|')
ax[2].semilogy(frq,abs(Y), 'r') #plitting the spectrum
ax[2].set_xlabel('Freq (Hz)')
ax[2].set_ylabel('|Y(freq)|')
plt.show()

