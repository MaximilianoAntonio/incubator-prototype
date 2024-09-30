#include <Arduino.h>
#include <PID_v1.h>

// Definición de pines
#define NTC_PIN A0 // pin NTC
#define LUZ_PIN 3  // pin Luz
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
double setpoint, input, output;

// Parámetros PID para los diferentes ensayos
double Kp[] = {40.46, 78.22, 72.31, 82.21, 196.53};
double Ki[] = {0.108, 0.075, 0.079, 0.183, 0.170};
double Kd[] = {15115.43, 81034.33, 66196.15, 36972.44, 113864.92};

// Variable para almacenar el índice del ensayo seleccionado
int ensayoSeleccionado = 0;

// PID object
PID myPID(&input, &output, &setpoint, Kp[0], Ki[0], Kd[0], DIRECT);

// Variables para control automático
bool control_automatico = false;

// Definir un arreglo para almacenar las últimas 5 lecturas de RPM
float ultimasRPM[5] = {0, 0, 0, 0, 0}; // Inicialmente con ceros
int indiceRPM = 0;  // Índice para llevar la cuenta de la posición actual
float promedioRPM = 0;  // Variable para almacenar el promedio de las últimas 5 lecturas  

// Función para calcular el promedio de los últimos 5 valores
float calcularPromedioRPM() {
  float suma = 0;
  for (int i = 0; i < 5; i++) {
    suma += ultimasRPM[i];
  }
  return suma / 5;  // Devolver el promedio
}

// Función de interrupción para contar los impulsos del ventilador
void medirVelocidad() {
  contadorImpulsos++;
}

// Función para seleccionar el ensayo adecuado según la temperatura deseada
void seleccionarEnsayo(double temperaturaDeseada) {
  if (temperaturaDeseada <= 31.55) {
    ensayoSeleccionado = 3; // Ensayo 4
  } else if (temperaturaDeseada <= 33.97) {
    ensayoSeleccionado = 4; // Ensayo 5
  } else if (temperaturaDeseada <= 39.34) {
    ensayoSeleccionado = 0; // Ensayo 1
  } else if (temperaturaDeseada <= 50.28) {
    ensayoSeleccionado = 2; // Ensayo 3
  } else if (temperaturaDeseada <= 51.66) {
    ensayoSeleccionado = 1; // Ensayo 2
  }
  
  // Ajustar los parámetros PID según el ensayo seleccionado
  myPID.SetTunings(Kp[ensayoSeleccionado], Ki[ensayoSeleccionado], Kd[ensayoSeleccionado]);
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

  // Inicializar PID
  setpoint = 25; // Temperatura inicial deseada, ajustable desde Python
  myPID.SetMode(AUTOMATIC);
  myPID.SetOutputLimits(0, 100); // El PID controla la potencia de la luz entre 0 y 100
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

    // Si el control automático está activado, actualizamos el PID
    if (control_automatico) {
      input = tempCelsius;  // Actualizar la entrada del PID con la temperatura medida
      myPID.Compute();      // Calcular la salida del PID
      int Salida = map(output, 0, 100, 0, 255);  // Mapear la salida del PID al rango de 0 a 255
      analogWrite(LUZ_PIN, Salida);  // Ajustar la potencia de la luz con la salida del PID
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

    // Calcular el promedio de las últimas 5 lecturas
    promedioRPM = calcularPromedioRPM();

    ultimoTiempoRPM = tiempoActual;  // Actualizar el tiempo de referencia
    tiempoAnteriorVel = tiempoActual; // Actualizar el tiempo de la última medición de velocidad
  }

  // Impresión de datos cada 300 ms
  if (millis() - tiempoImprimir >= 300) {
    Serial.print(tempCelsius);
    Serial.print(";");
    Serial.println(promedioRPM);  // Imprimir el promedio de RPM

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
      setpoint = valorSetpoint.toDouble();  // Actualizar el setpoint del PID
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
