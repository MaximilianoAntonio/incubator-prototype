import serial
from PyQt6 import uic, QtWidgets
from PyQt6.QtWidgets import QMainWindow, QFileDialog
from PyQt6.QtCore import QTimer, QDateTime, Qt  
import sys
import csv
import pyqtgraph as pg  
import numpy as np

class Plataforma(QMainWindow):
    def __init__(self):
        super().__init__()
        uic.loadUi("Plataforma.ui", self)
        
        # Configurar conexión serial
        self.serial_port = serial.Serial('COM18', 115200, timeout=1)

        # Variables para registro de datos
        self.datos = []
        self.registrando = False  # Inicialmente no se está registrando datos
        self.control_automatico = False  # Inicialización del control automático

        # Conectar señales de los sliders
        self.slider_luz.valueChanged.connect(self.enviar_luz)
        self.slider_ventilador.valueChanged.connect(self.enviar_ventilador)
        self.slider_setpoint.valueChanged.connect(self.enviar_setpoint) 
        self.Controlbox.stateChanged.connect(self.toggle_control_automatico)

        # Botones para iniciar y detener registro
        self.boton_iniciar.clicked.connect(self.iniciar_registro)
        self.boton_detener.clicked.connect(self.detener_registro)

        # Configurar temporizador para leer datos del Arduino
        self.timer = QTimer()
        self.timer.timeout.connect(self.leer_datos)
        self.timer.start(300)  # Cada 300 ms, ya está activo

        # Valores iniciales
        self.potencia_luz = 0
        self.potencia_ventilador = 0

        # Configurar gráficos en tiempo real
        self.tiempos = []
        self.temperaturas = []
        self.velocidades = []
        self.potencias_luz = []
        self.potencias_luz_aplicada = []
        self.potencias_ventilador = []

        # Configurar gráficos utilizando PyQtGraph
        self.graficos_setup()

    def graficos_setup(self):
        # Crear un widget para los gráficos
        self.win = pg.GraphicsLayoutWidget(show=True)
        self.layout_graficos.addWidget(self.win)  # layout_graficos es un QVBoxLayout en tu interfaz

        # Añadir gráficos
        self.plot_temp = self.win.addPlot(title="Temperatura vs Tiempo")
        self.curve_temp = self.plot_temp.plot(pen='r')

        self.win.nextRow()

        self.plot_vel = self.win.addPlot(title="Velocidad Ventilador vs Tiempo")
        self.curve_vel = self.plot_vel.plot(pen='b')
        self.plot_vel.setXLink(self.plot_temp)  # Sincronizar eje X

        self.win.nextRow()

        self.plot_potencia_luz = self.win.addPlot(title="Potencia Luz (%) vs Tiempo")
        self.curve_potencia_luz = self.plot_potencia_luz.plot(pen='g')
        self.plot_potencia_luz.setXLink(self.plot_temp)  # Sincronizar eje X

        self.win.nextRow()

        self.plot_potencia_luz_aplicada = self.win.addPlot(title="Potencia Luz Aplicada (%) vs Tiempo")
        self.curve_potencia_luz_aplicada = self.plot_potencia_luz_aplicada.plot(pen='m')  # Color magenta
        self.plot_potencia_luz_aplicada.setXLink(self.plot_temp)  # Sincronizar eje X

        self.win.nextRow()

        self.plot_potencia_vent = self.win.addPlot(title="Potencia Ventilador vs Tiempo")
        self.curve_potencia_vent = self.plot_potencia_vent.plot(pen='y')
        self.plot_potencia_vent.setXLink(self.plot_temp)  # Sincronizar eje X

    def toggle_control_automatico(self):
        """
        Esta función activa o desactiva el control automático según el estado del CheckBox.
        """
        if self.Controlbox.isChecked():
            # Activar el control automático (PID)
            self.control_automatico = True
            self.serial_port.write(b'AUTOMATIC_ON\n')  # Enviar comando a Arduino
            self.slider_luz.setEnabled(False)  # Desactivar el control manual
            self.slider_ventilador.setEnabled(False)
            self.slider_setpoint.setEnabled(True)  # Activar el control de setpoint
            self.label_control_estado.setText("Control automático")
        else:
            # Desactivar el control automático (PID)
            self.control_automatico = False
            self.serial_port.write(b'AUTOMATIC_OFF\n')  # Enviar comando a Arduino
            self.slider_luz.setEnabled(True)  # Activar el control manual
            self.slider_ventilador.setEnabled(True)
            self.slider_setpoint.setEnabled(False)  # Desactivar el control de setpoint
            self.label_control_estado.setText("Control manual")

    def enviar_luz(self, valor):
        comando = f'LUZ {valor}\n'
        self.serial_port.write(comando.encode('utf-8'))
        self.lcdLuz.display(valor)
        Amperios = np.interp(valor, [0, 100], [0, 1.6])
        RadiacionLuz = (12*Amperios)*0.03
        RadiacionCalor = (12*Amperios) - RadiacionLuz
        self.label_Radiacion_2.setText(f'Radiación Luz: {RadiacionLuz:.1f} W')
        self.label_Radiacion.setText(f'Radiación Calor: {RadiacionCalor:.1f} W')
        self.potencia_luz = valor  # Guardar el valor actual de la luz

    def enviar_ventilador(self, valor):
        comando = f'VENT {valor}\n'
        self.serial_port.write(comando.encode('utf-8'))
        self.lcdVentilador.display(valor)
        self.potencia_ventilador = valor  # Guardar el valor actual del ventilador

    def enviar_setpoint(self, valor):
        comando = f'SETPOINT {valor}\n'
        self.serial_port.write(comando.encode('utf-8'))
        self.lcdSetpoint.display(valor)

    def iniciar_registro(self):
        # Abrir diálogo para guardar el archivo CSV
        nombre_archivo, _ = QFileDialog.getSaveFileName(self, "Guardar archivo", "", "CSV Files (*.csv)")
        if nombre_archivo:
            self.archivo_csv = open(nombre_archivo, 'w', newline='')
            self.csv_writer = csv.writer(self.archivo_csv)
            # Encabezado del CSV con el símbolo %
            self.csv_writer.writerow(['Tiempo', 'Temperatura (°C)', 'Potencia Luz (%)', 'Potencia Luz Aplicada (%)', 'Velocidad Ventilador (RPM)', 'Potencia Ventilador (%)'])
            self.registrando = True
            self.tiempo_inicial = QDateTime.currentDateTime()
            self.statusBar().showMessage('Registro iniciado')
            self.datos = []  # Reiniciar datos

            # Reiniciar listas para gráficos
            self.tiempos = []
            self.temperaturas = []
            self.velocidades = []
            self.potencias_luz = []
            self.potencias_luz_aplicada = []
            self.potencias_ventilador = []

    def detener_registro(self):
        if self.registrando:
            self.archivo_csv.close()
            self.registrando = False
            self.statusBar().showMessage('Registro detenido')

    def leer_datos(self):
        while self.serial_port.in_waiting > 0:
            linea = self.serial_port.readline().decode('utf-8').strip()
            if linea:
                datos = linea.split(';')
                if len(datos) == 3:
                    temp = float(datos[0])
                    velocidad = float(datos[1])
                    potencia_luz_aplicada = float(datos[2])
                    self.label_temperatura.setText(f'Temperatura: {temp:.1f} °C')
                    self.label_velocidad.setText(f'Velocidad: {int(velocidad)} RPM')
                    self.label_potencia_luz.setText(f'Potencia Luz: {potencia_luz_aplicada:.0f}%')

                    if self.registrando:
                        tiempo_actual = QDateTime.currentDateTime()
                        tiempo_transcurrido = self.tiempo_inicial.msecsTo(tiempo_actual) / 1000.0  # En segundos

                        # Preparar los datos a escribir según el modo de control
                        if self.control_automatico:
                            # En control automático, no guardamos Potencia Luz (%) manual
                            fila = [tiempo_transcurrido, temp, '', potencia_luz_aplicada, velocidad, self.potencia_ventilador]
                        else:
                            # En modo manual, no guardamos Potencia Luz Aplicada (%)
                            fila = [tiempo_transcurrido, temp, self.potencia_luz, '', velocidad, self.potencia_ventilador]

                        # Escribir la fila en el CSV
                        self.csv_writer.writerow(fila)

                        # Actualizar datos para los gráficos
                        self.tiempos.append(tiempo_transcurrido)
                        self.temperaturas.append(temp)
                        self.velocidades.append(velocidad)
                        self.potencias_ventilador.append(self.potencia_ventilador)

                        if self.control_automatico:
                            self.potencias_luz_aplicada.append(potencia_luz_aplicada)
                            self.potencias_luz.append(None)  # No hay potencia luz manual en automático
                        else:
                            self.potencias_luz_aplicada.append(None)  # No hay potencia luz aplicada en manual
                            self.potencias_luz.append(self.potencia_luz)
                        
                        # Actualizar los gráficos en tiempo real
                        self.actualizar_graficos()

    def actualizar_graficos(self):
        self.curve_temp.setData(self.tiempos, self.temperaturas)
        self.curve_vel.setData(self.tiempos, self.velocidades)
        self.curve_potencia_vent.setData(self.tiempos, self.potencias_ventilador)

        # Actualizar el gráfico de potencia luz manual
        tiempos_potencia_luz = [t for t, p in zip(self.tiempos, self.potencias_luz) if p is not None]
        potencias_luz_validas = [p for p in self.potencias_luz if p is not None]
        if tiempos_potencia_luz and potencias_luz_validas:
            self.curve_potencia_luz.setData(tiempos_potencia_luz, potencias_luz_validas)
        else:
            self.curve_potencia_luz.clear()

        # Actualizar el gráfico de potencia luz aplicada
        tiempos_potencia_luz_aplicada = [t for t, p in zip(self.tiempos, self.potencias_luz_aplicada) if p is not None]
        potencias_luz_aplicadas_validas = [p for p in self.potencias_luz_aplicada if p is not None]
        if tiempos_potencia_luz_aplicada and potencias_luz_aplicadas_validas:
            self.curve_potencia_luz_aplicada.setData(tiempos_potencia_luz_aplicada, potencias_luz_aplicadas_validas)
        else:
            self.curve_potencia_luz_aplicada.clear()

def main():
    app = QtWidgets.QApplication(sys.argv)
    ventana = Plataforma()
    ventana.show()
    sys.exit(app.exec())

if __name__ == '__main__':
    main()
