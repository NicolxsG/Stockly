
#include <Arduino.h>
#include "HX711.h"

#if defined(ESP32)
  #include <esp_arduino_version.h>
#endif


#define HX711_DOUT 21
#define HX711_CLK   3

HX711 balanza;

const float FACTOR_ESCALA = 49.5;   // Ajustar con tu calibración real
const int MUESTRAS_TARA = 50;
const int MUESTRAS_PESO = 10;
const unsigned long PERIODO_LECTURA_PESO_MS = 500;
const long PESO_MINIMO_PRODUCTO_G = 20;


const int PWM_BAJO = 75;                  
const unsigned long TIEMPO_VUELTA_MS = 3500;
const float UMBRAL_PRODUCTO_CM = 12.0;   
const unsigned long TIMEOUT_ULTRASONIDO_US = 25000UL;
const unsigned long PERIODO_CHEQUEO_ULTRASONIDO_MS = 300;

// PWM ESP32
const uint32_t PWM_FRECUENCIA = 5000;
const uint8_t PWM_RESOLUCION_BITS = 8;

struct BandaTransportadora {
  const char* nombre;

  // Sensor ultrasónico
  uint8_t trigPin;
  uint8_t echoPin;

  // Motor
  uint8_t pwmPin;
  uint8_t in1Pin;
  uint8_t in2Pin;

  // Canal PWM para ESP32 Arduino Core 2.x
  uint8_t pwmChannel;

  bool moviendo;
  bool vueltaHechaSinProducto;
  unsigned long inicioMovimiento;
};


BandaTransportadora bandas[3] = {
  // nombre      TRIG ECHO PWM IN1 IN2 CANAL moviendo vuelta inicio
  { "Banda 1",    4,   5,  10, 11, 12,   0,   false, false, 0 },
  { "Banda 2",    6,   7,  13, 14, 15,   1,   false, false, 0 },
  { "Banda 3",    8,   9,  16, 17, 18,   2,   false, false, 0 }
};

bool sistemaListo = false;
unsigned long ultimoPesoMs = 0;
unsigned long ultimoChequeoUltrasonidoMs = 0;


void configurarPWM(uint8_t pin, uint8_t canal) {
#if defined(ESP32)
  #if ESP_ARDUINO_VERSION_MAJOR >= 3
    // Arduino-ESP32 Core 3.x
    ledcAttachChannel(pin, PWM_FRECUENCIA, PWM_RESOLUCION_BITS, canal);
  #else
    // Arduino-ESP32 Core 2.x
    ledcSetup(canal, PWM_FRECUENCIA, PWM_RESOLUCION_BITS);
    ledcAttachPin(pin, canal);
  #endif
#else
  pinMode(pin, OUTPUT);
#endif
}

void escribirPWM(uint8_t pin, uint8_t canal, uint8_t duty) {
#if defined(ESP32)
  #if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(pin, duty);
  #else
    ledcWrite(canal, duty);
  #endif
#else
  analogWrite(pin, duty);
#endif
}

void detenerBanda(uint8_t i);

void configurarMotores() {
  for (uint8_t i = 0; i < 3; i++) {
    pinMode(bandas[i].in1Pin, OUTPUT);
    pinMode(bandas[i].in2Pin, OUTPUT);

    configurarPWM(bandas[i].pwmPin, bandas[i].pwmChannel);

    escribirPWM(bandas[i].pwmPin, bandas[i].pwmChannel, 0);
    digitalWrite(bandas[i].in1Pin, LOW);
    digitalWrite(bandas[i].in2Pin, LOW);
  }
}

void iniciarBanda(uint8_t i) {
  BandaTransportadora &b = bandas[i];

  digitalWrite(b.in1Pin, HIGH);
  digitalWrite(b.in2Pin, LOW);

  escribirPWM(b.pwmPin, b.pwmChannel, PWM_BAJO);

  b.moviendo = true;
  b.inicioMovimiento = millis();

  Serial.print("[");
  Serial.print(b.nombre);
  Serial.println("] Sin producto -> inicia una vuelta con PWM bajo");
}

void detenerBanda(uint8_t i) {
  BandaTransportadora &b = bandas[i];

  escribirPWM(b.pwmPin, b.pwmChannel, 0);
  digitalWrite(b.in1Pin, LOW);
  digitalWrite(b.in2Pin, LOW);

  b.moviendo = false;
  b.vueltaHechaSinProducto = true;

  Serial.print("[");
  Serial.print(b.nombre);
  Serial.println("] Vuelta completa -> motor detenido");
}


void configurarUltrasonidos() {
  for (uint8_t i = 0; i < 3; i++) {
    pinMode(bandas[i].trigPin, OUTPUT);
    pinMode(bandas[i].echoPin, INPUT);
    digitalWrite(bandas[i].trigPin, LOW);
  }
}

float medirDistanciaCm(uint8_t i) {
  BandaTransportadora &b = bandas[i];

  digitalWrite(b.trigPin, LOW);
  delayMicroseconds(3);

  digitalWrite(b.trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(b.trigPin, LOW);

  unsigned long duracion = pulseIn(b.echoPin, HIGH, TIMEOUT_ULTRASONIDO_US);

  if (duracion == 0) {
    return 999.0;
  }

  return duracion / 58.0;
}

bool hayProducto(uint8_t i, float *distanciaLeida = nullptr) {
  float distancia = medirDistanciaCm(i);

  if (distanciaLeida != nullptr) {
    *distanciaLeida = distancia;
  }

  return distancia <= UMBRAL_PRODUCTO_CM;
}


void calibrarBascula() {
  Serial.println();
  Serial.println("        STOCKLY - ESP32-S3");

  balanza.begin(HX711_DOUT, HX711_CLK);

  Serial.println("Inicializando HX711...");
  delay(300);

  if (!balanza.is_ready()) {
    Serial.println("ADVERTENCIA: HX711 no detectado.");
    Serial.println("Revisa DOUT, CLK, VCC, GND y alimentacion.");
    // No bloquea el sistema para permitir probar bandas.
  } else {
    Serial.print("Lectura inicial ADC: ");
    Serial.println(balanza.read());
  }

  Serial.println("Retira todos los productos de la bascula.");
  Serial.print("Tarando a cero ");

  balanza.set_scale(FACTOR_ESCALA);

  for (int i = 0; i < 20; i++) {
    Serial.print("=");
    delay(120);
  }

  if (balanza.is_ready()) {
    balanza.tare(MUESTRAS_TARA);
    Serial.println(" OK");
  } else {
    Serial.println(" HX711 no listo");
  }

  Serial.println("Sistema listo.");
  Serial.println("Comandos: r=tara, p=probar todas, 1/2/3=probar banda");
  Serial.println();

  sistemaListo = true;
}

long leerPesoGramos() {
  if (!balanza.is_ready()) {
    return 0;
  }

  long peso = balanza.get_units(MUESTRAS_PESO);

  if (peso < 0 && peso > -PESO_MINIMO_PRODUCTO_G) {
    peso = 0;
  }

  return peso;
}

void imprimirPesoPeriodico() {
  if (millis() - ultimoPesoMs < PERIODO_LECTURA_PESO_MS) {
    return;
  }

  ultimoPesoMs = millis();

  long peso = leerPesoGramos();

  Serial.print("[Bascula] Peso: ");

  if (peso >= 1000) {
    Serial.print(peso / 1000.0, 2);
    Serial.println(" kg");
  } else {
    Serial.print(peso);
    Serial.println(" g");
  }
}


void actualizarBanda(uint8_t i) {
  BandaTransportadora &b = bandas[i];

  if (b.moviendo) {
    if (millis() - b.inicioMovimiento >= TIEMPO_VUELTA_MS) {
      detenerBanda(i);
    }
    return;
  }

  float distancia = 0;
  bool producto = hayProducto(i, &distancia);

  Serial.print("[");
  Serial.print(b.nombre);
  Serial.print("] Distancia: ");

  if (distancia >= 900.0) {
    Serial.print("sin eco");
  } else {
    Serial.print(distancia, 1);
    Serial.print(" cm");
  }

  Serial.print(" -> ");

  if (producto) {
    Serial.println("producto detectado");
    b.vueltaHechaSinProducto = false;
    return;
  }

  Serial.println("NO hay producto");

  if (!b.vueltaHechaSinProducto) {
    iniciarBanda(i);
  }
}

void actualizarBandas() {
  if (millis() - ultimoChequeoUltrasonidoMs < PERIODO_CHEQUEO_ULTRASONIDO_MS) {
    return;
  }

  ultimoChequeoUltrasonidoMs = millis();

  for (uint8_t i = 0; i < 3; i++) {
    actualizarBanda(i);
  }
}


void revisarComandosSerial() {
  if (!Serial.available()) {
    return;
  }

  char c = Serial.read();

  if (c == 'r' || c == 'R') {
    Serial.println("Recalibrando tara. Retira productos de la bascula...");
    delay(1500);

    if (balanza.is_ready()) {
      balanza.tare(MUESTRAS_TARA);
      Serial.println("Tara actualizada.");
    } else {
      Serial.println("HX711 no listo. No se pudo tarar.");
    }
  }

  if (c == 'p' || c == 'P') {
    Serial.println("Prueba manual: una vuelta en las 3 bandas.");
    for (uint8_t i = 0; i < 3; i++) {
      if (!bandas[i].moviendo) {
        bandas[i].vueltaHechaSinProducto = false;
        iniciarBanda(i);
      }
    }
  }

  if (c >= '1' && c <= '3') {
    uint8_t i = c - '1';

    Serial.print("Prueba manual: ");
    Serial.println(bandas[i].nombre);

    if (!bandas[i].moviendo) {
      bandas[i].vueltaHechaSinProducto = false;
      iniciarBanda(i);
    }
  }
}


void setup() {
  Serial.begin(115200);
  delay(1000);

  configurarMotores();
  configurarUltrasonidos();
  calibrarBascula();
}

void loop() {
  if (!sistemaListo) {
    return;
  }

  revisarComandosSerial();
  imprimirPesoPeriodico();
  actualizarBandas();
}
