import socket
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import collections
import array

# Konfiguration des Servers
SERVER_IP = '192.168.178.55'  # Dummy - enter ip of esp
SERVER_PORT = 12345  # Dummy - enter port

# Konfiguration des Plotters
SAMPLE_SIZE = 2  # Größe eines Samples in Byte
MAX_SAMPLES_PER_READ = 16000  # Maximale Anzahl an Samples pro Read - 4 sekunden - sample rate 16384
BUFFER_SIZE = SAMPLE_SIZE * MAX_SAMPLES_PER_READ

# Anzahl der Samples, die im beweglichen Fenster angezeigt werden sollen
WINDOW_SIZE = MAX_SAMPLES_PER_READ * 4

adc_buffer = collections.deque(maxlen=WINDOW_SIZE)

# Funktion zum Erstellen eines Socket-Clients und Verbinden zum Server
def connect_to_server():
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client_socket.connect((SERVER_IP, SERVER_PORT))
    return client_socket

# Funktion zum Lesen von Daten vom Server und Konvertieren in ADC-Werte
def read_adc_data(client_socket):
    data = client_socket.recv(BUFFER_SIZE)
    # adc values are shifted to 0 with +/- instead of 0 to Uint16_t_Max
    adc_values = np.frombuffer(data, dtype=np.int16)
    return adc_values

# Funktion zur Animation des Plots
def animate(i):
    adc_values = read_adc_data(client_socket)
    print("Num of samples recv:" + str(len(adc_values)))
    
    adc_buffer.extend(adc_values)

    #line.set_data(time, graphData)
    line.set_data(range(WINDOW_SIZE), adc_buffer)
    return line,

# Einrichten des Plots
xmin = 0
xmax = WINDOW_SIZE
ymin = -30000
ymax = 30000

fig = plt.figure()
ax = plt.axes(xlim=(xmin, xmax), ylim=(ymin, ymax))

#fig, ax = plt.subplots()
line = ax.plot([], [], lw=2)[0]

# Festlegen der x- und y-Achsenbeschriftungen
ax.set_xlabel('Samples')
ax.set_ylabel('ADC Value')
ax.set_title('ADC Samples in Echtzeit')

# Verbindung zum Server herstellen
client_socket = connect_to_server()

# Animation starten
ani = animation.FuncAnimation(fig, animate, interval=30)  # Aktualisierungsintervall in Millisekunden

# Plot anzeigen
plt.show()

# Socket-Verbindung schließen, wenn das Programm beendet wird
client_socket.close()