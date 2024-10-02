#include <Arduino.h>
#include <PID_v1.h>

// Definición de pines
#define NTC_PIN A0 // pin NTC
#define LUZ_PIN 11  // pin Luz
#define VENT_PIN 9 // pin Ventilador
#define VEL_PIN 2  // pin Velocidad Tacometro

// Constantes para el cálculo de la temperatura
#define A_COEFF 0.5458630405e-3
#define B_COEFF 2.439180157e-4
#define C_COEFF -0.0003705076153e-7

// Variables para medición de temperatura y control
float resistenciaNTC, logResistencia, tempKelvin, tempCelsius;
int lecturaNTC, potenciaVentilador, potenciaLuz, velocidadVentilador;
volatile int contadorImpulsos = 0; // Declarar como volatile
volatile float velocidadRPM = 0;   // Declarar como volatile
unsigned long tiempoAnteriorTemp, tiempoAnteriorVel, tiempoImprimir;
unsigned long ultimoTiempoRPM;

// Variables para lectura de comandos
String comando, valorLuz, valorVentilador;

// Variables PID
double Setpoint, Input, Output;
double Kp = 30.0, Ki = 1.0, Kd = 0.0;
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

// Variables para control automático
bool control_automatico = false;

// Definir un arreglo para almacenar las últimas 5 lecturas de RPM
float ultimasRPM[5] = {0, 0, 0, 0, 0}; // Inicialmente con ceros
int indiceRPM = 0;  // Índice para llevar la cuenta de la posición actual
float promedioRPM ; // Variable para almacenar el promedio de las últimas 5 lecturas de RPM

// Definir un arreglo para almacenar las últimas 5 lecturas de temperatura
float ultimasTemp[5] = {0, 0, 0, 0, 0}; // Inicialmente con ceros
int indiceTemp = 0;  // Índice para llevar la cuenta de la posición actual

// Función para calcular el promedio de los últimos 5 valores de RPM
float calcularPromedioRPM() {
  float suma = 0;
  for (int i = 0; i < 5; i++) {
    suma += ultimasRPM[i];
  }
  return suma / 5;  // Devolver el promedio
}

// Función para calcular el promedio de los últimos 5 valores de temperatura
float calcularPromedioTemp() {
  float suma = 0;
  for (int i = 0; i < 5; i++) {
    suma += ultimasTemp[i];
  }
  return suma / 5;  // Devolver el promedio
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
  pinMode(VEL_PIN, INPUT);

  // Configuración de interrupción para medir velocidad del ventilador
  attachInterrupt(digitalPinToInterrupt(VEL_PIN), medirVelocidad, FALLING);
  myPID.SetMode(AUTOMATIC);
  myPID.SetOutputLimits(0, 100); 
}

void loop() {

  // Medición de temperatura cada 250 ms
  if (millis() - tiempoAnteriorTemp >= 250) {
    // Lectura del sensor de temperatura
    lecturaNTC = analogRead(NTC_PIN);
    resistenciaNTC = 100000.0 * ((1023.0 / lecturaNTC) - 1.0); // Cálculo de la resistencia
    logResistencia = log(resistenciaNTC); // Logaritmo natural de la resistencia
    tempKelvin = 1.0 / (A_COEFF + B_COEFF * logResistencia + C_COEFF * pow(logResistencia, 3)); // Conversión a grados Kelvin
    tempCelsius = tempKelvin - 273.15; // Conversión a grados Celsius

    // Actualizar el arreglo con la nueva lectura de temperatura
    ultimasTemp[indiceTemp] = tempCelsius;
    indiceTemp = (indiceTemp + 1) % 5;  // Mover el índice circularmente entre 0 y 4

    // Si el control automático está activado, actualizamos el PID
    if (control_automatico) {
      Input = calcularPromedioTemp();
      myPID.Compute();
      analogWrite(LUZ_PIN, map(Output, 0, 100, 0, 255));
    }

    tiempoAnteriorTemp = millis(); // Actualizar el tiempo de la última medición de temperatura
  }

  // Cálculo de la velocidad del ventilador cada 1000 ms

  if (millis() - tiempoAnteriorVel >= 1000) {
    unsigned long tiempoActual = millis();
    unsigned long tiempoTranscurrido = tiempoActual - ultimoTiempoRPM;

    // Deshabilitar interrupciones mientras se lee y reinicia el contador
    noInterrupts();
    int pulsos = contadorImpulsos;
    contadorImpulsos = 0; // Reiniciar el contador de pulsos
    interrupts();

    if (tiempoTranscurrido > 0 && pulsos > 0) {
      // Suponiendo que el ventilador genera 2 pulsos por revolución
      velocidadRPM = (pulsos) * (6000.0 / tiempoTranscurrido);
    } else if (tiempoTranscurrido > 0 && pulsos == 0) {
      velocidadRPM = 0;  // Si no hay pulsos, la velocidad es 0
    }

    // Actualizar el arreglo con la nueva lectura de RPM
    ultimasRPM[indiceRPM] = velocidadRPM;
    indiceRPM = (indiceRPM + 1) % 5;  // Mover el índice circularmente entre 0 y 4

    // Calcular el promedio de las últimas 5 lecturas de RPM
    promedioRPM = calcularPromedioRPM();

    ultimoTiempoRPM = tiempoActual;  // Actualizar el tiempo de referencia
    tiempoAnteriorVel = tiempoActual; // Actualizar el tiempo de la última medición de velocidad
  }

  // Impresión de datos cada 300 ms
  if (millis() - tiempoImprimir >= 300) {
    Serial.print(calcularPromedioTemp()); // Imprimir el promedio de temperatura
    Serial.print(";");
    Serial.print(promedioRPM);  
    Serial.print(";");
    Serial.println(Output); 

    tiempoImprimir = millis(); // Actualizar el tiempo de la última impresión
  }

  if (Serial.available() > 0) {
    comando = Serial.readStringUntil('\n');

    // Activar o desactivar el control automático
    if (comando == "AUTOMATIC_ON") {
      control_automatico = true;  // Activar el control automático
    }
    if (comando == "AUTOMATIC_OFF") {
      control_automatico = false;  // Desactivar el control automático (modo manual)
    }

    // Comando para ajustar el setpoint y activar el control automático
    if (comando.startsWith("SETPOINT") && control_automatico) {
      String valorSetpoint = comando.substring(9);  // Extraer el valor del setpoint
      Setpoint = valorSetpoint.toDouble();  // Actualizar el setpoint del PID
    }

    // Comando para controlar la luz
    else if (comando.startsWith("LUZ") && !control_automatico) {
      valorLuz = comando.substring(4);
      potenciaLuz = valorLuz.toInt();
      potenciaLuz = map(potenciaLuz, 0, 100, 0, 255);
      analogWrite(LUZ_PIN, potenciaLuz);
    }

    // Comando para controlar el ventilador
    else if (comando.startsWith("VENT") && !control_automatico) {
      valorVentilador = comando.substring(5);
      velocidadVentilador = valorVentilador.toInt();
      velocidadVentilador = map(velocidadVentilador, 0, 100, 0, 255);
      // Corriente de arranque
      if (velocidadVentilador > 0) {
        analogWrite(VENT_PIN, 255);
        delay(200);
        analogWrite(VENT_PIN, velocidadVentilador);
      } else if (velocidadVentilador == 0) {
        analogWrite(VENT_PIN, velocidadVentilador);
      }
    }
  }
}
