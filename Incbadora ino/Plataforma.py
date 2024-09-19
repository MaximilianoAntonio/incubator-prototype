import serial 
import matplotlib.pyplot as plt
from PyQt6 import uic, QtWidgets
from PyQt6.QtWidgets import QMessageBox, QMainWindow, QApplication
from PyQt6.QtCore import QTimer
import sys


class Plataforma(QMainWindow):
    def __init__(self):
        super().__init__()
        uic.loadUi("Plataforma.ui", self)
        
        # Configurar conexión serial
        self.serial_port = serial.Serial('COM10', 115200, timeout=1)

        # Conectar señales de los sliders
        self.slider_luz.valueChanged.connect(self.enviar_luz)
        self.slider_ventilador.valueChanged.connect(self.enviar_ventilador)

        # Configurar temporizador para leer datos del Arduino
        self.timer = QTimer()
        self.timer.timeout.connect(self.leer_datos)
        self.timer.start(300)  # Cada 300 ms

    def enviar_luz(self, valor):
        comando = f'LUZ {valor}\n'
        self.serial_port.write(comando.encode('utf-8'))

    def enviar_ventilador(self, valor):
        comando = f'VENT {valor}\n'
        self.serial_port.write(comando.encode('utf-8'))

    def leer_datos(self):
        while self.serial_port.in_waiting > 0:
            linea = self.serial_port.readline().decode('utf-8').strip()
            if linea:
                datos = linea.split(';')
                if len(datos) == 2:
                    temp = datos[0]
                    velocidad = datos[1]
                    self.label_temperatura.setText(f'Temperatura: {temp} °C')
                    self.label_velocidad.setText(f'Velocidad: {velocidad} RPM')

def main():
    app = QtWidgets.QApplication(sys.argv)
    ventana = Plataforma()
    ventana.show()
    sys.exit(app.exec())

if __name__ == '__main__':
    main()


