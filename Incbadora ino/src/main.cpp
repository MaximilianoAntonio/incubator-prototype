#include <Arduino.h>

float Tref, vol, vastago, vastago2, resNTC, temperatura, t, DXDT, cont;
float a, b, c;
int pwm, pot, vel, pot2, pwm2, Vamp, velocidad;
unsigned long t0, t1, deltat;
unsigned long lastUpdateTime = 0;  // Para rastrear el tiempo de la última actualización
int pulsosPorRevolucion = 2;  // Estimación común de pulsos por revolución
volatile int contadorPulsos = 0;  // Contador de pulsos para la interrupción

const int pwmPin = 3;  // Pin de PWM

void velocimetro() {
  contadorPulsos++;  // Incrementa el número de pulsos cada vez que ocurre la interrupción
}

void setup() {
  Serial.begin(9600);  // Iniciar la comunicación serie
  pinMode(pwmPin, OUTPUT);  // Configurar el pin 3 como salida
  Serial.println("Envía un valor entre 0 y 255 para ajustar el PWM:");
  
  t0 = millis();
  t1 = millis();
  
  attachInterrupt(digitalPinToInterrupt(2), velocimetro, FALLING);  // Configurar interrupción
}

void loop() {
  if (Serial.available() > 0) {  // Verificar si hay datos disponibles en el puerto serie
    String input = Serial.readStringUntil('\n');  // Leer la línea completa hasta el salto de línea
    int pwmValue = input.toInt();  // Convertir el valor ingresado en un número entero

    // Validar que el valor esté entre 0 y 255
    if (pwmValue >= 0 && pwmValue <= 255) {
      analogWrite(pwmPin, pwmValue);  // Aplicar el valor de PWM al pin 3
      Serial.print("PWM ajustado a: ");
      Serial.println(pwmValue);  // Confirmar el valor ajustado
    } else {
      Serial.println("Por favor, ingresa un valor entre 0 y 255.");  // Mensaje de error
    }
  }

  // Calcular las RPM cada segundo (1000 milisegundos)
  unsigned long currentTime = millis();
  if (currentTime - lastUpdateTime >= 1000) {  // Actualizar cada segundo
    // Calcular RPM
    float revoluciones = (contadorPulsos / (float)pulsosPorRevolucion);  // Convertir pulsos a revoluciones
    float rpm = (revoluciones * 60.0);  // Convertir a revoluciones por minuto (RPM)
    
    // Mostrar las RPM
    Serial.print("RPM: ");
    Serial.println(rpm);
    
    // Reiniciar el contador de pulsos y el tiempo
    contadorPulsos = 0;
    lastUpdateTime = currentTime;
  }
}
