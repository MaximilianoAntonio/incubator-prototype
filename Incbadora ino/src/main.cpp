#include <Arduino.h>
#include <PID_v1.h>

// Definición de pines
#define NTC_PIN A0  // Pin NTC
#define LUZ_PIN 3  // Pin Luz
#define VENT_PIN 5  // Pin Ventilador
#define VEL_PIN 2   // Pin Velocidad Tacómetro
#define LED_VERDE_PIN 10  // Pin LED verde
#define LED_AMARILLO_PIN 11 // Pin LED amarillo
#define LED_ROJO_PIN 12    // Pin LED rojo

// Constantes para el cálculo de la temperatura
#define A_COEFF 0.5458630405e-3
#define B_COEFF 2.439180157e-4
#define C_COEFF -0.0003705076153e-7

// Variables para mediciones y control
float resistenciaNTC, logResistencia, tempKelvin, tempCelsius;
int lecturaNTC, potenciaVentilador, potenciaLuz;
volatile int contadorImpulsos = 0;
volatile float velocidadRPM = 0;
unsigned long tiempoAnteriorTemp, tiempoAnteriorVel, tiempoImprimir, ultimoTiempoRPM, tiempoAnteriorLeds;

// Variables para lectura de comandos
String comando, estadoAlarma;

// Variables PID
double Setpoint, Input, Output;
double Kp = 30.0, Ki = 1.0, Kd = 0.0;
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

// Variables para control automático
bool control_automatico = false;

// Arreglos para almacenar últimas 5 lecturas
float ultimasTemp[5] = {0};
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
  tiempoAnteriorLeds = millis();

  // Configuración de pines
  pinMode(NTC_PIN, INPUT);
  pinMode(LUZ_PIN, OUTPUT);
  pinMode(VENT_PIN, OUTPUT);
  pinMode(VEL_PIN, INPUT);
  pinMode(LED_VERDE_PIN, OUTPUT);
  pinMode(LED_AMARILLO_PIN, OUTPUT);
  pinMode(LED_ROJO_PIN, OUTPUT);
  pinMode(8, OUTPUT); // 5v fake

  digitalWrite(8, HIGH);

  // Configuración de interrupción para medir velocidad del ventilador
  attachInterrupt(digitalPinToInterrupt(VEL_PIN), medirVelocidad, FALLING);
  myPID.SetMode(AUTOMATIC);
  myPID.SetOutputLimits(0, 100);
}

void loop() {
  // Medición de temperatura cada 250 ms
  if (millis() - tiempoAnteriorTemp >= 250) {
    lecturaNTC = analogRead(NTC_PIN);

    if (lecturaNTC > 0) {
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
    } else {
      tempCelsius = NAN; // Marcar como desconectado si la lectura es inválida
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
      velocidadRPM = (pulsos / 2.0) * (60000.0 / tiempoTranscurrido);
    } else {
      velocidadRPM = 0; // RPM en 0 si no hay pulsos
    }

    ultimoTiempoRPM = tiempoActual;
    tiempoAnteriorVel = tiempoActual;
  }

  // Impresión de datos cada 300 ms
  if (millis() - tiempoImprimir >= 300) {
    Serial.print(calcularPromedio(ultimasTemp, 5));
    Serial.print(";");
    Serial.print(velocidadRPM);
    Serial.print(";");
    Serial.print(Output);  // Potencia de salida
    Serial.print(";");
    Serial.println(estadoAlarma);  // Estado de la alarma


    tiempoImprimir = millis();
  }

  // Control de LEDs y buzzer basado en temperatura promedio
  if (millis() - tiempoAnteriorLeds >= 100) {
    tiempoAnteriorLeds = millis();

    float tempPromedio = calcularPromedio(ultimasTemp, 5);
    bool ntcDesconectado = isnan(tempCelsius);

    // Operación normal (verde)
    if (tempPromedio >= Setpoint - 1 && tempPromedio <= Setpoint + 1 && !ntcDesconectado && velocidadRPM > 0) {
      digitalWrite(LED_VERDE_PIN, HIGH);
      digitalWrite(LED_AMARILLO_PIN, LOW);
      digitalWrite(LED_ROJO_PIN, LOW);
      estadoAlarma = "VERDE";

    // Alerta moderada (amarillo)
    } else if ((tempPromedio > Setpoint + 1 && tempPromedio <= Setpoint + 3) || 
               (tempPromedio < Setpoint - 1 && tempPromedio >= Setpoint - 3)) {
      digitalWrite(LED_VERDE_PIN, LOW);
      digitalWrite(LED_AMARILLO_PIN, HIGH);
      digitalWrite(LED_ROJO_PIN, LOW);
      estadoAlarma = "AMARILLO";
    // Alerta crítica (rojo y buzzer)
    } else if (tempPromedio > Setpoint + 3 || tempPromedio < Setpoint - 3 || ntcDesconectado || velocidadRPM == 0 ) {
      digitalWrite(LED_VERDE_PIN, LOW);
      digitalWrite(LED_AMARILLO_PIN, LOW);
      digitalWrite(LED_ROJO_PIN, HIGH);
      estadoAlarma = "ROJO";

    }
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
    } else if (comando.startsWith("VENT")) {
      potenciaVentilador = map(comando.substring(5).toInt(), 0, 100, 0, 255);
      analogWrite(VENT_PIN, potenciaVentilador);
    }
  }
}
