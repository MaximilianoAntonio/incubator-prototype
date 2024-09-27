#include <Arduino.h>

// Definición de pines
#define NTC_PIN A0 // pin NTC
#define LUZ_PIN 3 // pin Luz
#define VENT_PIN 9 // pin Ventilador
#define VEL_PIN 2 // pin Velocidad Tacometro

// Constantes para el cálculo de la temperatura
#define A_COEFF 0.5458630405e-3
#define B_COEFF 2.439180157e-4
#define C_COEFF -0.0003705076153e-7

// Variables para medición de temperatura y control
float resistenciaNTC, logResistencia, tempKelvin, tempCelsius;
int lecturaNTC, potenciaVentilador, potenciaLuz, velocidadVentilador;
volatile int contadorImpulsos = 0; // Declarar como volatile
volatile float velocidadRPM = 0; // Declarar como volatile
unsigned long tiempoAnteriorTemp, tiempoAnteriorVel, tiempoImprimir;
unsigned long ultimoTiempoRPM;

// Variables para lectura de comandos
String comando, valorLuz, valorVentilador;

// Función de interrupción para contar los impulsos del ventilador
void medirVelocidad() {
  contadorImpulsos++;
}

void setup() {
  Serial.begin(115200);

  TCCR1B = (TCCR1B & 0b11111000) | 0x01;  // Configura el prescaler a 1 (frecuencia máxima) al pin 9

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
    velocidadRPM = (pulsos / 2.0) * (6000.0 / tiempoTranscurrido);
  } else if (tiempoTranscurrido > 0 && pulsos == 0) {
    velocidadRPM = 0;  // Si no hay pulsos, la velocidad es 0
  }

  ultimoTiempoRPM = tiempoActual;  // Actualizar el tiempo de referencia
  tiempoAnteriorVel = tiempoActual; // Actualizar el tiempo de la última medición de velocidad
}

  // Impresión de datos cada 300 ms
  if (millis() - tiempoImprimir >= 300) {
    Serial.print(tempCelsius);
    Serial.print(";");
    Serial.println(velocidadRPM);

    tiempoImprimir = millis(); // Actualizar el tiempo de la última impresión
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
      //corriente de arranque
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
