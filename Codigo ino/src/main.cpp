#include <Arduino.h>

int pwmPin = 5; // Pin 5 para PWM
int pwmValue = 0; // Valor inicial de PWM

void setup() {
  // Inicializa la comunicación serie a 9600 baudios
  Serial.begin(9600);
  
  // Configura el pin 5 como salida
  pinMode(pwmPin, OUTPUT);
  
  // Mensaje de inicio
  Serial.println("Escribe un valor entre 0 y 255 para cambiar el PWM:");
}

void loop() {
  // Verifica si hay datos disponibles en el monitor serie
  if (Serial.available() > 0) {
    // Lee el valor introducido
    pwmValue = Serial.parseInt();
    
    // Verifica si el valor está dentro del rango permitido (0 a 255)
    if (pwmValue >= 0 && pwmValue <= 255) {
      // Cambia el valor de PWM en el pin 5
      analogWrite(pwmPin, pwmValue);
      
      // Muestra el valor ajustado en el monitor serie
      Serial.print("Valor de PWM ajustado a: ");
      Serial.println(pwmValue);
    } else {
      // Mensaje de error si el valor está fuera de rango
      Serial.println("Por favor, ingresa un valor entre 0 y 255.");
    }
  }
}
