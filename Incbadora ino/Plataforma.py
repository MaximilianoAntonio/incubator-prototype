import serial
from PyQt6 import uic, QtWidgets
from PyQt6.QtWidgets import QMainWindow, QFileDialog
from PyQt6.QtCore import QTimer, QDateTime, Qt
import sys
import csv
import pyqtgraph as pg
import numpy as np
import mysql.connector

class Plataforma(QMainWindow):
    def __init__(self):
        super().__init__()
        uic.loadUi("Plataforma.ui", self)

        # Configurar conexión serial
        try:
            self.serial_port = serial.Serial('COM8', 115200, timeout=1)
        except serial.SerialException as e:
            print(f"Error al conectar con el puerto COM8: {e}")
            self.serial_port = None

        # Configurar conexión a la base de datos
        try:
            self.conn = mysql.connector.connect(
                host="localhost",
                user="root",
                database="Incubadora"
            )
            self.cursor = self.conn.cursor()
        except mysql.connector.Error as e:
            print(f"Error al conectar a la base de datos: {e}")
            self.conn = None
            self.cursor = None

        # Variables para registro de datos
        self.datos = []
        self.registrando = False
        self.control_automatico = False

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
        self.timer.start(300)

        # Configurar temporizador para el registro periódico en base de datos
        self.timer_registro = QTimer()
        self.timer_registro.timeout.connect(self.registrar_periodico)
        self.timer_registro.start(60000)

        # Valores iniciales
        self.potencia_luz = 0
        self.potencia_ventilador = 0

        # Configurar listas para gráficos
        self.tiempos = []
        self.temperaturas = []
        self.velocidades = []
        self.potencias_luz = []
        self.potencias_luz_aplicada = []
        self.potencias_ventilador = []
        self.estado_alarma = []

        # Configurar gráficos en tiempo real
        self.graficos_setup()

    def graficos_setup(self):
        self.win = pg.GraphicsLayoutWidget(show=True)
        self.layout_graficos.addWidget(self.win)

        self.plot_temp = self.win.addPlot(title="Temperatura vs Tiempo")
        self.curve_temp = self.plot_temp.plot(pen='r')

        self.win.nextRow()

        self.plot_vel = self.win.addPlot(title="Velocidad Ventilador vs Tiempo")
        self.curve_vel = self.plot_vel.plot(pen='b')
        self.plot_vel.setXLink(self.plot_temp)

        self.win.nextRow()

        self.plot_potencia_luz = self.win.addPlot(title="Potencia Luz (%) vs Tiempo")
        self.curve_potencia_luz = self.plot_potencia_luz.plot(pen='g')
        self.plot_potencia_luz.setXLink(self.plot_temp)

        self.win.nextRow()

        self.plot_potencia_luz_aplicada = self.win.addPlot(title="Potencia Luz Aplicada (%) vs Tiempo")
        self.curve_potencia_luz_aplicada = self.plot_potencia_luz_aplicada.plot(pen='m')
        self.plot_potencia_luz_aplicada.setXLink(self.plot_temp)

        self.win.nextRow()

        self.plot_potencia_vent = self.win.addPlot(title="Potencia Ventilador vs Tiempo")
        self.curve_potencia_vent = self.plot_potencia_vent.plot(pen='y')
        self.plot_potencia_vent.setXLink(self.plot_temp)

    def toggle_control_automatico(self):
        if self.Controlbox.isChecked():
            self.control_automatico = True
            if self.serial_port:
                self.serial_port.write(b'AUTOMATIC_ON\n')
            self.slider_luz.setEnabled(False)
            self.slider_ventilador.setEnabled(True)
            self.slider_setpoint.setEnabled(True)
            self.label_control_estado.setText("Control automático")
        else:
            self.control_automatico = False
            if self.serial_port:
                self.serial_port.write(b'AUTOMATIC_OFF\n')
            self.slider_luz.setEnabled(True)
            self.slider_ventilador.setEnabled(True)
            self.slider_setpoint.setEnabled(False)
            self.label_control_estado.setText("Control manual")

    def enviar_luz(self, valor):
        if self.serial_port:
            comando = f'LUZ {valor}\n'
            self.serial_port.write(comando.encode('utf-8'))
        self.lcdLuz.display(valor)
        Amperios = np.interp(valor, [0, 100], [0, 1.6])
        RadiacionLuz = (12 * Amperios) * 0.03
        RadiacionCalor = (12 * Amperios) - RadiacionLuz
        self.label_Radiacion_2.setText(f'Radiación Luz: {RadiacionLuz:.1f} W')
        self.label_Radiacion.setText(f'Radiación Calor: {RadiacionCalor:.1f} W')
        self.potencia_luz = valor

    # Demás métodos se mantienen igual con los mismos ajustes.

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
            # Encabezado del CSV
            self.csv_writer.writerow(['Tiempo (s)', 'Temperatura (°C)', 'Potencia Luz (%)', 'Potencia Luz Aplicada (%)', 'Velocidad Ventilador (RPM)', 'Potencia Ventilador (%)'])
            self.registrando = True
            self.tiempo_inicial = QDateTime.currentDateTime()
            self.statusBar().showMessage('Registro iniciado')

            # Reiniciar datos
            self.tiempos = []
            self.temperaturas = []
            self.velocidades = []
            self.potencias_luz = []
            self.potencias_luz_aplicada = []
            self.potencias_ventilador = []
            self.estado_alarma = []

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
                if len(datos) == 4:
                    temp = float(datos[0])
                    velocidad = float(datos[1])
                    potencia_luz_aplicada_val = float(datos[2])
                    estado_alarma_val = datos[3]

                    # Actualizar etiquetas
                    self.label_temperatura.setText(f'Temperatura: {temp:.1f} °C')
                    self.label_velocidad.setText(f'Velocidad: {int(velocidad)} RPM')
                    self.label_potencia_luz.setText(f'Potencia Luz: {potencia_luz_aplicada_val:.0f}%')

                    # Actualizar listas para gráficos
                    tiempo_actual = QDateTime.currentDateTime()
                    tiempo_transcurrido = self.tiempo_inicial.msecsTo(tiempo_actual) / 1000.0 if self.registrando else tiempo_actual.toSecsSinceEpoch()

                    self.tiempos.append(tiempo_transcurrido)
                    self.temperaturas.append(temp)
                    self.velocidades.append(velocidad)
                    self.estado_alarma.append(estado_alarma_val)

                    # Dependiendo del modo de control, agregamos datos de potencia luz
                    if self.control_automatico:
                        self.potencias_luz_aplicada.append(potencia_luz_aplicada_val)
                        self.potencias_luz.append(None)
                    else:
                        self.potencias_luz_aplicada.append(None)
                        self.potencias_luz.append(self.potencia_luz)

                    self.potencias_ventilador.append(self.potencia_ventilador)

                    # Si estamos registrando en CSV, escribimos la línea correspondiente
                    if self.registrando:
                        if self.control_automatico:
                            # En control automático, la potencia luz manual no se registra, solo la aplicada
                            fila = [tiempo_transcurrido, temp, '', potencia_luz_aplicada_val, velocidad, self.potencia_ventilador]
                        else:
                            # En modo manual, se registra potencia luz manual y no la aplicada
                            fila = [tiempo_transcurrido, temp, self.potencia_luz, '', velocidad, self.potencia_ventilador]

                        self.csv_writer.writerow(fila)

                    # Actualizar gráficos
                    self.actualizar_graficos()

    def registrar_periodico(self):
        # Se toma el último valor de cada lista para registrar en la BD
        if self.temperaturas and self.velocidades and (self.potencias_luz or self.potencias_luz_aplicada):
            temp = self.temperaturas[-1]
            velocidad = self.velocidades[-1]

            # Intentamos usar la última potencia de luz aplicada o manual según corresponda
            # Si hay control automático, potencias_luz_aplicada tendrá datos válidos
            # Si es manual, potencias_luz tendrá datos válidos
            # Aquí, por simplicidad, usamos potencias_luz_aplicada si existe, sino potencias_luz.
            if any(p is not None for p in self.potencias_luz_aplicada):
                ultima_pot_aplicada = [p for p in self.potencias_luz_aplicada if p is not None][-1]
            else:
                # Tomamos la manual
                ultima_pot_aplicada = [p for p in self.potencias_luz if p is not None][-1]

            Amperios = np.interp(ultima_pot_aplicada, [0, 100], [0, 1.6])
            RadiacionLuz = (12*Amperios)*0.03
            RadiacionCalor = (12*Amperios) - RadiacionLuz

            estado_alarma_val = self.estado_alarma[-1] if self.estado_alarma else '0'

            self.registrar_en_base_datos(temp, velocidad, RadiacionCalor, estado_alarma_val)

    def registrar_en_base_datos(self, temperatura, velocidad, potencia_luz_aplicada, estado_alarma):
        """Registra los datos en la base de datos MySQL."""
        try:
            # Convertir valores a tipos compatibles con MySQL
            temperatura = float(temperatura)
            velocidad = float(velocidad)
            potencia_luz_aplicada = float(potencia_luz_aplicada)
            estado_alarma = str(estado_alarma)

            self.cursor.execute('''
                INSERT INTO registros (temperatura, velocidad, potencia, estado_alarma)
                VALUES (%s, %s, %s, %s)
            ''', (temperatura, velocidad, potencia_luz_aplicada, estado_alarma))
            self.conn.commit()
        except (mysql.connector.Error, ValueError) as e:
            print(f"Error al insertar datos: {e}")

    def actualizar_graficos(self):
        self.curve_temp.setData(self.tiempos, self.temperaturas)
        self.curve_vel.setData(self.tiempos, self.velocidades)
        self.curve_potencia_vent.setData(self.tiempos, self.potencias_ventilador)

        # Filtrar datos válidos para potencia luz (manual)
        tiempos_potencia_luz = [t for t, p in zip(self.tiempos, self.potencias_luz) if p is not None]
        potencias_luz_validas = [p for p in self.potencias_luz if p is not None]

        if tiempos_potencia_luz and potencias_luz_validas:
            self.curve_potencia_luz.setData(tiempos_potencia_luz, potencias_luz_validas)
        else:
            self.curve_potencia_luz.clear()

        # Filtrar datos válidos para potencia luz aplicada (automático)
        tiempos_potencia_luz_aplicada = [t for t, p in zip(self.tiempos, self.potencias_luz_aplicada) if p is not None]
        potencias_luz_aplicadas_validas = [p for p in self.potencias_luz_aplicada if p is not None]

        if tiempos_potencia_luz_aplicada and potencias_luz_aplicadas_validas:
            self.curve_potencia_luz_aplicada.setData(tiempos_potencia_luz_aplicada, potencias_luz_aplicadas_validas)
        else:
            self.curve_potencia_luz_aplicada.clear()

    def closeEvent(self, event):
        """Cierra la conexión a la base de datos al salir del programa."""
        if self.conn.is_connected():
            self.cursor.close()  # Cierra el cursor
            self.conn.close()  # Cierra la conexión a la base de datos
        event.accept()

def main():
    app = QtWidgets.QApplication(sys.argv)
    ventana = Plataforma()
    ventana.show()
    sys.exit(app.exec())

if __name__ == '__main__':
    main()