import socket
import sounddevice as sd
import numpy as np

# IP-Adresse und Port des ESP32
esp_ip = "192.168.178.55"  # Beispiel-IP-Adresse des ESP32
esp_port = 12345       # Beispiel-Port des ESP32

# Konfiguration für die Audioausgabe
sample_rate = 16000
channels = 1

# Funktion zum Empfangen und Abspielen von Audio
def play_audio(data, frames, time, status):
    audio_data = np.frombuffer(data, dtype=np.int16)  # Annahme von 16-Bit-Signed-Ints
    sd.play(audio_data, sample_rate)

# Socket erstellen und verbinden
mySock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
mySock.connect((esp_ip, esp_port))

print("Warten auf Daten vom ESP32...")

# Audioausgabe starten
with sd.OutputStream(callback=play_audio, channels=channels, samplerate=sample_rate):
    while True:
        data, addr = mySock.recvfrom(1024)  # Puffergröße anpassen, falls erforderlich
        print("Recveved")
        if len(data) == 0:
            break
        # Daten empfangen und verarbeiten, falls nötig
        # Hier wird angenommen, dass die Daten direkt als 16-Bit-Signed-Ints empfangen werden

print("Beendet")