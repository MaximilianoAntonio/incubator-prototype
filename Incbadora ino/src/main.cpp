#include <Arduino.h>

// Definición de pines
#define NTC_PIN A0
#define LUZ_PIN 3
#define VENT_PIN 5
#define VEL_PIN 2

// Constantes para el cálculo de la temperatura
#define A_COEFF 0.5458630405e-3
#define B_COEFF 2.439180157e-4
#define C_COEFF -0.0003705076153e-7

// Variables para medición de temperatura y control
float resistenciaNTC, logResistencia, tempKelvin, tempCelsius; 
int lecturaNTC, potenciaVentilador, potenciaLuz, velocidadVentilador, contadorImpulsos; 
float velocidadRPM; 
unsigned long tiempoAnteriorTemp, tiempoAnteriorVel, tiempoActual, tiempoImprimir, deltaTiempo;

// Variables para lectura de comandos
String comando, valorLuz, valorVentilador;

// Función de interrupción para contar los impulsos del ventilador
void medirVelocidad() {
  contadorImpulsos++;
  deltaTiempo = millis() - tiempoActual;
}

void setup() {
  Serial.begin(115200);

  // Inicialización de tiempos
  tiempoAnteriorTemp = millis();
  tiempoAnteriorVel = millis();
  tiempoImprimir = millis();
  tiempoActual = millis();

  // Configuración de pines
  pinMode(NTC_PIN, INPUT); 
  pinMode(LUZ_PIN, OUTPUT); 
  pinMode(VENT_PIN, OUTPUT);
  pinMode(VEL_PIN, INPUT);

  // Configuración de interrupción para medir velocidad del ventilador
  attachInterrupt(digitalPinToInterrupt(VEL_PIN), medirVelocidad, FALLING);
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

    tiempoAnteriorTemp = millis(); // Actualizar el tiempo de la última medición de temperatura
  }

  // Cálculo de la velocidad del ventilador cada 500 ms 
  if (millis() - tiempoAnteriorVel >= 500) {
    if (deltaTiempo > 0) {
      velocidadRPM = (contadorImpulsos / deltaTiempo) * 60000.0; // Conversión a RPM (revoluciones por minuto)

      // Reiniciar el tiempo y contador
      tiempoActual = millis();
      contadorImpulsos = 0;
      deltaTiempo = 0;
    }

    tiempoAnteriorVel = millis(); // Actualizar el tiempo de la última medición de velocidad
  }

  // Impresión de datos cada 300 ms
  if (millis() - tiempoImprimir >= 300) {

    Serial.println(tempCelsius);
    Serial.print(";");
    Serial.println(velocidadRPM);

    tiempoImprimir = millis(); // Actualizar el tiempo de la última impresión del menú
  }


  if (Serial.available() > 0) {

    comando = Serial.readStringUntil('\n');

    // Comando para controlar la luz
    if (comando.startsWith("LUZ")) {
      valorLuz = comando.substring(4);
      potenciaLuz = valorLuz.toInt();
      potenciaLuz = map(potenciaLuz, 0, 100, 0, 255);
      analogWrite(LUZ_PIN, potenciaLuz);
    }

    // Comando para controlar el ventilador
    else if (comando.startsWith("VENT")) {
      valorVentilador = comando.substring(5);
      velocidadVentilador = valorVentilador.toInt();
      velocidadVentilador = map(velocidadVentilador, 0, 100, 0, 255);
      analogWrite(VENT_PIN, velocidadVentilador);
    }
  }

}
