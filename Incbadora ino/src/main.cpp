#include <Arduino.h>
#include <PID_v1.h>

// Definición de pines
#define NTC_PIN A0 // Pin NTC
#define LUZ_PIN 11 // Pin Luz
#define VENT_PIN 9 // Pin Ventilador
#define VEL_PIN 2 // Pin Velocidad Tacómetro

// Constantes para el cálculo de la temperatura
#define A_COEFF 0.5458630405e-3
#define B_COEFF 2.439180157e-4
#define C_COEFF -0.0003705076153e-7

// Variables para mediciones y control
float resistenciaNTC, logResistencia, tempKelvin, tempCelsius;
int lecturaNTC, potenciaVentilador, potenciaLuz;
volatile int contadorImpulsos = 0;
volatile float velocidadRPM = 0;
unsigned long tiempoAnteriorTemp, tiempoAnteriorVel, tiempoImprimir, ultimoTiempoRPM;

// Variables para lectura de comandos
String comando;

// Variables PID
double Setpoint, Input, Output;
double Kp = 30.0, Ki = 1.0, Kd = 0.0;
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

// Variables para control automático
bool control_automatico = false;

// Arreglos para almacenar últimas 5 lecturas
float ultimasRPM[5] = {0};
float ultimasTemp[5] = {0};
int indiceRPM = 0;
int indiceTemp = 0;

// Funciones para calcular promedios
float calcularPromedio(float *valores, int tamano) {
  float suma = 0;
  for (int i = 0; i < tamano; i++) {
    suma += valores[i];
  }
  return suma / tamano;
}

// Función de interrupción para contar los impulsos del ventilador
void medirVelocidad() {
  contadorImpulsos++;
}

void setup() {
  Serial.begin(115200);

  // Inicialización de tiempos
  tiempoAnteriorTemp = millis();
  tiempoAnteriorVel = millis();
  tiempoImprimir = millis();
  ultimoTiempoRPM = millis();

  // Configuración de pines
  pinMode(NTC_PIN, INPUT);
  pinMode(LUZ_PIN, OUTPUT);
  pinMode(VENT_PIN, OUTPUT);
  pinMode(VEL_PIN, INPUT_PULLUP);

  // Configuración de interrupción para medir velocidad del ventilador
  attachInterrupt(digitalPinToInterrupt(VEL_PIN), medirVelocidad, FALLING);
  myPID.SetMode(AUTOMATIC);
  myPID.SetOutputLimits(0, 100);
}

void loop() {
  // Medición de temperatura cada 250 ms
  if (millis() - tiempoAnteriorTemp >= 250) {
    lecturaNTC = analogRead(NTC_PIN);
    resistenciaNTC = 100000.0 * ((1023.0 / lecturaNTC) - 1.0);
    logResistencia = log(resistenciaNTC);
    tempKelvin = 1.0 / (A_COEFF + B_COEFF * logResistencia + C_COEFF * pow(logResistencia, 3));
    tempCelsius = tempKelvin - 273.15;

    ultimasTemp[indiceTemp] = tempCelsius;
    indiceTemp = (indiceTemp + 1) % 5;

    if (control_automatico) {
      Input = calcularPromedio(ultimasTemp, 5);
      myPID.Compute();
      analogWrite(LUZ_PIN, map(Output, 0, 100, 0, 255));
    }

    tiempoAnteriorTemp = millis();
  }

  // Cálculo de la velocidad del ventilador cada 1000 ms
  if (millis() - tiempoAnteriorVel >= 1000) {
    unsigned long tiempoActual = millis();
    unsigned long tiempoTranscurrido = tiempoActual - ultimoTiempoRPM;

    noInterrupts();
    int pulsos = contadorImpulsos;
    contadorImpulsos = 0;
    interrupts();

    if (tiempoTranscurrido > 0) {
      velocidadRPM = (pulsos) * (6000.0 / tiempoTranscurrido);
    }

    ultimasRPM[indiceRPM] = velocidadRPM;
    indiceRPM = (indiceRPM + 1) % 5;

    ultimoTiempoRPM = tiempoActual;
    tiempoAnteriorVel = tiempoActual;
  }

  // Impresión de datos cada 300 ms
  if (millis() - tiempoImprimir >= 300) {
    Serial.print(calcularPromedio(ultimasTemp, 5));
    Serial.print(";");
    Serial.print(calcularPromedio(ultimasRPM, 5));
    Serial.print(";");
    Serial.println(Output);

    tiempoImprimir = millis();
  }

  // Lectura de comandos desde Serial
  if (Serial.available() > 0) {
    comando = Serial.readStringUntil('\n');

    if (comando == "AUTOMATIC_ON") {
      control_automatico = true;
    } else if (comando == "AUTOMATIC_OFF") {
      control_automatico = false;
    } else if (comando.startsWith("SETPOINT") && control_automatico) {
      Setpoint = comando.substring(9).toDouble();
    } else if (comando.startsWith("LUZ") && !control_automatico) {
      potenciaLuz = map(comando.substring(4).toInt(), 0, 100, 0, 255);
      analogWrite(LUZ_PIN, potenciaLuz);
    } else if (comando.startsWith("VENT") && !control_automatico) {
      potenciaVentilador = map(comando.substring(5).toInt(), 0, 100, 0, 255);
      if (potenciaVentilador > 0) {
        analogWrite(VENT_PIN, 255);
        delay(200);
      }
      analogWrite(VENT_PIN, potenciaVentilador);
    }
  }
}