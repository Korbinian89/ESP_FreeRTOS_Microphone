import socket
import array

# IP-Adresse und Port des Servers
host = '192.168.178.55'
port = 12345
numOfsamples = 2048 
sizeOfSample = 2 # 2 byte

hostname=socket.gethostname()
IPAddr=socket.gethostbyname(hostname)
print("Your Computer Name is:"+hostname)
print("Your Computer IP Address is:"+IPAddr)

# Socket erstellen und an den angegebenen Port binden
#server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
#server_socket.bind((host, port))

# Auf eingehende Verbindungen warten
#server_socket.listen()
#print("Server gestartet. Warte auf Verbindung...")

# Verbindung akzeptieren
#client_socket, address = server_socket.accept()
#print("Verbunden mit: ", address)

mySock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
mySock.connect((host, port))

n    = numOfsamples * sizeOfSample
data = bytearray()
while len(data) < n:
    packet = mySock.recv(n - len(data))
    print ( "Number of received: " + str(len(packet)))
    if not packet:
        break
    data.extend(packet)
dataInt = array.array('h', data)
print("Received:")
for d in dataInt:
    print(str(d))
    break

#data = mySock.recv(1024)
#print("Received: " + str(data))

mySock.close()

#while True:
#    # Daten empfangen
#    print("Warten auf Daten:")
#    data = client_socket.recv(32768).decode()
#    if not data:
#        break
#    print("Empfangene Daten: ", data)
#    break#

# Verbindung beenden
#client_socket.close()
#server_socket.close()