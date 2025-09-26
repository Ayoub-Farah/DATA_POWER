import subprocess
import sys

def install_package(package_name):
    """Install the package using pip if it's not installed."""
    try:
        __import__(package_name)
    except ImportError:
        print(f"{package_name} not found. Installing...")
        subprocess.check_call([sys.executable, "-m", "pip", "install", package_name])

# Vérification et installation des packages nécessaires
install_package('numpy')
install_package('matplotlib')
install_package('pandas')


import serial
import struct
import os
import io
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from datetime import datetime

IDLE = 0
RECORD = 1

class RecordedDatas:
    """
    1. Attend une séquence "begin record" pour démarrer l'enregistrement.
    2. Chaque ligne est supposée contenir 8 octets en hexadécimal.
    Les 8 octets sont convertis en flottant et enregistrés dans un fichier.
    3. Attendre une séquence "end record" pour terminer l'enregistrement.
    4 - Provide with a message size
    """

    def __init__(self, port, message_size, baudrate=115200):
        self.serial_port = serial.Serial(port, baudrate, timeout=1)
        self.serial_port.set_buffer_size(rx_size = 1000*10*4, tx_size = 1000*10*4)
        self.state = IDLE
        self.buffer = ""
        self.message_size = message_size
        self.f = io.StringIO()
        print("Connection established on", port)

    def read_serial(self):
        """Lire les données du port série et traiter les messages."""
        cpt = 0
        while True:
            data = self.serial_port.read(self.serial_port.in_waiting or 1).decode('utf-8', errors='replace')
            self.buffer += data
            if '\n' in self.buffer:
                datas_left = ''
                cr_idx = self.buffer.rfind('\n')
                txt_to_read = self.buffer[:cr_idx]
                
                
                # Détection de "begin record"
                if 'begin record' in txt_to_read:
                    idx = txt_to_read.find('begin record\r\n')
                    txt_to_read = txt_to_read[idx+self.message_size:]
                    self.state = RECORD

                # Détection de "end record"
                if 'end record' in txt_to_read:
                # if len(datas_left) == 0 :
                    idx = txt_to_read.find('end record')
                    txt_to_read = txt_to_read[:idx]
                    
                    
                    lines = txt_to_read.split("\r\n")
                    datas_left = self.save_datas(lines)
                    self.state = IDLE
                    
                

                    # Sauvegarder les données dans un fichier
                    return self.save_to_file()

                    

                # Enregistrer les données si en mode RECORD
                if self.state == RECORD:
                    lines = txt_to_read.split("\r\n")
                    datas_left = self.save_datas(lines)

                self.buffer = datas_left + self.buffer[cr_idx:]
            # print("buffer : ", self.buffer, "\n")
            # # print("data : ", data, "\n")
            # print("data left length : ", len(datas_left), "\n")
            # print("txt_to_read : ", txt_to_read, "\n")
            # print("lines : ", lines)


    def save_datas(self, lines):
        """Sauvegarder les données en les convertissant de l'hexadécimal au float."""
        datas_left = ''
        if lines[-1] != '':
            datas_left = lines.pop()
        else:
            lines.pop()

        for line in lines:
            if "#" in line:
                self.f.write("{}\n".format(line[1:]))
            else:
                try:
                    nombre = struct.unpack('>f', bytes.fromhex(line))[0]
                    self.f.write("{}\n".format(nombre))
                except:
                    self.f.write("{}\n".format(line))
        return datas_left

    def save_to_file(self):
        """Enregistre les données dans un fichier et produit un fichier CSV et PNG."""
        path_records = os.path.join(".", "Data_records")
        if not os.path.exists(path_records):
            os.makedirs(path_records)

        file_name = f"{datetime.now().strftime('%Y-%m-%d_%H-%M-%S')}-record"
        file_to_record = os.path.join(path_records, file_name)
        output_text = self.f.getvalue()
        self.f.close()
        self.f = io.StringIO()
        print(len(output_text.split('\n')))

        # Enregistrement dans un fichier texte
        with open(file_to_record + ".txt", "w+") as ff:
            ff.write(output_text)
        

        # Conversion des données en DataFrame
        data_frame = self.to_dataFrame(file_to_record + ".txt")
        data_frame.to_csv(file_to_record + ".csv")

        # Création et sauvegarde d'une figure
        figure = self.plot_df(data_frame)
        figure.savefig(file_to_record + ".png")

        return file_to_record

    def to_dataFrame(self, filename):
        """Convertit les données enregistrées en DataFrame pandas."""
        with open(filename, "r") as f:
            file = f.readlines()
        line1 = file.pop(0)
        if "#" in file[0] or file[0].startswith(" "):
            line2 = file.pop(0)
            idx = int(line2.replace("#", "").strip())
        else:
            idx = None
        datas = np.fromiter(file, dtype=float)
        names = line1.replace("#", "").split(",")
        datas = datas.reshape(-1, len(names) - 1)
        if idx:
            datas = np.roll(datas, -(idx + 1), axis=0)
        df = pd.DataFrame(datas, columns=names[:-1])
        return df

    def plot_df(self, df):
        """Tracer le DataFrame avec matplotlib."""
        fig, axs = plt.subplots(3, 1)
        tics = np.arange(len(df))

        # try:
        #     df["V_Low_estim"] = df["duty_cycle"] * df["V_high"]
        # except:
        #     pass

        if "k_acquire" in df:
            del df["k_acquire"]

        for s in df:
            if s.startswith("V"):
                axs[0].step(tics, df[s], label=s)
            elif s.startswith("I"):
                axs[1].step(tics, df[s], label=s)
            else:
                axs[2].step(tics, df[s], label=s)

        for ax in axs:
            ax.legend()
            ax.grid()

        return fig

    def close(self):
        """Fermer la connexion série."""
        self.serial_port.close()

# Exemple d'utilisation
# if __name__ == "__main__":
#     port = 'COM9'  # Replace with serial port
#     baudrate = 115200
#     recorded_data = RecordedDatas(port, baudrate)

#     try:
#         recorded_data.read_serial()
#     except KeyboardInterrupt:
#         print("Process interrupted by user.")
#     finally:
#         recorded_data.close()
