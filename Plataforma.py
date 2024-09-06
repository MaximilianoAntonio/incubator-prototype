import serial as sr
import matplotlib.pyplot as plt
import numpy as np
import csv
import time
import sys
from PyQt5 import uic    #agregar libreria de PyQt5 
from PyQt5.QtWidgets import QMainWindow,QApplication,QLabel
from PyQt5 import QtWidgets, QtCore
import pyqtgraph as pg
from PyQt5.QtWidgets import QLCDNumber
import mysql.connector as mysql

s = sr.Serial('COM7', 9600)

temp_array = []
for i in range(10):
    txtArduino = s.readline().decode('ascii')
    datos = txtArduino.split(";")
    temp_array.append(float(datos[0]))
    potencia = datos[1]
    rpm = datos[2]


    class grafico(QMainWindow):
    def __init__(self, parent=None):
        super().__init__()       #iniciar 
        uic.loadUi("interfaz_incubadora2.ui", self) #cargar
        
        #self.Tempertura_display = QLabel(self) 
        self.temp_label.setText(str(temp_array[-1]))
        self.pwm_label.setText(str(potencia))
        self.velocidad_label.setText(str(rpm))
        
        #self.grabando = False       
        
        self.Tref = 0
        
        #Grafico de temperatura   ###############
        self.graphWidget = pg.PlotWidget(self.gridLayoutWidget) 
        self.graphWidget.setObjectName("graphWidget")  #permite realizar cambios en sus propiedades
        self.gridLayout.addWidget(self.graphWidget, 1, 1, 1, 1)
        
        # colocar viñetas a los ejes, graphwidget llama al grafico
        self.graphWidget.plotItem.setLabel('left', 'Temperatura', units='°c')
        self.graphWidget.plotItem.setLabel('bottom', 'Tiempo', units='s')
        self.graphWidget.plotItem.setYRange(20, 35)   #limites eje y
        self.graphWidget.setBackground('k') #cambiar color de fondo
        
        self.x = list(range(1,11))  
        self.y = list(temp_array) 
        
        #Generamos el grafico
        pen = pg.mkPen(color='purple') #asigna color morado al grafico
        self.data_line = self.graphWidget.plot(self.x, self.y, pen=pen) #linea principal que genera grafico
        
        self.filas_csv = []  #guarda filas en csv
        self.tiempo = 0 #Tiempo en milisegundos
        self.comando_variable = "0000000"#comando
        
        self.pushButton.clicked.connect(self.comando)
        self.pushButton_2.clicked.connect(self.ingresar)  
        self.pushButton_3.clicked.connect(self.grabar_base)  #############
        self.pushButton_4.setEnabled(False)   #########
        self.pushButton_4.clicked.connect(self.detener_base)  ##############
        
        self.detener_button_2.setEnabled(False)
        self.grabar_button_2.clicked.connect(self.grabar)
        self.detener_button_2.clicked.connect(self.detener_guardar)  
        
        self.lcd_tiempo = self.findChild(QLCDNumber, 'lcdNumber')  
        self.lcd_potencia = self.findChild(QLCDNumber, 'lcdNumber_2')  
        self.lcd_rpm = self.findChild(QLCDNumber, 'lcdNumber_3')
        self.lcd_temperatura = self.findChild(QLCDNumber, 'lcdNumber_4')
        
        self.timer = QtCore.QTimer() #defino un timer
        self.timer.setInterval(500) #Tiempo de actualizacion del grafico en ms
        self.timer.timeout.connect(self.update_data) #una vez pasa el tiempo manda llamar funcion update_plot_data Es la que actualiza el graficoy controla velocidad
        self.timer.start() #reiniciar el timer
        
        # Configurar el temporizador para calcular la frecuencia y el promedio de temperatura cada 10 segundos
        self.timer_base = QtCore.QTimer()
        self.timer_base.setInterval(60000)  # 10 segundos
        self.timer_base.timeout.connect(self.update_base)
        self.timer_base.start()
        
    def grabar_base(self):
        self.grabando = True
        self.pushButton_4.setEnabled(True)   ##detener
        self.pushButton_3.setEnabled(False)  ##grabar
        
    def detener_base(self):
        self.grabando = False
        self.pushButton_4.setEnabled(False)
        self.pushButton_3.setEnabled(True)
    
    #Funcion para mysql    
    def update_base(self): #temperatura,potencia,velocidad
        if self.grabando == True:
            txtArduino = s.readline().decode('ascii')
            datos = txtArduino.split(";")
            temp = datos[0]
            potencia = datos[1]
            rpm = datos[2]
        
            con = mysql.connect(host = "localhost", user = "root", database = "base_datos_incubadora")
            cur = con.cursor()
            query="INSERT INTO registro_incubadora(id,Temperatura,Potencia,Velocidad) VALUES('','%s','%s','%s')" % (''.join(str(temp)+"°C"),''.join(str(potencia)+"W"),''.join(str(rpm)+"RPM")) 
        
            cur.execute(query) 
            con.commit()
            con.close()
        
    #Funcion para actualizar los datos    
    def update_data(self):
        txtArduino = s.readline().decode('ascii')
        datos = txtArduino.split(";")
        temp = datos[0]
        potencia = datos[1]
        rpm = datos[2]
        
        self.temp_label.setText(str(temp))
        self.pwm_label.setText(str(potencia))
        self.velocidad_label.setText(str(rpm))
        
        #OJO guarda muchos datos puede haber problema de memoria
        self.filas_csv.append([self.tiempo,float(temp),float(potencia),float(rpm)])
        
        self.tiempo = self.tiempo + 1 ########################################################################################
        ##################################################interfaz 2
        self.lcd_tiempo.display(self.tiempo)
        self.lcd_potencia.display(potencia)
        self.lcd_rpm.display(float(rpm))
        self.lcd_temperatura.display(float(temp))
        
        self.x = self.x[1:]  # Elimina primer elemento de x
        self.x.append(self.x[-1] + 1) 
        
        self.y = self.y[1:]  # Elimina primer elemento de y
        self.y = np.append(self.y,float(temp))
        self.data_line.setData(self.x, self.y) 

    def comando(self):
        self.comando_variable = self.linea_comando.text()
        s.write(self.comando_variable.encode('ascii'))
                
    def grabar(self):
        self.detener_button_2.setEnabled(True)
        self.grabar_button_2.setEnabled(False)
        self.filas_csv = [] ###Reinicia lista para comenzar a registrar cuando se presiona boton
        self.tiempo = 0
        
    def detener_guardar(self):
        self.grabar_button_2.setEnabled(True)
        self.detener_button_2.setEnabled(False)
        nombre_csv = self.nombre_csv_2.text() + ".csv"
        
        with open(nombre_csv,"w",newline='') as csvfile:
            writer = csv.writer(csvfile, delimiter=';')
            writer.writerow(["tiempo","temperatura","Potencia","RPM"])
            writer.writerows(self.filas_csv)
            
    def ingresar(self):
        #self.data_line = self.graphWidget.plot(self.x, self.y, pen=pg.mkPen('m'))  # Agrega una nueva línea de Tref
        self.Tref = self.lineEdit.text()
        s.write(self.Tref.encode('ascii'))
        self.tiempo = 0
    
if __name__ == "__main__":
    app = QApplication(sys.argv)
    myapp = grafico() #myapp ejecuta la clase que estas haciendo
    myapp.show() #hasta el show es por pauta, por definición
    app.aboutToQuit.connect(app.deleteLater)   #Estas dos lineas permite cerrar adecuadamente la aplicacion
    app.exec_()