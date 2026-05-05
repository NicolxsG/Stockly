
#include "HX711.h"

#define DOUT 6   // cambia si quieres
#define CLK  7

HX711 balanza;
void setup() {
  Serial.begin(115200);
  balanza.begin(DOUT, CLK);
  Serial.print("Lectura del valor del ADC:t");
  Serial.println(balanza.read());
  Serial.println("No ponga ningún objeto sobre la balanza");
  Serial.println("Destarando...");
  balanza.set_scale(); //La escala por defecto es 1
  balanza.tare(50);  //El peso actual es considerado Tara.
  Serial.println("Coloque un peso conocido:");
}

void loop() {

  Serial.print("Valor de lectura: \t");
  Serial.println(balanza.get_value(10),0);
  delay(100);
}
