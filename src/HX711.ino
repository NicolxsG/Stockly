#include "HX711.h"

#define DOUT 6
#define CLK  7

HX711 balanza;

void setup() {
  Serial.begin(115200);
  balanza.begin(DOUT, CLK);

  Serial.print("Lectura del valor del ADC:  ");
  Serial.println(balanza.read());

  Serial.println("No ponga ningun objeto sobre la balanza");
  Serial.println("Un momento por favor =D");
  Serial.print("Calibrando a CERO ");
    for (int i = 0; i <= 20; i++) {
  Serial.print("=");
  delay(200);
}
  Serial.println("> ¡¡Balanza Calibrada!!");


  balanza.set_scale(49.5);
  balanza.tare(50);

  Serial.println("¡Todo listo! Puede colocar el objeto");  
}

void loop() {
  // Convertimos a entero
  long peso = balanza.get_units(30);

  Serial.print("Peso: ");

  if (peso >= 1000) {
    // Mostrar en kg
    float kg = peso / 1000.0;
    Serial.print(kg, 2);  // 2 decimales en kg
    Serial.println(" kg");
  } else {
    // Mostrar en gramos enteros
    Serial.print(peso);
    Serial.println(" g");
  }

  delay(100);
}